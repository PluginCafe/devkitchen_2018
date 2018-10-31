// local header files and resources
#include "r20_features.h"
#include "c4d_symbols.h"

// classic API header files
#include "c4d_general.h"
#include "c4d_commanddata.h"
#include "c4d_basedocument.h"
#include "c4d_resource.h"
#include "lib_instanceobject.h"

// parameter IDs
#include "oinstance.h"

//----------------------------------------------------------------------------------------
/// An example command creating an instance object.
//----------------------------------------------------------------------------------------
class CreateMultiInstanceCommand : public CommandData
{
	INSTANCEOF(CreateMultiInstanceCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);
	static CreateMultiInstanceCommand* Alloc();
};

Bool CreateMultiInstanceCommand::Execute(BaseDocument* doc)
{
	// This example creates an instance object using multi-instances.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_instanceobject.html.

	iferr_scope_handler
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// get the selected object
	BaseObject* const activeObject = doc->GetActiveObject();
	if (activeObject == nullptr)
		return true;

	// create instance object
	InstanceObject* const instanceObject = InstanceObject::Alloc();
	if (instanceObject == nullptr)
		iferr_throw(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION));

	// insert object into the scene
	doc->StartUndo();
	doc->InsertObject(instanceObject, nullptr, nullptr);
	doc->AddUndo(UNDOTYPE::NEWOBJ, instanceObject);
	doc->EndUndo();

	// use the given reference object
	instanceObject->SetReferenceObject(activeObject) iferr_return;

	// set multi-instance mode
	if (!instanceObject->SetParameter(INSTANCEOBJECT_RENDERINSTANCE_MODE, INSTANCEOBJECT_RENDERINSTANCE_MODE_MULTIINSTANCE, DESCFLAGS_SET::NONE))
		iferr_throw(maxon::UnexpectedError(MAXON_SOURCE_LOCATION));

	// prepare matrices and colors
	maxon::BaseArray<Matrix>				 matrices;
	maxon::BaseArray<maxon::Color64> colors;

	const Int count = 100;
	matrices.Resize(count) iferr_return;
	colors.Resize(count) iferr_return;

	// generate positions and colors
	Float				position = 0.0;
	const Float step = 300.0;
	Float				hue	 = 0.0;
	const Float hueStep = 1.0 / count;

	for (Int i = 0; i < count; ++i)
	{
		// matrices
		matrices[i] = MatrixMove(Vector(position, 0.0, 0.0));
		position += step;

		// colors
		const Vector colorHSV = Vector(hue, 1.0, 1.0);
		const Vector colorRGB = HSVToRGB(colorHSV);
		colors[i] = maxon::Color64(colorRGB);
		hue += hueStep;
	}

	// store data in the instance object
	instanceObject->SetInstanceMatrices(matrices) iferr_return;
	instanceObject->SetInstanceColors(colors) iferr_return;

	EventAdd();

	return true;
};

CreateMultiInstanceCommand* CreateMultiInstanceCommand::Alloc()
{
	return NewObjClear(CreateMultiInstanceCommand);
}

//----------------------------------------------------------------------------------------
/// An example command reading multi-instance data.
//----------------------------------------------------------------------------------------
class ReadMultiInstancesCommand : public CommandData
{
	INSTANCEOF(ReadMultiInstancesCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);
	static ReadMultiInstancesCommand* Alloc();
};

Bool ReadMultiInstancesCommand::Execute(BaseDocument* doc)
{
	// This example accesses an instance object to read the multi-instance information.
	// With that information, "Null"-objects are created.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_instanceobject.html.

	iferr_scope_handler
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// get instance object

	const BaseObject* const activeObject = doc->GetActiveObject();
	if (activeObject == nullptr)
		return true;

	if (activeObject->IsInstanceOf(Oinstance) == false)
		iferr_throw(maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION));

	const InstanceObject* const instanceObject = static_cast<const InstanceObject*>(activeObject);

	// check instance count
	const UInt instanceCnt = instanceObject->GetInstanceCount();
	if (instanceCnt == 0)
		return true;

	// for each position, create a null object

	doc->StartUndo();

	maxon::AggregatedError aggError;

	for (UInt i = 0; i < instanceCnt; ++i)
	{
		// allocate a null object
		BaseObject* const nullObject = BaseObject::Alloc(Onull);
		if (nullObject == nullptr)
		{
			aggError.AddError(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't break the loop.");
			continue;
		}

		// set matrix
		const Matrix matrix = instanceObject->GetInstanceMatrix(i);
		nullObject->SetMg(matrix);

		// insert object
		doc->InsertObject(nullObject, nullptr, nullptr);
		doc->AddUndo(UNDOTYPE::NEWOBJ, nullObject);
	}

	doc->EndUndo();

	EventAdd();

	// check if any errors occurred
	if (aggError.GetCount() > 0)
	{
		iferr_throw(aggError);
	}

	return true;
};

ReadMultiInstancesCommand* ReadMultiInstancesCommand::Alloc()
{
	return NewObjClear(ReadMultiInstancesCommand);
}

void RegisterMultiInstancesExamples()
{
	// prepare aggregated error to collect errors while registering the plugins
	maxon::AggregatedError aggErr;

	const Bool createCommandRes = RegisterCommandPlugin(1050287, GeLoadString(IDS_CREATE_MULTIINSTANCE_COMMAND), 0, nullptr, ""_s, CreateMultiInstanceCommand::Alloc());
	if (createCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	const Bool readCommandRes = RegisterCommandPlugin(1050288, GeLoadString(IDS_READ_MULTIINSTACE_COMMAND), 0, nullptr, ""_s, ReadMultiInstancesCommand::Alloc());
	if (readCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	// check if any error occurred
	if (aggErr.GetCount() != 0)
	{
		DiagnosticOutput("Errors registering plugins: @", aggErr);
		DebugStop();
	}
}
