[![Build Status](https://secure.travis-ci.org/haiwen/ccnet.svg?branch=master)](http://travis-ci.org/haiwen/ccnet)

Ccnet is a framework for writing networked applications in C. It
provides the following basic services:

1. Peer identification
2. Connection Management
3. Service invocation
4. Message sending

In ccnet network, there are two types of nodes, i.e., client and server.
Server has the following functions:

1. User management
2. Group management
3. Cluster management

Dependency
==========

The following packages are required to build ccnet:

    valac >= 0.8
    libsearpc

Compile
=======

To compile the client components, just

    ./autogen.sh && ./configure && make && make install
