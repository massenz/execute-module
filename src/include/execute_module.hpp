// This file is (c) 2015 AlertAvert.com.  All rights reserved.

#include <iostream>

#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/module/anonymous.hpp>

#include <stout/duration.hpp>
#include <stout/json.hpp>
#include <stout/os.hpp>
#include <stout/protobuf.hpp>

#include <process/help.hpp>

#include "cmdexecute.hpp"
#include "config.h"


// The name of the (optional) environment variable for the timeout.
#define TIMEOUT_ENV_VAR "EXECUTE_TIMEOUT_SEC"

// Unless specified in the `Environment` field of the `CommandInfo` protobuf,
// as the variable `EXECUTE_TIMEOUT_SEC`, this value will be used for the
// command execution timeout.
#define TIMEOUT_DEFAULT_SEC 30


using std::string;

using process::Future;
using process::Process;
using process::ProcessBase;

// See the code style guide for why this is preferred to
// `using namespace process`.
// http://mesos.apache.org/documentation/latest/c++-style-guide/
namespace http = process::http;

using execute::RemoteCommandResult;


/**
 * Implements the Execute Module API.
 *
 * This is a running `process` that will respond to HTTP requests for the
 * `/remote/` command execution requests.
 *
 * See the `README` file for more details about the API, and the Apache Mesos
 * documentation about Anonymous modules for information about how to
 * instantiate this class within a running Mesos Agent.
 *
 * @see http://mesos.apache.org/documentation/latest/modules/
 */
class RemoteExecutionProcess : public Process<RemoteExecutionProcess>
{
public:
  RemoteExecutionProcess(
      const string& workDir,
      const string& sandboxDir
  ) : ProcessBase("remote"), workDir_(workDir), sandboxDir_(sandboxDir) {}

  virtual ~RemoteExecutionProcess() {}

private:
  Future<http::Response> execute(const http::Request &request);

  Future<http::Response> status(const http::Request &request);

  Future<http::Response> getTaskInfo(const http::Request &request);

  Future<http::Response> getAllTasks();

  void initialize();

  /**
   * Extracts the value for the timeout, in seconds.
   *
   * It will look up for a `TIMEOUT_SEC` variable defined in the
   * `Environment` field of the `commandInfo` and if it does not exist it
   * will use the `TIMEOUT_DEFAULT_SEC` default value.
   *
   * @param commandInfo the command to execute.
   * @return the timeout value, in seconds.
   */
  const Seconds getTimeout(const mesos::CommandInfo& commandInfo)
    const;

  std::map<pid_t, Future<RemoteCommandResult>> processes_;
  string workDir_;
  string sandboxDir_;
};


class RemoteExecutionAnonymous : public mesos::modules::Anonymous
{
public:
  RemoteExecutionAnonymous() : process(nullptr) {}

  virtual ~RemoteExecutionAnonymous()
  {
    if (process != nullptr) {
      LOG(INFO) << "Terminating module " << MODULE_NAME_STR << "...";
      terminate(process);
      wait(process);
      LOG(INFO) << "Module shutdown complete, deleting process";
      delete process;
    }
  }

  /**
   * Retrieves a "runtime context" from the Agent.
   *
   * Parses the flags from the Agent to retrieve information about the
   * running environment; in particular, retrieves Mesos working directory
   * and the Sandbox directory.
   *
   * @see MESOS-4253 for more details.
   * https://issues.apache.org/jira/browse/MESOS-4253
   *
   * @param flags the Agent's runtime options.
   */
  virtual void initialize(const flags::FlagsBase& flags)
  {
    string workDir;
    string sandboxDir;

    LOG(INFO) << "Executing init() for module; parsing runtime flags";
    for(auto flag : flags) {
      string name = flag.first;
      Option<string> value = flag.second.stringify(flags);
      if (name == "work_dir" && value.isSome()) {
        workDir = value.get();
      } else if (name == "sandbox_directory" && value.isSome()) {
        sandboxDir = value.get();
      }
    }
    LOG(INFO) << "Configured work dir to [" << workDir
              << "] and Sandbox dir to [" << sandboxDir << "]";
    process = new RemoteExecutionProcess(workDir, sandboxDir);
    spawn(process);
  }

private:
    RemoteExecutionProcess* process;
};
