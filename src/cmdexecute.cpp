// This file is (c) 2015 AlertAvert.com.  All rights reserved.



#include "cmdexecute.hpp"

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <glog/logging.h>

#include <process/collect.hpp>
#include <process/future.hpp>
#include <process/subprocess.hpp>

#include <stout/result.hpp>
#include <stout/try.hpp>

#include <execute.pb.h>


using std::map;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::vector;

using mesos::CommandInfo;
using execute::RemoteCommandResult;

using process::Failure;
using process::Future;
using process::Promise;
using process::Subprocess;

namespace execute {

void terminate(pid_t pid)
{
  Try<std::list<os::ProcessTree>> outcome = os::killtree(
      pid , SIGKILL);
  if (outcome.isError()) {
    LOG(ERROR) << "Could not terminate process [" << stringify(pid)
    << "]: " << outcome.error();
  }
}

CommandExecute::CommandExecute(const CommandInfo& commandInfo)
  : commandInfo_(commandInfo), valid_(false), subprocess_(None())
{
  if (commandInfo.shell()) {
    subprocess_ = process::subprocess(
        commandInfo.value(),
        Subprocess::PIPE(),
        Subprocess::PIPE(),
        Subprocess::PIPE()
    );
  } else {
    // `process::subprocess()` expects args[0] to be the name of the command.
    std::vector<std::string> args { commandInfo.value() };

    for (int i = 0; i < commandInfo.arguments_size(); ++i) {
      args.push_back(commandInfo.arguments(i));
    }

    subprocess_ = process::subprocess(
        commandInfo.value(),
        args,
        Subprocess::PIPE(),
        Subprocess::PIPE(),
        Subprocess::PIPE()
    );
  }
}

Future<RemoteCommandResult> CommandExecute::execute()
{
  if (!subprocess_.isSome()) {
    if (subprocess_.isError()) {
      return process::Failure(subprocess_.error());
    } else {
      return process::Failure("Could not create a process instance for " +
                              commandInfo_.value());
    }
  }

  Subprocess subprocess = subprocess_.get();
  string command = commandInfo_.value();
  pid_t pid_ = pid();

  Future<tuple<Future<Option<int>>, Future<string>, Future<string>>> waitRes =
      process::await(subprocess.status(), outData(), errData());

    Future<RemoteCommandResult> result = waitRes.then(
            [pid_, command](const tuple<Future<Option<int>>,
                              Future<string>,
                              Future<string>> &futuresTuple)
                   -> Future<RemoteCommandResult> {
    auto status = std::get<0>(futuresTuple);
    auto out = std::get<1>(futuresTuple);
    auto err = std::get<2>(futuresTuple);

    if (!status.isReady()) {
      terminate(pid_);
      return Failure("Could not obtain the exit code for the child process");
    }

    Option<int> retCode_ = status.get();
    if (retCode_.isNone()) {
      return Failure(
          "Execution of '" + command +
          "' as a subprocess failed (this does not mean it returned a "
              "non-zero exit code, but a more fundamental failure).");
    }

    int retCode = retCode_.get();
    RemoteCommandResult result;

    if (WIFSIGNALED(retCode)) {
      result.set_signaled(true);
      result.set_exitcode(WTERMSIG(retCode));
    } else if (WIFEXITED(retCode)) {
      result.set_exitcode(WEXITSTATUS(retCode));
    }

    if (out.isReady()) {
      result.set_stdout(out.get());
    } else {
      result.set_stdout("stdout is not available." +
                        (out.isFailed() ? " (Failure: " + out.failure() +
                                          ")" : ""));
    }

    if (err.isReady()) {
      result.set_stderr(err.get());
    } else if (err.isFailed()) {
      result.set_stderr(err.failure());
    }

    // Note that we return a "successful" Future even in the case the
    // command failed: this is so that the caller can inspect the return
    // code; whether it was signaled; and `stderr` and take action.
    // See the method's documentation for more details.
    return result;
  });

  // See "Effective Modern C++", Item 31 for why we do not use default
  // capture modes, and instead capture arguments explicitly.
  result.onFailed([pid_, command](const string& message) {
    LOG(ERROR) << "Command: '" << command << "' failed: " << message << "\n"
               << "Trying to cleanup process '" << pid_ << "'";
    terminate(pid_);
  }).onDiscard([pid_]() {
    terminate(pid_);
  });

  return result;
}
} // namespace execute {
