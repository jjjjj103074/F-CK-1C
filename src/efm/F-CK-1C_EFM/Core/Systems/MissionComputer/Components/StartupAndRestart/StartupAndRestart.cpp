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
	executive_.start();
}
}
