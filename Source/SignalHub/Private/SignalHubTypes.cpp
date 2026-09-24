#include "SignalHubTypes.h"

DEFINE_LOG_CATEGORY(LogSignalHub);

void FSignalHubLimits::Clamp()
{
	MaxQueuedSignals = FMath::Clamp(MaxQueuedSignals, 1, 65536);
	MaxSignalsPerTick = FMath::Clamp(MaxSignalsPerTick, 1, MaxQueuedSignals);
	MaxCascadeDepth = FMath::Clamp(MaxCascadeDepth, 1, 1024);
	MaxDispatchesPerRoot = FMath::Clamp(MaxDispatchesPerRoot, 1, 1048576);
}
