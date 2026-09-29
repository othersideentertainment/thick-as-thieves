// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphPropertyNode.h"

// tat
#include "Quests/Modules/TATQuestGraphUtil.h"

// ue
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphPropertyNode)


UTATQuestGraphPropertyNode::UTATQuestGraphPropertyNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Set Property"));
#endif
}

TOptional<FVariant> UTATQuestGraphPropertyNode::GetValueAsVariant() const
{
   switch (PropertyType)
   {
   case ETATQuestGraphPropertyType::EndgameDuration:
      return FVariant(EndgameDuration);
   case ETATQuestGraphPropertyType::MatchMainPhaseDuration:
      return FVariant(Duration);
   default:
      break;
   }
   return NullOpt;
}

bool UTATQuestGraphPropertyNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if (!Super::ExecuteNode(params, ctx))
   {
      return false;
   }
   if (TOptional<FVariant> value = GetValueAsVariant())
   {
      TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("[SetProperty] Setting property %s to %s"),
         *StaticEnum<ETATQuestGraphPropertyType>()->GetNameStringByValue(static_cast<int64>(PropertyType)),
         *TATQuestGraphUtil::VariantToDebugString(*value));
      if (ctx.Properties.Contains(PropertyType))
      {
         ctx.Properties[PropertyType] = *value;
      }
      else
      {
         ctx.Properties.Add(PropertyType, *value);
      }
   }
   else
   {
      TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("[SetProperty] Ignoring due to unhandled property type %s"),
         *StaticEnum<ETATQuestGraphPropertyType>()->GetNameStringByValue(static_cast<int64>(PropertyType)));
   }
   return true;
}

FString UTATQuestGraphPropertyNode::GetNodeDebugName() const
{
   return FString::Printf(TEXT("SetPropertyNode(%s = %s)"),
      *StaticEnum<ETATQuestGraphPropertyType>()->GetNameStringByValue(static_cast<int64>(PropertyType)),
      *TATQuestGraphUtil::VariantToDebugString(GetValueAsVariant()));
}

#if WITH_EDITOR

EDataValidationResult UTATQuestGraphPropertyNode::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);
   int32 numIssues = 0;
   if (PropertyType == ETATQuestGraphPropertyType::None || PropertyType == ETATQuestGraphPropertyType::MAX)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("%s: Invalid property type"), *GetNodeDebugName())));
      numIssues++;
   }
   return CombineDataValidationResults(baseResult, (numIssues == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}

void UTATQuestGraphPropertyNode::GetCustomGraphContextMenuActions(TArray<FOSEGenericGraphNodeCustomAction>& outActions) const
{
   const UEnum* propTypeEnum = StaticEnum<ETATQuestGraphPropertyType>();
   for (int32 i = 0; i < static_cast<int32>(ETATQuestGraphPropertyType::MAX); i++)
   {
      const ETATQuestGraphPropertyType propertyType = static_cast<ETATQuestGraphPropertyType>(i);
      if (propertyType == ETATQuestGraphPropertyType::None)
      {
         continue;
      }
      FOSEGenericGraphNodeCustomAction& action = outActions.Emplace_GetRef();
      action.Category = FText::FromString(TEXT("Quest Properties"));
      action.Label = propTypeEnum->GetDisplayNameTextByValue(i);
      action.TooltipText = FText::FormatOrdered(INVTEXT("Add a node that sets the value of the '{0}' property"),
         StaticEnum<ETATQuestGraphPropertyType>()->GetDisplayNameTextByValue(static_cast<int64>(propertyType)));
      action.CreateNode = [propertyType](UObject* outer) -> UOSEGenericGraphNode*
      {
         UTATQuestGraphPropertyNode* newNode = NewObject<UTATQuestGraphPropertyNode>(outer);
         check(newNode != nullptr);
         newNode->PropertyType = propertyType;
         return newNode;
      };
   }
}

FText UTATQuestGraphPropertyNode::GetNodeDisplayTitle() const
{
   return FText::FormatOrdered(INVTEXT("Set {0}"), StaticEnum<ETATQuestGraphPropertyType>()->GetDisplayNameTextByValue(static_cast<int64>(PropertyType)));
}

FText UTATQuestGraphPropertyNode::GetNodeDisplaySubtitle() const
{
   switch (PropertyType)
   {
   case ETATQuestGraphPropertyType::EndgameDuration:
      return FText::AsTimespan(FTimespan::FromSeconds(FMath::RoundToInt32(EndgameDuration)));
   case ETATQuestGraphPropertyType::MatchMainPhaseDuration:
      return FText::AsTimespan(FTimespan::FromSeconds(FMath::RoundToInt32(Duration)));
   default:
      break;
   }

   // Generic case for types not explicitly handled above
   TOptional<FVariant> value = GetValueAsVariant();
   if (value && value->GetType() != EVariantTypes::Empty)
   {
      return FText::FormatOrdered(INVTEXT("{0}"), FText::FromString(*TATQuestGraphUtil::VariantToDebugString(*value, false)));
   }
   return FText::GetEmpty();
}

#endif // WITH_EDITOR
