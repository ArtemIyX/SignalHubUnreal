#include "SignalKey.h"

#include "UObject/Class.h"

bool FSignalKey::operator==(const FSignalKey& InOther) const
{
	if (Storage == InOther.Storage)
	{
		return true;
	}
	if (!Storage.IsValid() || !InOther.Storage.IsValid() || Storage->GetTypeId() != InOther.Storage->GetTypeId())
	{
		return false;
	}
	return Storage->Equals(*InOther.Storage);
}

FSignalReflectedStructKeyStorage::FSignalReflectedStructKeyStorage(const UScriptStruct* InStruct, const void* InValue)
	: ScriptStruct(InStruct)
	, TypeId({ ESignalTypeDomain::BuiltIn, InStruct ? InStruct->GetFName() : NAME_None })
{
	if (ScriptStruct && InValue)
	{
		Value.InitializeAs(const_cast<UScriptStruct*>(ScriptStruct), reinterpret_cast<const uint8*>(InValue));
		ValueHash = HashCombine(FCrc::StrCrc32(*ScriptStruct->GetPathName()), ScriptStruct->GetStructTypeHash(Value.GetMemory()));
	}
}

bool FSignalReflectedStructKeyStorage::Equals(const ISignalKeyStorage& InOther) const
{
	const FSignalReflectedStructKeyStorage* other = static_cast<const FSignalReflectedStructKeyStorage*>(&InOther);
	return ScriptStruct == other->ScriptStruct && ScriptStruct && ScriptStruct->CompareScriptStruct(Value.GetMemory(), other->Value.GetMemory(), 0);
}

FString FSignalReflectedStructKeyStorage::Describe() const
{
	return ScriptStruct ? ScriptStruct->GetPathName() : TEXT("Invalid");
}

FSignalKey MakeSignalStructKey(const UScriptStruct* InStruct, const void* InValue)
{
	if (!InStruct || !InValue) return {};
	FSignalKey result;
	result.Storage = MakeShared<FSignalReflectedStructKeyStorage, ESPMode::ThreadSafe>(InStruct, InValue);
	return result;
}
