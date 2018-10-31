// local header files and resources
#include "r20_features.h"
#include "c4d_symbols.h"
#include "fcheckerboard.h"

// classic API header files
#include "c4d_general.h"
#include "c4d_commanddata.h"
#include "c4d_basedocument.h"
#include "c4d_fielddata.h"
#include "c4d_fielddata.h"
#include "c4d_fieldplugin.h"
#include "c4d_resource.h"
#include "lib_description.h"
#include "customgui_field.h"

// parameter IDs
#include "onull.h"
#include "obase.h"
#include "ofalloff_panel.h"

// MAXON API header files
#include "maxon/apibase.h"
#include "maxon/lib_math.h"
#include "maxon/kdtree.h"

//----------------------------------------------------------------------------------------
/// An example command that samples a field object.
//----------------------------------------------------------------------------------------
class SampleFieldObjectCommand : public CommandData
{
	INSTANCEOF(SampleFieldObjectCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);
	static SampleFieldObjectCommand* Alloc();
};

Bool SampleFieldObjectCommand::Execute(BaseDocument* doc)
{
	// This example shows how to sample a field object.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_fieldobject.html.

	iferr_scope_handler
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// get selected object
	BaseObject* const object = doc->GetActiveObject();
	if (object == nullptr)
		return true;

	// check if the selected object is a field
	if (object->IsInstanceOf(Ofield) == false)
		iferr_throw(maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION));

	FieldObject* const fieldObject = static_cast<FieldObject*>(object);

	// prepare "caller"
	// we need to fake the caller since we sample the field from a CommandData plugin
	AutoAlloc<BaseList2D> caller { Onull };
	if (caller == nullptr)
		iferr_throw(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION));

	// prepare arrays for sample points

	const Int		sampleCnt = 100;
	const Float stepSize	= 10.0;

	maxon::BaseArray<maxon::Vector> positions;
	positions.Resize(sampleCnt) iferr_return;

	maxon::BaseArray<maxon::Vector> uvws;
	uvws.Resize(sampleCnt) iferr_return;

	maxon::BaseArray<maxon::Vector> directions;
	directions.Resize(sampleCnt) iferr_return;

	// set positions
	Float64 xOffset = 0.0;
	for (maxon::Vector& pos : positions)
	{
		pos.x = xOffset;
		xOffset += stepSize;
	}

	// define points to sample
	FieldInput points(positions.GetFirst(),
										directions.GetFirst(),
										uvws.GetFirst(),
										sampleCnt,
										Matrix());

	// prepare results
	FieldOutput results;
	results.Resize(sampleCnt, FIELDSAMPLE_FLAG::VALUE) iferr_return;
	FieldOutputBlock block = results.GetBlock();

	// define context
	const FieldInfo info = FieldInfo::Create(caller, points, FIELDSAMPLE_FLAG::VALUE) iferr_return;

	// shared data utility
	FieldShared shared;

	// sample the field object
	fieldObject->InitSampling(info, shared) iferr_return;
	fieldObject->Sample(points, block, info) iferr_return;
	fieldObject->FreeSampling(info, shared);

	// start undo-step
	doc->StartUndo();

	maxon::AggregatedError aggError;

	// create a null object for each sample point
	for (Int32 i = 0; i < sampleCnt; ++i)
	{
		// allocate null object
		BaseObject* const null = BaseObject::Alloc(Onull);
		if (null == nullptr)
		{
			aggError.AddError(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't break the loop.");
			continue;
		}

		// set position
		const Vector pos = positions[i];
		null->SetRelPos(pos);

		// set color
		const Float	 value = block._value[i];
		const Vector hsv = Vector(value, 1, 1);
		const Vector color = HSVToRGB(hsv);
		null->SetParameter(ID_BASEOBJECT_COLOR, color, DESCFLAGS_SET::NONE);

		// display options
		const Float radius = value * stepSize * 0.5;
		null->SetParameter(NULLOBJECT_RADIUS, radius, DESCFLAGS_SET::NONE);
		null->SetParameter(NULLOBJECT_DISPLAY, NULLOBJECT_DISPLAY_SPHERE, DESCFLAGS_SET::NONE);
		null->SetParameter(ID_BASEOBJECT_USECOLOR, ID_BASEOBJECT_USECOLOR_ALWAYS, DESCFLAGS_SET::NONE);

		// insert object
		doc->InsertObject(null, nullptr, nullptr);
		doc->AddUndo(UNDOTYPE::NEWOBJ, null);
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

SampleFieldObjectCommand* SampleFieldObjectCommand::Alloc()
{
	return NewObjClear(SampleFieldObjectCommand);
}




//----------------------------------------------------------------------------------------
/// An example command that samples the field list of a plain effector.
//----------------------------------------------------------------------------------------
class SampleFieldListCommand : public CommandData
{
	INSTANCEOF(SampleFieldListCommand, CommandData)

public:
	Bool Execute(BaseDocument* doc);
	static SampleFieldListCommand* Alloc();
};

Bool SampleFieldListCommand::Execute(BaseDocument* doc)
{
	// This example shows how to access a plain effector, how to get the field list parameter
	// and how to sample that field list.
	// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_fieldlist.html.

	iferr_scope_handler
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return false;
	};

	// get selected object
	BaseObject* const plainEffector = doc->GetActiveObject();
	if (plainEffector == nullptr)
		return true;

	// check if object is a plain effector
	if (plainEffector->IsInstanceOf(1021337) == false)
		iferr_throw(maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION));

	// get FieldList parameter
	const DescID fieldParameterID(FIELDS);
	GeData			 data;
	if (!plainEffector->GetParameter(fieldParameterID, data, DESCFLAGS_GET::NONE))
		iferr_throw(maxon::UnexpectedError(MAXON_SOURCE_LOCATION));

	// get FieldList object
	CustomDataType* const customData = data.GetCustomDataType(CUSTOMDATATYPE_FIELDLIST);
	FieldList* const			fieldList	 = static_cast<FieldList*>(customData);

	if (fieldList == nullptr)
		iferr_throw(maxon::UnexpectedError(MAXON_SOURCE_LOCATION));

	// prepare arrays
	const Int		sampleCnt = 100;
	const Float stepSize	= 10.0;

	maxon::BaseArray<maxon::Vector> positions;
	positions.Resize(sampleCnt) iferr_return;
	maxon::BaseArray<maxon::Vector> uvws;
	uvws.Resize(sampleCnt) iferr_return;
	maxon::BaseArray<maxon::Vector> directions;
	directions.Resize(sampleCnt) iferr_return;

	// set positions
	Float64 xOffset = 0.0;
	for (maxon::Vector& pos : positions)
	{
		pos.x = xOffset;
		xOffset += stepSize;
	}

	// define points to sample
	FieldInput points(positions.GetFirst(),
										directions.GetFirst(),
										uvws.GetFirst(),
										sampleCnt,
										Matrix());

	// sample
	FieldOutput results = fieldList->SampleListSimple(*plainEffector, points) iferr_return;

	// create a null object for each sample point

	doc->StartUndo();

	maxon::AggregatedError aggError;

	for (Int32 i = 0; i < sampleCnt; ++i)
	{
		// allocate null object
		BaseObject* const null = BaseObject::Alloc(Onull);
		if (null == nullptr)
		{
			aggError.AddError(maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't break the loop.");
			continue;
		}

		// set position
		const Vector pos = positions[i];
		const Float	 value = results._value[i];
		null->SetRelPos(pos);

		// set color
		const Vector hsv = Vector(value, 1, 1);
		const Vector color = HSVToRGB(hsv);
		null->SetParameter(ID_BASEOBJECT_COLOR, color, DESCFLAGS_SET::NONE);

		// set display options
		const Float radius = value * stepSize * 0.5;
		null->SetParameter(NULLOBJECT_RADIUS, radius, DESCFLAGS_SET::NONE);
		null->SetParameter(NULLOBJECT_DISPLAY, NULLOBJECT_DISPLAY_SPHERE, DESCFLAGS_SET::NONE);
		null->SetParameter(ID_BASEOBJECT_USECOLOR, ID_BASEOBJECT_USECOLOR_ALWAYS, DESCFLAGS_SET::NONE);

		// insert object
		doc->InsertObject(null, nullptr, nullptr);
		doc->AddUndo(UNDOTYPE::NEWOBJ, null);
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

SampleFieldListCommand* SampleFieldListCommand::Alloc()
{
	return NewObjClear(SampleFieldListCommand);
}



//----------------------------------------------------------------------------------------
/// An example field subdividing space in a checkerboard pattern.
/// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_fielddata.html-
//----------------------------------------------------------------------------------------
class CheckerboardField : public FieldData
{
	INSTANCEOF(CheckerboardField, FieldData)

public:
	static NodeData* Alloc();
	virtual Bool Init(GeListNode* node);
	virtual maxon::Result<void> InitSampling(FieldObject& op, const FieldInfo& info, FieldShared& shared);
	virtual void FreeSampling(FieldObject& op, const FieldInfo& info, FieldShared& shared);
	virtual maxon::Result<void> Sample(const FieldObject& op, const FieldInput& inputs, FieldOutputBlock& outputs, const FieldInfo& info) const;
	virtual FIELDOBJECT_FLAG GetFieldFlags(const FieldObject& op, BaseDocument* doc) const;

private:
	//----------------------------------------------------------------------------------------
	/// Creates a square wave.
	/// @param[in] value							Input value mapped to a pulse wave.
	/// @return												Returns true for maximum value, false for minimum value.
	//----------------------------------------------------------------------------------------
	maxon::Bool CalculateCell(maxon::Float64 value) const;

	//----------------------------------------------------------------------------------------
	/// Samples the 3D checkerboard.
	/// @param[in] position						3D position vector.
	/// @return												Returns 1.0 for maximum value, 0.0 for minimum value.
	//----------------------------------------------------------------------------------------
	maxon::Float CalculateValue(const maxon::Vector& position) const;

private:
	Float _size;			///< size of a full oscillation
	Float _sizeHalf;	///< half of _size. For speed-up in CalculateCell().
};

Bool CheckerboardField::Init(GeListNode* node)
{
	// set default parameter value
	node->SetParameter(FIELD_CHECKERBOARD_SIZE, 200.0, DESCFLAGS_SET::NONE);

	return true;
}

NodeData* CheckerboardField::Alloc()
{
	iferr (NodeData * const result = NewObj(CheckerboardField))
	{
		// if an error occurred, print the error to the IDE console and trigger a debug stop
		err.DiagOutput();
		err.DbgStop();
		return nullptr;
	}
	return result;
}

maxon::Result<void> CheckerboardField::InitSampling(FieldObject& op, const FieldInfo& info, FieldShared& shared)
{
	// get the size of the checkerboard oscillation
	GeData data;
	op.GetParameter(FIELD_CHECKERBOARD_SIZE, data, DESCFLAGS_GET::NONE);

	// store values
	_size = data.GetFloat();
	_sizeHalf = _size * .5;

	return maxon::OK;
}

void CheckerboardField::FreeSampling(FieldObject& op, const FieldInfo& info, FieldShared& shared)
{
	// freeing internal data after sampling
}

maxon::Bool CheckerboardField::CalculateCell(maxon::Float64 value) const
{
	maxon::Float remainder = maxon::FMod(value, _size);

	if (remainder < 0)
	{
		remainder = _size + remainder;
	}

	return remainder >= _sizeHalf;
}

maxon::Float CheckerboardField::CalculateValue(const maxon::Vector& position) const
{
	const maxon::Bool xres = CalculateCell(position.x);
	const maxon::Bool yres = CalculateCell(position.y);
	const maxon::Bool zres = CalculateCell(position.z);

	const maxon::Bool xyRes = xres != yres;

	if (xyRes != zres)
		return 1.0;

	return 0.0;
}


maxon::Result<void> CheckerboardField::Sample(const FieldObject& op, const FieldInput& inputs, FieldOutputBlock& outputs, const FieldInfo& info) const
{
	// check if outputs are prepared
	if (outputs._value.IsEmpty())
		return maxon::OK;

	// check flags
	if (info._flags & FIELDSAMPLE_FLAG::VALUE || info._flags & FIELDSAMPLE_FLAG::ALL)
	{
		// matrix used to transform sample points into world space
		const Matrix transformationMatrix = ~((~info._inputData._transform) * op.GetMg());

		// handle each input position
		for (Int i = inputs._blockCount - 1; i >= 0; --i)
		{
			// get position
			const Vector pos = transformationMatrix * inputs._position[i];

			// calculate value
			const Float value = CalculateValue(pos);

			// set value
			outputs._value[i] = value;
		}
	}
	return maxon::OK;
}

FIELDOBJECT_FLAG CheckerboardField::GetFieldFlags(const FieldObject& op, BaseDocument* doc) const
{
	return FIELDOBJECT_FLAG::NONE;
}


//----------------------------------------------------------------------------------------
/// An example field layer setting the value based on the distance oft the sampling points.
/// See https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_manual_fieldlayerdata.html.
//----------------------------------------------------------------------------------------
class NextNeighborDistanceFieldLayer : public FieldLayerData
{
	INSTANCEOF(NextNeighborDistanceFieldLayer, FieldLayerData)

public:
	static NodeData* Alloc();
	virtual maxon::Result<void> InitSampling(FieldLayer& layer, const FieldInfo& info, FieldShared& shared);
	virtual maxon::Result<void> Sample(const FieldLayer& layer, const FieldInput& inputs, FieldOutputBlock& outputs, const FieldInfo& info) const;
	virtual void FreeSampling(FieldLayer& layer, const FieldInfo& info, FieldShared& shared);
	virtual Bool IsEqual(const FieldLayer& layer, const FieldLayerData& comp) const;

private:
	//----------------------------------------------------------------------------------------
	/// Caclulates the non-normalized value for the position at the given index.
	/// @param[in] inputs							FieldInput object defining the points to sample.
	/// @param[in] i									Index of the current sampling point.
	/// @return												The result value.
	//----------------------------------------------------------------------------------------
	maxon::Result<maxon::Float> CalculateValue(const FieldInput& inputs, maxon::Int i) const;
};

NodeData* NextNeighborDistanceFieldLayer::Alloc()
{
	iferr (NodeData * const result = NewObj(NextNeighborDistanceFieldLayer))
	{
		DebugStop();
		return nullptr;
	}
	return result;
}

maxon::Result<void> NextNeighborDistanceFieldLayer::InitSampling(FieldLayer& layer, const FieldInfo& info, FieldShared& shared)
{
	return maxon::OK;
}

maxon::Result<maxon::Float> NextNeighborDistanceFieldLayer::CalculateValue(const FieldInput& inputs, maxon::Int i) const
{
	iferr_scope;

	// prepare KDTree
	maxon::KDTree tree;
	tree.Init(1) iferr_return;

	// insert points into tree
	for (Int intern = inputs._blockCount - 1; intern >= 0; --intern)
	{
		// exclude the current point
		if (intern != i)
		{
			const Vector treePoint = inputs._position[intern];
			tree.Insert(treePoint, intern) iferr_return;
		}
	}

	// balance tree
	tree.Balance();

	// get current point position
	const Vector point = inputs._position[i];

	// find nearest neighbor
	const maxon::Int nearestIndex = tree.FindNearest(0, point, nullptr);

	// get neighbor position
	const Vector nearestPoint = inputs._position[nearestIndex];

	// get neighbor distance
	const Vector diff	 = nearestPoint - point;
	const Float	 value = diff.GetLength();

	return value;
}

maxon::Result<void> NextNeighborDistanceFieldLayer::Sample(const FieldLayer& layer, const FieldInput& inputs, FieldOutputBlock& outputs, const FieldInfo& info) const
{
	iferr_scope;

	// check if outputs are prepared
	if (outputs._value.IsEmpty())
		return maxon::OK;

	maxon::Float maxValue = 0.0;

	// handle each input position
	for (Int i = inputs._blockCount - 1; i >= 0; --i)
	{
		if (MAXON_UNLIKELY(outputs._deactivated[i]))
			continue;

		// get distance based value
		const Float value = CalculateValue(inputs, i) iferr_return;

		// store max. value
		if (value > maxValue)
			maxValue = value;

		// store value
		outputs._value[i] = value;
	}

	// apparently nothing found
	if (maxValue == 0.0)
		return maxon::OK;

	// normalize values
	const Float factor = 1 / maxValue;
	for (Int i = inputs._blockCount - 1; i >= 0; --i)
	{
		outputs._value[i] *= factor;
	}


	return maxon::OK;
}

void NextNeighborDistanceFieldLayer::FreeSampling(FieldLayer& layer, const FieldInfo& info, FieldShared& shared)
{
	// free internal data after sampling
}

Bool NextNeighborDistanceFieldLayer::IsEqual(const FieldLayer& layer, const FieldLayerData& comp) const
{
	return true;
}


void RegisterMographFieldsExamples()
{
	// prepare aggregated error to collect errors while registering the plugins
	maxon::AggregatedError aggErr;


	const Bool objectCommandRes = RegisterCommandPlugin(1050268, GeLoadString(IDS_SAMPLE_FIELDOBJECT_COMMAND), 0, nullptr, ""_s, SampleFieldObjectCommand::Alloc());
	if (objectCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	const Bool listCommandRes = RegisterCommandPlugin(1050269, GeLoadString(IDS_SAMPLE_FIELDLIST_COMMAND), 0, nullptr, ""_s, SampleFieldListCommand::Alloc());
	if (listCommandRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	const String checkerboardFieldName = GeLoadString(IDS_FCHECKERBOX);
	const Bool	 fieldObjectRes = RegisterFieldPlugin(1050278, checkerboardFieldName, checkerboardFieldName, 0, CheckerboardField::Alloc, "Fcheckerboard"_s, nullptr, 0);
	if (fieldObjectRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	const String nextNeighborName = GeLoadString(IDS_NEXTNEIGHBOR_LAYER);
	const Bool	 fieldLayerRes = RegisterFieldLayerPlugin(1050284,
		nextNeighborName,
		nextNeighborName,
		nextNeighborName,
		FIELDLAYER_PREMULTIPLIED | FIELDLAYER_DIRECT,
		NextNeighborDistanceFieldLayer::Alloc,
		"Flnextneighbordistance"_s,
		nullptr,
		0,
		nullptr);

	if (fieldLayerRes == false)
		aggErr.AddError(maxon::UnexpectedError(MAXON_SOURCE_LOCATION)) iferr_ignore("Don't skip registration.");


	// check if any error occurred
	if (aggErr.GetCount() != 0)
	{
		DiagnosticOutput("Errors registering plugins: @", aggErr);
		DebugStop();
	}
}
