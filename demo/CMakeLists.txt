################################
# DEMO
################################
set(SRC_DEMO_PROC_H processors/echo-proc.h)
set(SRC_DEMO_PROC processors/echo-proc.c)

# Add demo-server
add_executable(ccnet-demo-server
  ${SRC_DEMO_PROC_H} ${SRC_DEMO_PROC}
  ccnet-demo-server.c
  )
target_link_libraries(ccnet-demo-server
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0 ${JANSSON_LIBRARIES}
  ${LIBSEARPC_LIBRARIES}
  libccnet
  )

# Add demo-client
add_executable(ccnet-demo-client ccnet-demo-client.c)
target_link_libraries(ccnet-demo-client
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0 ${JANSSON_LIBRARIES}
  ${LIBSEARPC_LIBRARIES}
  libccnet
  )
