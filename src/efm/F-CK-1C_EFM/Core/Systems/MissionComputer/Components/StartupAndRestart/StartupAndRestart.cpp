#include "StartupAndRestart.h"

#include "../Executive/Executive.h"

namespace Core::Systems::Mc
{
StartupAndRestart::StartupAndRestart(Executive& executive, LifecycleStatus& lifecycle)
    : executive_(executive),
      lifecycle_(lifecycle)
{
}

void StartupAndRestart::start()
{
	// 尚未實作完整啟動程序；排程啟用不代表任務功能已就緒。
	executive_.start();
}
}
