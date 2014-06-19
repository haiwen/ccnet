################################
# NET
################################

set(SRC_NET_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(SRC_NET_COMMON_PROC_DIR ${SRC_NET_COMMON_DIR}/processors)
include_directories(${SRC_NET_COMMON_DIR})

# Abandoned
set(SRC_NET_CLUSTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cluster)
if(CCNET_ENABLE_CLUSTER)
  add_subdirectory(cluster)
endif(CCNET_ENABLE_CLUSTER)

# Client
set(SRC_NET_DAEMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/daemon)
if(CCNET_ENABLE_CLIENT)
  add_subdirectory(daemon)
endif(CCNET_ENABLE_CLIENT)

# Server
set(SRC_NET_SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/server)
if(CCNET_ENABLE_SERVER)
  add_subdirectory(server)
endif(CCNET_ENABLE_SERVER)
