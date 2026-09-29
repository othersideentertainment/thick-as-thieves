/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#pragma once
#include "Async/Future.h"
#include "Wwise/AdapterTypes/WwiseFuture.h"

template<typename ReturnType>
class WwiseUnrealFuture : public IWwiseFuture<TFuture<ReturnType>>
{
public:
	WwiseUnrealFuture(): IWwiseFuture<TFuture<ReturnType>>()
	{
	}

	WwiseUnrealFuture(TFuture<ReturnType>&& Future)
	: IWwiseFuture<TFuture<ReturnType>>(std::move(Future))
	{}

	ReturnType Get() const
	{
		return this->Future.Get();
	}
};

template<typename ReturnType>
class WwiseUnrealSharedFuture : public IWwiseSharedFuture<TSharedFuture<ReturnType>>
{
public:
	WwiseUnrealSharedFuture(): IWwiseSharedFuture<TFuture<ReturnType>>()
	{
	}

	WwiseUnrealSharedFuture(TSharedFuture<ReturnType>&& Future)
	: IWwiseSharedFuture<TSharedFuture<ReturnType>>(std::move(Future))
	{}

	ReturnType Get() const
	{
		return this->Future.Get();
	}
};
