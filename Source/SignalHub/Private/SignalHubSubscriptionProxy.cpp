#include "SignalHubSubscriptionProxy.h"

#include "SignalHubSubsystem.h"

void USignalHubSubscription::Activate()
{
}

bool USignalHubSubscription::Cancel()
{
	if (!Handle.IsValid()) return false;
	USignalHubSubsystem* hub = Hub.Get();
	const bool wasActive = hub && hub->Unsubscribe(Handle);
	Invalidate();
	return wasActive;
}

bool USignalHubSubscription::IsActive() const
{
	return Handle.IsValid() && Hub.IsValid();
}

void USignalHubSubscription::Initialize(USignalHubSubsystem* InHub, FSignalSubscriptionHandle InHandle)
{
	Hub = InHub;
	Handle = InHandle;
	if (InHub) RegisterWithGameInstance(InHub->GetGameInstance());
}

void USignalHubSubscription::Invalidate()
{
	Handle.Reset();
	Hub.Reset();
	SetReadyToDestroy();
}
