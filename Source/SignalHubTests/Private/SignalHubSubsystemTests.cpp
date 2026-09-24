#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "SignalHubSubsystem.h"
#include "SignalHubSubscriptionProxy.h"
#include "Subsystems/SubsystemCollection.h"

#define SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter

namespace
{
struct FSignalHubFixture
{
	FSignalHubFixture()
	{
		GameInstance = NewObject<UGameInstance>(GetTransientPackage());
		Hub = NewObject<USignalHubSubsystem>(GameInstance);
		Hub->Initialize(Collection);
	}

	~FSignalHubFixture()
	{
		if (Hub) Hub->Deinitialize();
	}

	UGameInstance* GameInstance = nullptr;
	USignalHubSubsystem* Hub = nullptr;
	FSubsystemCollection<UGameInstanceSubsystem> Collection;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubSubscribePublishTest, "SignalHub.Unit.Publish.OneListener", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubSubscribePublishTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	int32 callbackCount = 0;
	int32 receivedValue = 0;
	const FSignalSubscribeOutcome subscription = fixture.Hub->Subscribe<int32>(FName(TEXT("Unit.Publish.One")), nullptr,
		[&callbackCount, &receivedValue](const int32 value, const FSignalContext&) { ++callbackCount; receivedValue = value; });
	TestTrue(TEXT("Subscription binds"), subscription.IsBound());
	TestEqual(TEXT("Publish delivers synchronously"), fixture.Hub->Publish(FName(TEXT("Unit.Publish.One")), 77), ESignalPublishResult::Delivered);
	TestEqual(TEXT("One listener runs once"), callbackCount, 1);
	TestEqual(TEXT("Listener receives exact payload"), receivedValue, 77);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubUnsubscribeTest, "SignalHub.Unit.Subscribe.UnsubscribeIdempotent", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubUnsubscribeTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	const FSignalSubscribeOutcome subscription = fixture.Hub->Subscribe<int32>(FName(TEXT("Unit.Unsubscribe")), nullptr, [](const int32, const FSignalContext&) {});
	TestTrue(TEXT("First unsubscribe succeeds"), fixture.Hub->Unsubscribe(subscription.Handle));
	TestFalse(TEXT("Second unsubscribe is idempotent"), fixture.Hub->Unsubscribe(subscription.Handle));
	TestEqual(TEXT("No-listener path is silent"), fixture.Hub->Publish(FName(TEXT("Unit.Unsubscribe")), 1), ESignalPublishResult::NoListeners);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubSchemaTest, "SignalHub.Unit.Subscribe.SecondWrongType", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubSchemaTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	TestTrue(TEXT("First schema binds"), fixture.Hub->Subscribe<int32>(FName(TEXT("Unit.Schema")), nullptr, [](const int32, const FSignalContext&) {}).IsBound());
	const FSignalSubscribeOutcome mismatch = fixture.Hub->Subscribe<FString>(FName(TEXT("Unit.Schema")), nullptr, [](const FString&, const FSignalContext&) {});
	TestEqual(TEXT("Live channel rejects another schema"), mismatch.Result, ESignalSubscribeResult::PayloadTypeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubReentrantTest, "SignalHub.Unit.Reentry.SameKeyDeferred", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubReentrantTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	TArray<int32> order;
	const FName key(TEXT("Unit.Reentry"));
	fixture.Hub->Subscribe<int32>(key, nullptr, [&fixture, &order, key](const int32 value, const FSignalContext&)
	{
		order.Add(value);
		if (value == 1) fixture.Hub->Publish(key, 2);
	});
	fixture.Hub->Subscribe<int32>(key, nullptr, [&order](const int32 value, const FSignalContext&) { order.Add(value * 10); });
	TestEqual(TEXT("Root publication completes"), fixture.Hub->Publish(key, 1), ESignalPublishResult::Delivered);
	TestEqual(TEXT("Nested publish waits for root fanout"), order, TArray<int32>({ 1, 10, 2, 20 }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubEmptySignalTest, "SignalHub.Unit.Payload.Empty", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubEmptySignalTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	int32 count = 0;
	const FName key(TEXT("Unit.Empty"));
	fixture.Hub->Subscribe<FSignalEmptyPayload>(key, nullptr, [&count](const FSignalEmptyPayload&, const FSignalContext&) { ++count; });
	TestEqual(TEXT("Empty signal is delivered"), fixture.Hub->PublishEmpty(key), ESignalPublishResult::Delivered);
	TestEqual(TEXT("Empty listener runs"), count, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubSubscriptionProxyTest, "SignalHub.Unit.BlueprintProxy.CancelIdempotent", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubSubscriptionProxyTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	const FSignalSubscribeOutcome subscription = fixture.Hub->Subscribe<int32>(FName(TEXT("Unit.Proxy")), nullptr, [](const int32, const FSignalContext&) {});
	USignalHubSubscription* proxy = NewObject<USignalHubSubscription>(fixture.GameInstance);
	proxy->Initialize(fixture.Hub, subscription.Handle);
	TestTrue(TEXT("Initialized proxy is active"), proxy->IsActive());
	TestTrue(TEXT("First cancel removes subscription"), proxy->Cancel());
	TestFalse(TEXT("Canceled proxy is inactive"), proxy->IsActive());
	TestFalse(TEXT("Second cancel is idempotent"), proxy->Cancel());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubStaleGenerationTest, "SignalHub.Unit.Subscribe.StaleGeneration", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubStaleGenerationTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	const FName key(TEXT("Unit.Generation"));
	const FSignalSubscribeOutcome first = fixture.Hub->Subscribe<int32>(key, nullptr, [](const int32, const FSignalContext&) {});
	fixture.Hub->Unsubscribe(first.Handle);
	const FSignalSubscribeOutcome replacement = fixture.Hub->Subscribe<int32>(key, nullptr, [](const int32, const FSignalContext&) {});
	TestNotEqual(TEXT("Recreated channel gets a new generation"), first.Handle.Generation, replacement.Handle.Generation);
	TestFalse(TEXT("Old handle cannot remove replacement"), fixture.Hub->Unsubscribe(first.Handle));
	TestTrue(TEXT("Replacement remains active"), fixture.Hub->IsBound(MakeSignalKey(key)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubUnsubscribeAllTest, "SignalHub.Unit.Subscribe.UnsubscribeAllOwner", SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS)

bool FSignalHubUnsubscribeAllTest::RunTest(const FString& InParameters)
{
	FSignalHubFixture fixture;
	UObject* owner = NewObject<UObject>(fixture.GameInstance);
	fixture.Hub->Subscribe<int32>(FName(TEXT("Unit.Owner.A")), owner, [](const int32, const FSignalContext&) {});
	fixture.Hub->Subscribe<int32>(FName(TEXT("Unit.Owner.B")), owner, [](const int32, const FSignalContext&) {});
	fixture.Hub->Subscribe<int32>(FName(TEXT("Unit.Owner.C")), nullptr, [](const int32, const FSignalContext&) {});
	TestEqual(TEXT("All owner listeners are removed"), fixture.Hub->UnsubscribeAll(owner), 2);
	TestFalse(TEXT("Owner key A is unbound"), fixture.Hub->IsBound(MakeSignalKey(FName(TEXT("Unit.Owner.A")))));
	TestTrue(TEXT("Ownerless key remains bound"), fixture.Hub->IsBound(MakeSignalKey(FName(TEXT("Unit.Owner.C")))));
	return true;
}

#undef SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS
