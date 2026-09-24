#include "SignalPayload.h"

#include "UObject/Class.h"


FSignalReflectedStructPayloadStorage::FSignalReflectedStructPayloadStorage(const UScriptStruct* InStruct, const void* InValue)
	: TypeId({ ESignalTypeDomain::BuiltIn, InStruct ? InStruct->GetFName() : NAME_None, InStruct ? FName(InStruct->GetPathName()) : NAME_None })
{
	if (InStruct && InValue)
	{
		Value.InitializeAs(const_cast<UScriptStruct*>(InStruct), reinterpret_cast<const uint8*>(InValue));
	}
}

FSignalPayload MakeSignalStructPayload(const UScriptStruct* InStruct, const void* InValue)
{
	if (!InStruct || !InValue) return {};
	FSignalPayload result;
	result.Storage = MakeShared<FSignalReflectedStructPayloadStorage, ESPMode::ThreadSafe>(InStruct, InValue);
	return result;
}

void FSignalReflectedStructPayloadStorage::AddReferencedObjects(FReferenceCollector& InCollector) const
{
	const_cast<FInstancedStruct&>(Value).AddStructReferencedObjects(InCollector);
}
