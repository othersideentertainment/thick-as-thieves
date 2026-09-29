// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

// tat
#include "Math/TATXoshiroRandomStream.h"

// ue
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(TATXoshiroRandomStreamTest_MakeSeed_IntInt, "TAT.XoshiroRandomStream.MakeSeed.IntInt", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATXoshiroRandomStreamTest_MakeSeed_IntInt::RunTest(const FString& parameters)
{
   auto checkValue = [this](int32 a, int32 b, int64 expected) {
      TStringBuilder<64> builder;
      builder.Appendf(TEXT("MakeSeed(%x, %x)"), a, b);
      TestEqual(*builder, FTATXoshiroRandomStream::MakeSeed(a, b), expected);
   };
    
   checkValue(0x00000000, 0x00000000, 0x0000000000000000);
   checkValue(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
   checkValue(0x12345678, 0xFFFFFFFF, 0x12345678FFFFFFFF);
   checkValue(0x12345678, 0x87654321, 0x1234567887654321);
   return true;
}
#endif
