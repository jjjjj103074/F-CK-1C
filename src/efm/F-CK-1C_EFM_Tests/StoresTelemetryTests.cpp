#include "SystemPipelineTestFixture.h"

#include "DcsBridge/Internal/DebugTelemetry/DebugTelemetryHub.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>

namespace Core::Systems::Catalog::MissionComputer
{
SystemEntry create_entry();
}

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace std::chrono;
constexpr nanoseconds kPeriod(31'250'000);

struct Fixture
{
	DcsBridge::Internal::DebugTelemetryHub hub{ DebugTelemetryPublishErrorPolicy::ReportAndRethrow };
	std::unique_ptr<SystemPipeline> pipeline;
	FrameInput frame;

	Fixture()
	{
		hub.begin_flight();
		pipeline =
		    std::make_unique<SystemPipeline>(FlightSetupContext{ StartMode::HotGround, {}, {}, hub },
		                                     std::vector<SystemEntry>{ Catalog::MissionComputer::create_entry() });
		hub.seal_schema();
		frame.cockpit.mission_adapter.available = true;
		for (auto& station : frame.cockpit.weapon_stations.stations)
			station.configuration = StoreConfiguration::Empty;
	}

	void step(nanoseconds time)
	{
		SystemPipelineTest::step_pipeline_to(*pipeline, frame, {}, time);
	}

	template <typename T> T value(const std::string& name)
	{
		const auto snapshot = hub.latest_snapshot();
		const auto channel = std::find_if(snapshot.channels.begin(),
		                                  snapshot.channels.end(),
		                                  [&](const DebugTelemetryChannelDescriptor& descriptor)
		                                  {
			                                  return descriptor.name == "mc_stores_" + name;
		                                  });
		if (channel == snapshot.channels.end())
			throw std::runtime_error("Missing stores diagnostic channel: " + name);
		return std::get<T>(snapshot.values[static_cast<std::size_t>(channel - snapshot.channels.begin())].value());
	}
};

void test_inventory_diagnostics_follow_real_mc_steps(Tests::Context& context)
{
	Fixture f;
	auto& source = f.frame.cockpit.weapon_stations;
	source.status = { true, 2, ObservationInvalidReason::None };
	source.stations[0] = {
		StoreConfiguration::Equipped, 3, WeaponCategory::Other, WeaponRole::Other, "bomb-rack", "4.5.9.42", "RT"
	};
	source.stations[8] = {
		StoreConfiguration::Equipped, 2, WeaponCategory::Missile, WeaponRole::Other, "opaque;\"id\"", "4.4.8.99", "LT"
	};
	f.frame.cockpit.gun_ammunition = { { true, 2, ObservationInvalidReason::None }, 0 };
	f.step(kPeriod);
	TEST_EXPECT(context, f.value<bool>("observation_available"));
	TEST_EXPECT(context, f.value<std::string>("observation_revision") == "2");
	TEST_EXPECT(context, f.value<std::int64_t>("missile_count") == 2);
	TEST_EXPECT(context, f.value<std::int64_t>("aa_count") == 0);
	TEST_EXPECT(context, f.value<bool>("missile_count_complete"));
	TEST_EXPECT(context, f.value<std::string>("station_01").find("Q=3 OTHER/OTHER") != std::string::npos);
	TEST_EXPECT(context,
	            f.value<std::string>("station_09").find("LT CONFIG=EQUIPPED Q=2 MSL/OTHER L=1 OK=1") !=
	                std::string::npos);
	TEST_EXPECT(context, f.value<std::string>("station_09_identity") == "TYPE=4.4.8.99;CLSID=opaque;\"id\"");
	TEST_EXPECT(context, f.value<std::string>("station_09_input").find("CLSID=opaque;\"id\"") != std::string::npos);
	TEST_EXPECT(context, f.value<bool>("gun_api_available"));
	TEST_EXPECT(context, f.value<bool>("gun_available"));
	TEST_EXPECT(context, f.value<std::int64_t>("gun_source") == 1);
	TEST_EXPECT(context, f.value<double>("gun_api_count") == 0.0);
	TEST_EXPECT_NEAR(context, f.value<double>("gun_count"), 0.0, 1.0e-12);
	f.frame.cockpit.gun_ammunition = { { true, 4, ObservationInvalidReason::None }, 17 };
	f.step(kPeriod * 2);
	TEST_EXPECT_NEAR(context, f.value<double>("gun_api_count"), 17.0, 1.0e-12);
	TEST_EXPECT(context, f.value<std::string>("observation_revision") == "2");
	TEST_EXPECT_NEAR(context, f.value<double>("observation_age_s"), 0.03125, 1.0e-12);
	source.status = { false, 4, ObservationInvalidReason::StationApiError };
	f.frame.cockpit.gun_ammunition.status.available = false;
	f.step(kPeriod * 3);
	TEST_EXPECT(context, !f.value<bool>("observation_available"));
	TEST_EXPECT(context, f.value<std::int64_t>("observation_reason") == 5);
	TEST_EXPECT(context, f.value<std::string>("station_09_input") == "UNAVAILABLE");
	TEST_EXPECT(context, f.value<std::string>("station_09").find("Q=2 MSL/OTHER L=1 OK=0") != std::string::npos);
	TEST_EXPECT(context, f.value<std::string>("station_09_identity") == "TYPE=4.4.8.99;CLSID=opaque;\"id\"");
	TEST_EXPECT(context, !f.value<bool>("missile_count_complete"));
	TEST_EXPECT(context, !f.value<bool>("gun_api_available"));
	source.status = { true, 6, ObservationInvalidReason::None };
	source.rearm_revision = 1;
	source.stations[8] = {
		StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::AirToAir, "replacement", "4.4.7.7", "LT"
	};
	f.step(kPeriod * 4);
	TEST_EXPECT(context, f.value<std::string>("rearm_revision") == "1");
	TEST_EXPECT(context, f.value<std::int64_t>("aa_count") == 1);
	TEST_EXPECT(context, f.value<std::string>("station_09").find("Q=1 MSL/AA L=2 OK=1") != std::string::npos);
	TEST_EXPECT(context, f.value<std::string>("station_09_identity") == "TYPE=4.4.7.7;CLSID=replacement");
	TEST_EXPECT(context, !f.hub.status().publication_failed);
	const auto samples = f.hub.sample_through(kPeriod * 4);
	TEST_EXPECT(context, samples.size() == 9);
	TEST_EXPECT(context, f.hub.schema().size() <= 60);
}

void test_unknown_classification_is_visible(Tests::Context& context)
{
	Fixture f;
	auto& source = f.frame.cockpit.weapon_stations;
	source.status = { true, 2, ObservationInvalidReason::None };
	source.stations[4] = {
		StoreConfiguration::Equipped, 1, WeaponCategory::Unknown, WeaponRole::Unknown, "unclassified", "0.0.0.0", "MF"
	};
	f.step(kPeriod);
	TEST_EXPECT(context, f.value<bool>("observation_available"));
	TEST_EXPECT(context, !f.value<bool>("missile_count_complete"));
	TEST_EXPECT(context, !f.value<bool>("aa_count_complete"));
	TEST_EXPECT(context, f.value<std::string>("station_05").find("UNKNOWN/UNKNOWN") != std::string::npos);
	TEST_EXPECT(context, f.value<std::string>("station_05_identity").find("unclassified") != std::string::npos);
}
void test_selection_and_stale_inventory_are_visible(Tests::Context& context)
{
	Fixture f;
	auto& source = f.frame.cockpit.weapon_stations;
	source.status = { true, 2, ObservationInvalidReason::None };
	source.stations[2] = { StoreConfiguration::Equipped, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "aa" };
	f.pipeline->send({ CommandId::SelectDogfightMode, 1.0 });
	f.step(kPeriod);
	TEST_EXPECT(context, f.value<std::int64_t>("selection_requested") == 2);
	TEST_EXPECT(context, f.value<std::int64_t>("selection_acknowledged") == -1);
	TEST_EXPECT(context, f.value<std::string>("selection_request_revision") == "1");
	TEST_EXPECT(context, f.value<bool>("selection_pending"));
	source.selected_station = 2;
	source.selection_ack_revision = 1;
	source.status.revision = 4;
	f.step(kPeriod * 2);
	TEST_EXPECT(context, f.value<std::int64_t>("selection_acknowledged") == 2);
	TEST_EXPECT(context, f.value<std::string>("selection_acknowledged_revision") == "1");
	TEST_EXPECT(context, !f.value<bool>("selection_pending"));
	f.step(kPeriod * 20);
	TEST_EXPECT(context, !f.value<bool>("observation_available"));
	TEST_EXPECT(context, f.value<std::int64_t>("observation_reason") == 7);
	TEST_EXPECT(context, !f.value<bool>("aa_count_complete"));
	TEST_EXPECT(context, f.value<std::string>("station_03").find("Q=2 MSL/AA L=1 OK=0") != std::string::npos);
}

}

void run_stores_telemetry_tests(Tests::Context& context)
{
	test_inventory_diagnostics_follow_real_mc_steps(context);
	test_unknown_classification_is_visible(context);
	test_selection_and_stale_inventory_are_visible(context);
}
