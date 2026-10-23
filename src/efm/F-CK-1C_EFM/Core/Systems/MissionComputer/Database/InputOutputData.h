#pragma once

namespace Core::Systems::Mc
{
/// @brief MC 的具型別輸入快照；任務功能確定後才增加欄位與 Pipeline 宣告。
struct MissionInputs
{
};

/// @brief MC 待發布的具型別輸出；尚未定義任何功能輸出。
struct MissionOutputs
{
};

/// @brief 由 MC 持有的輸入輸出分區；每個未來輸出須指定單一寫入者。
struct InputOutputData
{
	MissionInputs input;
	MissionOutputs output;
};
}
