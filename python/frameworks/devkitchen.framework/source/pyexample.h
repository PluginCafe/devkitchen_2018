#ifndef DEVKITCHEN_PYEXAMPLE_H__
#define DEVKITCHEN_PYEXAMPLE_H__

#include "maxon/object.h"
#include "c4d.h"
namespace maxon
{

enum class OBJECTTYPE
{
	CUBE = Ocube,
	SPHERE = Osphere
} MAXON_ENUM_LIST (OBJECTTYPE, "devkitchen.python.objecttype");

class ExampleInterface
{
	MAXON_INTERFACE_NONVIRTUAL (ExampleInterface, MAXON_REFERENCE_NONE, "devkitchen.python.exampleinterface");

public:
	static MAXON_METHOD Result<BaseObject*>CreateObject(OBJECTTYPE oType);
};

#include "pyexample1.hxx"
#include "pyexample2.hxx"

}

#endif