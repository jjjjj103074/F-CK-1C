#pragma once
#include "../System.h"
#include "../ExternalActionOutbox.h"
#include "../TimedAction.h"
#include "../../Contracts/CockpitContracts.h"
namespace Core::Systems
{
/// @brief 依 Pipeline 要求推進雷達設備操作，發布來源觀測及操作進度。
class RadarSystem final : public System
{
public:
	RadarSystem() = default;
	RadarSystem(const RadarSystem&) = delete;
	RadarSystem& operator=(const RadarSystem&) = delete;
	RadarSystem(RadarSystem&&) = delete;
	RadarSystem& operator=(RadarSystem&&) = delete;
	void setup(SystemSetup& setup) override;
	void step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result) override;

private:
	void update_cursor(const SystemStepContext& context);
	void execute(const RadarRequest& request);
	bool apply_mode(TargetAcquisitionMode mode, std::uint64_t parent);
	void set_scan(RadarSearchMode mode, std::uint64_t parent);
	void begin_acquisition(std::uint64_t parent, AcquisitionInput input);
	void cancel_acquisition(std::uint64_t parent);
	void finish_acquisition(std::uint64_t parent);
	RadarSnapshot output_;
	ExternalActionOutbox actions_;
	TimedAction acquisition_;
	std::uint64_t last_request_ = 0;
	bool radar_on_requested_ = false;
	bool stt_cancel_requested_ = false;
	std::array<bool, 4> cursor_held_{};
	bool cursor_initialized_ = false;
	double cursor_azimuth_rad_ = 0.0;
	double cursor_range_m_ = 0.0;
	std::chrono::nanoseconds cursor_updated_at_{};
};
}
