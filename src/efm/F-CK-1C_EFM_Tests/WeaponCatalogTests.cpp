#include "TestHarness.h"
#include "Common/Configuration/Configuration.h"
#include "Core/Systems/MissionComputer/Database/WeaponCatalogData.h"

namespace
{
using namespace Core;
using namespace Core::Systems::Mc;
void expect_rejected(Tests::Context& c, const std::string& source)
{
	bool rejected = false;
	try
	{
		auto document = Configuration::Tools::parse_jsonc(source);
		WeaponCatalogConfiguration configuration;
		read_configuration(document.root(), configuration);
	}
	catch (const Configuration::Error&)
	{
		rejected = true;
	}
	TEST_EXPECT(c, rejected);
}
}
void run_weapon_catalog_tests(Tests::Context& c)
{
	const auto loaded = Configuration::load<WeaponCatalogConfiguration>("Weapons/WeaponCatalog.json");
	TEST_EXPECT(c, loaded.succeeded());
	if (!loaded.succeeded())
		return;
	TEST_EXPECT(c, loaded.value().entries.size() == 46);
	WeaponCatalogData catalog(loaded.value());
	for (const auto& entry : loaded.value().entries)
		for (const auto& identifier : entry.identifiers)
		{
			const auto found = catalog.find(identifier);
			TEST_EXPECT(c, found && found->id == entry.weapon->id);
		}
	for (const char* identifier : { "{AIM-9P}", "{AIM-9P3}", "{AIM-9P5}", "{AIM-9M}", "{AIM-9X}" })
	{
		const auto weapon = catalog.find(identifier);
		TEST_EXPECT(c,
		            weapon && weapon->native_ir && weapon->family == "aim-9" && weapon->selection_group == "aim-9" &&
		                weapon->procedure == WeaponProcedure::Sidewinder);
	}
	const auto sparrow = catalog.find("{AIM-7P}");
	TEST_EXPECT(c, sparrow && sparrow->procedure == WeaponProcedure::Sparrow);
	const auto amraam = catalog.find("{40EF17B7-F508-45de-8566-6FFECC0C1AB8}");
	TEST_EXPECT(c, amraam && amraam->procedure == WeaponProcedure::Amraam);
	TEST_EXPECT(c, catalog.find("new-air-to-air-missile") == nullptr);
	WeaponDefinitionRef retained;
	{
		WeaponCatalogData temporary;
		retained = temporary.find("{AIM-9P5}");
	}
	TEST_EXPECT(c, retained && retained->name == "AIM-9P5");
	TEST_EXPECT(c, retained.use_count() == 1);
	TEST_EXPECT(c, sizeof(WeaponDefinitionRef) <= 2 * sizeof(void*));
	const auto source = Configuration::Tools::read_file("FM/Weapons/WeaponCatalog.json");
	const auto invalid = [&](const std::string& from, const std::string& to)
	{
		auto text = source;
		const auto at = text.find(from);
		TEST_EXPECT(c, at != std::string::npos);
		if (at != std::string::npos)
		{
			text.replace(at, from.size(), to);
			expect_rejected(c, text);
		}
	};
	invalid("\"schema_version\": 1", "\"schema_version\": 2");
	invalid("\"id\": \"aam_medium\"", "\"id\": \"aam_light\"");
	invalid("\"id\": \"aim-9e\"", "\"id\": \"aim-9b\"");
	invalid("\"clsid\": \"{AIM-9E}\"", "\"clsid\": \"{AIM-9B}\"");
	invalid("\"guidance\": \"infrared\"", "\"guidance\": \"active_radar\"");
	invalid("\"procedure\": \"aim9\"", "\"procedure\": \"aim7\"");
	invalid("\"group\": \"aam_light\"", "\"group\": \"unknown\"");
	invalid("\"category\": \"missile\"", "\"category\": \"bomb\"");
}
