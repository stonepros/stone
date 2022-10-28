#include "StonexProtocol.h"
#include "StonexAuthorizeHandler.h"
#include "common/dout.h"

#define dout_subsys stone_subsys_auth

bool StonexAuthorizeHandler::verify_authorizer(
  StoneContext *cct,
  const KeyStore& keys,
  const stone::bufferlist& authorizer_data,
  size_t connection_secret_required_len,
  stone::bufferlist *authorizer_reply,
  EntityName *entity_name,
  uint64_t *global_id,
  AuthCapsInfo *caps_info,
  CryptoKey *session_key,
  std::string *connection_secret,
  std::unique_ptr<AuthAuthorizerChallenge> *challenge)
{
  auto iter = authorizer_data.cbegin();

  if (!authorizer_data.length()) {
    ldout(cct, 1) << "verify authorizer, authorizer_data.length()=0" << dendl;
    return false;
  }

  StoneXServiceTicketInfo auth_ticket_info;

  bool isvalid = stonex_verify_authorizer(cct, keys, iter,
					 connection_secret_required_len,
					 auth_ticket_info,
					 challenge, connection_secret,
					 authorizer_reply);

  if (isvalid) {
    *caps_info = auth_ticket_info.ticket.caps;
    *entity_name = auth_ticket_info.ticket.name;
    *global_id = auth_ticket_info.ticket.global_id;
    *session_key = auth_ticket_info.session_key;
  }

  return isvalid;
}

// Return type of crypto used for this session's data;  for stonex, symmetric authentication

int StonexAuthorizeHandler::authorizer_session_crypto() 
{
  return SESSION_SYMMETRIC_AUTHENTICATE;
}
