#include "TestHarness.h"
#include "TestFileUtils.h"

#include "DcsBridge/Internal/AbiBoundary.h"
#include "DcsBridge/Internal/EventLog.h"
#include "DcsBridge/Internal/ProcessBridgeContext.h"

#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace
{
constexpr int kUnknownThrownValue = 42;
constexpr double kNeutralResult = -1.0;

std::unique_ptr<Core::Fck1cEfm> throw_during_core_creation()
{
	throw std::runtime_error("configuration failure");
}

cockpit_param_api provide_empty_cockpit_api()
{
	return {};
}

std::string read_log(const TestFiles::TemporaryDirectory& root)
{
	return TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
}

void report_standard_exception(DcsBridge::Internal::EventLog& log)
{
	try
	{
		throw std::runtime_error("test failure");
	}
	catch (...)
	{
		DcsBridge::Internal::report_abi_exception(
			"test_callback", std::current_exception(), &log);
	}
}

void report_unknown_exception(DcsBridge::Internal::EventLog& log)
{
	try
	{
		throw kUnknownThrownValue;
	}
	catch (...)
	{
		DcsBridge::Internal::report_abi_exception(
			"unknown_callback", std::current_exception(), &log);
	}
}

double guarded_return(
	DcsBridge::Internal::EventLog& log,
	bool& cleanup_called) try
{
	throw std::runtime_error("guarded failure");
}
FCK1C_ABI_CATCH_RETURN(
	"guarded_return", &log, kNeutralResult, cleanup_called = true)

void guarded_void(
	DcsBridge::Internal::EventLog& log,
	bool& cleanup_called) try
{
	throw std::runtime_error("guarded void failure");
}
FCK1C_ABI_CATCH_VOID("guarded_void", &log, cleanup_called = true)

void test_exception_details_are_logged(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	DcsBridge::Internal::EventLog log(root.path().string().c_str());
	report_standard_exception(log);
	report_unknown_exception(log);

	const std::string content = read_log(root);
	TEST_EXPECT(context, content.find(
		"][-][ERROR] callback=test_callback unhandled_exception=test failure "
		"caught_at=c_abi_boundary\n") != std::string::npos);
	TEST_EXPECT(context, content.find(
		"][-][ERROR] callback=unknown_callback "
		"unhandled_exception=unknown C++ exception "
		"caught_at=c_abi_boundary\n") != std::string::npos);
}

void test_guard_returns_neutral_and_runs_cleanup(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	DcsBridge::Internal::EventLog log(root.path().string().c_str());
	bool return_cleanup_called = false;
	bool void_cleanup_called = false;

	TEST_EXPECT_NEAR(
		context,
		guarded_return(log, return_cleanup_called),
		kNeutralResult,
		0.0);
	guarded_void(log, void_cleanup_called);
	TEST_EXPECT(context, return_cleanup_called);
	TEST_EXPECT(context, void_cleanup_called);

	const std::string content = read_log(root);
	TEST_EXPECT(context, content.find(
		"callback=guarded_return unhandled_exception=guarded failure") !=
		std::string::npos);
	TEST_EXPECT(context, content.find(
		"callback=guarded_void unhandled_exception=guarded void failure") !=
		std::string::npos);
}

void test_missing_log_does_not_throw(Tests::Context& context)
{
	bool returned = false;
	try
	{
		throw std::runtime_error("initialization failure");
	}
	catch (...)
	{
		DcsBridge::Internal::report_abi_exception(
			"initialization_callback", std::current_exception(), nullptr);
		returned = true;
	}
	TEST_EXPECT(context, returned);
}

void test_process_context_exposes_initialization_failure(Tests::Context& context)
{
	DcsBridge::Internal::ProcessBridgeContext process_context(
		provide_empty_cockpit_api,
		throw_during_core_creation,
		nullptr);
	TEST_EXPECT(context, process_context.try_event_log() == nullptr);

	bool initialization_error_caught = false;
	try
	{
		(void)process_context.get(nullptr);
	}
	catch (const std::runtime_error& error)
	{
		initialization_error_caught =
			std::string(error.what()) == "configuration failure";
	}
	TEST_EXPECT(context, initialization_error_caught);
}
}

void run_abi_boundary_tests(Tests::Context& context)
{
	test_exception_details_are_logged(context);
	test_guard_returns_neutral_and_runs_cleanup(context);
	test_missing_log_does_not_throw(context);
	test_process_context_exposes_initialization_failure(context);
}
