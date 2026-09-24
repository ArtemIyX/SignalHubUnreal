#include "Misc/AutomationTest.h"
#include "SignalHubTypes.h"
#include "SignalKey.h"
#include "SignalPayload.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubNativeKeyEqualityTest, "SignalHub.Unit.Key.NativeEquality", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSignalHubNativeKeyEqualityTest::RunTest(const FString& InParameters)
{
	const FSignalKey first = MakeSignalKey(int32(7));
	const FSignalKey same = MakeSignalKey(int32(7));
	const FSignalKey differentType = MakeSignalKey(uint32(7));
	TestTrue(TEXT("Equal values with the same type compare equal"), first == same);
	TestEqual(TEXT("Equal values with the same type hash equally"), GetTypeHash(first), GetTypeHash(same));
	TestFalse(TEXT("Exact type is part of key identity"), first == differentType);
	TestEqual(TEXT("Correct typed key access succeeds"), *first.TryGet<int32>(), 7);
	TestNull(TEXT("Wrong typed key access fails"), first.TryGet<uint32>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubDefaultLimitsTest, "SignalHub.Unit.Config.DefaultLimits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSignalHubDefaultLimitsTest::RunTest(const FString& InParameters)
{
	const FSignalHubLimits limits;
	TestEqual(TEXT("Default queue capacity"), limits.MaxQueuedSignals, 4096);
	TestEqual(TEXT("Default tick capacity"), limits.MaxSignalsPerTick, 512);
	TestEqual(TEXT("Default cascade depth"), limits.MaxCascadeDepth, 32);
	TestEqual(TEXT("Default root dispatch capacity"), limits.MaxDispatchesPerRoot, 1024);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubReflectedStructKeyTest, "SignalHub.Unit.Key.NativeStructCppThunkParity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSignalHubReflectedStructKeyTest::RunTest(const FString& InParameters)
{
	const FVector firstValue(1.0, 2.0, 3.0);
	const FVector sameValue(1.0, 2.0, 3.0);
	const FVector differentValue(3.0, 2.0, 1.0);
	const UScriptStruct* vectorStruct = TBaseStructure<FVector>::Get();
	const FSignalKey first = MakeSignalStructKey(vectorStruct, &firstValue);
	const FSignalKey same = MakeSignalStructKey(vectorStruct, &sameValue);
	const FSignalKey different = MakeSignalStructKey(vectorStruct, &differentValue);
	TestTrue(TEXT("Reflected key is valid"), first.IsValid());
	TestTrue(TEXT("Equal reflected values compare equal"), first == same);
	TestEqual(TEXT("Equal reflected values hash equally"), GetTypeHash(first), GetTypeHash(same));
	TestFalse(TEXT("Different reflected values do not compare equal"), first == different);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubReflectedPayloadTest, "SignalHub.Unit.Payload.StructCopy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSignalHubReflectedPayloadTest::RunTest(const FString& InParameters)
{
	FVector source(1.0, 2.0, 3.0);
	const UScriptStruct* vectorStruct = TBaseStructure<FVector>::Get();
	const FSignalPayload payload = MakeSignalStructPayload(vectorStruct, &source);
	source.X = 99.0;
	const FVector* storedValue = static_cast<const FVector*>(payload.TryGetStruct(vectorStruct));
	TestNotNull(TEXT("Reflected payload has matching struct storage"), storedValue);
	TestEqual(TEXT("Reflected payload owns its source copy"), storedValue->X, 1.0);
	TestFalse(TEXT("Reflected payload is not producer-thread safe by default"), payload.IsWorkerCopySafe());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubPayloadIsolationTest, "SignalHub.Unit.Payload.SourceMutation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSignalHubPayloadIsolationTest::RunTest(const FString& InParameters)
{
	int32 value = 17;
	const FSignalPayload payload = MakeSignalPayload(value);
	value = 23;
	const int32* storedValue = payload.TryGet<int32>();
	TestNotNull(TEXT("Payload has its exact stored type"), storedValue);
	TestEqual(TEXT("Payload owns a source copy"), *storedValue, 17);
	TestNull(TEXT("Wrong payload access is rejected"), payload.TryGet<FString>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSignalHubLimitsClampTest, "SignalHub.Unit.Config.InvalidLimitsClamped", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSignalHubLimitsClampTest::RunTest(const FString& InParameters)
{
	FSignalHubLimits limits;
	limits.MaxQueuedSignals = 0;
	limits.MaxSignalsPerTick = -1;
	limits.MaxCascadeDepth = 0;
	limits.MaxDispatchesPerRoot = 0;
	limits.Clamp();
	TestTrue(TEXT("Queue limit clamps positive"), limits.MaxQueuedSignals > 0);
	TestTrue(TEXT("Tick limit clamps within queue limit"), limits.MaxSignalsPerTick > 0 && limits.MaxSignalsPerTick <= limits.MaxQueuedSignals);
	TestTrue(TEXT("Cascade limit clamps positive"), limits.MaxCascadeDepth > 0);
	TestTrue(TEXT("Dispatch limit clamps positive"), limits.MaxDispatchesPerRoot > 0);
	return true;
}
