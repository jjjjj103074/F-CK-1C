#pragma once

#include "../Common/PathUtils.h"
#include <windows.h>

namespace DcsBridge
{
static const size_t kModulePathMax = 1024;

struct ModulePaths
{
	char mod_root_path[kModulePathMax];
	char fm_config_path[kModulePathMax];
	bool initialized;
};

inline void set_project_paths_from_config(ModulePaths& paths, const char* cfg_path)
{
	if (!cfg_path || cfg_path[0] == '\0')
	{
		return;
	}

	Common::copy_path(paths.fm_config_path, sizeof(paths.fm_config_path), cfg_path);
	Common::normalize_path_separators(paths.fm_config_path);

	char fm_dir[kModulePathMax];
	Common::copy_path(fm_dir, sizeof(fm_dir), paths.fm_config_path);
	Common::path_dirname(fm_dir, kModulePathMax);

	if (Common::path_has_component_suffix(fm_dir, "FM"))
	{
		Common::copy_path(paths.mod_root_path, sizeof(paths.mod_root_path), fm_dir);
		Common::path_dirname(paths.mod_root_path, kModulePathMax);
	}
	else
	{
		Common::copy_path(paths.mod_root_path, sizeof(paths.mod_root_path), fm_dir);
	}
}

inline bool try_set_mod_root(ModulePaths& paths, const char* root)
{
	char cfg_path[kModulePathMax];
	Common::build_path(
		{ cfg_path, sizeof(cfg_path) }, { root, "FM\\config.lua" });
	if (!Common::path_file_exists(cfg_path))
	{
		return false;
	}

	Common::copy_path(paths.mod_root_path, sizeof(paths.mod_root_path), root);
	Common::normalize_path_separators(paths.mod_root_path);
	Common::copy_path(paths.fm_config_path, sizeof(paths.fm_config_path), cfg_path);
	return true;
}

inline bool current_module_directory(char* output, const void* module_address)
{
	HMODULE module = nullptr;
	if (!GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCSTR>(module_address),
		&module))
	{
		return false;
	}
	char module_path[kModulePathMax];
	const DWORD length = GetModuleFileNameA(
		module, module_path, static_cast<DWORD>(sizeof(module_path)));
	if (length == 0 || length >= sizeof(module_path))
	{
		return false;
	}
	Common::normalize_path_separators(module_path);
	Common::copy_path(output, kModulePathMax, module_path);
	Common::path_dirname(output, kModulePathMax);
	return true;
}

inline void normalize_module_root_candidate(char* root)
{
	if (Common::path_has_component_suffix(root, "bin"))
	{
		Common::path_dirname(root, kModulePathMax);
		return;
	}
	const bool legacy_build_folder =
		Common::path_has_component_suffix(root, "vc100.debug") ||
		Common::path_has_component_suffix(root, "vc100.release");
	if (!legacy_build_folder)
	{
		return;
	}
	Common::path_dirname(root, kModulePathMax);
	Common::path_dirname(root, kModulePathMax);
	Common::path_dirname(root, kModulePathMax);
}

inline bool try_module_directory_candidates(
	ModulePaths& paths,
	const char* module_dir)
{
	char root[kModulePathMax];
	Common::copy_path(root, sizeof(root), module_dir);
	normalize_module_root_candidate(root);
	if (try_set_mod_root(paths, root))
	{
		return true;
	}
	Common::copy_path(root, sizeof(root), module_dir);
	Common::path_dirname(root, kModulePathMax);
	Common::path_dirname(root, kModulePathMax);
	Common::path_dirname(root, kModulePathMax);
	return try_set_mod_root(paths, root);
}

inline void initialize_project_paths(ModulePaths& paths)
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
	char module_dir[kModulePathMax];
	if (!current_module_directory(module_dir, &paths.initialized))
	{
		return;
	}
	(void)try_module_directory_candidates(paths, module_dir);
}

inline void build_mod_path(
	ModulePaths& paths,
	const Common::PathTarget& target,
	const char* relative)
{
	initialize_project_paths(paths);
	Common::build_path(target, { paths.mod_root_path, relative });
}

inline void configure_module_paths(ModulePaths& paths, const char* cfg_path)
{
	if (cfg_path && cfg_path[0] != '\0')
	{
		set_project_paths_from_config(paths, cfg_path);
		paths.initialized = true;
		return;
	}

	initialize_project_paths(paths);
}
}
