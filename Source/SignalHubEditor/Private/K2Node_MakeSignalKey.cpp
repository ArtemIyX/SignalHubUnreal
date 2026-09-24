#include "K2Node_MakeSignalKey.h"

#include "EdGraphSchema_K2.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "SignalHubBlueprintLibrary.h"
#include "SignalKey.h"

void UK2Node_MakeSignalKey::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, TEXT("Value"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, FSignalKey::StaticStruct(), TEXT("Key"));
}

FText UK2Node_MakeSignalKey::GetNodeTitle(ENodeTitleType::Type InTitleType) const
{
	return NSLOCTEXT("SignalHub", "MakeSignalKeyTitle", "Make Signal Key");
}

FText UK2Node_MakeSignalKey::GetTooltipText() const
{
	return NSLOCTEXT("SignalHub", "MakeSignalKeyTooltip", "Creates an exact runtime-only SignalHub key from a supported value.");
}

FText UK2Node_MakeSignalKey::GetMenuCategory() const
{
	return NSLOCTEXT("SignalHub", "MenuCategory", "Signal Hub");
}

void UK2Node_MakeSignalKey::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	UClass* actionKey = GetClass();
	if (InActionRegistrar.IsOpenForRegistration(actionKey)) InActionRegistrar.AddBlueprintAction(actionKey, UBlueprintNodeSpawner::Create(actionKey));
}

void UK2Node_MakeSignalKey::PinConnectionListChanged(UEdGraphPin* InPin)
{
	Super::PinConnectionListChanged(InPin);
	UEdGraphPin* valuePin = GetValuePin();
	if (!valuePin || InPin != valuePin) return;
	if (valuePin->LinkedTo.IsEmpty())
	{
		valuePin->PinType = FEdGraphPinType();
		valuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
	}
	else
	{
		valuePin->PinType = valuePin->LinkedTo[0]->PinType;
	}
	GetGraph()->NotifyGraphChanged();
}

void UK2Node_MakeSignalKey::ExpandNode(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph)
{
	Super::ExpandNode(InCompilerContext, InSourceGraph);
	UEdGraphPin* valuePin = GetValuePin();
	if (!valuePin || valuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		InCompilerContext.MessageLog.Error(*NSLOCTEXT("SignalHub", "UnresolvedMakeKey", "SignalHub: Key type is unresolved. Connect the Value pin.").ToString(), this);
		BreakAllNodeLinks();
		return;
	}
	UK2Node_CallFunction* call = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
	call->SetFromFunction(USignalHubBlueprintLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(USignalHubBlueprintLibrary, MakeSignalKeyWildcard)));
	call->AllocateDefaultPins();
	UEdGraphPin* callValue = call->FindPinChecked(TEXT("InValue"));
	callValue->PinType = valuePin->PinType;
	InCompilerContext.MovePinLinksToIntermediate(*valuePin, *callValue);
	InCompilerContext.MovePinLinksToIntermediate(*GetKeyPin(), *call->GetReturnValuePin());
	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_MakeSignalKey::GetValuePin() const { return FindPinChecked(TEXT("Value")); }
UEdGraphPin* UK2Node_MakeSignalKey::GetKeyPin() const { return FindPinChecked(TEXT("Key")); }
