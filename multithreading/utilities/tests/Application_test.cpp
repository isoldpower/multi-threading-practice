#include <gtest/gtest.h>
#include <utilities/Application.h>
#include <utilities/tests/Output.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using multithreading::utilities::Application;
using multithreading::utilities::ApplicationInfo;
using multithreading::utilities::tests::suppress_stdout;

TEST(ApplicationTest, SafeStartReturnsTaskResult) {
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    const auto result = app.SafeStart([]() {
        return 42;
    });

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result, 42);
}

TEST(ApplicationTest, SafeStartReturnsNulloptOnException) {
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    suppress_stdout([&]() {
        const auto result = app.SafeStart([]() -> int {
            throw std::runtime_error("intentional error");
        });

        EXPECT_FALSE(result.has_value());
    });
}

TEST(ApplicationTest, BeforeTaskIsCalledBeforeMainTask) {
    std::vector<std::string> call_order;
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask =
                [&call_order]() {
                    call_order.emplace_back("before");
                },
            .afterTask = std::nullopt
        }
    );

    app.SafeStart([&call_order]() {
        call_order.emplace_back("task");
        return 0;
    });

    ASSERT_EQ(call_order.size(), 2);
    EXPECT_EQ(call_order.at(0), "before");
    EXPECT_EQ(call_order.at(1), "task");
}

TEST(ApplicationTest, AfterTaskIsCalledWithTaskResult) {
    std::optional<int> received_value;
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = [&received_value](int value) {
                received_value = value;
            }
        }
    );

    app.SafeStart([]() {
        return 99;
    });

    EXPECT_TRUE(received_value.has_value());
    EXPECT_EQ(received_value, 99);
}

TEST(ApplicationTest, AfterTaskIsCalledAfterMainTask) {
    std::vector<std::string> call_order;
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = [&call_order](int) {
                call_order.emplace_back("after");
            }
        }
    );

    app.SafeStart([&call_order]() {
        call_order.emplace_back("task");
        return 0;
    });

    ASSERT_EQ(call_order.size(), 2);
    EXPECT_EQ(call_order.at(0), "task");
    EXPECT_EQ(call_order.at(1), "after");
}

TEST(ApplicationTest, BeforeTaskIsNotCalledWhenAbsent) {
    size_t call_count = 0;
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    app.SafeStart([&call_count]() {
        ++call_count;
        return 0;
    });

    EXPECT_EQ(call_count, 1);
}

TEST(ApplicationTest, AfterTaskIsNotCalledOnException) {
    bool after_called = false;
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = [&after_called](int) {
                after_called = true;
            }
        }
    );

    suppress_stdout([&]() {
        app.SafeStart([]() -> int {
            throw std::runtime_error("intentional error");
        });

        EXPECT_FALSE(after_called);
    });
}

TEST(ApplicationTest, BeforeAndAfterBothCallableTogther) {
    std::vector<std::string> call_order;
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask =
                [&call_order]() {
                    call_order.emplace_back("before");
                },
            .afterTask =
                [&call_order](int) {
                    call_order.emplace_back("after");
                }
        }
    );

    app.SafeStart([&call_order]() {
        call_order.emplace_back("task");
        return 0;
    });

    ASSERT_EQ(call_order.size(), 3);
    EXPECT_EQ(call_order.at(0), "before");
    EXPECT_EQ(call_order.at(1), "task");
    EXPECT_EQ(call_order.at(2), "after");
}

TEST(ApplicationTest, SafeStartQueueExecutesAllTasks) {
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    const auto results = app.SafeStartQueue<3>(
        {{[]() {
              return 1;
          },
          []() {
              return 2;
          },
          []() {
              return 3;
          }}}
    );

    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(results.at(0), 1);
    EXPECT_EQ(results.at(1), 2);
    EXPECT_EQ(results.at(2), 3);
}

TEST(ApplicationTest, SafeStartQueueContinuesAfterException) {
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    suppress_stdout([&]() {
        const auto results = app.SafeStartQueue<3>(
            {{[]() {
                  return 1;
              },
              []() -> int {
                  throw std::runtime_error("intentional error");
              },
              []() {
                  return 3;
              }}}
        );

        EXPECT_TRUE(results.at(0).has_value());
        EXPECT_FALSE(results.at(1).has_value());
        EXPECT_TRUE(results.at(2).has_value());
    });
}

TEST(ApplicationTest, SafeStartQueueWithSingleTask) {
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "TestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    const auto results = app.SafeStartQueue<1>({{[]() {
        return 7;
    }}});

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.at(0), 7);
}

TEST(ApplicationTest, ExceptionMessageContainsAppName) {
    Application<int> app(
        ApplicationInfo<int>{
            .appName = "MyTestApp",
            .appVersion = "1.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    testing::internal::CaptureStdout();
    app.SafeStart([]() -> int {
        throw std::runtime_error("something broke");
    });
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("MyTestApp", 0), std::string::npos);
    EXPECT_NE(output.find("something broke", 0), std::string::npos);
}