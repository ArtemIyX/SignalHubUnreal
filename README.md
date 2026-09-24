# SignalHub

SignalHub is a per-`UGameInstance` typed in-process signal router for Unreal Engine 5.7.

It routes exact key and payload pairs without a global registry. Each game instance owns an isolated hub, so PIE sessions, clients, and servers do not share subscriptions.

## Enable the plugin

1. Enable **Signal Hub** in Edit → Plugins.
2. Restart the editor if prompted.
3. Add `SignalHub` to a consuming C++ module:

```csharp
PublicDependencyModuleNames.AddRange(new[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "SignalHub"
});
```

4. Configure queue and dispatch limits in Project Settings → Plugins → Signal Hub.

`SignalHub` is local process state. It does not replicate signals. Replicate gameplay data separately, then publish locally on each receiving game instance when appropriate.

## Core model

Each channel is identified by:

- an exact `FSignalKey`;
- one exact payload type, selected by the first listener.

These keys do not match each other:

```cpp
MakeSignalKey(int32(7));
MakeSignalKey(uint32(7));
MakeSignalKey(FName(TEXT("7")));
```

An existing key cannot change its payload schema until its last listener is removed. A later subscription using a different payload type returns `PayloadTypeMismatch`.

## C++ quick start

### Define a payload

Use a normal copyable value type. Unreal structs work well.

```cpp
USTRUCT(BlueprintType)
struct FHealthChangedSignal
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float Current = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Maximum = 0.0f;
};
```

### Subscribe

Store the returned handle if the listener needs explicit cancellation.

```cpp
#include "SignalHubSubsystem.h"

void UMyWidgetController::BeginListening()
{
    USignalHubSubsystem* hub = GetGameInstance()->GetSubsystem<USignalHubSubsystem>();
    if (!hub) return;

    const FName key(TEXT("UI.Health.Changed"));
    const FSignalSubscribeOutcome result = hub->Subscribe<FHealthChangedSignal>(
        key,
        this,
        [this](const FHealthChangedSignal& payload, const FSignalContext& context)
        {
            UpdateHealth(payload.Current, payload.Maximum);
        });

    if (result.IsBound()) healthSubscription = result.Handle;
}
```

The owner argument is weak. Passing `this` prevents a destroyed UObject from being invoked. Ownerless lambdas remain active until explicitly unsubscribed or the subsystem is destroyed.

### Publish

```cpp
void UMyHealthComponent::BroadcastHealthChanged()
{
    USignalHubSubsystem* hub = GetWorld()->GetGameInstance()->GetSubsystem<USignalHubSubsystem>();
    if (!hub) return;

    const ESignalPublishResult result = hub->Publish(
        FName(TEXT("UI.Health.Changed")),
        FHealthChangedSignal { CurrentHealth, MaxHealth });

    // Delivered: synchronous game-thread delivery.
    // Queued: safe worker-thread payload accepted for later game-thread delivery.
    // NoListeners: normal silent no-op.
}
```

### Unsubscribe

```cpp
void UMyWidgetController::EndListening()
{
    if (USignalHubSubsystem* hub = GetGameInstance()->GetSubsystem<USignalHubSubsystem>())
    {
        hub->Unsubscribe(healthSubscription);
    }
    healthSubscription.Reset();
}
```

Use `UnsubscribeAll(this)` to remove all listeners owned by one UObject.

## Threading and ordering

- Subscribe and unsubscribe are game-thread operations.
- A worker-thread publish is accepted only for worker-copy-safe payloads.
- Worker publications are delivered later on the owning game thread.
- Callbacks never execute while SignalHub holds its registry lock.
- Nested publications are deferred breadth-first until the current listener fan-out completes.
- Queue, cascade depth, and per-root dispatch limits protect against producer overload and recursive loops.

Do not access UObjects from a worker payload producer. Publish a data-only payload and resolve UObject state on the game thread.

## Blueprint usage

The editor module provides these nodes under **Signal Hub**:

- **Make Signal Key**: Converts a supported value into an opaque `FSignalKey`.
- **Publish Signal**: Publishes independent wildcard key and payload inputs.

Typical graph:

```text
Event Any Damage
  → Make Signal Key
      Value: "UI.Health.Changed"
  → Publish Signal
      Key: Make Signal Key output
      Payload: FHealthChangedSignal
```

Use an `FName`, string, boolean, byte, numeric primitive, or reflected struct as a wildcard value. Float and double keys reject NaN and infinity; `-0` is normalized to `+0`.

The visible listener-node workflow is still under development. Until it is available, bind Blueprint-facing listeners through a C++ owner class that subscribes with `USignalHubSubsystem` and forwards to Blueprint events.

## Diagnostics

`USignalHubBlueprintLibrary::GetSignalHubDiagnostics` returns:

- active channel count;
- active listener count;
- pending worker signals;
- queue overflow count and peak queue depth;
- cascade rejection count and peak cascade depth.

No-listener publication is intentionally silent. Queue and root-dispatch overflow warnings are rate-limited.

## Runtime-only values

`FSignalKey` is an opaque runtime value. Do not save, replicate, or use it as a config default. Rebuild it from its source value after loading.

## LICENSE

The project is licensed under [MIT](LICENSE)
