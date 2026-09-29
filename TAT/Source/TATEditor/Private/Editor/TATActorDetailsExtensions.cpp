// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATActorDetailsExtensions.h"

// ue
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "LayersModule.h"


void TATActorDetailsExtensions::ExtendActorDetails(class IDetailLayoutBuilder& detailBuilder, const FGetSelectedActors& getSelectedActors)
{
   // NOTE: This was adapted from `FActorDetails::AddLayersCategory`, which is present, but not actually called
   if( !FModuleManager::Get().IsModuleLoaded( TEXT("Layers") ) )
   {
      return;
   }

   FLayersModule& layersModule = FModuleManager::LoadModuleChecked< FLayersModule >( TEXT("Layers") );

   const FText layerCategory = NSLOCTEXT("TATActorDetailsExtensions", "LayersCategory", "Layers");

   detailBuilder.EditCategory( "Layers", layerCategory, ECategoryPriority::Uncommon )
   .AddCustomRow( FText::GetEmpty() )
   [
      layersModule.CreateLayerCloud( getSelectedActors.Execute() )
   ];
}
