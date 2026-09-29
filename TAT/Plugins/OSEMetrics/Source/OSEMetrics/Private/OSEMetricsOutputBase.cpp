// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEMetricsOutputBase.h"

// ose
#include "OSEMetricsSystem.h"

FOSEMetricsOutputBase::FOSEMetricsOutputBase()
{
}

FOSEMetricsOutputBase::~FOSEMetricsOutputBase()
{
}

TSharedPtr<FOSEMetricsSystem> FOSEMetricsOutputBase::_GetMetricsSystem() const
{
   return _weakSystem.Pin();
}
