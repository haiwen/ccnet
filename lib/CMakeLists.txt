################################
# LIB
################################

# Generate rpc-table
set(SRC_LIB_RPCTABLE_H
  ${CMAKE_CURRENT_SOURCE_DIR}/searpc-signature.h
  ${CMAKE_CURRENT_SOURCE_DIR}/searpc-marshal.h)
add_custom_command(
  PRE_BUILD
  OUTPUT ${SRC_LIB_RPCTABLE_H}
  COMMAND ${PYTHON_EXECUTABLE} ${SearpcCodegen_EXECUTABLE} rpc_table.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

# Generate vala
set(SRC_LIB_CCNETOBJ_H ${CMAKE_CURRENT_SOURCE_DIR}/ccnet-object.h)
add_custom_command(
  PRE_BUILD
  OUTPUT ${SRC_LIB_CCNETOBJ_H}
  COMMAND ${Valac_EXECUTABLE} --pkg posix ccnetobj.vala -C -H ${SRC_LIB_CCNETOBJ_H}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
set(SRC_LIB_CCNETOBJ ${CMAKE_CURRENT_SOURCE_DIR}/ccnetobj.c)
add_custom_command(
  PRE_BUILD
  OUTPUT ${SRC_LIB_CCNETOBJ}
  COMMAND ${Valac_EXECUTABLE} --pkg posix ccnetobj.vala -C -o ${SRC_LIB_CCNETOBJ}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

add_definitions(-DG_LOG_DOMAIN=\"Ccnet\")
include_directories(${JANSSON_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${GLIB2_INCLUDE_DIR})
include_directories(${SQLITE3_INCLUDE_DIR})
include_directories(${UUID_INCLUDE_DIR})
include_directories(${SRC_INCLUDE_CCNET_DIR})
include_directories(${SRC_LIB_DIR})
include_directories(${LIBEVENT_INCLUDE_DIR})
include_directories(${LIBSEARPC_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SRC_LIB
  ccnet-client.c packet-io.c libccnet_utils.c
  message.c proc-factory.c processor.c buffer.c
  peer.c sendcmd-proc.c mqclient-proc.c invoke-service-proc.c
  marshal.c mainloop.c cevent.c timer.c ccnet-session-base.c job-mgr.c
  rpcserver-proc.c ccnetrpc-transport.c threaded-rpcserver-proc.c
  async-rpc-proc.c ccnet-rpc-wrapper.c client-pool.c
  ${SRC_LIB_CCNETOBJ}
  )
set(SRC_LIBD
  utils.c db.c job-mgr.c rsa.c bloom-filter.c marshal.c
  net.c timer.c ccnet-session-base.c
  ${SRC_LIB_CCNETOBJ}
  )

# Add libccnet
add_library(libccnet ${CCNET_LIBTYPE}
  ${SRC_LIB_RPCTABLE_H}
  ${SRC_LIB_CCNETOBJ_H} ${SRC_LIB_CCNETOBJ}
  ${SRC_LIB_H} ${SRC_LIB}
  )
target_link_libraries(libccnet
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0
  ${OPENSSL_LIBRARIES}
  ${UUID_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${LIBEVENT_LIBRARIES}
  ${LIBSEARPC_LIBRARIES}
  ${JANSSON_LIBRARIES}
  )
set_target_properties(libccnet PROPERTIES COMPILE_FLAGS "-DCCNET_LIB")
set_target_properties(libccnet PROPERTIES OUTPUT_NAME libccnet)
set_target_properties(libccnet PROPERTIES SOVERSION 0)
set_target_properties(libccnet PROPERTIES PREFIX "")

# Add libccnetd
add_library(libccnetd STATIC
  ${SRC_LIB_RPCTABLE_H}
  ${SRC_LIB_CCNETOBJ_H} ${SRC_LIB_CCNETOBJ}
  ${SRC_LIB_H} ${SRC_LIBD}
  )
target_link_libraries(libccnetd
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0
  ${OPENSSL_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${LIBEVENT_LIBRARIES}
  ${UUID_LIBRARIES}
  ${LIBSEARPC_LIBRARIES}
  ${JANSSON_LIBRARIES}
  )
set_target_properties(libccnetd PROPERTIES OUTPUT_NAME libccnetd)
set_target_properties(libccnetd PROPERTIES PREFIX "")

# Install library
install(TARGETS libccnet
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  )
# Install missing headers
install(FILES ${SRC_LIB_CCNETOBJ_H}
  DESTINATION ${INCLUDE_INSTALL_DIR}/ccnet
  )

# PKG-CONFIG FILE
set(LIBCCNET_PC_REQUIRES "")
set(LIBCCNET_PC_REQUIRES "${CCNET_PC_REQUIRES} gobject-2.0 gio-2.0 jansson openssl")
set(LIBCCNET_PC_LIBS "")
set(LIBCCNET_PC_LIBS "${CCNET_PC_LIBS} -lglib-2.0 -ljansson -lssl")
set(LIBCCNET_VERSION_STRING ${CCNET_VERSION_STRING})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libccnet.pc.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/libccnet.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libccnet.pc
  DESTINATION ${LIB_INSTALL_DIR}/pkgconfig
  )

