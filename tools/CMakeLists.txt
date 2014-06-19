################################
# TOOLS
################################

include_directories(${SRC_LIB_DIR})
include_directories(${GLIB2_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
# Add ccnet-init
add_executable(ccnet-init ccnet-init.c)
target_link_libraries(ccnet-init
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0
  ${OPENSSL_LIBRARIES}
  libccnetd
  )

# Install
install(TARGETS ccnet-init
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  )

# Install ccnet-admin.py
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ccnet-admin.py
  DESTINATION ${BIN_INSTALL_DIR}
  )
