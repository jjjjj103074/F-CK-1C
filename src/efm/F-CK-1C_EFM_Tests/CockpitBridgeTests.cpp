#include "TestHarness.h"

#include "DcsBridge/Internal/CockpitBridge.h"
#include "DcsIds/CockpitParams.g.h"

#include <array>
#include <cstring>
#include <limits>

namespace
{
constexpr double kTolerance = 1e-9;

struct FakeParameter
{
	const char* name;
	double value;
	bool available;
};

class FakeCockpit final
{
public:
	FakeCockpit()
		: parameters_({ {
			{ DcsIds::CockpitParams::TemperatureC, 0.0, true },
			{ DcsIds::CockpitParams::MaxPowerSwitch, 0.0, true },
			{ DcsIds::CockpitParams::MaxPowerReady, 0.0, true },
			{ DcsIds::CockpitParams::ApMasterEngaged, 0.0, true },
			{ DcsIds::CockpitParams::ApPitchCommand, 0.0, true },
			{ DcsIds::CockpitParams::ApRollCommand, 0.0, true },
			{ DcsIds::CockpitParams::ApThrottleCommand, 0.0, true },
			{ DcsIds::CockpitParams::ApBypassActive, 0.0, true },
			{ DcsIds::CockpitParams::ApAutoThrottleEngaged, 0.0, true }
		} })
	{
	}

	void set(const char* name, double value)
	{
		find(name)->value = value;
	}

	void set_available(const char* name, bool available)
	{
		find(name)->available = available;
	}

	double value(const char* name)
	{
		return find(name)->value;
	}

	void* handle(const char* name)
	{
		FakeParameter* parameter = find(name);
		return parameter->available ? parameter : nullptr;
	}

private:
	FakeParameter* find(const char* name)
	{
		for (FakeParameter& parameter : parameters_)
		{
			if (std::strcmp(parameter.name, name) == 0)
			{
				return &parameter;
			}
		}
		return nullptr;
	}

	std::array<FakeParameter, 9> parameters_;
};

FakeCockpit* g_fake_cockpit = nullptr;

void* get_parameter_handle(const char* name)
{
	return g_fake_cockpit->handle(name);
}

void update_parameter_number(void* handle, double value)
{
	static_cast<FakeParameter*>(handle)->value = value;
}

bool parameter_value_to_number(
	const void* handle,
	double& result,
	bool interpolated)
{
	(void)interpolated;
	result = static_cast<const FakeParameter*>(handle)->value;
	return true;
}

cockpit_param_api make_api(FakeCockpit& cockpit)
{
	g_fake_cockpit = &cockpit;
	cockpit_param_api api = {};
	api.pfn_ed_cockpit_get_parameter_handle = get_parameter_handle;
	api.pfn_ed_cockpit_update_parameter_with_number = update_parameter_number;
	api.pfn_ed_cockpit_parameter_value_to_number = parameter_value_to_number;
	return api;
}

void test_autopilot_typed_values(Tests::Context& context)
{
	FakeCockpit cockpit;
	cockpit.set(DcsIds::CockpitParams::ApMasterEngaged, 1.0);
	cockpit.set(DcsIds::CockpitParams::ApPitchCommand, 1.5);
	cockpit.set(DcsIds::CockpitParams::ApRollCommand, -1.5);
	cockpit.set(DcsIds::CockpitParams::ApThrottleCommand, 0.4);
	cockpit.set(DcsIds::CockpitParams::ApAutoThrottleEngaged, 1.0);
	DcsBridge::Internal::CockpitBridge bridge(make_api(cockpit));
	const Core::AutopilotCommand command = bridge.read_step_input().autopilot;
	TEST_EXPECT(context, command.master);
	TEST_EXPECT(context, !command.bypass);
	TEST_EXPECT(context, command.auto_throttle_engaged);
	TEST_EXPECT_NEAR(context, command.pitch_command, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, command.roll_command, -1.0, kTolerance);
	TEST_EXPECT_NEAR(context, command.throttle_command, 0.4, kTolerance);
}

void test_autopilot_neutral_rules(Tests::Context& context)
{
	FakeCockpit cockpit;
	cockpit.set(DcsIds::CockpitParams::ApMasterEngaged, 1.0);
	cockpit.set(DcsIds::CockpitParams::ApBypassActive, 1.0);
	cockpit.set(DcsIds::CockpitParams::ApPitchCommand, 0.8);
	cockpit.set(DcsIds::CockpitParams::ApThrottleCommand, 0.7);
	DcsBridge::Internal::CockpitBridge bridge(make_api(cockpit));
	const Core::AutopilotCommand command = bridge.read_step_input().autopilot;
	TEST_EXPECT(context, command.master && command.bypass);
	TEST_EXPECT_NEAR(context, command.pitch_command, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, command.roll_command, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, command.throttle_command, 0.0, kTolerance);
}

void test_missing_autopilot_parameter_is_neutral(Tests::Context& context)
{
	FakeCockpit cockpit;
	cockpit.set_available(DcsIds::CockpitParams::ApRollCommand, false);
	DcsBridge::Internal::CockpitBridge bridge(make_api(cockpit));
	const DcsBridge::Internal::CockpitStepInput first = bridge.read_step_input();
	const Core::AutopilotCommand command = first.autopilot;
	TEST_EXPECT(context, !command.master);
	TEST_EXPECT(context, !command.auto_throttle_engaged);
	TEST_EXPECT_NEAR(context, command.pitch_command, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, command.throttle_command, 0.0, kTolerance);
	TEST_EXPECT(context, first.events.count == 1);
	TEST_EXPECT(context, std::strcmp(
		first.events.items[0].parameter_name,
		DcsIds::CockpitParams::ApRollCommand) == 0);
	TEST_EXPECT(
		context,
		first.events.items[0].type ==
			DcsBridge::Internal::CockpitParameterEventType::Error);
	TEST_EXPECT(context, bridge.read_step_input().events.count == 0);
	cockpit.set_available(DcsIds::CockpitParams::ApRollCommand, true);
	const DcsBridge::Internal::CockpitStepInput recovered = bridge.read_step_input();
	TEST_EXPECT(context, recovered.events.count == 1);
	TEST_EXPECT(
		context,
		recovered.events.items[0].type ==
			DcsBridge::Internal::CockpitParameterEventType::Recovery);
}

void test_max_power_and_temperature(Tests::Context& context)
{
	FakeCockpit cockpit;
	DcsBridge::Internal::CockpitBridge bridge(make_api(cockpit));
	Core::MaxPowerCommand max_power = bridge.read_step_input().max_power;
	TEST_EXPECT_NEAR(context, max_power.ready, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, max_power.value, 1.0, kTolerance);
	cockpit.set(DcsIds::CockpitParams::MaxPowerReady, 1.0);
	cockpit.set(DcsIds::CockpitParams::MaxPowerSwitch, 0.0);
	max_power = bridge.read_step_input().max_power;
	TEST_EXPECT_NEAR(context, max_power.ready, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, max_power.value, 0.0, kTolerance);
	cockpit.set(DcsIds::CockpitParams::MaxPowerSwitch, 1.0);
	max_power = bridge.read_step_input().max_power;
	TEST_EXPECT_NEAR(context, max_power.value, 1.0, kTolerance);
	TEST_EXPECT(context, bridge.export_temperature(15.0).count == 0);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::TemperatureC), 288.0, kTolerance);
}

void test_invalid_numeric_is_neutral_then_recovers(Tests::Context& context)
{
	FakeCockpit cockpit;
	cockpit.set(DcsIds::CockpitParams::ApMasterEngaged, 1.0);
	cockpit.set(
		DcsIds::CockpitParams::ApPitchCommand,
		std::numeric_limits<double>::infinity());
	DcsBridge::Internal::CockpitBridge bridge(make_api(cockpit));
	const DcsBridge::Internal::CockpitStepInput invalid = bridge.read_step_input();
	TEST_EXPECT(context, !invalid.autopilot.master);
	TEST_EXPECT(context, invalid.events.count == 1);
	TEST_EXPECT(context, invalid.events.items[0].has_value);
	TEST_EXPECT(context, std::strcmp(
		invalid.events.items[0].reason,
		"invalid_numeric") == 0);
	cockpit.set(DcsIds::CockpitParams::ApPitchCommand, 0.25);
	const DcsBridge::Internal::CockpitStepInput recovered = bridge.read_step_input();
	TEST_EXPECT(context, recovered.autopilot.master);
	TEST_EXPECT_NEAR(context, recovered.autopilot.pitch_command, 0.25, kTolerance);
	TEST_EXPECT(context, recovered.events.count == 1);
	TEST_EXPECT(
		context,
		recovered.events.items[0].type ==
			DcsBridge::Internal::CockpitParameterEventType::Recovery);
}
}

void run_cockpit_bridge_tests(Tests::Context& context)
{
	test_autopilot_typed_values(context);
	test_autopilot_neutral_rules(context);
	test_missing_autopilot_parameter_is_neutral(context);
	test_max_power_and_temperature(context);
	test_invalid_numeric_is_neutral_then_recovers(context);
}
