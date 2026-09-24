#pragma once

#include "EdGraph/EdGraphPin.h"
#include "K2Node.h"
#include "K2Node_ExtractSignalPayload.generated.h"

UCLASS()
class SIGNALHUBEDITOR_API UK2Node_ExtractSignalPayload final : public UK2Node
{
	GENERATED_BODY()

public:
#pragma region Overrides
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type InTitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;
	virtual void PostReconstructNode() override;
	virtual void PinConnectionListChanged(UEdGraphPin* InPin) override;
	virtual void ExpandNode(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph) override;
#pragma endregion

private:
	UPROPERTY()
	FEdGraphPinType PayloadPinType;

	void UpdatePayloadPinType();
	UEdGraphPin* GetEnvelopePin() const;
	UEdGraphPin* GetPayloadPin() const;
};
