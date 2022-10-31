// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

#ifndef STONE_RGW_KMIP_CLIENT_H
#define STONE_RGW_KMIP_CLIENT_H

class RGWKMIPManager;

class RGWKMIPTransceiver {
public:
  enum kmip_operation {
    CREATE,
    LOCATE,
    GET,
    GET_ATTRIBUTES,
    GET_ATTRIBUTE_LIST,
    DESTROY
  };
  StoneContext *cct;
  kmip_operation operation;
  char *name = 0;
  char *unique_id = 0;
  // output - must free
  char *out = 0;    // unique_id, several
  struct {    // unique_ids, locate
    char **strings;
    int string_count;
  } outlist[1] = {{0, 0}};
  struct {    // key, get
    unsigned char *data;
    int keylen;
  } outkey[1] = {0, 0};
  // end must free
  int ret;
  bool done;
  stone::mutex lock = stone::make_mutex("rgw_kmip_req::lock");
  stone::condition_variable cond;

  int wait(optional_yield y);
  RGWKMIPTransceiver(StoneContext * const cct,
    kmip_operation operation)
  : cct(cct),
    operation(operation),
    ret(-EDOM),
    done(false)
  {}
  ~RGWKMIPTransceiver();

  int send();
  int process(optional_yield y);
};

class RGWKMIPManager {
protected:
  StoneContext *cct;
  bool is_started = false;
  RGWKMIPManager(StoneContext *cct) : cct(cct) {};
public:
  virtual ~RGWKMIPManager() { };
  virtual int start() = 0;
  virtual void stop() = 0;
  virtual int add_request(RGWKMIPTransceiver*) = 0;
};

void rgw_kmip_client_init(RGWKMIPManager &);
void rgw_kmip_client_cleanup();
#endif
