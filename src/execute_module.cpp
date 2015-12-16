/*
 * This file is (c) 2015 AlertAvert.com.  All rights reserved.
 */

#include <iostream>

#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/module/anonymous.hpp>

#include <stout/json.hpp>
#include <stout/os.hpp>
#include <stout/protobuf.hpp>
#include <stout/try.hpp>

#include <process/help.hpp>
#include <process/process.hpp>
#include <process/subprocess.hpp>

#include "cmdexecute.hpp"
#include "config.h"
#include "execute.pb.h"


using namespace std;

using namespace mesos;
using namespace mesos::modules;

using namespace process;

using execute::RemoteCommandResult;

class RemoteExecutionProcess : public Process<RemoteExecutionProcess>
{
public:
  RemoteExecutionProcess() : ProcessBase("remote") { }
  virtual ~RemoteExecutionProcess() { }

private:
  Future<http::Response> activate(const http::Request &request);

  Future<http::Response> active(const http::Request &request);

  void initialize()
  {
    route("/execute",
          HELP(
              TLDR("Execute a remote command on the agent."),
              USAGE("/remote/execute"),
              DESCRIPTION("See: TODO: Insert URL with documentation",
                          "HTTP POST only.",
                          "Content-Type: JSON",
                          "Use the following format:",
                          "{",
                          "    \"command\": <shell cmd to execute>",
                          "    \"arguments\": [",
                          "            ... list of command args ...",
                          "    ],",
                          "    \"shell\": true | false",
                          "}",
                          "(see RemoteCommandInfo for more details)."
              )
          ),
          &RemoteExecutionProcess::activate);
    route("/status",
          HELP(
              TLDR("Status of this endpoint."),
              USAGE("/remote/status"),
              DESCRIPTION("Status of this endpoint, mostly for monitoring purposes only",
                          "HTTP GET only. No parameters."
              )
          ),
          &RemoteExecutionProcess::active
    );
  }
};

Future<http::Response> RemoteExecutionProcess::activate(
    const http::Request &request)
{
  if (request.method != "POST") {
    return http::MethodNotAllowed(
        "Expecting a 'POST' request, received '" + request.method + "'");
  }

  auto contentType = request.headers.get("Content-Type");
  if (contentType.isSome() && contentType.get() != "application/json") {
    return http::UnsupportedMediaType("Currently only JSON is supported");
  }
  Try<JSON::Value> body_ = JSON::parse(request.body);
  if (body_.isError()) {
    return http::BadRequest(body_.error());
  }

  Try<execute::RemoteCommandInfo> info =
      ::protobuf::parse<execute::RemoteCommandInfo>(body_.get());
  if (info.isError()) {
    return http::BadRequest(info.error());
  }
  auto commandInfo = info.get();

  vector<string> args;
  for (int i = 0; i < commandInfo.arguments_size(); ++i) {
    args.push_back(commandInfo.arguments(i));
  }

  LOG(INFO) << "Running '" << commandInfo.command() << "'; "
            << "with " << stringify(args);
  execute::CommandExecute cmd(commandInfo);

  // TODO(marco): generate unique ID to track outcome of job
   LOG(INFO) << "Running process [" << cmd.pid() << "]";

  Future<RemoteCommandResult> result_ = cmd.execute();
  result_.then([commandInfo](const Future<RemoteCommandResult>& future) {
        if (future.isFailed()) {
          LOG(ERROR) << "The execution failed: " << future.failure();

          // TODO(marco): reconcile failure with pending jobs.
          return Nothing();
        }
        auto result = future.get();
        LOG(INFO) << "Result of '" << commandInfo.command() << "'was: "
                  << result.stdout();

        // TODO(marco): update status of pending job.
        return Nothing();
      }).after(Seconds(30), [commandInfo, cmd](const Future<Nothing> &future) {
        LOG(ERROR) << "Command " << commandInfo.command()
                   << " timed out, giving up";

        // TODO(marco): update status of job to timed out.
        cmd.cleanup();
        return Nothing();
      });

  http::Response response = http::OK("{\"result\": \"OK\", \"pid\": \"" +
                                     stringify(cmd.pid()) + "\"}");
  response.headers["Content-Type"] = "application/json";
  return response;
}

Future<http::Response> RemoteExecutionProcess::active(
    const http::Request &request)
{
  return http::OK("Remote execution is active");
}


class RemoteExecutionAnonymous : public Anonymous
{
public:
  RemoteExecutionAnonymous()
  {
    process = new RemoteExecutionProcess();
    spawn(process);
  }

  virtual ~RemoteExecutionAnonymous()
  {
    terminate(process);
    wait(process);
    delete process;
  }

private:
  RemoteExecutionProcess* process;
};

namespace {

// Module "main".
Anonymous* createAnonymous(const Parameters& parameters)
{
  return new RemoteExecutionAnonymous();
}

}


// Declares a life cycle module named 'RemoteExecution'.
Module<Anonymous> MODULE_NAME(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "AlertAvert.com",
    "marco@alertavert.com",
    "RemoteExecution module",
    NULL,
    createAnonymous);

