# Operators Module - Remote Commands Execution

:Author: Marco Massenzio (marco@alertavert.com)
:Revision: 0.1
:Created: 2015-12-15
:Updated: 2014-12-15

## Motivation

We wish to enable operators to execute remotely arbitrary commands on Apache
Mesos Agent/Master nodes, out-of-band from the normal task execution framework.

For more information, please consult the references listed in [Prerequisites]
[#Prerequsites].

## Goals

This is a Mesos "anonymous" module which can be loaded using the enclosed
`modules.json` descriptor; this module will add [endpoints][#Endpoints] such
that:

- we can monitor the status (active/inactive) of this module;
- we can remotely execute arbitrary (shell) commands, with optional arguments;
- we can retrieve the outcome of the command;
- we can terminate previously launched processes.

This is not meant to offer full remote shell functionality, however.


## API

```
  POST /remote/execute

  Request format: RemoteCommandInfo
```

Executes a command on the Agent.

```
  GET /remote/status

  Request format: none
```

Returns a `200 OK` response if this module is active.


## Build

### Prerequisites

You obviously need [Apache Mesos](http://mesos.apache.org) to build this project: in particular,
you will need both the includes (`mesos`, `stout` and `libprocess`) and the shared `libmesos`
library.

In addition, Mesos needs access to `picojson.h` and a subset of the `boost` header files: see the
[3rdparty](https://github.com/apache/mesos/tree/master/3rdparty/libprocess/3rdparty) folder in
the mirrored github repository for Mesos, and in particular the
[boost-1.53.0.tar.gz](https://github.com/apache/mesos/blob/master/3rdparty/libprocess/3rdparty/boost-1.53.0.tar.gz)
archive.

The "easiest" way to obtain all the prerequisites would probably be to clone the Mesos
repository, build mesos and then install it in a local folder that you will then need to
configure using the `LOCAL_INSTALL_DIR` property (see `Build` below).

Finally, you need the `libsvn` library (this is required by Mesos): on OSX this can be obtained
using `brew`:

```
    brew install svn
```


### Google Protocol Buffers

Apache Mesos makes extensive use of [Protocol Buffers](https://developers.google.com/protocol-buffers)
and this project uses them too (see the `proto/execute.proto` file).

In order to build this module, you will need to download, build and install Google's protobuf
version **2.5.0** (this is the most recent version used by Mesos - using a more recent one will
cause compile and runtime errors) - see the link above for more details.

We assume that the `protoc` binary will be installed in the same `LOCAL_INSTALL_DIR` location;
assuming that this is set to be the `$LOCAL_INSTALL` env variable:

```
    cd protobuf-2.5.0/
    ./configure --prefix $LOCAL_INSTALL
    make -j 4 && make install
```
see the protobuf documentation for more info.


### CMake

This module uses [cmake](https://cmake.org) to build the module and the tests; there are
currently two targets: `execmod` and `execmod_test`, the library and the tests, respectively.

It also needs a number of libraries and includes (see `Prerequisites` above) that we assume to be
 in the `include` and `lib` subdirectories of a directory located at `LOCAL_INSTALL_DIR`; this
 can be set either using an environment variable (`$LOCAL_INSTALL`) or a `cmake` property
 (`-DLOCAL_INSTALL_DIR`):

```
    mkdir build && cd build
    cmake -DLOCAL_INSTALL_DIR=${HOME}/usr_local `pwd`/..
    make
    ./execmod_test
```

## Usage

```
TODO: add instructions to launch the Mesos Agent with this module loaded
```

### Tests

Launch the `execmod_test` binary:

```
    cd build && ./execmod_test
```

In due course, we may add a `make check` target to achieve the same.
