#pragma once

#include "K2Node.h"
#include "K2Node_MakeSignalKey.generated.h"

UCLASS()
class SIGNALHUBEDITOR_API UK2Node_MakeSignalKey final : public UK2Node
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type InTitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual void PinConnectionListChanged(UEdGraphPin* InPin) override;
	virtual void ExpandNode(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph) override;

private:
	UEdGraphPin* GetValuePin() const;
	UEdGraphPin* GetKeyPin() const;
};
