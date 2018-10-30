#include "c4d.h"

// MAXON API header files
#include "pyexample.h"
#include "maxon/interfacebase.h"
#include "maxon/configuration.h"

namespace maxon
{
class ExampleImpl : protected ExampleInterface
{
	MAXON_IMPLEMENTATION (ExampleImpl)

public:
	static Result<BaseObject*>CreateObject(OBJECTTYPE oType)
	{
		iferr_scope;
		AutoAlloc<BaseObject> obj{Int32(oType)};
		if (obj == nullptr)
			return maxon::OutOfMemoryError (MAXON_SOURCE_LOCATION);

		return obj.Release();
	}
};
MAXON_IMPLEMENTATION_REGISTER(ExampleImpl);

MAXON_CONFIGURATION_INT (yourCustomConfiguration, Int32(OBJECTTYPE::SPHERE), Ocube, Osphere, CONFIGURATION_CATEGORY::REGULAR, "yourCustomConfiguration");

}
