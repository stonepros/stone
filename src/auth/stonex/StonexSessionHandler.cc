// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2004-2009 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include "StonexSessionHandler.h"
#include "StonexProtocol.h"

#include <errno.h>
#include <sstream>

#include "common/config.h"
#include "include/stone_features.h"
#include "msg/Message.h"

#define dout_subsys stone_subsys_auth

namespace {
#ifdef WITH_SEASTAR
  crimson::common::ConfigProxy& conf(StoneContext*) {
    return crimson::common::local_conf();
  }
#else
  ConfigProxy& conf(StoneContext* cct) {
    return cct->_conf;
  }
#endif
}

int StonexSessionHandler::_calc_signature(Message *m, uint64_t *psig)
{
  const stone_msg_header& header = m->get_header();
  const stone_msg_footer& footer = m->get_footer();

  if (!HAVE_FEATURE(features, STONEX_V2)) {
    // legacy pre-mimic behavior for compatibility

    // optimized signature calculation
    // - avoid temporary allocated buffers from encode_encrypt[_enc_bl]
    // - skip the leading 4 byte wrapper from encode_encrypt
    struct {
      __u8 v;
      stone_le64 magic;
      stone_le32 len;
      stone_le32 header_crc;
      stone_le32 front_crc;
      stone_le32 middle_crc;
      stone_le32 data_crc;
    } __attribute__ ((packed)) sigblock = {
      1, init_le64(AUTH_ENC_MAGIC), init_le32(4*4),
      init_le32(header.crc), init_le32(footer.front_crc),
      init_le32(footer.middle_crc), init_le32(footer.data_crc)
    };

    char exp_buf[CryptoKey::get_max_outbuf_size(sizeof(sigblock))];

    try {
      const CryptoKey::in_slice_t in {
	sizeof(sigblock),
	reinterpret_cast<const unsigned char*>(&sigblock)
      };
      const CryptoKey::out_slice_t out {
	sizeof(exp_buf),
	reinterpret_cast<unsigned char*>(&exp_buf)
      };
      key.encrypt(cct, in, out);
    } catch (std::exception& e) {
      lderr(cct) << __func__ << " failed to encrypt signature block" << dendl;
      return -1;
    }

    *psig = *reinterpret_cast<stone_le64*>(exp_buf);
  } else {
    // newer mimic+ signatures
    struct {
      stone_le32 header_crc;
      stone_le32 front_crc;
      stone_le32 front_len;
      stone_le32 middle_crc;
      stone_le32 middle_len;
      stone_le32 data_crc;
      stone_le32 data_len;
      stone_le32 seq_lower_word;
    } __attribute__ ((packed)) sigblock = {
      init_le32(header.crc),
      init_le32(footer.front_crc),
      init_le32(header.front_len),
      init_le32(footer.middle_crc),
      init_le32(header.middle_len),
      init_le32(footer.data_crc),
      init_le32(header.data_len),
      init_le32(header.seq)
    };

    char exp_buf[CryptoKey::get_max_outbuf_size(sizeof(sigblock))];

    try {
      const CryptoKey::in_slice_t in {
	sizeof(sigblock),
	reinterpret_cast<const unsigned char*>(&sigblock)
      };
      const CryptoKey::out_slice_t out {
	sizeof(exp_buf),
	reinterpret_cast<unsigned char*>(&exp_buf)
      };
      key.encrypt(cct, in, out);
    } catch (std::exception& e) {
      lderr(cct) << __func__ << " failed to encrypt signature block" << dendl;
      return -1;
    }

    struct enc {
      stone_le64 a, b, c, d;
    } *penc = reinterpret_cast<enc*>(exp_buf);
    *psig = penc->a ^ penc->b ^ penc->c ^ penc->d;
  }

  ldout(cct, 10) << __func__ << " seq " << m->get_seq()
		 << " front_crc_ = " << footer.front_crc
		 << " middle_crc = " << footer.middle_crc
		 << " data_crc = " << footer.data_crc
		 << " sig = " << *psig
		 << dendl;
  return 0;
}

int StonexSessionHandler::sign_message(Message *m)
{
  // If runtime signing option is off, just return success without signing.
  if (!conf(cct)->stonex_sign_messages) {
    return 0;
  }

  uint64_t sig;
  int r = _calc_signature(m, &sig);
  if (r < 0)
    return r;

  stone_msg_footer& f = m->get_footer();
  f.sig = sig;
  f.flags = (unsigned)f.flags | STONE_MSG_FOOTER_SIGNED;
  ldout(cct, 20) << "Putting signature in client message(seq # " << m->get_seq()
		 << "): sig = " << sig << dendl;
  return 0;
}

int StonexSessionHandler::check_message_signature(Message *m)
{
  // If runtime signing option is off, just return success without checking signature.
  if (!conf(cct)->stonex_sign_messages) {
    return 0;
  }
  if ((features & STONE_FEATURE_MSG_AUTH) == 0) {
    // it's fine, we didn't negotiate this feature.
    return 0;
  }

  uint64_t sig;
  int r = _calc_signature(m, &sig);
  if (r < 0)
    return r;

  if (sig != m->get_footer().sig) {
    // Should have been signed, but signature check failed.  PLR
    if (!(m->get_footer().flags & STONE_MSG_FOOTER_SIGNED)) {
      ldout(cct, 0) << "SIGN: MSG " << m->get_seq() << " Sender did not set STONE_MSG_FOOTER_SIGNED." << dendl;
    }
    ldout(cct, 0) << "SIGN: MSG " << m->get_seq() << " Message signature does not match contents." << dendl;
    ldout(cct, 0) << "SIGN: MSG " << m->get_seq() << "Signature on message:" << dendl;
    ldout(cct, 0) << "SIGN: MSG " << m->get_seq() << "    sig: " << m->get_footer().sig << dendl;
    ldout(cct, 0) << "SIGN: MSG " << m->get_seq() << "Locally calculated signature:" << dendl;
    ldout(cct, 0) << "SIGN: MSG " << m->get_seq() << "    sig_check:" << sig << dendl;

    // For the moment, printing an error message to the log and
    // returning failure is sufficient.  In the long term, we should
    // probably have code parsing the log looking for this kind of
    // security failure, particularly when there are large numbers of
    // them, since the latter is a potential sign of an attack.  PLR

    ldout(cct, 0) << "Signature failed." << dendl;
    return (SESSION_SIGNATURE_FAILURE);
  }

  return 0;
}
