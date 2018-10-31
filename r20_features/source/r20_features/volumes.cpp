// local header files and resources
#include "r20_features.h"
#include "c4d_symbols.h"

// classic API header files
#include "c4d_general.h"
#include "c4d_commanddata.h"
#include "c4d_basedocument.h"
#include "c4d_resource.h"
#include "lib_description.h"
#include "lib_volumebuilder.h"
#include "lib_volumeobject.h"
#include "lib_noise.h"

// parameter IDs
#include "onull.h"
#include "ovolumebuilder.h"

// MAXON API header files
#include "maxon/volume.h"
#include "maxon/volumeiterators.h"
#include "maxon/volumetools.h"
#include "maxon/volumeaccessors.h"
#include "maxon/volumecommands.h"
#include "maxon/lib_math.h"

//----------------------------------------------------------------------------------------
/// An example command creating a VolumeBuilder object.
//----------------------------------------------------------------------------------------
class CreateVolumeBuilderCommand : public CommandData
{
	INSTANCEOF(CreateVolumeBuilderCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);
	static CreateVolumeBuilderCommand* Alloc();
};

Bool CreateVolumeBuilderCommand::Execute(BaseDocument* doc)
{
	// This example shows how to create a VolumeBuilder object and how to add scene objects to the builder.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_volumebuilder.html.

	iferr_scope_handler
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// prepare array for object selection
	AutoAlloc<AtomArray> objectSelection;
	if (objectSelection == nullptr)
		iferr_throw(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION));

	// get object selection
	doc->GetActiveObjects(objectSelection, GETACTIVEOBJECTFLAGS::NONE);

	// check if any objects are selected
	const Int32 selectionCount = objectSelection->GetCount();
	if (selectionCount == 0)
		return true;

	// create volume builder and mesher

	VolumeBuilder* volumeBuilder = VolumeBuilder::Alloc();
	BaseObject*		 mesher = BaseObject::Alloc(1039861);

	// check for successful allocation
	if (volumeBuilder == nullptr || mesher == nullptr)
	{
		VolumeBuilder::Free(volumeBuilder);
		BaseObject::Free(mesher);
		iferr_throw(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION));
	}

	// insert volume builder and mesher into the BaseDocument

	doc->StartUndo();

	doc->InsertObject(mesher, nullptr, nullptr);
	doc->AddUndo(UNDOTYPE::NEWOBJ, mesher);

	doc->InsertObject(volumeBuilder, mesher, nullptr);
	doc->AddUndo(UNDOTYPE::NEWOBJ, volumeBuilder);

	// add the selected objects to the volume builder

	maxon::AggregatedError aggError;

	for (Int32 i = 0; i < selectionCount; ++i)
	{
		// get selected object
		C4DAtom* const atom = objectSelection->GetIndex(i);
		BaseObject* const object = static_cast<BaseObject*>(atom);

		if (object == nullptr)
		{
			aggError.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't break the loop.");
			continue;
		}

		// add the object
		const Int32 index = 0;
		if (volumeBuilder->AddSceneObject(object, index, true, BOOLTYPE::UNION, MIXTYPE::NORMAL) == false)
		{
			aggError.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't break the loop.");
		}
		
		// hide object
		doc->AddUndo(UNDOTYPE::CHANGE_SMALL, object);
		object->SetEditorMode(MODE_OFF);
		object->SetRenderMode(MODE_OFF);
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

CreateVolumeBuilderCommand* CreateVolumeBuilderCommand::Alloc()
{
	return NewObjClear(CreateVolumeBuilderCommand);
}

//----------------------------------------------------------------------------------------
/// An example command reading volume data from a Volume Builder.
//----------------------------------------------------------------------------------------
class ReadVolumeCommand : public CommandData
{
	INSTANCEOF(ReadVolumeCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);
	static ReadVolumeCommand* Alloc();
};

Bool ReadVolumeCommand::Execute(BaseDocument* doc)
{
	// This example shows how to access the volume object in a volume builder object
	// and how to iterate over all volume data.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_volumebuilder.html.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_volumeobject.html.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_maxonapi_volumeinterface.html.

	iferr_scope_handler
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// access the selected object
	BaseObject* const object = doc->GetActiveObject();
	if (object == nullptr)
		return true;

	// check if the selected object is a volume builder
	if (object->IsInstanceOf(Ovolumebuilder) == false)
		iferr_throw(maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION));

	// access the volume builder cache
	BaseObject* const cache = object->GetCache();
	if (cache == nullptr)
		iferr_throw(maxon::UnexpectedError(MAXON_SOURCE_LOCATION));

	// check if the cache object is a volume object
	if (cache->IsInstanceOf(Ovolume) == false)
		iferr_throw(maxon::UnexpectedError(MAXON_SOURCE_LOCATION));

	// get the volume data stored in the volume object
	VolumeObject* const		 volumeObject = static_cast<VolumeObject*>(cache);
	const maxon::VolumeRef volume = volumeObject->GetVolume();

	// create iterator
	maxon::GridIteratorRef<maxon::Float32, maxon::ITERATORTYPE::ON> iterator = maxon::GridIteratorRef<maxon::Float32, maxon::ITERATORTYPE::ON>::Create() iferr_return;
	iterator.Init(volume) iferr_return;

	// get radius (based on the voxel size)
	GeData data;
	object->GetParameter(DescID(ID_VOLUMEBUILDER_GRID_SIZE), data, DESCFLAGS_GET::NONE);
	const Float radius = data.GetFloat() * .5;

	// start undo-step
	doc->StartUndo();

	maxon::AggregatedError aggError;

	// get transformation matrix
	const maxon::Matrix transform = volume.GetGridTransform();

	// check every cell with content
	for (; iterator.IsNotAtEnd(); iterator.StepNext())
	{
		// create a null objects
		BaseObject* const null = BaseObject::Alloc(Onull);
		if (null == nullptr)
		{
			aggError.AddError(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't break the loop.");
			continue;
		}

		// insert the null into the document
		doc->InsertObject(null, nullptr, nullptr);
		doc->AddUndo(UNDOTYPE::NEWOBJ, null);

		// get coordinates
		const maxon::IntVector32 coord = iterator.GetCoords();

		// get world space coordinates
		Vector pos;
		pos.x = coord.x;
		pos.y = coord.y;
		pos.z = coord.z;
		pos = transform * pos;
		null->SetRelPos(pos);

		// set display options
		null->SetParameter(DescID(NULLOBJECT_DISPLAY), NULLOBJECT_DISPLAY_CUBE, DESCFLAGS_SET::NONE);
		null->SetParameter(DescID(NULLOBJECT_RADIUS), radius, DESCFLAGS_SET::NONE);
		null->SetParameter(DescID(NULLOBJECT_ORIENTATION), NULLOBJECT_ORIENTATION_ZY, DESCFLAGS_SET::NONE);

		// get value and store it as the object's name
		const Float32 value = iterator.GetValue();
		const String	valueStr = FormatString("@", value);
		null->SetName(valueStr);
	}

	doc->EndUndo();

	EventAdd();

	// check if any errors occurred
	if (aggError.GetCount() > 0)
	{
		iferr_throw(aggError);
	}

	return true;
}

ReadVolumeCommand* ReadVolumeCommand::Alloc()
{
	return NewObjClear(ReadVolumeCommand);
}

//----------------------------------------------------------------------------------------
/// An example command creating a new volume object.
//----------------------------------------------------------------------------------------
class CreateVolumeCommand : public CommandData
{
	INSTANCEOF(CreateVolumeCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);
	static CreateVolumeCommand* Alloc();
};

Bool CreateVolumeCommand::Execute(BaseDocument* doc)
{
	// This example shows how to create a volume object and how to write volume data.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_volumeobject.html.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_maxonapi_volumeinterface.html.

	iferr_scope_handler
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// create volume object and volume mesher

	VolumeObject* volumeObj = VolumeObject::Alloc();
	BaseObject*		mesher = BaseObject::Alloc(1039861);

	// check for successful allocation
	if (volumeObj == nullptr || mesher == nullptr)
	{
		VolumeObject::Free(volumeObj);
		BaseObject::Free(mesher);
		iferr_throw(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION));
	}

	// add a phong tag to the mesher
	mesher->SetPhong(true, true, DegToRad(60.0));

	// insert the mesher and the volume object into the scene
	doc->StartUndo();
	doc->InsertObject(mesher, nullptr, nullptr);
	doc->AddUndo(UNDOTYPE::NEWOBJ, mesher);
	doc->InsertObject(volumeObj, mesher, nullptr);
	doc->AddUndo(UNDOTYPE::NEWOBJ, volumeObj);
	doc->EndUndo();

	EventAdd();

	// create volume
	maxon::VolumeRef volume = maxon::VolumeToolsInterface::CreateNewFloat32Volume(0.0) iferr_return;
	volume.SetGridClass(GRIDCLASS::SDF);
	volume.SetGridName("Example Grid"_s);
	const Vector				scaleFactor { 10.0 };
	const maxon::Matrix scaleMatrix = MatrixScale(scaleFactor);
	volume.SetGridTransform(scaleMatrix);

	// create accessor
	maxon::GridAccessorRef<Float32> access = maxon::GridAccessorRef<Float32>::Create() iferr_return;
	access.Init(volume, maxon::VOLUMESAMPLER::NEAREST) iferr_return;

	// create noise generator
	AutoAlloc<C4DNoise> noise(123);
	if (noise == nullptr)
		iferr_throw(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION));

	const Int32 dimension = 100;
	const Int32 halfDimension = 50;
	const Float noiseScale = 25.0;

	// for each cell of the volume define a value
	for (Int32 x = 0; x < dimension; ++x)
	{
		for (Int32 y = 0; y < dimension; ++y)
		{
			for (Int32 z = 0; z < dimension; ++z)
			{
				// create coordinates in the range of -halfDimension / +halfDimension
				Vector pos;
				pos.x = Float(x - halfDimension);
				pos.y = Float(y - halfDimension);
				pos.z = Float(z - halfDimension);

				// create coordinates in the range of -1 / +1
				const Vector scaledPos = pos / Float(halfDimension);

				// sample noise
				const Float	 noiseFactor = noise->Noise(NoiseType::NOISE_NOISE, false, scaledPos);
				// map noise values into the range of -v / +v.
				const Float	 value = (noiseFactor - 0.5) * noiseScale;

				// create int coordinates
				maxon::IntVector32 coords;
				coords.x = x;
				coords.z = z;
				coords.y = y;

				// set value
				access.SetValue(coords, Float32(value)) iferr_return;
			}
		}
	}

	// store volume data in the volume object
	volumeObj->SetVolume(volume);

	return true;
}

CreateVolumeCommand* CreateVolumeCommand::Alloc()
{
	return NewObjClear(CreateVolumeCommand);
}

//----------------------------------------------------------------------------------------
/// An example command that executes a volume operation on the selected objects to create a new object.
//----------------------------------------------------------------------------------------
class CombineObjectsCommand : public CommandData
{
	INSTANCEOF(CombineObjectsCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);

	static CombineObjectsCommand* Alloc();
};

Bool CombineObjectsCommand::Execute(BaseDocument* doc)
{
	// This example shows how to use volume commands to create volume data from a polygon object,
	// how to execute a volume operation on the create volume data and how to create polygon data
	// from a given volume object.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_maxonapi_volumetools.html.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_maxonapi_command.html.

	// temporary volume objects; must be freed later
	VolumeObject* volumeObjectA = nullptr;
	VolumeObject* volumeObjectB = nullptr;
	VolumeObject* resultVolume	= nullptr;

	iferr_scope_handler
	{
		// ensure cleanup in case of an error
		VolumeObject::Free(volumeObjectA);
		VolumeObject::Free(volumeObjectB);
		VolumeObject::Free(resultVolume);

		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// prepare array for object selection
	AutoAlloc<AtomArray> objectSelection;
	if (objectSelection == nullptr)
		iferr_throw(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION));

	// get object selection
	doc->GetActiveObjects(objectSelection, GETACTIVEOBJECTFLAGS::NONE);

	// check if two objects are selected
	const Int32 selectionCount = objectSelection->GetCount();
	if (selectionCount != 2)
		iferr_throw(maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION));

	// check if the selected objects are polygon objects

	C4DAtom* const atomA = objectSelection->GetIndex(0);
	C4DAtom* const atomB = objectSelection->GetIndex(1);

	if (atomA == nullptr || atomA->IsInstanceOf(Opolygon) == false)
		iferr_throw(maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION));

	if (atomB == nullptr || atomB->IsInstanceOf(Opolygon) == false)
		iferr_throw(maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION));

	BaseObject* const objectA = static_cast<BaseObject*>(atomA);
	BaseObject* const objectB = static_cast<BaseObject*>(atomB);

	MAXON_SCOPE
	{
		// create volume objects from the given polygon objects

		// store object pointers in a BaseArray
		maxon::BaseArray<BaseObject*> sourceObjects;
		sourceObjects.Append(objectA) iferr_return;
		sourceObjects.Append(objectB) iferr_return;

		// prepare volume command data and store the BaseArray
		maxon::VolumeCommandData data;
		data.op = &sourceObjects;

		// prepare command context
		maxon::LegacyCommandDataRef	context = maxon::LegacyCommandDataClasses::VOLUMEDATA().Create() iferr_return;

		// set context settings
		context.Set(MESHTOVOLUMESETTINGS::GRIDSIZE, 10.0) iferr_return;
		context.SetLegacyData<maxon::VolumeCommandData>(data) iferr_return;

		// get command
		const auto command = maxon::CommandClasses::MESHTOVOLUME();

		// execute command on the given context
		const maxon::COMMANDRESULT res = context.Invoke(command, false) iferr_return;

		// check command success
		if (res != maxon::COMMANDRESULT::OK)
			iferr_throw(maxon::UnknownError(MAXON_SOURCE_LOCATION));

		// store command results
		const maxon::VolumeCommandData& result = context.GetLegacyData<maxon::VolumeCommandData>() iferr_return;
		if (result.result.GetCount() == 2)
		{
			volumeObjectA = static_cast<VolumeObject*>(result.result[0]);
			volumeObjectB = static_cast<VolumeObject*>(result.result[1]);
		}
	}
	MAXON_SCOPE
	{
		// execute a volume command on the given volume objects

		// store object pointers in a BaseArray
		maxon::BaseArray<BaseObject*> sourceObjects;
		sourceObjects.Append(volumeObjectA) iferr_return;
		sourceObjects.Append(volumeObjectB) iferr_return;

		// prepare volume command data and store the BaseArray
		maxon::VolumeCommandData data;
		data.op = &sourceObjects;

		// prepare command context
		maxon::LegacyCommandDataRef	context = maxon::LegacyCommandDataClasses::VOLUMEDATA().Create() iferr_return;

		// set context settings
		// 1 equals BOOLTYPE::DIFF
		context.Set(BOOLESETTINGS::BOOLETYPE, 1) iferr_return;
		context.SetLegacyData<maxon::VolumeCommandData>(data) iferr_return;

		// get command
		const auto command = maxon::CommandClasses::BOOLE();

		// execute command on the given context
		const maxon::COMMANDRESULT res = context.Invoke(command, false) iferr_return;

		// check command success
		if (res != maxon::COMMANDRESULT::OK)
			iferr_throw(maxon::UnknownError(MAXON_SOURCE_LOCATION));

		// store command results
		const maxon::VolumeCommandData& result = context.GetLegacyData<maxon::VolumeCommandData>() iferr_return;
		if (result.result.GetCount() == 1)
		{
			resultVolume = static_cast<VolumeObject*>(result.result[0]);
		}
	}
	MAXON_SCOPE
	{
		// create a polygon object from the given volume

		// store object pointer in a BaseArray
		maxon::BaseArray<BaseObject*> sourceObjects;
		sourceObjects.Append(resultVolume) iferr_return;

		// prepare volume command data and store the BaseArray
		maxon::VolumeCommandData data;
		data.op = &sourceObjects;

		// prepare command context
		maxon::LegacyCommandDataRef	context = maxon::LegacyCommandDataClasses::VOLUMEDATA().Create() iferr_return;

		// set context settings
		context.SetLegacyData<maxon::VolumeCommandData>(data) iferr_return;

		// get command
		const auto command = maxon::CommandClasses::VOLUMETOMESH();

		// execute command on the given context
		const maxon::COMMANDRESULT res = context.Invoke(command, false) iferr_return;

		// check command success
		if (res != maxon::COMMANDRESULT::OK)
			iferr_throw(maxon::UnknownError(MAXON_SOURCE_LOCATION));

		// access command results
		const maxon::VolumeCommandData& result = context.GetLegacyData<maxon::VolumeCommandData>() iferr_return;
		if (result.result.GetCount() == 1)
		{
			doc->StartUndo();
			
			// get the created polygon object and insert it into the scene
			BaseObject* const mesh = result.result[0];
			doc->InsertObject(mesh, nullptr, nullptr);
			doc->AddUndo(UNDOTYPE::NEWOBJ, mesh);

			// hide the selected objects

			doc->AddUndo(UNDOTYPE::CHANGE_SMALL, objectA);
			objectA->SetEditorMode(MODE_OFF);
			objectA->SetRenderMode(MODE_OFF);

			doc->AddUndo(UNDOTYPE::CHANGE_SMALL, objectB);
			objectB->SetEditorMode(MODE_OFF);
			objectB->SetRenderMode(MODE_OFF);

			doc->EndUndo();

			EventAdd();
		}
		else
		{
			iferr_throw(maxon::UnexpectedError(MAXON_SOURCE_LOCATION));
		}
	}

	// free data; consider also finally()
	VolumeObject::Free(resultVolume);
	VolumeObject::Free(volumeObjectA);
	VolumeObject::Free(volumeObjectB);

	return true;
}

CombineObjectsCommand* CombineObjectsCommand::Alloc()
{
	return NewObjClear(CombineObjectsCommand);
}

void RegisterVolumeExamples()
{
	// prepare aggregated error to collect errors while registering the plugins
	maxon::AggregatedError aggErr;


	const Bool builderCommandRes = RegisterCommandPlugin(1050257, GeLoadString(IDS_CREATE_VOLUMEBUILDER_COMMAND), 0, nullptr, ""_s, CreateVolumeBuilderCommand::Alloc());
	if (builderCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	const Bool readCommandRes = RegisterCommandPlugin(1050258, GeLoadString(IDS_READ_VOLUME_COMMAND), 0, nullptr, ""_s, ReadVolumeCommand::Alloc());
	if (readCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	const Bool volumeCommandRes = RegisterCommandPlugin(1050265, GeLoadString(IDS_CREATE_VOLUME_COMMAND), 0, nullptr, ""_s, CreateVolumeCommand::Alloc());
	if (volumeCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	const Bool substractCommandRes = RegisterCommandPlugin(1050266, GeLoadString(IDS_SUBSTRACT_OBJECTS_COMMAND), 0, nullptr, ""_s, CombineObjectsCommand::Alloc());
	if (substractCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	// check if any error occurred
	if (aggErr.GetCount() != 0)
	{
		DiagnosticOutput("Errors registering plugins: @", aggErr);
		DebugStop();
	}
}
