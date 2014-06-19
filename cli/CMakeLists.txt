################################
# CLI
################################

include_directories(${SRC_LIB_DIR})
include_directories(${GLIB2_INCLUDE_DIR})
include_directories(${LIBSEARPC_INCLUDE_DIR})
# Add ccnet-tool
add_executable(ccnet-tool ccnet-tool.c)
target_link_libraries(ccnet-tool
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0
  ${LIBSEARPC_LIBRARIES}
  libccnet
  )

# Add ccnet-servtool
add_executable(ccnet-servtool ccnet-servtool.c)
target_link_libraries(ccnet-servtool
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0
  ${LIBSEARPC_LIBRARIES}
  libccnet
  )

# Install
install(TARGETS ccnet-tool ccnet-servtool
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  )
