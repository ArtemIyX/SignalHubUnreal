#pragma once

#include "CoreMinimal.h"
#include "SignalKey.h"

class ISignalPayloadStorage
{
public:
	virtual ~ISignalPayloadStorage() = default;
	virtual const FSignalTypeId& GetTypeId() const = 0;
	virtual bool IsWorkerCopySafe() const = 0;
};

template <typename TPayload>
struct TSignalPayloadTraits
{
	static constexpr bool bThreadSafeCopy = TIsTriviallyCopyConstructible<TPayload>::Value;
};

template <typename TPayload>
class TSignalNativePayloadStorage final : public ISignalPayloadStorage
{
public:
	explicit TSignalNativePayloadStorage(const TPayload& InValue) : Value(InValue), TypeId(TSignalTypeTraits<TPayload>::Get()) {}
	virtual const FSignalTypeId& GetTypeId() const override { return TypeId; }
	virtual bool IsWorkerCopySafe() const override { return TSignalPayloadTraits<TPayload>::bThreadSafeCopy; }
	const TPayload& Get() const { return Value; }

private:
	TPayload Value;
	FSignalTypeId TypeId;
};

class SIGNALHUB_API FSignalPayload
{
public:
	FSignalPayload() = default;
	bool IsValid() const { return Storage.IsValid(); }
	const FSignalTypeId* GetTypeId() const { return Storage.IsValid() ? &Storage->GetTypeId() : nullptr; }
	bool IsWorkerCopySafe() const { return Storage.IsValid() && Storage->IsWorkerCopySafe(); }

	template <typename TPayload>
	const TPayload* TryGet() const
	{
		using FPayloadType = std::decay_t<TPayload>;
		if (!Storage.IsValid() || Storage->GetTypeId() != TSignalTypeTraits<FPayloadType>::Get())
		{
			return nullptr;
		}
		return &static_cast<const TSignalNativePayloadStorage<FPayloadType>&>(*Storage).Get();
	}

private:
	template <typename TPayload> friend FSignalPayload MakeSignalPayload(const TPayload& InPayload);
	TSharedPtr<const ISignalPayloadStorage, ESPMode::ThreadSafe> Storage;
};

template <typename TPayload>
FSignalPayload MakeSignalPayload(const TPayload& InPayload)
{
	using FPayloadType = std::decay_t<TPayload>;
	static_assert(std::is_copy_constructible_v<FPayloadType>, "SignalHub payloads must be copyable.");
	FSignalPayload result;
	result.Storage = MakeShared<TSignalNativePayloadStorage<FPayloadType>, ESPMode::ThreadSafe>(InPayload);
	return result;
}
