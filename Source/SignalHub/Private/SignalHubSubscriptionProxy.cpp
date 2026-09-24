#include "SignalHubSubscriptionProxy.h"

#include "SignalHubSubsystem.h"

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
}

void USignalHubSubscription::Invalidate()
{
	Handle.Reset();
	Hub.Reset();
}
