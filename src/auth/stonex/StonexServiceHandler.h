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

#ifndef STONE_STONEXSERVICEHANDLER_H
#define STONE_STONEXSERVICEHANDLER_H

#include "auth/AuthServiceHandler.h"
#include "auth/Auth.h"

class KeyServer;
struct StoneXAuthenticate;
struct StoneXServiceTicketInfo;

class StonexServiceHandler  : public AuthServiceHandler {
  KeyServer *key_server;
  uint64_t server_challenge;

public:
  StonexServiceHandler(StoneContext *cct_, KeyServer *ks) 
    : AuthServiceHandler(cct_), key_server(ks), server_challenge(0) {}
  ~StonexServiceHandler() override {}
  
  int handle_request(
    stone::buffer::list::const_iterator& indata,
    size_t connection_secret_required_length,
    stone::buffer::list *result_bl,
    AuthCapsInfo *caps,
    CryptoKey *session_key,
    std::string *connection_secret) override;

private:
  int do_start_session(bool is_new_global_id,
		       stone::buffer::list *result_bl,
		       AuthCapsInfo *caps) override;

  int verify_old_ticket(const StoneXAuthenticate& req,
			StoneXServiceTicketInfo& old_ticket_info,
			bool& should_enc_ticket);
  void build_stonex_response_header(int request_type, int status,
				   stone::buffer::list& bl);
};

#endif
