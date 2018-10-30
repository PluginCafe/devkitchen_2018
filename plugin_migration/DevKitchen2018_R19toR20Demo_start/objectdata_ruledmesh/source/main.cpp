#include "c4d_plugin.h"
#include "c4d_resource.h"

#include "main.h"

Bool PluginStart(void)
{
	if (!RegisterRuledMesh())
		return false;
	
	return true;
}

void PluginEnd(void)
{
}

Bool PluginMessage(Int32 id, void* data)
{
	switch (id)
	{
	case C4DPL_INIT_SYS:
		if (!resource.Init())
			return false;		// don't start plugin without resource
		
		return true;
	}

	return false;
}