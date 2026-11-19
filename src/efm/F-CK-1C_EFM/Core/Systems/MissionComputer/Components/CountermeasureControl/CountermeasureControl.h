#pragma once

#include "../../../ExternalActionOutbox.h"
#include "../../../../Contracts/Commands.h"

namespace Core::Systems::Mc
{
/// @brief 接收 CMS 四向操作，將前向施放要求加入外部動作佇列。
class CountermeasureControl final
{
public:
	/// @brief 綁定由 MC 持有的外部動作佇列。
	explicit CountermeasureControl(ExternalActionOutbox& actions);
	/// @brief 前向按下啟動固定程序；其餘方向保留程序 6 與電子干擾操作入口。
	void handle(const Command& command);

private:
	ExternalActionOutbox& actions_; ///< 追加交由 Bridge／Lua 執行的反制語意動作。
};
}
