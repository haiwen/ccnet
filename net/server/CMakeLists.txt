################################
# NET-SERVER
################################

set(SRC_NET_COMMON_FOR_SERVER
  ../common/ccnet-db.c
  ../common/session.c ../common/peer-mgr.c ../common/packet-io.c
  ../common/message.c ../common/perm-mgr.c
  ../common/log.c ../common/peer.c ../common/algorithms.c
  ../common/handshake.c ../common/processor.c
  ../common/getgateway.c ../common/connect-mgr.c
  ../common/message-manager.c
  ../common/proc-factory.c
  ../common/ccnet-config.c
  ../common/rpc-service.c
  ../common/peermgr-message.c
  ../common/processors/sendmsg-proc.c ../common/processors/rcvmsg-proc.c
  ../common/processors/rcvcmd-proc.c
  ../common/processors/putpubinfo-proc.c
  ../common/processors/getpubinfo-proc.c
  ../common/processors/keepalive2-proc.c
  ../common/processors/mqserver-proc.c
  ../common/processors/service-proxy-proc.c
  ../common/processors/service-stub-proc.c
  ../common/processors/rpcserver-proc.c
  ../common/processors/threaded-rpcserver-proc.c
  ../common/processors/echo-proc.c
  ../common/processors/sendsessionkey-proc.c
  ../common/processors/recvsessionkey-proc.c
  ../common/processors/sendsessionkey-v2-proc.c
  ../common/processors/recvsessionkey-v2-proc.c
  )

add_definitions(-DCCNET_SERVER)
include_directories(${JANSSON_INCLUDE_DIR})
include_directories(${GLIB2_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${SRC_NET_COMMON_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${SRC_INCLUDE_CCNET_DIR})
include_directories(${SRC_LIB_DIR})
include_directories(${SQLITE3_INCLUDE_DIR})
include_directories(${LIBSEARPC_INCLUDE_DIR})
include_directories(${LIBEVENT_INCLUDE_DIR})
include_directories(${LIBZDB_INCLUDE_DIR})
set(SRC_NET_SERVER
  ccnet-server.c server-session.c user-mgr.c group-mgr.c org-mgr.c
  )

# Add ccnet
add_executable(ccnet-server
  ${SRC_NET_SERVER}
  ${SRC_NET_COMMON_FOR_SERVER}
  )
target_link_libraries(ccnet-server
  ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0
  ${OPENSSL_LIBRARIES} ${LIBEVENT_LIBRARIES}
  ${LIBZDB_LIBRARIES} ${LIBSEARPC_LIBRARIES}
  ${UUID_LIBRARIES} ${SQLITE3_LIBRARIES}
  libccnetd pthread
  )

# Install
install(TARGETS ccnet-server
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  )
