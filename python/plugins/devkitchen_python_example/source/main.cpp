// classic API header files
#include "c4d_plugin.h"
#include "pyexample.h"


// If the classic API is used PluginStart(), PluginMessage() and PluginEnd() must be implemented.
::Bool PluginStart()
{
	return true;
}

void PluginEnd()
{
}

::Bool PluginMessage (::Int32 id, void* data)
{
	iferr_scope_handler
	{
	return false;
	};

	switch (id)
	{
		case C4DPL_PROGRAM_STARTED:
		{
			break;
		}
	}

	return true;
}
