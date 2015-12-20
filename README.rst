============================================
Operators Module - Remote Commands Execution
============================================

:Author: Marco Massenzio (marco@alertavert.com)
:Revision: 0.2
:Created: 2015-12-15
:Updated: 2015-12-20

Motivation
----------

We wish to enable operators to execute remotely arbitrary commands on Apache
Mesos Agent/Master nodes, out-of-band from the normal task execution framework.

For more information, please consult the references listed in `Prerequisites`_.

Goals
-----

This is a `Mesos anonymous module`_ which can be loaded using the enclosed
``modules.json`` descriptor; this module will add `Endpoints`_ such
that:

- we can monitor the status (active/inactive) of this module;
- we can remotely execute arbitrary (shell) commands, with optional arguments;
- we can retrieve the outcome of the command;
- we can terminate previously launched processes.

This is **not** meant to offer full remote shell functionality, however.

License & Allowed Use
---------------------

The code in this repository is **not released under an Open Source license**.
 
This *may* change in due course, but currently the only allowed use is for 
training and learning purposes: the code is meant to be used by developers of
 Mesos Modules to learn how to create their own module.
 
We explicitly disallow usage of this code, or any derivation thereof, in any 
commercial software deployed in Production for use by external users 
(regardless of whether the intended use).

If you wish to use this code in Production and/or modify it, please contact 
the author directly at the following address::

  marco (at) alertavert (dot) com


Endpoints
---------

API
^^^

::

  POST /remote/execute

  Request format: RemoteCommandInfo

Executes a command on the Agent.

::

  GET /remote/status

  Request format: none

Returns a `200 OK` response if this module is active.


Build
-----

Prerequisites
^^^^^^^^^^^^^

You obviously need `Apache Mesos <http://mesos.apache.org>`_ to build this 
project: in particular, you will need both the includes (``mesos``, ``stout`` 
and ``libprocess``) and the shared ``libmesos.so`` library.

In addition, Mesos needs access to ``picojson.h`` and a subset of the ``boost`` 
header files: see the 
`3rdparty <https://github.com/apache/mesos/tree/master/3rdparty/libprocess/3rdparty>`_ 
folder in the mirrored github repository for Mesos, and in particular the
`boost-1.53.0.tar.gz <https://github.com/apache/mesos/blob/master/3rdparty/libprocess/3rdparty/boost-1.53.0.tar.gz>`_
archive.

The "easiest" way to obtain all the prerequisites would probably be to clone the Mesos
repository, build mesos and then install it in a local folder that you will then need to
configure using the ``LOCAL_INSTALL_DIR`` property (see `CMake`_ below).

Finally, you need the ``libsvn`` library (this is required by Mesos): on OSX 
this can be obtained using ``brew``::

    brew install svn

Google Protocol Buffers
^^^^^^^^^^^^^^^^^^^^^^^

Apache Mesos makes extensive use of `Protocol Buffers <https://developers.google.com/protocol-buffers>`_
and this project uses them too (see the ``proto/execute.proto`` file).

In order to build this module, you will need to download, build and install Google's protobuf
version **2.5.0** (this is the most recent version used by Mesos - using a more recent one will
cause compile and runtime errors) - see the link above for more details.

We assume that the ``protoc`` binary will be installed in the same ``LOCAL_INSTALL_DIR`` location;
assuming that this is set to be the ``$LOCAL_INSTALL`` env variable::

    cd protobuf-2.5.0/
    ./configure --prefix $LOCAL_INSTALL
    make -j 4 && make install

see the protobuf documentation for more info.


CMake
^^^^^

This module uses `cmake <https://cmake.org`_ to build the module and the 
tests; there are currently two targets: ``execmod`` and ``execmod_test``, the
library and the tests, respectively.

It also needs a number of libraries and includes (see `Prerequisites`_) that 
we assume to be in the ``include`` and ``lib`` subdirectories of a directory 
located at ``${LOCAL_INSTALL_DIR}``; this can be set either using an environment 
variable (``$LOCAL_INSTALL``) or a ``cmake`` property (``-DLOCAL_INSTALL_DIR``)::

    mkdir build && cd build
    cmake -DLOCAL_INSTALL_DIR=/path/to/usr/local ..
    make

    # If you want to run the tests in the execmod_test target:
    ctest


Usage
-----

::

  TODO: add instructions to launch the Mesos Agent with this module loaded

Tests
-----

Run ``ctest`` from the ``build`` directory, or launch the `execmod_test` 
binary::

    cd build && ./execmod_test
    
    
.. _Mesos anonymous module: http://mesos.apache.org/documentation/latest/modules/
