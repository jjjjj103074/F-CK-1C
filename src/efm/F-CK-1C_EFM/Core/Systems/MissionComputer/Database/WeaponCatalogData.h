#pragma once

#include "../../../Contracts/WeaponContracts.h"
#include "Common/Configuration/ConfigurationTools.h"
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Core::Systems::Mc
{
struct WeaponCatalogEntry
{
	WeaponDefinitionRef weapon;
	std::vector<std::string> identifiers;
};
struct WeaponCatalogConfiguration
{
	std::vector<WeaponCatalogEntry> entries;
};
void read_configuration(const Configuration::Value& value, WeaponCatalogConfiguration& result);

/// @brief 保存本次飛行使用的共用武器定義，以 DCS 掛載識別查詢。
class WeaponCatalogData final
{
public:
	WeaponCatalogData();
	explicit WeaponCatalogData(WeaponCatalogConfiguration configuration);
	WeaponDefinitionRef find(std::string_view identifier) const;

private:
	WeaponCatalogConfiguration configuration_;
	std::unordered_map<std::string, std::size_t> identifiers_;
};
}
