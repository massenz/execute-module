// This file is (c) 2015 AlertAvert.com.  All rights reserved.

#include <thread>
#include <chrono>

#include <gtest/gtest.h>

#include <mesos/mesos.hpp>

#include <process/future.hpp>

#include <stout/duration.hpp>
#include <stout/os.hpp>
#include <stout/os/exists.hpp>

#include "cmdexecute.hpp"

using namespace std::chrono;

using namespace execute;
using namespace process;

using mesos::CommandInfo;

using std::this_thread::sleep_for;


TEST(CommandExecute, CanExecuteSimpleCmd)
{
  CommandInfo info;
  info.set_value("ls");
  info.add_arguments("-la");
  info.add_arguments("/tmp");

  info.set_shell(false);

  CommandExecute exec(info);
  Future<RemoteCommandResult> future = exec.execute();

  ASSERT_TRUE(future.await(Milliseconds(200)));
  ASSERT_EQ(EXIT_SUCCESS, future.get().exitcode());
}


TEST(CommandExecute, CanGetStdout)
{
  CommandInfo info;
  info.set_value("echo");
  info.add_arguments("foo");
  info.add_arguments("bar");

  info.set_shell(false);

  CommandExecute exec(info);
  Future<RemoteCommandResult> future = exec.execute();

  ASSERT_TRUE(future.await(Milliseconds(500)));
  ASSERT_EQ(EXIT_SUCCESS, future.get().exitcode());
  ASSERT_EQ("foo bar\n", future.get().stdout());
}


TEST(CommandExecute, CleansOnTimeout)
{
  CommandInfo info;
  info.set_value("sleep 5");

  CommandExecute exec(info);
  Future<RemoteCommandResult> future = exec.execute();

  ASSERT_TRUE(os::exists(exec.pid()));
  ASSERT_FALSE(future.await(Milliseconds(100)));

  // Upon discarding, we should clean up the PID
  future.discard();
  sleep_for(milliseconds(200));
  ASSERT_FALSE(os::exists(exec.pid()));
}
