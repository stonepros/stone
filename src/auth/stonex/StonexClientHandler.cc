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


#include <errno.h>

#include "StonexClientHandler.h"
#include "StonexProtocol.h"

#include "auth/KeyRing.h"
#include "include/random.h"
#include "common/stone_context.h"
#include "common/config.h"
#include "common/dout.h"

#define dout_subsys stone_subsys_auth
#undef dout_prefix
#define dout_prefix *_dout << "stonex client: "

using std::string;

using stone::bufferlist;

void StonexClientHandler::reset()
{
  ldout(cct,10) << __func__ << dendl;
  starting = true;
  server_challenge = 0;
}

int StonexClientHandler::build_request(bufferlist& bl) const
{
  ldout(cct, 10) << "build_request" << dendl;

  if (need & STONE_ENTITY_TYPE_AUTH) {
    /* authenticate */
    StoneXRequestHeader header;
    header.request_type = STONEX_GET_AUTH_SESSION_KEY;
    encode(header, bl);

    CryptoKey secret;
    const bool got = keyring->get_secret(cct->_conf->name, secret);
    if (!got) {
      ldout(cct, 20) << "no secret found for entity: " << cct->_conf->name << dendl;
      return -ENOENT;
    }

    // is the key OK?
    if (!secret.get_secret().length()) {
      ldout(cct, 20) << "secret for entity " << cct->_conf->name << " is invalid" << dendl;
      return -EINVAL;
    }

    StoneXAuthenticate req;
    req.client_challenge = stone::util::generate_random_number<uint64_t>();
    std::string error;
    stonex_calc_client_server_challenge(cct, secret, server_challenge,
				       req.client_challenge, &req.key, error);
    if (!error.empty()) {
      ldout(cct, 20) << "stonex_calc_client_server_challenge error: " << error << dendl;
      return -EIO;
    }

    req.old_ticket = ticket_handler->ticket;

    // for nautilus+ servers: request other keys at the same time
    req.other_keys = need;

    if (req.old_ticket.blob.length()) {
      ldout(cct, 20) << "old ticket len=" << req.old_ticket.blob.length() << dendl;
    }

    encode(req, bl);

    ldout(cct, 10) << "get auth session key: client_challenge "
		   << std::hex << req.client_challenge << std::dec << dendl;
    return 0;
  }

  if (_need_tickets()) {
    /* get service tickets */
    ldout(cct, 10) << "get service keys: want=" << want << " need=" << need << " have=" << have << dendl;

    StoneXRequestHeader header;
    header.request_type = STONEX_GET_PRINCIPAL_SESSION_KEY;
    encode(header, bl);

    StoneXAuthorizer *authorizer = ticket_handler->build_authorizer(global_id);
    if (!authorizer)
      return -EINVAL;
    bl.claim_append(authorizer->bl);
    delete authorizer;

    StoneXServiceTicketRequest req;
    req.keys = need;
    encode(req, bl);
  }

  return 0;
}

bool StonexClientHandler::_need_tickets() const
{
  // do not bother (re)requesting tickets if we *only* need the MGR
  // ticket; that can happen during an upgrade and we want to avoid a
  // loop.  we'll end up re-requesting it later when the secrets
  // rotating.
  return need && need != STONE_ENTITY_TYPE_MGR;
}

int StonexClientHandler::handle_response(
  int ret,
  bufferlist::const_iterator& indata,
  CryptoKey *session_key,
  std::string *connection_secret)
{
  ldout(cct, 10) << this << " handle_response ret = " << ret << dendl;
  
  if (ret < 0)
    return ret; // hrm!

  if (starting) {
    StoneXServerChallenge ch;
    try {
      decode(ch, indata);
    } catch (stone::buffer::error& e) {
      ldout(cct, 1) << __func__ << " failed to decode StoneXServerChallenge: "
		    << e.what() << dendl;
      return -EPERM;
    }
    server_challenge = ch.server_challenge;
    ldout(cct, 10) << " got initial server challenge "
		   << std::hex << server_challenge << std::dec << dendl;
    starting = false;

    tickets.invalidate_ticket(STONE_ENTITY_TYPE_AUTH);
    return -EAGAIN;
  }

  struct StoneXResponseHeader header;
  try {
    decode(header, indata);
  } catch (stone::buffer::error& e) {
    ldout(cct, 1) << __func__ << " failed to decode StoneXResponseHeader: "
		  << e.what() << dendl;
    return -EPERM;
  }

  switch (header.request_type) {
  case STONEX_GET_AUTH_SESSION_KEY:
    {
      ldout(cct, 10) << " get_auth_session_key" << dendl;
      CryptoKey secret;
      const bool got = keyring->get_secret(cct->_conf->name, secret);
      if (!got) {
	ldout(cct, 0) << "key not found for " << cct->_conf->name << dendl;
	return -ENOENT;
      }
	
      if (!tickets.verify_service_ticket_reply(secret, indata)) {
	ldout(cct, 0) << "could not verify service_ticket reply" << dendl;
	return -EACCES;
      }
      ldout(cct, 10) << " want=" << want << " need=" << need << " have=" << have << dendl;
      if (!indata.end()) {
	bufferlist cbl, extra_tickets;
	using stone::decode;
	try {
	  decode(cbl, indata);
	  decode(extra_tickets, indata);
	} catch (stone::buffer::error& e) {
	  ldout(cct, 1) << __func__ << " failed to decode tickets: "
			<< e.what() << dendl;
	  return -EPERM;
	}
	ldout(cct, 10) << " got connection bl " << cbl.length()
		       << " and extra tickets " << extra_tickets.length()
		       << dendl;
	if (session_key && connection_secret) {
	  StoneXTicketHandler& ticket_handler =
	    tickets.get_handler(STONE_ENTITY_TYPE_AUTH);
	  if (session_key) {
	    *session_key = ticket_handler.session_key;
	  }
	  if (cbl.length() && connection_secret) {
	    auto p = cbl.cbegin();
	    string err;
	    if (decode_decrypt(cct, *connection_secret, *session_key, p,
			       err)) {
	      lderr(cct) << __func__ << " failed to decrypt connection_secret"
			 << dendl;
	    } else {
	      ldout(cct, 10) << " got connection_secret "
			     << connection_secret->size() << " bytes" << dendl;
	    }
	  }
	  if (extra_tickets.length())  {
	    auto p = extra_tickets.cbegin();
	    if (!tickets.verify_service_ticket_reply(
		  *session_key, p)) {
	      lderr(cct) << "could not verify extra service_tickets" << dendl;
	    } else {
	      ldout(cct, 10) << " got extra service_tickets" << dendl;
	    }
	  }
	}
      }
      validate_tickets();
      if (_need_tickets())
	ret = -EAGAIN;
      else
	ret = 0;
      }
    break;

  case STONEX_GET_PRINCIPAL_SESSION_KEY:
    {
      StoneXTicketHandler& ticket_handler = tickets.get_handler(STONE_ENTITY_TYPE_AUTH);
      ldout(cct, 10) << " get_principal_session_key session_key " << ticket_handler.session_key << dendl;
  
      if (!tickets.verify_service_ticket_reply(ticket_handler.session_key, indata)) {
        ldout(cct, 0) << "could not verify service_ticket reply" << dendl;
        return -EACCES;
      }
      validate_tickets();
      if (!_need_tickets()) {
	ret = 0;
      }
    }
    break;

  case STONEX_GET_ROTATING_KEY:
    {
      ldout(cct, 10) << " get_rotating_key" << dendl;
      if (rotating_secrets) {
	RotatingSecrets secrets;
	CryptoKey secret_key;
	const bool got = keyring->get_secret(cct->_conf->name, secret_key);
        if (!got) {
          ldout(cct, 0) << "key not found for " << cct->_conf->name << dendl;
          return -ENOENT;
        }
	std::string error;
	if (decode_decrypt(cct, secrets, secret_key, indata, error)) {
	  ldout(cct, 0) << "could not set rotating key: decode_decrypt failed. error:"
	    << error << dendl;
	  return -EINVAL;
	} else {
	  rotating_secrets->set_secrets(std::move(secrets));
	}
      }
    }
    break;

  default:
   ldout(cct, 0) << " unknown request_type " << header.request_type << dendl;
   stone_abort();
  }
  return ret;
}


AuthAuthorizer *StonexClientHandler::build_authorizer(uint32_t service_id) const
{
  ldout(cct, 10) << "build_authorizer for service " << stone_entity_type_name(service_id) << dendl;
  return tickets.build_authorizer(service_id);
}


bool StonexClientHandler::build_rotating_request(bufferlist& bl) const
{
  ldout(cct, 10) << "build_rotating_request" << dendl;
  StoneXRequestHeader header;
  header.request_type = STONEX_GET_ROTATING_KEY;
  encode(header, bl);
  return true;
}

void StonexClientHandler::prepare_build_request()
{
  ldout(cct, 10) << "validate_tickets: want=" << want << " need=" << need
		 << " have=" << have << dendl;
  validate_tickets();
  ldout(cct, 10) << "want=" << want << " need=" << need << " have=" << have
		 << dendl;

  ticket_handler = &(tickets.get_handler(STONE_ENTITY_TYPE_AUTH));
}

void StonexClientHandler::validate_tickets()
{
  // lock should be held for write
  tickets.validate_tickets(want, have, need);
}

bool StonexClientHandler::need_tickets()
{
  validate_tickets();

  ldout(cct, 20) << "need_tickets: want=" << want
		 << " have=" << have
		 << " need=" << need
		 << dendl;

  return _need_tickets();
}
