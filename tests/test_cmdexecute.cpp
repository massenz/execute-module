// This file is (c) 2015 AlertAvert.com.  All rights reserved.

#include <thread>
#include <chrono>

#include <gtest/gtest.h>

#include <process/future.hpp>

#include <stout/duration.hpp>
#include <stout/os.hpp>
#include <stout/os/exists.hpp>

#include "cmdexecute.hpp"

using namespace std::chrono;

using namespace execute;
using namespace process;

using std::this_thread::sleep_for;

TEST(CommandExecute, CanExecuteSimpleCmd)
{
    RemoteCommandInfo info;
    info.set_command("ls");
    info.add_arguments("-la");
    info.add_arguments("/tmp");

    CommandExecute exec(info);
    Future<RemoteCommandResult> future = exec.execute();

    ASSERT_TRUE(future.await(Milliseconds(200)));
    ASSERT_EQ(EXIT_SUCCESS, future.get().exitcode());
}

TEST(CommandExecute, CanGetStdout)
{
    RemoteCommandInfo info;
    info.set_command("echo");
    info.add_arguments("foo");
    info.add_arguments("bar");

    CommandExecute exec(info);
    Future<RemoteCommandResult> future = exec.execute();

    ASSERT_TRUE(future.await(Milliseconds(500)));
    ASSERT_EQ(EXIT_SUCCESS, future.get().exitcode());
    ASSERT_EQ("foo bar\n", future.get().stdout());
}

TEST(CommandExecute, CleansOnTimeout)
{
    RemoteCommandInfo info;
    info.set_command("sleep");
    info.add_arguments("5");

    CommandExecute exec(info);
    Future<RemoteCommandResult> future = exec.execute();

    ASSERT_TRUE(os::exists(exec.pid()));
    ASSERT_FALSE(future.await(Milliseconds(100)));

    // Upon discarding, we should clean up the PID
    future.discard();
    sleep_for(milliseconds(200));
    ASSERT_FALSE(os::exists(exec.pid()));
}
