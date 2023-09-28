#include "includes.h"
#include "spdlog/sinks/ostream_sink.h"
#include "test_sink.h"

template <class T>
std::string log_info(const T &what, spdlog::level logger_level = spdlog::level::info) {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);

    spdlog::logger oss_logger("oss", oss_sink);
    oss_logger.set_level(logger_level);
    oss_logger.set_pattern("%v");
    oss_logger.info(what);

    return oss.str().substr(0, oss.str().length() - strlen(spdlog::details::os::default_eol));
}

TEST_CASE("basic_logging ", "[basic_logging]") {
    // const char
    REQUIRE(log_info("Hello") == "Hello");
    REQUIRE(log_info("").empty());

    // std::string
    REQUIRE(log_info(std::string("Hello")) == "Hello");
    REQUIRE(log_info(std::string()).empty());
}

TEST_CASE("log_levels", "[log_levels]") {
    REQUIRE(log_info("Hello", spdlog::level::err).empty());
    REQUIRE(log_info("Hello", spdlog::level::critical).empty());
    REQUIRE(log_info("Hello", spdlog::level::info) == "Hello");
    REQUIRE(log_info("Hello", spdlog::level::debug) == "Hello");
    REQUIRE(log_info("Hello", spdlog::level::trace) == "Hello");
}

TEST_CASE("level_to_string_view", "[convert_to_string_view]") {
    REQUIRE(spdlog::to_string_view(spdlog::level::trace) == "trace");
    REQUIRE(spdlog::to_string_view(spdlog::level::debug) == "debug");
    REQUIRE(spdlog::to_string_view(spdlog::level::info) == "info");
    REQUIRE(spdlog::to_string_view(spdlog::level::warn) == "warning");
    REQUIRE(spdlog::to_string_view(spdlog::level::err) == "error");
    REQUIRE(spdlog::to_string_view(spdlog::level::critical) == "critical");
    REQUIRE(spdlog::to_string_view(spdlog::level::off) == "off");
}

TEST_CASE("to_short_string_view", "[convert_to_short_c_str]") {
    REQUIRE(spdlog::to_short_string_view(spdlog::level::trace) == "T");
    REQUIRE(spdlog::to_short_string_view(spdlog::level::debug) == "D");
    REQUIRE(spdlog::to_short_string_view(spdlog::level::info) == "I");
    REQUIRE(spdlog::to_short_string_view(spdlog::level::warn) == "W");
    REQUIRE(spdlog::to_short_string_view(spdlog::level::err) == "E");
    REQUIRE(spdlog::to_short_string_view(spdlog::level::critical) == "C");
    REQUIRE(spdlog::to_short_string_view(spdlog::level::off) == "O");
}

TEST_CASE("to_level_enum", "[convert_to_level_enum]") {
    REQUIRE(spdlog::level_from_str("trace") == spdlog::level::trace);
    REQUIRE(spdlog::level_from_str("debug") == spdlog::level::debug);
    REQUIRE(spdlog::level_from_str("info") == spdlog::level::info);
    REQUIRE(spdlog::level_from_str("warning") == spdlog::level::warn);
    REQUIRE(spdlog::level_from_str("warn") == spdlog::level::warn);
    REQUIRE(spdlog::level_from_str("error") == spdlog::level::err);
    REQUIRE(spdlog::level_from_str("critical") == spdlog::level::critical);
    REQUIRE(spdlog::level_from_str("off") == spdlog::level::off);
    REQUIRE(spdlog::level_from_str("null") == spdlog::level::off);
}

TEST_CASE("periodic flush", "[periodic_flush]") {
    using spdlog::sinks::test_sink_mt;
    auto logger = spdlog::create<test_sink_mt>("periodic_flush");
    auto test_sink = std::static_pointer_cast<test_sink_mt>(logger->sinks()[0]);

    spdlog::flush_every(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1250));
    REQUIRE(test_sink->flush_counter() == 1);
    spdlog::flush_every(std::chrono::seconds(0));
    spdlog::drop_all();
}

TEST_CASE("clone-logger", "[clone]") {
    using spdlog::sinks::test_sink_mt;
    auto test_sink = std::make_shared<test_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("orig", test_sink);
    logger->set_pattern("%v");
    auto cloned = logger->clone("clone");

    REQUIRE(cloned->name() == "clone");
    REQUIRE(logger->sinks() == cloned->sinks());
    REQUIRE(logger->log_level() == cloned->log_level());
    REQUIRE(logger->flush_level() == cloned->flush_level());
    logger->info("Some message 1");
    cloned->info("Some message 2");

    REQUIRE(test_sink->lines().size() == 2);
    REQUIRE(test_sink->lines()[0] == "Some message 1");
    REQUIRE(test_sink->lines()[1] == "Some message 2");

    spdlog::drop_all();
}

TEST_CASE("clone async", "[clone]") {
    using spdlog::sinks::test_sink_st;
    spdlog::init_thread_pool(4, 1);
    auto test_sink = std::make_shared<test_sink_st>();
    auto logger = std::make_shared<spdlog::async_logger>("orig", test_sink, spdlog::thread_pool());
    logger->set_pattern("%v");
    auto cloned = logger->clone("clone");

    REQUIRE(cloned->name() == "clone");
    REQUIRE(logger->sinks() == cloned->sinks());
    REQUIRE(logger->log_level() == cloned->log_level());
    REQUIRE(logger->flush_level() == cloned->flush_level());

    logger->info("Some message 1");
    cloned->info("Some message 2");

    spdlog::details::os::sleep_for_millis(100);

    REQUIRE(test_sink->lines().size() == 2);
    REQUIRE(test_sink->lines()[0] == "Some message 1");
    REQUIRE(test_sink->lines()[1] == "Some message 2");

    spdlog::drop_all();
}

TEST_CASE("default logger API", "[default logger]") {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("oss", oss_sink));
    spdlog::set_pattern("*** %v");

    spdlog::default_logger()->set_level(spdlog::level::trace);
    spdlog::trace("hello trace");
    REQUIRE(oss.str() == "*** hello trace" + std::string(spdlog::details::os::default_eol));

    oss.str("");
    spdlog::debug("hello debug");
    REQUIRE(oss.str() == "*** hello debug" + std::string(spdlog::details::os::default_eol));

    oss.str("");
    spdlog::info("Hello");
    REQUIRE(oss.str() == "*** Hello" + std::string(spdlog::details::os::default_eol));

    oss.str("");
    spdlog::warn("Hello again {}", 2);
    REQUIRE(oss.str() == "*** Hello again 2" + std::string(spdlog::details::os::default_eol));

    oss.str("");
    spdlog::critical(std::string("some string"));
    REQUIRE(oss.str() == "*** some string" + std::string(spdlog::details::os::default_eol));

    oss.str("");
    spdlog::set_level(spdlog::level::info);
    spdlog::debug("should not be logged");
    REQUIRE(oss.str().empty());
    spdlog::drop_all();
    spdlog::set_pattern("%v");
}
