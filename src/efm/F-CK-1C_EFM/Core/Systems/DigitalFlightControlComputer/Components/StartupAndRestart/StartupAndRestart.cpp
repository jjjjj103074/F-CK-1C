#include "StartupAndRestart.h"

#include "../Executive/Executive.h"

namespace Core::Systems::Dflcc
{
StartupAndRestart::StartupAndRestart(Executive& executive)
	: executive_(executive)
{
}

void StartupAndRestart::start()
{
	// 尚未實作：冷啟動檢查、各元件的初始化程序、斷電恢復與故障重啟。
	executive_.start();
}
}
