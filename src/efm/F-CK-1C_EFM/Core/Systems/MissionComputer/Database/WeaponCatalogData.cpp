#include "WeaponCatalogData.h"
#include "Common/Configuration/Configuration.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
using Configuration::Value;
template <class T> T enumeration(const Value& value, std::initializer_list<std::pair<const char*, T>> choices)
{
	const auto text = value.string();
	for (const auto& choice : choices)
		if (text == choice.first)
			return choice.second;
	value.require(false, "Unknown weapon classification");
	throw std::logic_error("Unreachable weapon classification");
}
std::string identity(const Value& value)
{
	auto text = value.string();
	value.require(!text.empty() && text.size() <= 128, "Weapon identity must contain 1 to 128 bytes");
	return text;
}
Core::Systems::Mc::WeaponCatalogConfiguration load_catalog()
{
	auto result = Configuration::load<Core::Systems::Mc::WeaponCatalogConfiguration>("Weapons/WeaponCatalog.json");
	if (!result.succeeded())
		throw std::runtime_error("Weapon catalog initialization failed");
	return std::move(result.value());
}
}

namespace Core::Systems::Mc
{
void read_configuration(const Configuration::Value& value, WeaponCatalogConfiguration& result)
{
	value.object({ "schema_version", "groups", "stores" });
	value.member("schema_version")
	    .require(value.member("schema_version").number() == 1, "Unsupported weapon catalog schema");
	const auto groups = value.member("groups");
	std::vector<std::string> group_ids;
	for (std::size_t i = 0; i < groups.array_size(); ++i)
	{
		const auto group = groups.element(i);
		group.object({ "id", "air_to_air" });
		const auto id = identity(group.member("id"));
		group.require(std::find(group_ids.begin(), group_ids.end(), id) == group_ids.end(), "Duplicate weapon group");
		(void)group.member("air_to_air").boolean();
		group_ids.push_back(id);
	}
	groups.require(!group_ids.empty(), "Weapon catalog requires loadout groups");
	const auto stores = value.member("stores");
	stores.require(stores.array_size() > 0, "Weapon catalog requires registered stores");
	std::vector<std::string> ids, identifiers;
	result.entries.clear();
	for (std::size_t i = 0; i < stores.array_size(); ++i)
	{
		const auto item = stores.element(i);
		item.object({ "id",
		              "name",
		              "family",
		              "category",
		              "role",
		              "guidance",
		              "selection_group",
		              "procedure",
		              "native_ir",
		              "radar_cue",
		              "aliases",
		              "loadouts" });
		WeaponCatalogEntry entry;
		auto& weapon = entry.weapon;
		weapon.registered = true;
		weapon.id = identity(item.member("id"));
		item.require(std::find(ids.begin(), ids.end(), weapon.id) == ids.end(), "Duplicate weapon model");
		ids.push_back(weapon.id);
		weapon.name = identity(item.member("name"));
		weapon.family = identity(item.member("family"));
		weapon.selection_group = identity(item.member("selection_group"));
		weapon.category = enumeration<WeaponCategory>(item.member("category"),
		                                              { { "missile", WeaponCategory::Missile },
		                                                { "bomb", WeaponCategory::Bomb },
		                                                { "tank", WeaponCategory::FuelTank },
		                                                { "pod", WeaponCategory::Pod },
		                                                { "rack", WeaponCategory::Rack },
		                                                { "other", WeaponCategory::Other } });
		weapon.role = enumeration<WeaponRole>(item.member("role"),
		                                      { { "air_to_air", WeaponRole::AirToAir },
		                                        { "air_to_ground", WeaponRole::AirToGround },
		                                        { "support", WeaponRole::Support },
		                                        { "other", WeaponRole::Other } });
		weapon.guidance = enumeration<WeaponGuidance>(item.member("guidance"),
		                                              { { "none", WeaponGuidance::None },
		                                                { "infrared", WeaponGuidance::Infrared },
		                                                { "semi_active_radar", WeaponGuidance::SemiActiveRadar },
		                                                { "active_radar", WeaponGuidance::ActiveRadar },
		                                                { "laser", WeaponGuidance::Laser },
		                                                { "satellite", WeaponGuidance::Satellite },
		                                                { "unknown", WeaponGuidance::Unknown } });
		weapon.procedure = enumeration<WeaponProcedure>(item.member("procedure"),
		                                                { { "unsupported", WeaponProcedure::Unsupported },
		                                                  { "aim9", WeaponProcedure::Sidewinder },
		                                                  { "aim7", WeaponProcedure::Sparrow },
		                                                  { "aim120", WeaponProcedure::Amraam } });
		weapon.native_ir = item.member("native_ir").boolean();
		weapon.radar_cue = item.member("radar_cue").boolean();
		item.require(!weapon.radar_cue || weapon.native_ir, "Radar cue requires the native infrared interface");
		item.require(!weapon.native_ir || weapon.procedure == WeaponProcedure::Sidewinder,
		             "Native infrared interface requires the Sidewinder procedure");
		if (weapon.procedure != WeaponProcedure::Unsupported)
		{
			item.require(weapon.category == WeaponCategory::Missile && weapon.role == WeaponRole::AirToAir,
			             "Missile procedure requires an air-to-air missile");
			const auto guidance = weapon.procedure == WeaponProcedure::Sidewinder ? WeaponGuidance::Infrared
			                      : weapon.procedure == WeaponProcedure::Sparrow  ? WeaponGuidance::SemiActiveRadar
			                                                                      : WeaponGuidance::ActiveRadar;
			item.require(weapon.guidance == guidance, "Missile procedure and guidance must agree");
		}
		const auto add_identifier = [&](const Value& source)
		{
			const auto id = identity(source);
			source.require(std::find(identifiers.begin(), identifiers.end(), id) == identifiers.end(),
			               "Duplicate DCS weapon identifier");
			identifiers.push_back(id);
			entry.identifiers.push_back(id);
		};
		const auto aliases = item.member("aliases");
		for (std::size_t j = 0; j < aliases.array_size(); ++j)
			add_identifier(aliases.element(j));
		const auto loadouts = item.member("loadouts");
		loadouts.require(loadouts.array_size() > 0, "Weapon model requires a registered loadout");
		for (std::size_t j = 0; j < loadouts.array_size(); ++j)
		{
			const auto loadout = loadouts.element(j);
			loadout.object({ "clsid", "group", "parameters" });
			add_identifier(loadout.member("clsid"));
			const auto group = identity(loadout.member("group"));
			loadout.require(std::find(group_ids.begin(), group_ids.end(), group) != group_ids.end(),
			                "Unknown loadout group");
		}
		result.entries.push_back(std::move(entry));
	}
}

WeaponCatalogData::WeaponCatalogData()
    : WeaponCatalogData(load_catalog())
{
}
WeaponCatalogData::WeaponCatalogData(WeaponCatalogConfiguration configuration)
    : configuration_(std::move(configuration))
{
	for (std::size_t i = 0; i < configuration_.entries.size(); ++i)
		for (const auto& identifier : configuration_.entries[i].identifiers)
			if (!identifiers_.emplace(identifier, i).second)
				throw std::invalid_argument("Duplicate DCS weapon identifier");
}
const WeaponDescriptor* WeaponCatalogData::find(std::string_view identifier) const
{
	const auto found = identifiers_.find(std::string(identifier));
	return found == identifiers_.end() ? nullptr : &configuration_.entries[found->second].weapon;
}
}
