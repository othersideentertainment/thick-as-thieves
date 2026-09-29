// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Modules/TATQuestGraphTextReplacementNode.h"

// tat
#include "Quests/TATQuestFormatParamSource.h"


// ue
#include "Misc/DataValidation.h"
#include "StructUtils/InstancedStruct.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphTextReplacementNode)


UTATQuestGraphTextReplacementNode::UTATQuestGraphTextReplacementNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = INVTEXT("Text Replacement");
#endif
}

bool UTATQuestGraphTextReplacementNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   for(const TPair<FString, FText>& pair : FormatParams)
   {
      ctx.ClueFormatParams.Add(pair.Key, pair.Value);
   }
   return true;
}

FString UTATQuestGraphTextReplacementNode::GetNodeDebugName() const
{
   return FString::Printf(TEXT("TextReplacement(%s)"), *_GetDetailsString());
}

#if WITH_EDITOR
EDataValidationResult UTATQuestGraphTextReplacementNode::IsDataValid(FDataValidationContext& context) const
{
   return Super::IsDataValid(context);
}

FText UTATQuestGraphTextReplacementNode::GetNodeDisplayTitle() const
{
   return INVTEXT("Text Replacement");
}

FText UTATQuestGraphTextReplacementNode::GetNodeDisplaySubtitle() const
{
   return FText::FromString(_GetDetailsString());
}
#endif


FString UTATQuestGraphTextReplacementNode::_GetDetailsString() const
{
   FString result;
   for(const TPair<FString, FText>& pair : FormatParams)
   {
      if(result.Len() > 0)
      {
         result.Append(TEXT(", "));
      }
      result.Appendf(TEXT("{%s} = %s"), *pair.Key, *pair.Value.ToString());
   }
   return result;
}

UTATQuestGraphTextReplacementSourceNode::UTATQuestGraphTextReplacementSourceNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = INVTEXT("Text Replacement Source");
#endif
}

bool UTATQuestGraphTextReplacementSourceNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   const FTATQuestFormatParamSource::FParams sourceParams = { .MapSeed = params.MapSeed };
   FTATClueFormatParams& formatParams = ctx.ClueFormatParams;
   for(const FInstancedStruct& source : FormatParamSources)
   {
      if(source.IsValid())
      {
         source.Get<FTATQuestFormatParamSource>().AddTextReplacement(sourceParams,
            [&formatParams](const FString& key, const FText& text) { formatParams.Add(key, text); });
      }
   }
   return true;
}

FString UTATQuestGraphTextReplacementSourceNode::GetNodeDebugName() const
{
   return FString::Printf(TEXT("TextReplacementSource(%s)"), *_GetDetailsString());
}

#if WITH_EDITOR
EDataValidationResult UTATQuestGraphTextReplacementSourceNode::IsDataValid(FDataValidationContext& context) const
{
   // TODO: improve node context in messaging
   bool hadErrors = false;
   auto reportError = [&context, &hadErrors](const FText& message)
   {
      hadErrors = true;
      context.AddError(message);
   };
   
   for(int i = 0; i < FormatParamSources.Num(); ++i)
   {
      const FInstancedStruct& source = FormatParamSources[i];
      if(source.IsValid())
      {
         source.Get<FTATQuestFormatParamSource>().Validate([reportError, i](const FText& message)
         {
            reportError(FText::FormatOrdered(INVTEXT("FormatParamSources[{0}]: {1}"), i, message));
         });
      }
      else
      {
         reportError(FText::FormatOrdered(INVTEXT("FormatParamSources[{0}]: No source set"), i));
      }
   }
   return hadErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

FText UTATQuestGraphTextReplacementSourceNode::GetNodeDisplayTitle() const
{
   return INVTEXT("Text Replacement Source");
}

FText UTATQuestGraphTextReplacementSourceNode::GetNodeDisplaySubtitle() const
{
   return FText::FromString(_GetDetailsString());
}
#endif


FString UTATQuestGraphTextReplacementSourceNode::_GetDetailsString() const
{
   FString result;
   for(const FInstancedStruct& source : FormatParamSources)
   {
      if(source.IsValid())
      {
         if(result.Len() > 0)
         {
            result.Append(TEXT(", "));
         }
         // TODO: actual details?
         result.Append(source.GetScriptStruct()->GetName());
      }
   }
   return result;
}
