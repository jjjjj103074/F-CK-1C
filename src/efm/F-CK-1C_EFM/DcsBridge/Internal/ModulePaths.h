#pragma once

#include "../../Common/PathUtils.h"
#include <limits>
#include <string>
#include <utility>
#include <windows.h>

namespace DcsBridge
{
inline constexpr std::size_t kInitialModulePathCapacity = 1024;

struct ModulePaths
{
	std::string mod_root_path;
	std::string fm_config_path;
	bool initialized;
};

struct ModulePathSource
{
	const char* config_path;
	const void* module_address;
};

inline void set_project_paths_from_config(ModulePaths& paths, const std::string& cfg_path)
{
	if (cfg_path.empty())
	{
		return;
	}

	paths.fm_config_path = cfg_path;
	Common::normalize_path_separators(paths.fm_config_path);

	std::string fm_dir = paths.fm_config_path;
	Common::path_dirname(fm_dir);

	if (Common::path_has_component_suffix(fm_dir, "FM"))
	{
		paths.mod_root_path = fm_dir;
		Common::path_dirname(paths.mod_root_path);
	}
	else
	{
		paths.mod_root_path = fm_dir;
	}
}

inline bool try_set_mod_root(ModulePaths& paths, const std::string& root)
{
	const std::string cfg_path = Common::build_path({ root.c_str(), "FM\\config.lua" });
	if (!Common::path_file_exists(cfg_path))
	{
		return false;
	}

	paths.mod_root_path = root;
	Common::normalize_path_separators(paths.mod_root_path);
	paths.fm_config_path = cfg_path;
	return true;
}

inline bool current_module_directory(std::string& output, const void* module_address)
{
	HMODULE module = nullptr;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                        reinterpret_cast<LPCSTR>(module_address),
	                        &module))
	{
		return false;
	}
	std::string module_path(kInitialModulePathCapacity, '\0');
	while (true)
	{
		const DWORD length = GetModuleFileNameA(module, module_path.data(), static_cast<DWORD>(module_path.size()));
		if (length == 0)
		{
			return false;
		}
		if (length < module_path.size())
		{
			module_path.resize(length);
			break;
		}
		if (module_path.size() > (std::numeric_limits<DWORD>::max)() / 2)
		{
			return false;
		}
		module_path.resize(module_path.size() * 2);
	}
	Common::normalize_path_separators(module_path);
	Common::path_dirname(module_path);
	output = std::move(module_path);
	return true;
}

inline void normalize_module_root_candidate(std::string& root)
{
	if (Common::path_has_component_suffix(root, "bin"))
	{
		Common::path_dirname(root);
		return;
	}
	const bool legacy_build_folder = Common::path_has_component_suffix(root, "vc100.debug") ||
	                                 Common::path_has_component_suffix(root, "vc100.release");
	if (!legacy_build_folder)
	{
		return;
	}
	Common::path_dirname(root);
	Common::path_dirname(root);
	Common::path_dirname(root);
}

inline bool try_module_directory_candidates(ModulePaths& paths, const std::string& module_dir)
{
	std::string root = module_dir;
	normalize_module_root_candidate(root);
	if (try_set_mod_root(paths, root))
	{
		return true;
	}
	root = module_dir;
	Common::path_dirname(root);
	Common::path_dirname(root);
	Common::path_dirname(root);
	return try_set_mod_root(paths, root);
}

inline void initialize_project_paths(ModulePaths& paths, const void* module_address)
{
	if (paths.initialized)
	{
		return;
	}
	paths.initialized = true;
	if (Common::path_file_exists(paths.fm_config_path))
	{
		set_project_paths_from_config(paths, paths.fm_config_path);
		return;
	}
	std::string module_dir;
	if (!module_address || !current_module_directory(module_dir, module_address))
	{
		return;
	}
	(void)try_module_directory_candidates(paths, module_dir);
}

inline void configure_module_paths(ModulePaths& paths, const ModulePathSource& source)
{
	if (source.config_path && source.config_path[0] != '\0')
	{
		set_project_paths_from_config(paths, source.config_path);
		paths.initialized = true;
		return;
	}

	initialize_project_paths(paths, source.module_address);
}
}
