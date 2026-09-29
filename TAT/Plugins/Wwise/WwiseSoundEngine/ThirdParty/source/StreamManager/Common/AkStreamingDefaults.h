/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

  Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkStreamingDefaults.h
//
// Default values for streaming and I/O device settings.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_STREAMING_DEFAULTS_H_
#define _AK_STREAMING_DEFAULTS_H_

#include "AkPlatformStreamingDefaults.h"

// Device settings.
#define AK_DEFAULT_DEVICE_IO_POOL_SIZE		(2*1024*1024)	// 2 MB for I/O. Pool is split up in blocks of size AkDeviceSettings::uGranularity.
															// The smaller the granularity, the smaller this pool can be to handle the same number of streams.
															// However, having a small granularity is often inefficient regarding I/O throughput.
															// As a rule of thumb, use the smallest granularity that does not degrade I/O throughput.
															// Then adjust the I/O pool size in order to handle the number number of streams you expect to be using.
															// Consider that each stream will be at least double or triple-buffered (in fact, this depends on the target buffering length).
#define AK_DEFAULT_DEVICE_GRANULARITY		(16*1024)		// 16 KB. Completely arbitrary (see note above).

#define AK_DEFAULT_DEVICE_THREAD_PRIORITY	(AK_THREAD_PRIORITY_ABOVE_NORMAL)	// I/O thread spends most of its time sleeping or waiting for a device. 
															// It should have a high priority, in order to quickly determine the next task and send it to the 
															// Low-Level IO, then wait for it.

#define AK_DEFAULT_DEVICE_BUFFERING_LENGTH	(380.f)			// 380 ms. 
#define AK_DEFAULT_MAX_CONCURRENT_IO		(8)				// Default is arbitrarily set to 8 for deferred device.

#define AK_DEFAULT_DEVICE_CACHE_ENABLED	(false)			// Caching is disabled by default.

#endif //_STREAMING_DEFAULTS_H_
