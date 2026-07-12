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
	Common::build_path(cfg_path, sizeof(cfg_path), root, "FM\\config.lua");
	if (!Common::path_file_exists(cfg_path))
	{
		return false;
	}

	Common::copy_path(paths.mod_root_path, sizeof(paths.mod_root_path), root);
	Common::normalize_path_separators(paths.mod_root_path);
	Common::copy_path(paths.fm_config_path, sizeof(paths.fm_config_path), cfg_path);
	return true;
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

	HMODULE module = nullptr;
	if (GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCSTR>(&paths.initialized),
		&module))
	{
		char module_path[kModulePathMax];
		const DWORD len = GetModuleFileNameA(module, module_path, (DWORD)sizeof(module_path));
		if (len > 0 && len < sizeof(module_path))
		{
			Common::normalize_path_separators(module_path);

			char module_dir[kModulePathMax];
			Common::copy_path(module_dir, sizeof(module_dir), module_path);
			Common::path_dirname(module_dir, kModulePathMax);

			char root[kModulePathMax];
			Common::copy_path(root, sizeof(root), module_dir);

			if (Common::path_has_component_suffix(root, "bin"))
			{
				Common::path_dirname(root, kModulePathMax);
			}
			else if (Common::path_has_component_suffix(root, "vc100.debug") || Common::path_has_component_suffix(root, "vc100.release"))
			{
				Common::path_dirname(root, kModulePathMax);
				Common::path_dirname(root, kModulePathMax);
				Common::path_dirname(root, kModulePathMax);
			}

			if (try_set_mod_root(paths, root))
			{
				return;
			}

			Common::copy_path(root, sizeof(root), module_dir);
			Common::path_dirname(root, kModulePathMax);
			Common::path_dirname(root, kModulePathMax);
			Common::path_dirname(root, kModulePathMax);
			if (try_set_mod_root(paths, root))
			{
				return;
			}
		}
	}
}

inline const char* active_fm_config_path(ModulePaths& paths)
{
	initialize_project_paths(paths);
	return paths.fm_config_path;
}

inline void build_mod_path(ModulePaths& paths, char* out, size_t out_size, const char* relative)
{
	initialize_project_paths(paths);
	Common::build_path(out, out_size, paths.mod_root_path, relative);
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
