#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"
#include "SignalHubTypes.h"
#include <type_traits>
#include "SignalKey.generated.h"

enum class ESignalTypeDomain : uint8 { BuiltIn, Native };

struct SIGNALHUB_API FSignalTypeId
{
	ESignalTypeDomain Domain = ESignalTypeDomain::Native;
	FName Name;

	bool operator==(const FSignalTypeId& InOther) const { return Domain == InOther.Domain && Name == InOther.Name; }
	FString Describe() const { return Name.ToString(); }
};

FORCEINLINE uint32 GetTypeHash(const FSignalTypeId& InTypeId)
{
	return HashCombine(::GetTypeHash(static_cast<uint8>(InTypeId.Domain)), FCrc::StrCrc32(*InTypeId.Name.ToString()));
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
	explicit TSignalNativeKeyStorage(TKey&& InValue) : Value(MoveTemp(InValue)), TypeId(TSignalTypeTraits<TKey>::Get()), ValueHash(::GetTypeHash(Value)) {}
	explicit TSignalNativeKeyStorage(const TKey& InValue) : Value(InValue), TypeId(TSignalTypeTraits<TKey>::Get()), ValueHash(::GetTypeHash(Value)) {}

	virtual const FSignalTypeId& GetTypeId() const override { return TypeId; }
	virtual uint32 GetValueHash() const override { return ValueHash; }
	virtual bool Equals(const ISignalKeyStorage& InOther) const override
	{
		const TSignalNativeKeyStorage* other = static_cast<const TSignalNativeKeyStorage*>(&InOther);
		return Value == other->Value;
	}
	virtual FString Describe() const override { return TypeId.Describe(); }

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
	bool operator==(const FSignalKey& InOther) const;

private:
	template <typename TKey> friend FSignalKey MakeSignalKey(TKey&& InKey);
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
