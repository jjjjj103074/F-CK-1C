#pragma once

namespace Core::Systems::Dflcc
{
class Executive;

/// @brief 管理 DFLCC 啟動與重新啟動程序的元件。
/// 啟動入口啟用 Executive；本元件不參與週期排程，也不直接存取資料庫。
/// 尚未實作：冷啟動檢查、各元件的初始化程序、斷電恢復與故障重啟。
class StartupAndRestart final
{
public:
	/// @brief 綁定啟動程序完成後接手週期運算的排程器。
	/// @param executive 已完成建構且生命週期涵蓋本元件的排程器。
	explicit StartupAndRestart(Executive& executive);

	/// @brief 啟用 Executive 後返回；週期運算由外部模擬時間推進。
	/// @throws std::logic_error Executive 已啟動。
	void start();

private:
	Executive& executive_; ///< 啟動程序完成後接手的排程器；不轉移所有權。
};
}
