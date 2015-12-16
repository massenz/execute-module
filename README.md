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

## Prerequisites

## Endpoints

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


## Usage
