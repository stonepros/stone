if(WITH_SELINUX)
  install(FILES stone_selinux.8
    DESTINATION ${STONE_MAN_DIR}/man8)
endif()
