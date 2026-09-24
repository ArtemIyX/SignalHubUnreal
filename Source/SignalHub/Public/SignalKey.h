#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"
#include "StructUtils/InstancedStruct.h"
#include "SignalHubTypes.h"
#include <type_traits>
#include "SignalKey.generated.h"

enum class ESignalTypeDomain : uint8 { BuiltIn, Native };

struct FSignalKey;
SIGNALHUB_API FSignalKey MakeSignalStructKey(const UScriptStruct* InStruct, const void* InValue);

struct SIGNALHUB_API FSignalTypeId
{
	ESignalTypeDomain Domain = ESignalTypeDomain::Native;
	FName Name;
	FName ReflectedPath;

	bool operator==(const FSignalTypeId& InOther) const { return Domain == InOther.Domain && Name == InOther.Name && ReflectedPath == InOther.ReflectedPath; }
	FString Describe() const { return ReflectedPath.IsNone() ? Name.ToString() : ReflectedPath.ToString(); }
};

FORCEINLINE uint32 GetTypeHash(const FSignalTypeId& InTypeId)
{
	return HashCombine(HashCombine(::GetTypeHash(static_cast<uint8>(InTypeId.Domain)), FCrc::StrCrc32(*InTypeId.Name.ToString())), FCrc::StrCrc32(*InTypeId.ReflectedPath.ToString()));
}

class ISignalKeyStorage
{
public:
	virtual ~ISignalKeyStorage() = default;
	virtual const FSignalTypeId& GetTypeId() const = 0;
	virtual uint32 GetValueHash() const = 0;
	virtual bool Equals(const ISignalKeyStorage& InOther) const = 0;
	virtual FString Describe() const = 0;
};

class SIGNALHUB_API FSignalReflectedStructKeyStorage final : public ISignalKeyStorage
{
public:
	FSignalReflectedStructKeyStorage(const UScriptStruct* InStruct, const void* InValue);
	virtual const FSignalTypeId& GetTypeId() const override { return TypeId; }
	virtual uint32 GetValueHash() const override { return ValueHash; }
	virtual bool Equals(const ISignalKeyStorage& InOther) const override;
	virtual FString Describe() const override;

private:
	const UScriptStruct* ScriptStruct = nullptr;
	FInstancedStruct Value;
	FSignalTypeId TypeId;
	uint32 ValueHash = 0;
};

inline uint32 SignalHubGetValueHash(const FName& InValue)
{
	return InValue.GetComparisonIndex().ToUnstableInt();
}

template <typename TValue>
auto SignalHubGetValueHash(const TValue& InValue) -> decltype(GetTypeHash(InValue))
{
	return GetTypeHash(InValue);
}

template <typename TKey>
struct TSignalTypeTraits
{
	static FSignalTypeId Get()
	{
		return { ESignalTypeDomain::Native, FName(TEXT(__FUNCSIG__)) };
	}
};

template <typename TKey>
class TSignalNativeKeyStorage final : public ISignalKeyStorage
{
public:
	explicit TSignalNativeKeyStorage(TKey&& InValue) : Value(MoveTemp(InValue)), TypeId(TSignalTypeTraits<TKey>::Get()), ValueHash(SignalHubGetValueHash(Value)) {}
	explicit TSignalNativeKeyStorage(const TKey& InValue) : Value(InValue), TypeId(TSignalTypeTraits<TKey>::Get()), ValueHash(SignalHubGetValueHash(Value)) {}

	virtual const FSignalTypeId& GetTypeId() const override { return TypeId; }
	virtual uint32 GetValueHash() const override { return ValueHash; }
	virtual bool Equals(const ISignalKeyStorage& InOther) const override
	{
		const TSignalNativeKeyStorage* other = static_cast<const TSignalNativeKeyStorage*>(&InOther);
		return Value == other->Value;
	}
	virtual FString Describe() const override { return TypeId.Describe(); }
	const TKey& Get() const { return Value; }

private:
	TKey Value;
	FSignalTypeId TypeId;
	uint32 ValueHash;
};

USTRUCT(BlueprintType)
struct SIGNALHUB_API FSignalKey
{
	GENERATED_BODY()

	bool IsValid() const { return Storage.IsValid(); }
	const FSignalTypeId* GetTypeId() const { return Storage.IsValid() ? &Storage->GetTypeId() : nullptr; }
	uint32 GetValueHash() const { return Storage.IsValid() ? Storage->GetValueHash() : 0; }
	FString Describe() const { return Storage.IsValid() ? Storage->Describe().Left(256) : TEXT("Invalid"); }
	template <typename TKey>
	const TKey* TryGet() const
	{
		using FKeyType = std::decay_t<TKey>;
		if (!Storage.IsValid() || Storage->GetTypeId() != TSignalTypeTraits<FKeyType>::Get()) return nullptr;
		return &static_cast<const TSignalNativeKeyStorage<FKeyType>&>(*Storage).Get();
	}
	bool operator==(const FSignalKey& InOther) const;

private:
	template <typename TKey> friend FSignalKey MakeSignalKey(TKey&& InKey);
	friend SIGNALHUB_API FSignalKey MakeSignalStructKey(const UScriptStruct* InStruct, const void* InValue);
	TSharedPtr<const ISignalKeyStorage, ESPMode::ThreadSafe> Storage;
};

FORCEINLINE uint32 GetTypeHash(const FSignalKey& InKey)
{
	const FSignalTypeId* typeId = InKey.GetTypeId();
	return typeId ? HashCombine(::GetTypeHash(*typeId), InKey.GetValueHash()) : 0;
}

template <typename TKey>
FSignalKey MakeSignalKey(TKey&& InKey)
{
	using FKeyType = std::decay_t<TKey>;
	static_assert(std::is_copy_constructible_v<FKeyType>, "SignalHub keys must be copyable.");
	FSignalKey result;
	result.Storage = MakeShared<TSignalNativeKeyStorage<FKeyType>, ESPMode::ThreadSafe>(Forward<TKey>(InKey));
	return result;
}
