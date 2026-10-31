#pragma once

namespace Core::Systems::Mc
{
class Executive;
struct LifecycleStatus;

/// @brief 啟用 MC 的 Executive 排程。
class StartupAndRestart final
{
public:
	StartupAndRestart(Executive& executive, LifecycleStatus& lifecycle);
	/// @brief 呼叫 Executive::start 啟用排程。
	/// @throws std::logic_error Executive 已啟動。
	void start();

private:
	Executive& executive_;
	LifecycleStatus& lifecycle_;
};
}
