#include "FlightControlComputer.h"

namespace Core::Systems::Catalog::FlightControlComputer
{
    /// @brief 登記 FLCC 的系統識別碼與延後建立實例的方法。
    /// @return 回傳 SystemPipeline 建立飛機 FLCC 實例的目錄項目；不建立實例。
    SystemEntry create_entry()
    {
        SystemEntry entry;

        entry.id = "flight_control_computer"; // Pipeline 識別系統及回報錯誤時使用的 ID。

        // Pipeline 建立一架飛機時才呼叫工廠，並傳入該次飛行的初始資料。
        entry.factory = [](const FlightSetupContext &setup)
        {
            return std::make_unique<Core::Systems::FlightControlComputer>(
                setup.start_mode,               // 這次啟動模式。
                setup.initial_throttle_levers); // 首次更新前的油門桿位置。
        };
        return entry;
    }
}
