#!/bin/bash

set -e -x

PREFIX=$HOME/opt/local
export CPPFLAGS="$CPPFLAGS -I$PREFIX/include"
export LDFLAGS="$LDFLAGS -L$PREFIX/lib -L$PREFIX/lib64"
export PATH="$PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PREFIX/lib64/pkgconfig:$PKG_CONFIG_PATH"

install_deps() {
    pushd $HOME

    # download precompiled libzdb
    zdb_bin=libzdb-bin_2.11.1.tar.gz
    wget https://dl.bintray.com/lins05/generic/libzdb-bin/$zdb_bin
    tar xf $zdb_bin
    sed -i -e "s|prefix=/opt/local|prefix=$HOME/opt/local|g" $HOME/opt/local/lib/pkgconfig/zdb.pc
    find $HOME/opt

    # build libsearpc
    git clone --depth=1 --branch=master git://github.com/haiwen/libsearpc.git /tmp/libsearpc
    pushd /tmp/libsearpc
    ./autogen.sh
    ./configure --prefix=$PREFIX
    make -j2 && make install
    popd

    popd
}

install_deps

./configure --prefix=$PREFIX --enable-server --enable-client
make -j2
make check
