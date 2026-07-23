#include "TestHarness.h"
#include "TestFileUtils.h"

#include "DcsBridge/Internal/EventLog.h"
#include "DcsBridge/Internal/StateCsvWriter.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
constexpr std::size_t kExpectedColumnCount = 76;
constexpr std::chrono::seconds kWriterTimeout(3);
constexpr std::chrono::milliseconds kPollInterval(10);

std::vector<std::string> split(const std::string& text, char separator)
{
	std::vector<std::string> parts;
	std::istringstream input(text);
	std::string part;
	while (std::getline(input, part, separator))
	{
		parts.push_back(part);
	}
	return parts;
}

std::vector<std::string> expected_header()
{
	return split(
		"sequence,simulation_time_s,"
		"flight_altitude_asl_m,flight_altitude_agl_m,flight_position_world_z_m,flight_mach,flight_g_load,flight_angle_of_attack_deg,flight_angle_of_slide_deg,flight_atmosphere_temperature_k,"
		"force_moment_force_x_N,force_moment_force_y_N,force_moment_force_z_N,force_moment_moment_x_N_m,force_moment_moment_y_N_m,force_moment_moment_z_N_m,force_moment_center_of_mass_x_m,force_moment_center_of_mass_y_m,force_moment_center_of_mass_z_m,"
		"engine_0_switch_on,engine_0_throttle_input,engine_0_throttle_output,engine_0_power_readout,engine_0_thrust_force_N,engine_0_afterburner_ratio,engine_0_afterburner_lit,engine_0_nozzle_aperture,"
		"engine_1_switch_on,engine_1_throttle_input,engine_1_throttle_output,engine_1_power_readout,engine_1_thrust_force_N,engine_1_afterburner_ratio,engine_1_afterburner_lit,engine_1_nozzle_aperture,"
		"controls_pitch_input,controls_roll_input,controls_yaw_input,controls_elevator_command,controls_aileron_command,controls_rudder_command,controls_flaps_position,controls_slats_position,controls_airbrake_position,"
		"landing_gear_gear_position,landing_gear_nose_wheel_steering,landing_gear_brake_left,landing_gear_brake_right,landing_gear_wheel_spin_0,landing_gear_wheel_spin_1,landing_gear_wheel_spin_2,"
		"suspension_wheel_0_acting_force_x_N,suspension_wheel_0_acting_force_y_N,suspension_wheel_0_acting_force_z_N,suspension_wheel_0_compression_m,suspension_wheel_0_force_magnitude_N,suspension_wheel_0_weight_on_wheel,"
		"suspension_wheel_1_acting_force_x_N,suspension_wheel_1_acting_force_y_N,suspension_wheel_1_acting_force_z_N,suspension_wheel_1_compression_m,suspension_wheel_1_force_magnitude_N,suspension_wheel_1_weight_on_wheel,"
		"suspension_wheel_2_acting_force_x_N,suspension_wheel_2_acting_force_y_N,suspension_wheel_2_acting_force_z_N,suspension_wheel_2_compression_m,suspension_wheel_2_force_magnitude_N,suspension_wheel_2_weight_on_wheel,"
		"suspension_any_weight_on_wheels,suspension_on_ground,fuel_internal,fuel_external,fuel_total,fuel_total_flow_kg_per_s,shake_amplitude",
		',');
}

void assign_flight_and_force(Core::FrameOutput& output)
{
	output.flight = { 1, 2, 3, 4, 5, 6, 7, 8 };
	output.force_moment.force = { 9, 10, 11 };
	output.force_moment.moment = { 12, 13, 14 };
	output.force_moment.center_of_mass = { 15, 16, 17 };
}

void assign_engines_and_controls(Core::FrameOutput& output)
{
	output.engines[0] = { true, 18, 19, 20, 21, 22, false, 23 };
	output.engines[1] = { false, 24, 25, 26, 27, 28, true, 29 };
	output.controls = { 30, 31, 32, 33, 34, 35, 36, 37, 38 };
}

void assign_gear_suspension_and_fuel(Core::FrameOutput& output)
{
	output.landing_gear = { 39, 40, 41, 42, { 43, 44, 45 } };
	output.suspension.wheels[0] = { { 46, 47, 48 }, 49, 50, true };
	output.suspension.wheels[1] = { { 51, 52, 53 }, 54, 55, false };
	output.suspension.wheels[2] = { { 56, 57, 58 }, 59, 60, true };
	output.suspension.any_weight_on_wheels = true;
	output.suspension.on_ground = false;
	output.fuel = { 61, 62, 63, 64 };
	output.shake_amplitude = 65;
}

Core::FrameOutput numbered_output()
{
	Core::FrameOutput output;
	output.simulation_time_s = 0.125;
	output.availability = { true, true, true, true, true, { true, true, true } };
	assign_flight_and_force(output);
	assign_engines_and_controls(output);
	assign_gear_suspension_and_fuel(output);
	return output;
}

std::vector<std::string> expected_numbered_row()
{
	return split(
		"7,0.125,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,"
		"True,18,19,20,21,22,False,23,False,24,25,26,27,28,True,29,"
		"30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,"
		"46,47,48,49,50,True,51,52,53,54,55,False,56,57,58,59,60,True,"
		"True,False,61,62,63,64,65",
		',');
}

std::size_t line_count(const std::string& text)
{
	return static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n'));
}

bool wait_for_lines(const std::filesystem::path& path, std::size_t expected)
{
	const auto deadline = std::chrono::steady_clock::now() + kWriterTimeout;
	while (std::chrono::steady_clock::now() < deadline)
	{
		if (line_count(TestFiles::read_text_while_open(path)) >= expected)
		{
			return true;
		}
		std::this_thread::sleep_for(kPollInterval);
	}
	return false;
}

void test_schema_and_complete_row(Tests::Context& context)
{
	std::string header = DcsBridge::Internal::state_csv_header();
	if (!header.empty() && header.back() == '\n')
	{
		header.pop_back();
	}
	const std::vector<std::string> header_cells = split(header, ',');
	TEST_EXPECT(context, header_cells == expected_header());
	TEST_EXPECT(context, header_cells.size() == kExpectedColumnCount);
	const DcsBridge::Internal::TelemetryRecord record = {
		1, 1, 7, numbered_output() };
	const auto row = DcsBridge::Internal::format_state_csv_row(record);
	TEST_EXPECT(context, row.valid);
	std::string text(row.data.data(), row.size - 1);
	const std::vector<std::string> cells = split(text, ',');
	TEST_EXPECT(context, cells == expected_numbered_row());
	TEST_EXPECT(context, cells.size() == header_cells.size());
}

void test_unavailable_fields_and_round_trip(Tests::Context& context)
{
	Core::FrameOutput output;
	output.simulation_time_s = 0.10000000000000002;
	const auto formatted = DcsBridge::Internal::format_state_csv_row({ 1, 1, 0, output });
	TEST_EXPECT(context, formatted.valid);
	const std::vector<std::string> cells = split(
		std::string(formatted.data.data(), formatted.size - 1), ',');
	TEST_EXPECT(context, std::strtod(cells[1].c_str(), nullptr) == output.simulation_time_s);
	for (std::size_t index = 2; index <= 9; ++index)
	{
		TEST_EXPECT(context, cells[index] == "-");
	}
	TEST_EXPECT(context, cells[10] == "0");
	TEST_EXPECT(context, cells[16] == "-");
	TEST_EXPECT(context, cells[19] == "False");
	for (std::size_t index = 51; index <= 70; ++index)
	{
		TEST_EXPECT(context, cells[index] == "-");
	}
	TEST_EXPECT(context, cells[71] == "0");
}

void test_latest_only_mailbox(Tests::Context& context)
{
	DcsBridge::Internal::LatestTelemetryMailbox mailbox;
	Core::FrameOutput first;
	Core::FrameOutput second;
	second.simulation_time_s = 2.0;
	mailbox.publish(1, 1, first);
	mailbox.publish(1, 2, second);
	const auto read = mailbox.wait_until(
		(std::chrono::steady_clock::time_point::max)());
	const auto& latest = read.record;
	TEST_EXPECT(context, !read.stopping);
	TEST_EXPECT(context, latest && latest->publish_version == 2);
	TEST_EXPECT(context, latest && latest->sequence == 2);
	TEST_EXPECT(context, latest && latest->output.simulation_time_s == 2.0);
	const auto row = latest
		? DcsBridge::Internal::format_state_csv_row(*latest)
		: DcsBridge::Internal::FormattedStateCsvRow{};
	TEST_EXPECT(context, row.valid && split(
		std::string(row.data.data(), row.size - 1), ',')[0] == "2");
	TEST_EXPECT(context, !mailbox.take_latest().has_value());
	mailbox.publish(2, 0, first);
	const auto next_flight = mailbox.take_latest();
	TEST_EXPECT(context, next_flight && next_flight->publish_version == 3);
	TEST_EXPECT(context, next_flight && next_flight->flight_id == 2);
	TEST_EXPECT(context, next_flight && next_flight->sequence == 0);
}

void test_file_lifecycle_and_sequences(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root("csv");
	TEST_EXPECT(context, root.valid());
	const auto log_directory = root.path() / "log";
	std::filesystem::create_directories(log_directory);
	const auto active = log_directory / "fck1c_state.csv";
	const auto old = log_directory / "fck1c_state.csv.old";
	TestFiles::write_text(active, "previous active\n");
	TestFiles::write_text(old, "obsolete old\n");
	DcsBridge::Internal::EventLog event_log(root.path().string().c_str());
	{
		DcsBridge::Internal::StateCsvWriter writer(
			root.path().string().c_str(), event_log);
		TEST_EXPECT(context, writer.is_ready());
		TEST_EXPECT(context, TestFiles::read_text_while_open(old) == "previous active\n");
		TEST_EXPECT(context, TestFiles::read_text_while_open(active) ==
			DcsBridge::Internal::state_csv_header());
		Core::FrameOutput output;
		writer.publish_start(output);
		TEST_EXPECT(context, wait_for_lines(active, 2));
		output.simulation_time_s = 0.001;
		writer.publish_step(output);
		TEST_EXPECT(context, wait_for_lines(active, 3));
		output.simulation_time_s = 0.002;
		writer.publish_step(output);
		TEST_EXPECT(context, wait_for_lines(active, 4));
		output.simulation_time_s = 0.0;
		writer.publish_start(output);
		TEST_EXPECT(context, wait_for_lines(active, 5));
	}
	const std::vector<std::string> lines = split(
		TestFiles::read_text_while_open(active), '\n');
	TEST_EXPECT(context, lines.size() == 5);
	TEST_EXPECT(context, split(lines[1], ',')[0] == "0");
	TEST_EXPECT(context, split(lines[2], ',')[0] == "1");
	TEST_EXPECT(context, split(lines[3], ',')[0] == "2");
	TEST_EXPECT(context, split(lines[4], ',')[0] == "0");
	TEST_EXPECT(context, TestFiles::read_text_while_open(old) == "previous active\n");
}

void test_failure_report_and_next_start_retry(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root("csv");
	const auto log_directory = root.path() / "log";
	std::filesystem::create_directories(log_directory);
	const auto blocking_old = log_directory / "fck1c_state.csv.old";
	std::filesystem::create_directory(blocking_old);
	DcsBridge::Internal::EventLog event_log(root.path().string().c_str());
	DcsBridge::Internal::StateCsvWriter writer(root.path().string().c_str(), event_log);
	TEST_EXPECT(context, !writer.is_ready());
	TEST_EXPECT(context, writer.last_error_code() != 0);
	const auto event_path = log_directory / "fck1c_efm.log";
	const std::string error = TestFiles::read_text_while_open(event_path);
	TEST_EXPECT(context, error.find("[ERROR] csv operation=remove_old path=") !=
		std::string::npos);
	TEST_EXPECT(context, error.find(" os_error=") != std::string::npos);
	std::filesystem::remove(blocking_old);
	Core::FrameOutput output;
	writer.publish_start(output);
	const auto active = log_directory / "fck1c_state.csv";
	TEST_EXPECT(context, wait_for_lines(active, 2));
	TEST_EXPECT(context, writer.is_ready());
	TEST_EXPECT(context, writer.last_error_code() == 0);
}
}

void run_state_csv_writer_tests(Tests::Context& context)
{
	test_schema_and_complete_row(context);
	test_unavailable_fields_and_round_trip(context);
	test_latest_only_mailbox(context);
	test_file_lifecycle_and_sequences(context);
	test_failure_report_and_next_start_retry(context);
}
