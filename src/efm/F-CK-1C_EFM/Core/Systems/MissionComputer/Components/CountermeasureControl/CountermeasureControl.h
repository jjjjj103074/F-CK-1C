#pragma once
#include "../../../../Contracts/Commands.h"
#include "../../../../Contracts/CountermeasureContracts.h"
namespace Core::Systems::Mc
{
/// @brief 將 CMS 前向及左向操作轉為程序要求，保存至反制系統接受為止。
class CountermeasureControl final
{
public:
	void handle(const Command& command);
	void acknowledge(std::uint64_t sequence);
	const CountermeasureRequests& pending() const noexcept
	{
		return pending_;
	}

private:
	CountermeasureRequests pending_;
	std::uint64_t last_sequence_ = 0;
};
}
