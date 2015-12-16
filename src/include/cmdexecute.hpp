/*
 * This file is (c) 2015 AlertAvert.com.  All rights reserved.
 */

#pragma once

#include <process/io.hpp>
#include <process/subprocess.hpp>

#include <stout/os/killtree.hpp>
#include <stout/os/process.hpp>

#include "execute.pb.h"

namespace execute {

class CommandExecute
{
public:
  CommandExecute(const execute::RemoteCommandInfo& commandInfo);

  /**
   * Executes the child process.
   *
   * The returned future "unpacks" the various in/out streams, takes care of
   * retrieving the outcome of the command and decoding it, so that the caller
   * can simply access the `CommandResult` and access those values in a
   * more streamlined fashion.
   *
   * <strong>Note</strong> we return a "successful" `Future` even if the
   * command failed: this allows us to return a "richer" information set to the
   * caller (namely, the `CommandResult` containing the exit code, whether it
   * was signaled and stdout, stderr) as opposed to a simple string.
   *
   * The rationale is that the Future succeeded, and it was the `command` that
   * failed, and we provide the information necessary to discover why (and,
   * possibly, take corrective action).
   *
   * The typical calling pattern, then would be something along the lines of:
   *
   *     Future<RemoteCommandResult> future = subprocess.execute();
   *
   *     // either wait for the Future, or put the following in a `then` clause:
   *     if (future.isReady()) {
   *       const CommandResult& result = future.get();
   *       if (result.returnCode == EXIT_SUCCESS) {
   *         // all went well, you can inspect stdout, or whatever.
   *       } else {
   *         // something went wrong, let's notify the user.
   *         LOG(ERROR) << "The command '" << result.invocation.command << "' "
   *                    << "failed: " << (result.stderr.isSome() ?
   *                                      result.stderr.get() :
   *                                      "no information");
   *       }
   *     } else {
   *       // The future failed, something prevented the command to complete at
   *       // all and/or the calling process to get access to stdin/stdout
   *       pipes.
   *     }
   *
   * @return a `Future` that will deliver the outcome of the command execution.
   * @see execute::RemoteCommandResult
   */
  process::Future<execute::RemoteCommandResult> execute();


  /**
   * Terminates the process that was started via the `execute()` call.
   *
   * This is useful when waiting on the `Future` returned by `execute()`, and
   * the wait times out: the caller may want to try and clean up the process.
   */
  void cleanup() const
  {
    if (subprocess_.isSome()) {
      auto pid = subprocess_.get().pid();
      Try<std::list<os::ProcessTree>> outcome = os::killtree(
         pid , SIGKILL);
      if (outcome.isError()) {
        LOG(ERROR) << "Could not terminate process [" << stringify(pid)
                   << "]: " << outcome.error();
      }
    }
  }

  process::Future<std::string> outData() const
  {
    if (!subprocess_.isSome()) {
      return process::Failure("Not a valid process");
    }
    return subprocess_.get().out().isSome() ?
           process::io::read(subprocess_.get().out().get()) :
           process::Failure("Cannot obtain stdout for PID: " +
                            stringify(subprocess_.get().pid()));
  }


  process::Future <std::string> errData() const
  {
    if (!subprocess_.isSome()) {
      return process::Failure("Not a valid process");
    }
    return (subprocess_.get().err().isSome() ?
            process::io::read(subprocess_.get().err().get()) :
            process::Failure("Cannot obtain stderr for PID: " +
                             stringify(subprocess_.get().pid())));
  }

  pid_t pid() const
  {
    return subprocess_.isSome() ? subprocess_->pid() : -1;
  }

private:
  Result<process::Subprocess> subprocess_;
  execute::RemoteCommandInfo commandInfo_;
  bool valid_;
};
} // namespace execute {
