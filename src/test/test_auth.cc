// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "include/types.h"
#include "include/stringify.h"
#include "auth/Auth.h"
#include "gtest/gtest.h"
#include "common/stone_context.h"
#include "global/global_context.h"
#include "auth/AuthRegistry.h"

#include <sstream>

TEST(AuthRegistry, con_modes)
{
  auto cct = g_stone_context;
  AuthRegistry reg(cct);
  std::vector<uint32_t> modes;

  const std::vector<uint32_t> crc_secure = { STONE_CON_MODE_CRC,
					     STONE_CON_MODE_SECURE };
  const std::vector<uint32_t> secure_crc = { STONE_CON_MODE_SECURE,
					     STONE_CON_MODE_CRC };
  const std::vector<uint32_t> secure = { STONE_CON_MODE_SECURE };

  cct->_conf.set_val(
    "enable_experimental_unrecoverable_data_corrupting_features", "*");

  // baseline: everybody agrees
  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  cct->_conf.set_val("ms_cluster_mode", "crc secure");
  cct->_conf.set_val("ms_service_mode", "crc secure");
  cct->_conf.set_val("ms_client_mode", "crc secure");
  cct->_conf.set_val("ms_mon_cluster_mode", "crc secure");
  cct->_conf.set_val("ms_mon_service_mode", "crc secure");
  cct->_conf.set_val("ms_mon_client_mode", "crc secure");
  cct->_conf.apply_changes(NULL);

  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);
  ASSERT_EQ((uint32_t)STONE_CON_MODE_CRC, reg.pick_mode(STONE_ENTITY_TYPE_OSD,
						       STONE_AUTH_STONEX,
						       crc_secure));

  // what mons prefer secure, internal to mon cluster only
  cct->_conf.set_val("ms_mon_cluster_mode", "secure");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_OSD);

  /* mon/mgr are treated the same, and relevant config is ms_mon_cluster_mode */
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MON);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  // how all cluster -> mon connections secure?
  cct->_conf.set_val("ms_mon_service_mode", "secure");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_OSD);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MON);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MDS, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);


  // how about client -> mon connections?
  cct->_conf.set_val("ms_mon_client_mode", "secure");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  //  ms_mon)client_mode doesn't does't affect daemons, though...
  cct->_conf.set_val("ms_mon_service_mode", "crc secure");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MON);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MDS, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  // how about all internal cluster connection secure?
  cct->_conf.set_val("ms_cluster_mode", "secure");
  cct->_conf.set_val("ms_mon_service_mode", "secure");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_OSD);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_CLIENT, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MGR);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MDS, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_CLIENT, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MDS);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_CLIENT, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MON);
  reg.get_supported_modes(STONE_ENTITY_TYPE_CLIENT, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  // how about all connections to the cluster?
  cct->_conf.set_val("ms_service_mode", "secure");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MDS, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, crc_secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_OSD);
  reg.get_supported_modes(STONE_ENTITY_TYPE_CLIENT, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MGR);
  reg.get_supported_modes(STONE_ENTITY_TYPE_CLIENT, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MDS, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  cct->_set_module_type(STONE_ENTITY_TYPE_MDS);
  reg.get_supported_modes(STONE_ENTITY_TYPE_CLIENT, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  // client forcing things?
  cct->_conf.set_val("ms_cluster_mode", "crc secure");
  cct->_conf.set_val("ms_service_mode", "crc secure");
  cct->_conf.set_val("ms_client_mode", "secure");
  cct->_conf.set_val("ms_mon_cluster_mode", "crc secure");
  cct->_conf.set_val("ms_mon_service_mode", "crc secure");
  cct->_conf.set_val("ms_mon_client_mode", "secure");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MDS, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure);

  // client *preferring* secure?
  cct->_conf.set_val("ms_cluster_mode", "crc secure");
  cct->_conf.set_val("ms_service_mode", "crc secure");
  cct->_conf.set_val("ms_client_mode", "secure crc");
  cct->_conf.set_val("ms_mon_cluster_mode", "crc secure");
  cct->_conf.set_val("ms_mon_service_mode", "crc secure");
  cct->_conf.set_val("ms_mon_client_mode", "secure crc");
  cct->_conf.apply_changes(NULL);

  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MON, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure_crc);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MGR, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure_crc);
  reg.get_supported_modes(STONE_ENTITY_TYPE_OSD, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure_crc);
  reg.get_supported_modes(STONE_ENTITY_TYPE_MDS, STONE_AUTH_STONEX, &modes);
  ASSERT_EQ(modes, secure_crc);

  // back to normalish, for the benefit of the next test(s)
  cct->_set_module_type(STONE_ENTITY_TYPE_CLIENT);  
}
