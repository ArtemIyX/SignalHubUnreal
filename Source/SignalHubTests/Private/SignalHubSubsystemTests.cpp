#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "SignalHubSubsystem.h"
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

#undef SIGNAL_HUB_SUBSYSTEM_TEST_FLAGS
