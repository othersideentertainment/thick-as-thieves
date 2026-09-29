// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Internationalization/Text.h"

// Just a little helper struct to cache interact prompt text that is parameterized
// with some formatting
template <typename TState>
struct TTATParameterizedTextCache
{
public:

   template<typename TFunc>
   const FText& Get(const TState& state, TFunc&& generateText)
   {
      if(_text.IsEmpty() || (state != _state))
      {
         _text = generateText(state);
         _state = state;
      }

      return _text;
   }

private:
   TState _state = TState{};
   FText _text;
};

// For when the parameters are not expected to change for a caller
struct FTATSimpleTextCache
{
public:

   template<typename TFunc>
   const FText& Get(TFunc&& generateText) const
   {
      if (_text.IsEmpty())
      {
         _text = generateText();
      }

      return _text;
   }

private:
   mutable FText _text;
};
