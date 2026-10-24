#pragma once

#include "../System.h"
#include "Components/Executive/Executive.h"
#include "Components/FailureManager/FailureManager.h"
#include "Components/ModeManagement/ModeManagement.h"
#include "Components/StartupAndRestart/StartupAndRestart.h"
#include "Components/SystemMonitor/SystemMonitor.h"
#include "Database/ExecutiveTables.h"
#include "Database/InputOutputData.h"
#include "Database/SystemStatusTables.h"
#include "Database/ExternalActionQueue.h"
#include "Database/MissionData.h"

namespace Core::Systems
{
/// @brief 一台 MC 的組裝與 Pipeline 介面；直接持有功能元件與具型別資料庫。
/// 模式、掛載、武器操作與感測器協調共用一個 Executive，DCS 操作由 Lua 介面執行。
class MissionComputer final : public System
{
public:
	/// @brief 建立固定地址的資料庫及元件，再透過 StartupAndRestart 啟用 Executive。
	MissionComputer();
	MissionComputer(const MissionComputer&) = delete;
	MissionComputer& operator=(const MissionComputer&) = delete;
	MissionComputer(MissionComputer&&) = delete;
	MissionComputer& operator=(MissionComputer&&) = delete;

	/// @brief 與 DFLCC 相同的供電入口；目前只控制 step，不執行斷電或重啟程序。
	void set_powered(bool powered) noexcept;
	bool powered() const noexcept;

	/// @brief 宣告 MC 輸入輸出、命令處理器與專案暫定頻率。
	void setup(SystemSetup& setup) override;
	/// @brief 讀取當前觀測、確認已執行操作、推進內部排程並發布具型別快照。
	void step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result) override;

private:
	void read_inputs(const SystemStepContext& context, const AircraftDataView& aircraft);
	void consume_commands();
	void publish_outputs(SystemResult& result);

	bool powered_ = true; ///< 暫定預設有電；尚未連接電源系統。

	// 資料庫先於所有使用者建立，並在它們之後銷毀；與 DFLCC 的組裝順序一致。
	Mc::ExecutiveTables executive_tables_;
	Mc::InputOutputData input_output_data_;
	Mc::SystemStatusTables system_status_tables_;
	Mc::ModeData mode_data_;
	Mc::SensorData sensor_data_;
	Mc::StoresData stores_data_;
	Mc::WeaponControlData weapon_data_;
	Mc::ExternalActionQueue external_actions_;

	Mc::SystemMonitor system_monitor_;
	Mc::FailureManager failure_manager_;
	Mc::StoresManagement stores_management_;
	Mc::SensorCoordination sensor_coordination_;
	Mc::WeaponControl weapon_control_;
	Mc::ModeManagement mode_management_;
	Mc::Executive executive_;
	Mc::StartupAndRestart startup_and_restart_;
};
}
