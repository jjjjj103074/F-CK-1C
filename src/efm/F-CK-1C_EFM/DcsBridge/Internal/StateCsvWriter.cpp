#include "StateCsvWriter.h"

#include "LogFileLifecycle.h"
#include "../../Common/PathUtils.h"

#include <cerrno>
#include <charconv>
#include <cstring>
#include <limits>
#include <share.h>
#include <system_error>

namespace
{
constexpr const char* kStateCsvFileName = "fck1c_state.csv";
constexpr std::chrono::milliseconds kCsvFlushInterval(100);
constexpr size_t kCsvErrorMessageCapacity = 1400;
constexpr size_t kUnsignedIntegerTextCapacity = 32;
constexpr size_t kDoubleTextCapacity = 64;
constexpr int kRoundTripDoubleDigits = std::numeric_limits<double>::max_digits10;

constexpr const char* kStateCsvHeader =
	"sequence,simulation_time_s,"
	"flight_altitude_asl_m,flight_altitude_agl_m,flight_position_world_z_m,"
	"flight_mach,flight_g_load,flight_angle_of_attack_deg,"
	"flight_angle_of_slide_deg,flight_atmosphere_temperature_k,"
	"force_moment_force_x_N,force_moment_force_y_N,force_moment_force_z_N,"
	"force_moment_moment_x_N_m,force_moment_moment_y_N_m,force_moment_moment_z_N_m,"
	"force_moment_center_of_mass_x_m,force_moment_center_of_mass_y_m,"
	"force_moment_center_of_mass_z_m,"
	"engine_0_switch_on,engine_0_throttle_input,engine_0_throttle_output,"
	"engine_0_power_readout,engine_0_thrust_force_N,engine_0_afterburner_ratio,"
	"engine_0_afterburner_lit,engine_0_nozzle_aperture,"
	"engine_1_switch_on,engine_1_throttle_input,engine_1_throttle_output,"
	"engine_1_power_readout,engine_1_thrust_force_N,engine_1_afterburner_ratio,"
	"engine_1_afterburner_lit,engine_1_nozzle_aperture,"
	"controls_pitch_input,controls_roll_input,controls_yaw_input,"
	"controls_elevator_command,controls_aileron_command,controls_rudder_command,"
	"controls_flaps_position,controls_slats_position,controls_airbrake_position,"
	"landing_gear_gear_position,landing_gear_nose_wheel_steering,"
	"landing_gear_brake_left,landing_gear_brake_right,"
	"landing_gear_wheel_spin_0,landing_gear_wheel_spin_1,landing_gear_wheel_spin_2,"
	"suspension_wheel_0_acting_force_x_N,suspension_wheel_0_acting_force_y_N,"
	"suspension_wheel_0_acting_force_z_N,suspension_wheel_0_compression_m,"
	"suspension_wheel_0_force_magnitude_N,suspension_wheel_0_weight_on_wheel,"
	"suspension_wheel_1_acting_force_x_N,suspension_wheel_1_acting_force_y_N,"
	"suspension_wheel_1_acting_force_z_N,suspension_wheel_1_compression_m,"
	"suspension_wheel_1_force_magnitude_N,suspension_wheel_1_weight_on_wheel,"
	"suspension_wheel_2_acting_force_x_N,suspension_wheel_2_acting_force_y_N,"
	"suspension_wheel_2_acting_force_z_N,suspension_wheel_2_compression_m,"
	"suspension_wheel_2_force_magnitude_N,suspension_wheel_2_weight_on_wheel,"
	"suspension_any_weight_on_wheels,suspension_on_ground,"
	"fuel_internal,fuel_external,fuel_total,shake_amplitude\n";

class CsvRowBuilder final
{
public:
	explicit CsvRowBuilder(DcsBridge::Internal::FormattedStateCsvRow& row)
		: row_(row)
	{
	}

	bool append_uint64(std::uint64_t value)
	{
		char value_text[kUnsignedIntegerTextCapacity];
		const std::to_chars_result result =
			std::to_chars(value_text, value_text + sizeof(value_text), value);
		return result.ec == std::errc() &&
			append_cell(value_text, static_cast<size_t>(result.ptr - value_text));
	}

	bool append_double(double value, bool available = true)
	{
		if (!available)
		{
			return append_text("-");
		}
		char value_text[kDoubleTextCapacity];
		const std::to_chars_result result = std::to_chars(
			value_text,
			value_text + sizeof(value_text),
			value,
			std::chars_format::general,
			kRoundTripDoubleDigits);
		return result.ec == std::errc() &&
			append_cell(value_text, static_cast<size_t>(result.ptr - value_text));
	}

	bool append_bool(bool value, bool available = true)
	{
		return available
			? append_text(value ? "True" : "False")
			: append_text("-");
	}

	bool finish()
	{
		if (row_.size + 2 > row_.data.size())
		{
			return false;
		}
		row_.data[row_.size++] = '\n';
		row_.data[row_.size] = '\0';
		row_.valid = true;
		return true;
	}

private:
	bool append_text(const char* value)
	{
		return append_cell(value, strlen(value));
	}

	bool append_cell(const char* value, size_t length)
	{
		const size_t separator_size = first_cell_ ? 0 : 1;
		if (row_.size + separator_size + length + 1 > row_.data.size())
		{
			return false;
		}
		if (!first_cell_)
		{
			row_.data[row_.size++] = ',';
		}
		memcpy(row_.data.data() + row_.size, value, length);
		row_.size += length;
		first_cell_ = false;
		return true;
	}

	DcsBridge::Internal::FormattedStateCsvRow& row_;
	bool first_cell_ = true;
};

bool append_flight(CsvRowBuilder& row, const Core::FrameOutput& output)
{
	const Core::FrameDataAvailability& available = output.availability;
	const Core::FlightOutput& flight = output.flight;
	return row.append_double(flight.altitude_asl_m, available.atmosphere) &&
		row.append_double(
			flight.altitude_agl_m,
			available.atmosphere && available.surface) &&
		row.append_double(flight.position_world_z_m, available.world_kinematics) &&
		row.append_double(
			flight.mach,
			available.atmosphere && available.world_kinematics) &&
		row.append_double(flight.g_load, available.body_kinematics) &&
		row.append_double(flight.angle_of_attack_deg, available.body_kinematics) &&
		row.append_double(flight.angle_of_slide_deg, available.body_kinematics) &&
		row.append_double(flight.atmosphere_temperature_k, available.atmosphere);
}

bool append_vector(CsvRowBuilder& row, const Common::Vec3& value, bool available = true)
{
	return row.append_double(value.x, available) &&
		row.append_double(value.y, available) &&
		row.append_double(value.z, available);
}

bool append_force_moment(CsvRowBuilder& row, const Core::FrameOutput& output)
{
	return append_vector(row, output.force_moment.force) &&
		append_vector(row, output.force_moment.moment) &&
		append_vector(
			row,
			output.force_moment.center_of_mass,
			output.availability.mass);
}

bool append_engine(CsvRowBuilder& row, const Core::EngineOutput& engine)
{
	return row.append_bool(engine.switch_on) &&
		row.append_double(engine.throttle_input) &&
		row.append_double(engine.throttle_output) &&
		row.append_double(engine.power_readout) &&
		row.append_double(engine.thrust_force) &&
		row.append_double(engine.afterburner_ratio) &&
		row.append_bool(engine.afterburner_lit) &&
		row.append_double(engine.nozzle_aperture);
}

bool append_controls(CsvRowBuilder& row, const Core::ControlOutput& controls)
{
	return row.append_double(controls.pitch_input) &&
		row.append_double(controls.roll_input) &&
		row.append_double(controls.yaw_input) &&
		row.append_double(controls.elevator_command) &&
		row.append_double(controls.aileron_command) &&
		row.append_double(controls.rudder_command) &&
		row.append_double(controls.flaps_position) &&
		row.append_double(controls.slats_position) &&
		row.append_double(controls.airbrake_position);
}

bool append_landing_gear(CsvRowBuilder& row, const Core::LandingGearOutput& gear)
{
	return row.append_double(gear.gear_position) &&
		row.append_double(gear.nose_wheel_steering) &&
		row.append_double(gear.brake_left) &&
		row.append_double(gear.brake_right) &&
		row.append_double(gear.wheel_spin[0]) &&
		row.append_double(gear.wheel_spin[1]) &&
		row.append_double(gear.wheel_spin[2]);
}

bool append_suspension_wheel(
	CsvRowBuilder& row,
	const Core::SuspensionWheelOutput& wheel,
	bool available)
{
	return append_vector(row, wheel.acting_force, available) &&
		row.append_double(wheel.compression, available) &&
		row.append_double(wheel.force_magnitude, available) &&
		row.append_bool(wheel.weight_on_wheel, available);
}

bool append_suspension(CsvRowBuilder& row, const Core::FrameOutput& output)
{
	const Core::SuspensionOutput& suspension = output.suspension;
	const auto& available = output.availability.suspension;
	const bool any_available = available[0] || available[1] || available[2];
	return append_suspension_wheel(row, suspension.wheels[0], available[0]) &&
		append_suspension_wheel(row, suspension.wheels[1], available[1]) &&
		append_suspension_wheel(row, suspension.wheels[2], available[2]) &&
		row.append_bool(suspension.any_weight_on_wheels, any_available) &&
		row.append_bool(suspension.on_ground, any_available);
}

bool append_fuel(CsvRowBuilder& row, const Core::FuelOutput& fuel)
{
	return row.append_double(fuel.internal_fuel) &&
		row.append_double(fuel.external_fuel) &&
		row.append_double(fuel.total_fuel);
}

int io_error_code()
{
	return errno != 0 ? errno : EIO;
}
}

namespace DcsBridge
{
namespace Internal
{
void LatestTelemetryMailbox::publish(
	std::uint64_t flight_id,
	std::uint64_t sequence,
	const Core::FrameOutput& output)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		pending_ = { next_publish_version_++, flight_id, sequence, output };
	}
	condition_.notify_one();
}

std::optional<TelemetryRecord> LatestTelemetryMailbox::take_latest()
{
	std::lock_guard<std::mutex> lock(mutex_);
	std::optional<TelemetryRecord> result = pending_;
	pending_.reset();
	return result;
}

MailboxRead LatestTelemetryMailbox::wait_until(
	const std::chrono::steady_clock::time_point& deadline)
{
	std::unique_lock<std::mutex> lock(mutex_);
	const auto ready = [this]() { return pending_.has_value() || stopping_; };
	if (deadline == std::chrono::steady_clock::time_point::max())
	{
		condition_.wait(lock, ready);
	}
	else
	{
		(void)condition_.wait_until(lock, deadline, ready);
	}
	MailboxRead result = { pending_, stopping_ };
	pending_.reset();
	return result;
}

void LatestTelemetryMailbox::stop()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		stopping_ = true;
	}
	condition_.notify_one();
}

const char* state_csv_header()
{
	return kStateCsvHeader;
}

std::size_t state_csv_header_size()
{
	return strlen(kStateCsvHeader);
}

FormattedStateCsvRow format_state_csv_row(const TelemetryRecord& record)
{
	FormattedStateCsvRow result;
	CsvRowBuilder row(result);
	const Core::FrameOutput& output = record.output;
	const bool appended = row.append_uint64(record.sequence) &&
		row.append_double(output.simulation_time_s) &&
		append_flight(row, output) &&
		append_force_moment(row, output) &&
		append_engine(row, output.engines[0]) &&
		append_engine(row, output.engines[1]) &&
		append_controls(row, output.controls) &&
		append_landing_gear(row, output.landing_gear) &&
		append_suspension(row, output) &&
		append_fuel(row, output.fuel) &&
		row.append_double(output.shake_amplitude);
	if (appended)
	{
		(void)row.finish();
	}
	return result;
}

StateCsvWriter::StateCsvWriter(const char* module_root, EventLog& event_log)
	: event_log_(event_log),
	last_flush_time_(std::chrono::steady_clock::now())
{
	Common::copy_path(module_root_, sizeof(module_root_), module_root);
	initialize_execution_file();
	start_worker();
}

StateCsvWriter::~StateCsvWriter()
{
	mailbox_.stop();
	if (worker_.joinable())
	{
		worker_.join();
	}
	close_file();
}

void StateCsvWriter::publish_start(const Core::FrameOutput& output)
{
	std::lock_guard<std::mutex> lock(publication_mutex_);
	++current_flight_id_;
	next_sequence_ = 0;
	mailbox_.publish(current_flight_id_, next_sequence_, output);
}

void StateCsvWriter::publish_step(const Core::FrameOutput& output)
{
	std::lock_guard<std::mutex> lock(publication_mutex_);
	++next_sequence_;
	mailbox_.publish(current_flight_id_, next_sequence_, output);
}

bool StateCsvWriter::is_ready() const
{
	return ready_.load();
}

int StateCsvWriter::last_error_code() const
{
	return last_error_code_.load();
}

void StateCsvWriter::initialize_execution_file()
{
	const LogFilePreparation location =
		prepare_rotating_log_file(module_root_, kStateCsvFileName);
	Common::copy_path(active_path_, sizeof(active_path_), location.active_path);
	if (!location.ready)
	{
		report_error({ location.failed_operation, location.error_code, std::nullopt, 0 });
		return;
	}
	rotation_complete_ = true;
	file_ = _fsopen(active_path_, "wb", _SH_DENYNO);
	if (file_ == nullptr)
	{
		report_error({ "open", io_error_code(), std::nullopt, 0 });
		return;
	}
	if (!write_header())
	{
		report_error({ "write_header", io_error_code(), std::nullopt, 0 });
		close_file();
		return;
	}
	header_written_ = true;
	ready_.store(true);
}

void StateCsvWriter::start_worker()
{
	try
	{
		worker_ = std::thread(&StateCsvWriter::worker_loop, this);
	}
	catch (const std::system_error& error)
	{
		const int error_code = error.code().value() != 0
			? error.code().value()
			: EAGAIN;
		report_error({ "start_worker", error_code, std::nullopt, 0 });
		ready_.store(false);
		close_file();
	}
}

void StateCsvWriter::worker_loop()
{
	while (true)
	{
		const auto deadline = dirty_
			? last_flush_time_ + kCsvFlushInterval
			: std::chrono::steady_clock::time_point::max();
		const MailboxRead incoming = mailbox_.wait_until(deadline);
		if (incoming.record)
		{
			process_record(*incoming.record);
		}
		flush_if_due(incoming.stopping);
		if (incoming.stopping)
		{
			return;
		}
	}
}

void StateCsvWriter::process_record(const TelemetryRecord& record)
{
	if (file_ == nullptr && failed_flight_id_ == record.flight_id)
	{
		return;
	}
	if (file_ == nullptr && !retry_file(record))
	{
		return;
	}
	const FormattedStateCsvRow row = format_state_csv_row(record);
	if (!row.valid)
	{
		fail_current_flight({
			"format_row", EINVAL, record.output.simulation_time_s, record.flight_id });
		return;
	}
	if (fwrite(row.data.data(), 1, row.size, file_) != row.size)
	{
		fail_current_flight({
			"write_row", io_error_code(), record.output.simulation_time_s, record.flight_id });
		return;
	}
	dirty_ = true;
	last_written_record_ = record;
}

bool StateCsvWriter::retry_file(const TelemetryRecord& record)
{
	if (!rotation_complete_)
	{
		const LogFilePreparation location =
			prepare_rotating_log_file(module_root_, kStateCsvFileName);
		Common::copy_path(active_path_, sizeof(active_path_), location.active_path);
		if (!location.ready)
		{
			fail_current_flight({
				location.failed_operation,
				location.error_code,
				record.output.simulation_time_s,
				record.flight_id });
			return false;
		}
		rotation_complete_ = true;
	}
	file_ = _fsopen(active_path_, header_written_ ? "ab" : "wb", _SH_DENYNO);
	if (file_ == nullptr)
	{
		fail_current_flight({
			"open", io_error_code(), record.output.simulation_time_s, record.flight_id });
		return false;
	}
	if (!header_written_ && !write_header())
	{
		fail_current_flight({
			"write_header", io_error_code(), record.output.simulation_time_s, record.flight_id });
		return false;
	}
	header_written_ = true;
	failed_flight_id_ = 0;
	last_error_code_.store(0);
	ready_.store(true);
	return true;
}

bool StateCsvWriter::write_header()
{
	const size_t length = state_csv_header_size();
	return fwrite(state_csv_header(), 1, length, file_) == length &&
		fflush(file_) == 0;
}

void StateCsvWriter::flush_if_due(bool force)
{
	if (!dirty_ || file_ == nullptr)
	{
		return;
	}
	const auto now = std::chrono::steady_clock::now();
	if (!force && now < last_flush_time_ + kCsvFlushInterval)
	{
		return;
	}
	if (fflush(file_) != 0)
	{
		fail_current_flight({
			"flush",
			io_error_code(),
			last_written_record_.output.simulation_time_s,
			last_written_record_.flight_id });
		return;
	}
	dirty_ = false;
	last_flush_time_ = now;
}

void StateCsvWriter::fail_current_flight(const Failure& failure)
{
	failed_flight_id_ = failure.flight_id;
	dirty_ = false;
	ready_.store(false);
	close_file();
	report_error(failure);
}

void StateCsvWriter::report_error(const Failure& failure)
{
	last_error_code_.store(failure.error_code);
	char message[kCsvErrorMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"csv operation=%s path=%s os_error=%d",
		failure.operation ? failure.operation : "unknown",
		active_path_[0] != '\0' ? active_path_ : "<unresolved>",
		failure.error_code);
	(void)event_log_.write({ EventLevel::Error, failure.simulation_time_s, message });
}

void StateCsvWriter::close_file()
{
	if (file_ != nullptr)
	{
		fclose(file_);
		file_ = nullptr;
	}
}
}
}
