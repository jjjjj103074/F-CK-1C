#pragma once

#include "../System.h"
#include "Components/Executive/Executive.h"
#include "Components/FailureManager/FailureManager.h"
#include "Components/StartupAndRestart/StartupAndRestart.h"
#include "Components/SystemMonitor/SystemMonitor.h"
#include "Database/ExecutiveTables.h"
#include "Database/InputOutputData.h"
#include "Database/SystemStatusTables.h"

namespace Core::Systems
{
/// @brief 一台 MC 的組裝與 Pipeline 介面；直接持有四個元件及三個資料庫。
/// 目前只有排程與組裝骨架，尚未實作任務功能、監控、故障處置或完整啟動程序。
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

	/// @brief 宣告專案暫定頻率；本階段不宣告 AircraftData 輸入輸出或操作處理器。
	void setup(SystemSetup& setup) override;
	/// @brief 有電時讀取輸入、推進內部排程，再發布輸出；輸入輸出目前為空骨架。
	void step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result) override;

private:
	void read_inputs(const AircraftDataView& aircraft);
	void publish_outputs(SystemResult& result) const;

	bool powered_ = true; ///< 暫定預設有電；尚未連接電源系統。

	// 資料庫先於所有使用者建立，並在它們之後銷毀；與 DFLCC 的組裝順序一致。
	Mc::ExecutiveTables executive_tables_;
	Mc::InputOutputData input_output_data_;
	Mc::SystemStatusTables system_status_tables_;

	Mc::SystemMonitor system_monitor_;
	Mc::FailureManager failure_manager_;
	Mc::Executive executive_;
	Mc::StartupAndRestart startup_and_restart_;
};
}
