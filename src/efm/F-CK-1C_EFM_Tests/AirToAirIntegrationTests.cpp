#include "TestHarness.h"
#include "Core/Systems/MissionComputer/Components/StoresManagement/StoresManagement.h"
#include "Core/Systems/MissionComputer/Common/MissileSelection.h"
#include "Core/Systems/RadarSystem/Internal/SearchProfiles.h"
#include "Core/Systems/DisplaySystem/Internal/RadarPresentation.h"
namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace Core::Systems::Mc;
struct StoresFixture
{
	MissionInputs input;
	StoresData stores;
	StoresEvents events;
	ExternalActionOutbox actions;
	StoresCommandQueue commands{ actions };
	StoresManagement sm{ input, stores, events, commands, actions };
	StoresFixture()
	{
		input.cockpit.weapon_stations.status = { true, 2, ObservationInvalidReason::None };
		for (auto& s : input.cockpit.weapon_stations.stations)
			s.configuration = StoreConfiguration::Empty;
		auto& a = input.cockpit.weapon_stations.stations;
		a[0] = {
			StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::AirToAir, "{AIM-9M}", "m", "RT"
		};
		a[8] = {
			StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::AirToAir, "{AIM-9X}", "x", "LT"
		};
		commands.submit(SelectStoresForMode{ OperatingMode::AirToAir });
		step();
		acknowledge();
	}
	void step()
	{
		sm.step({}, .03125);
	}
	void acknowledge()
	{
		auto& o = input.cockpit.weapon_stations;
		o.selected_station = stores.selection.requested_station;
		o.selection_ack_revision = stores.selection.request_revision;
		o.status.revision += 2;
		step();
	}
};
}
void run_air_to_air_integration_tests(Tests::Context& c)
{
	{
		StoresFixture f;
		TEST_EXPECT(c, f.stores.seeker_control.active && !f.stores.seeker_control.uncaged);
		TEST_EXPECT(c, count_selected_missile_group(f.stores.inventory, 0).count == 2);
		f.commands.submit(SetSeekerUncaged{});
		f.step();
		TEST_EXPECT(c, f.stores.seeker_control.uncaged);
		f.commands.submit(AimIrSeeker{ true, .2, .1 });
		f.step();
		TEST_EXPECT(c, f.stores.seeker_control.radar_slaved);
		f.commands.submit(SelectStoresForMode{ OperatingMode::AirToAir, MissileSelectionStep::Station });
		f.step();
		TEST_EXPECT(c, f.stores.selection.requested_station == 8);
		f.acknowledge();
		TEST_EXPECT(c, f.stores.seeker_control.active && !f.stores.seeker_control.uncaged);
		TEST_EXPECT(c, f.stores.seeker_control.station == 8);
		f.input.cockpit.weapon_stations.stations[0].count = 0;
		f.input.cockpit.weapon_stations.status.revision += 2;
		f.step();
		TEST_EXPECT(c, count_selected_missile_group(f.stores.inventory, 8).count == 1);
		f.commands.submit(SelectStoresForMode{ OperatingMode::Navigation });
		f.step();
		TEST_EXPECT(c, !f.stores.seeker_control.active);
	}
	{
		StoresFixture f;
		f.input.cockpit.weapon_stations.stations[0].weapon_identifier = "{AIM-9P}";
		f.input.cockpit.weapon_stations.status.revision += 2;
		f.step();
		f.commands.submit(AimIrSeeker{ true, .3, .1 });
		f.step();
		TEST_EXPECT(c, !f.stores.seeker_control.radar_slaved);
		TEST_EXPECT_NEAR(c, f.stores.seeker_control.azimuth_rad, 0.0, 1e-12);
	}
	{
		const auto wide = RadarProfiles::air_combat(TargetAcquisitionMode::WideScan);
		const auto vertical = RadarProfiles::air_combat(TargetAcquisitionMode::VerticalScan);
		TEST_EXPECT_NEAR(c, wide.azimuth_width_rad, Common::rad(30), 1e-12);
		TEST_EXPECT_NEAR(c, wide.elevation_height_rad, Common::rad(20), 1e-12);
		TEST_EXPECT_NEAR(c, vertical.elevation_center_rad - vertical.elevation_height_rad / 2, 0.0, 1e-12);
		TEST_EXPECT_NEAR(c, vertical.elevation_center_rad + vertical.elevation_height_rad / 2, Common::rad(60), 1e-12);
		RadarSnapshot radar;
		radar.status = { true, 1 };
		radar.scan = vertical;
		radar.active_search_mode = RadarSearchMode::AirCombat;
		radar.observation.status = { true, 1, ObservationInvalidReason::None };
		radar.observation.mode = RadarMode::Search;
		const auto picture = DisplayPresentation::radar(radar, OperatingMode::Dogfight, {});
		TEST_EXPECT_NEAR(c, picture.capture_center_y_mrad - picture.bore_half_height_mrad, 0.0, 1e-9);
	}
}
