#include "K2Node_PublishSignal.h"

#include "EdGraphSchema_K2.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "SignalHubBlueprintLibrary.h"
#include "SignalHubTypes.h"

void UK2Node_PublishSignal::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), TEXT("WorldContextObject"));
	UEdGraphPin* keyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, TEXT("Key"));
	if (KeyPinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard && !KeyPinType.PinCategory.IsNone()) keyPin->PinType = KeyPinType;
	UEdGraphPin* payloadPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, TEXT("Payload"));
	if (PayloadPinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard && !PayloadPinType.PinCategory.IsNone()) payloadPin->PinType = PayloadPinType;
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Byte, StaticEnum<ESignalPublishResult>(), TEXT("Result"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

FText UK2Node_PublishSignal::GetNodeTitle(ENodeTitleType::Type InTitleType) const { return NSLOCTEXT("SignalHub", "PublishSignalTitle", "Publish Signal"); }
FText UK2Node_PublishSignal::GetTooltipText() const { return NSLOCTEXT("SignalHub", "PublishSignalTooltip", "Publishes an exact SignalHub key and payload in this game instance."); }
FText UK2Node_PublishSignal::GetMenuCategory() const { return NSLOCTEXT("SignalHub", "MenuCategory", "Signal Hub"); }

void UK2Node_PublishSignal::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	UClass* actionKey = GetClass();
	if (InActionRegistrar.IsOpenForRegistration(actionKey)) InActionRegistrar.AddBlueprintAction(actionKey, UBlueprintNodeSpawner::Create(actionKey));
}

void UK2Node_PublishSignal::UpdateWildcardPinType(UEdGraphPin* InPin, FEdGraphPinType& InOutStoredType)
{
	if (!InPin) return;
	if (InPin->LinkedTo.IsEmpty())
	{
		InOutStoredType = FEdGraphPinType();
		InPin->PinType = FEdGraphPinType();
		InPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
	}
	else
	{
		InOutStoredType = InPin->LinkedTo[0]->PinType;
		InPin->PinType = InOutStoredType;
	}
}

void UK2Node_PublishSignal::PinConnectionListChanged(UEdGraphPin* InPin)
{
	Super::PinConnectionListChanged(InPin);
	UEdGraphPin* keyPin = FindPin(TEXT("Key"));
	UEdGraphPin* payloadPin = FindPin(TEXT("Payload"));
	if (keyPin && (InPin == keyPin || keyPin->LinkedTo.Contains(InPin))) UpdateWildcardPinType(keyPin, KeyPinType);
	else if (payloadPin && (InPin == payloadPin || payloadPin->LinkedTo.Contains(InPin))) UpdateWildcardPinType(payloadPin, PayloadPinType);
	else return;
	if (UBlueprint* blueprint = GetTypedOuter<UBlueprint>()) FBlueprintEditorUtils::MarkBlueprintAsModified(blueprint);
	GetGraph()->NotifyGraphChanged();
}

void UK2Node_PublishSignal::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateWildcardPinType(FindPin(TEXT("Key")), KeyPinType);
	UpdateWildcardPinType(FindPin(TEXT("Payload")), PayloadPinType);
}

void UK2Node_PublishSignal::ExpandNode(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph)
{
	Super::ExpandNode(InCompilerContext, InSourceGraph);
	UEdGraphPin* key = FindPinChecked(TEXT("Key"));
	UEdGraphPin* payload = FindPinChecked(TEXT("Payload"));
	if (key->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard || payload->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		InCompilerContext.MessageLog.Error(*NSLOCTEXT("SignalHub", "UnresolvedPublish", "SignalHub: Key and Payload pins must be resolved.").ToString(), this);
		BreakAllNodeLinks();
		return;
	}
	UK2Node_CallFunction* call = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
	call->SetFromFunction(USignalHubBlueprintLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USignalHubBlueprintLibrary, PublishSignalWildcard)));
	call->AllocateDefaultPins();
	UEdGraphPin* callKey = call->FindPinChecked(TEXT("InKey"));
	UEdGraphPin* callPayload = call->FindPinChecked(TEXT("InPayload"));
	callKey->PinType = key->PinType;
	callPayload->PinType = payload->PinType;
	InCompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *call->GetExecPin());
	InCompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TEXT("WorldContextObject")), *call->FindPinChecked(TEXT("WorldContextObject")));
	InCompilerContext.MovePinLinksToIntermediate(*key, *callKey);
	InCompilerContext.MovePinLinksToIntermediate(*payload, *callPayload);
	InCompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TEXT("Result")), *call->GetReturnValuePin());
	InCompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *call->GetThenPin());
	BreakAllNodeLinks();
}
