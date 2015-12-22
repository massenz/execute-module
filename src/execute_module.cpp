// This file is (c) 2015 AlertAvert.com.  All rights reserved.

#include <iostream>

#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/module/anonymous.hpp>

#include <stout/json.hpp>
#include <stout/os.hpp>
#include <stout/protobuf.hpp>

#include <process/help.hpp>

#include "cmdexecute.hpp"
#include "config.h"


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

  Future<http::Response> getTaskInfo(const http::Request &request);

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
              DESCRIPTION("Status of this endpoint, mostly for monitoring "
                          "purposes only",
                          "HTTP GET only. No parameters."
              )
          ),
          &RemoteExecutionProcess::active
    );
    route("/task",
          HELP(
              TLDR("Current state of the task; or its outcome."),
              USAGE("/remote/task  {pid: <PID>}"),
              DESCRIPTION("Current state of the command.",
                          "The only argument given (`pid`) should match the ",
                          "returned PID from a previous /remote/execute "
                          "invocation.",
                          "It will return the corresponding RemoteCommandResult "
                          "information, JSON-encoded."
                          "HTTP GET only."
              )
          ),
          &RemoteExecutionProcess::getTaskInfo
    );
  }

  std::map<pid_t, Future<RemoteCommandResult>> processes_;

    Future<http::Response> getAllTasks();
};

Future<http::Response> RemoteExecutionProcess::activate(
    const http::Request &request)
{
  if (request.method != "POST") {
    return http::MethodNotAllowed({"POST"},
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

  execute::CommandExecute cmd(commandInfo);
  LOG(INFO) << "Running '" << commandInfo.command() << "' "
            << "with args " << stringify(args)
            << "; as PID [" << cmd.pid() << "]";

  Future<RemoteCommandResult> result_ = cmd.execute();

  // If we are unable to get the subprocess initialized, the Future will fail
  // immediately: note that this is fundamentally different from a command
  // failure (which would result in a non-zero exit code).
  if (result_.isFailed()) {
    return http::InternalServerError(result_.failure());
  }

  // Inserting the Future in the map that keeps track of progress and is used
  // when querying about the state of a task (/remote/task).
  processes_[cmd.pid()] = result_;

  result_.then([commandInfo](const Future<RemoteCommandResult>& future) {
        if (future.isFailed()) {
          LOG(ERROR) << "The execution failed: " << future.failure();
          return Nothing();
        }
        auto result = future.get();
        LOG(INFO) << "Result of '" << commandInfo.command() << "' was "
            << (result.exitcode() == EXIT_SUCCESS ? "successful" : " an error")
            << (result.stdout().empty() ? "" : "\n" + result.stdout());
        return Nothing();
      }).after(Seconds(commandInfo.timeout()),
               [commandInfo, cmd](const Future<Nothing>& future) {
        LOG(ERROR) << "Command " << commandInfo.command()
                   << " timed out after " << commandInfo.timeout()
                   << " seconds. Aborting process " << cmd.pid();

        // TODO(marco): update status of job to timed out.
        // Currently, we notice a timed out job because its `signaled` status
        // is `true` and the `exitCode` is 9 (SIGTERM).
        cmd.cleanup();
        return Nothing();
      });

  http::Response response = http::OK(
      "{\"result\": \"started\", \"pid\": " + stringify(cmd.pid()) + "}");
  response.headers["Content-Type"] = "application/json";
  return response;
}

Future<http::Response> RemoteExecutionProcess::active(
    const http::Request &request)
{
  JSON::Object body;
  body.values["result"] = "ok";
  body.values["status"] = "active";
  body.values["release"] = RELEASE_STR;

  http::Response response = http::OK(stringify(body));
  response.headers["Content-Type"] = "application/json";
  return response;
}

Future<http::Response> RemoteExecutionProcess::getTaskInfo(
    const http::Request &request)
{
  if (request.method == "GET") {
    return getAllTasks();
  }

  if (request.method != "POST") {
    return http::MethodNotAllowed(
        {"GET", "POST"},
        "Expecting a 'POST' request, received '" + request.method + "'");
  }

  auto contentType = request.headers.get("Content-Type");
  if (contentType.isSome() && contentType.get() != "application/json") {
    return http::UnsupportedMediaType("Currently only JSON is supported");
  }

  Try<JSON::Object> body_ = JSON::parse<JSON::Object>(request.body);
  if (body_.isError()) {
    return http::BadRequest(body_.error());
  }
  auto body = body_.get();
  if (body.values.count("pid") == 0) {
    return http::BadRequest("No PID specified in JSON body (\"pid\")");
  }

  pid_t pid = body.values["pid"].as<JSON::Number>().as<int>();
  LOG(INFO) << "Retrieving outcome for PID '" << pid << "'";

  if (processes_.count(pid) > 0) {
    Future<RemoteCommandResult> result_ = processes_[pid];
    if (result_.isReady()) {
      return http::OK(JSON::protobuf(result_.get()));
    } else if (result_.isFailed()) {
      return http::OK(
          "{\"result\": \"" + result_.failure() + "\", \"pid\": " +
          stringify(pid) + "}");
    } else {
      return http::OK(
          "{\"result\": \"pending\", \"pid\": " + stringify(pid) + "}");
    }
  }
  return http::NotFound("{\"pid\": " + stringify(pid) + "}");
}

Future<http::Response> RemoteExecutionProcess::getAllTasks()
{
  std::vector<pid_t> pids;
  for (auto proc : processes_) {
    pids.push_back(proc.first);
  }
  auto response = http::OK("{\"ids\": " + stringify(pids) + "}");
  response.headers["Content-Type"] = "application/json";
  return response;
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
