#include "K2Node_ExtractSignalPayload.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "SignalHubBlueprintLibrary.h"
#include "SignalHubBlueprintTypes.h"

void UK2Node_ExtractSignalPayload::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FSignalBlueprintEnvelope::StaticStruct(), TEXT("Envelope"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, TEXT("Payload"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, TEXT("Success"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

FText UK2Node_ExtractSignalPayload::GetNodeTitle(ENodeTitleType::Type InTitleType) const
{
	return NSLOCTEXT("SignalHub", "ExtractSignalPayloadTitle", "Extract Signal Payload");
}

FText UK2Node_ExtractSignalPayload::GetTooltipText() const
{
	return NSLOCTEXT("SignalHub", "ExtractSignalPayloadTooltip", "Extracts the signal payload into the type connected to Payload. Success is false when the payload type does not match.");
}

FText UK2Node_ExtractSignalPayload::GetMenuCategory() const
{
	return NSLOCTEXT("SignalHub", "MenuCategory", "Signal Hub");
}

void UK2Node_ExtractSignalPayload::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	UClass* actionKey = GetClass();
	if (InActionRegistrar.IsOpenForRegistration(actionKey)) InActionRegistrar.AddBlueprintAction(actionKey, UBlueprintNodeSpawner::Create(actionKey));
}

void UK2Node_ExtractSignalPayload::PinConnectionListChanged(UEdGraphPin* InPin)
{
	Super::PinConnectionListChanged(InPin);
	UEdGraphPin* payloadPin = GetPayloadPin();
	if (!payloadPin || InPin != payloadPin) return;
	if (payloadPin->LinkedTo.IsEmpty())
	{
		payloadPin->PinType = FEdGraphPinType();
		payloadPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
	}
	else payloadPin->PinType = payloadPin->LinkedTo[0]->PinType;
	GetGraph()->NotifyGraphChanged();
}

void UK2Node_ExtractSignalPayload::ExpandNode(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph)
{
	Super::ExpandNode(InCompilerContext, InSourceGraph);
	UEdGraphPin* payloadPin = GetPayloadPin();
	if (!payloadPin || payloadPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		InCompilerContext.MessageLog.Error(*NSLOCTEXT("SignalHub", "UnresolvedExtractPayload", "SignalHub: Payload type is unresolved. Connect the Payload pin.").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	UK2Node_CallFunction* call = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
	call->SetFromFunction(USignalHubBlueprintLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USignalHubBlueprintLibrary, TryExtractSignalPayload)));
	call->AllocateDefaultPins();
	UEdGraphPin* callPayload = call->FindPinChecked(TEXT("OutPayload"));
	callPayload->PinType = payloadPin->PinType;
	InCompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *call->GetExecPin());
	InCompilerContext.MovePinLinksToIntermediate(*GetEnvelopePin(), *call->FindPinChecked(TEXT("InEnvelope")));
	InCompilerContext.MovePinLinksToIntermediate(*payloadPin, *callPayload);
	InCompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TEXT("Success")), *call->GetReturnValuePin());
	InCompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *call->GetThenPin());
	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_ExtractSignalPayload::GetEnvelopePin() const { return FindPinChecked(TEXT("Envelope")); }
UEdGraphPin* UK2Node_ExtractSignalPayload::GetPayloadPin() const { return FindPinChecked(TEXT("Payload")); }
