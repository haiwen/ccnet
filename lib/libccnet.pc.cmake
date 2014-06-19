prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@/@BIN_INSTALL_DIR@
libdir=@CMAKE_INSTALL_PREFIX@/@LIB_INSTALL_DIR@
includedir=@CMAKE_INSTALL_PREFIX@/@INCLUDE_INSTALL_DIR@

Name: libccnet
Description: Client library for accessing ccnet service.
Version: @LIBCCNET_VERSION_STRING@
Requires: @LIBCCNET_PC_REQUIRES@
Libs.private: @LIBCCNET_PC_LIBS@
Libs: -L${libdir} -lccnet @LIBCCNET_LIBS@
Cflags: -I${includedir} @LIBCCNET_CFLAGS@
