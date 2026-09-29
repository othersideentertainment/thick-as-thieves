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

#ifdef TESTING_WWISE_PROJECT_DATABASE
#include <catch2/catch.hpp>
#include "AdapterTypes/WwiseWrapperTypes.h"
#else
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#endif
#include "Wwise/AdapterTypes/Unreal/WwiseProjectDatabaseTestingUtils.h"

SCENARIO("Wwise::WwiseProjectDatabase::AdapterTypes")
{
	SECTION("Guid")
	{
		//Guid String Constructor
		WwiseDBGuid Guid(WwiseDBString("{ABCDDCBA-1234-4321-1357-BA9876543210}"));
		CHECK(Guid.ToString() == WwiseDBString("{ABCDDCBA-1234-4321-1357-BA9876543210}"));

		//Guid String Constructor with bad format
		WwiseDBGuid BadGuid(WwiseDBString("I am GUID"));
		CHECK(!BadGuid.IsValid());

		//Guid String Constructor with good format but bad hexadecimal format
		BadGuid = WwiseDBGuid(WwiseDBString("{ABCDDCBA-1234-4321-1357-BH9876543210}"));
		CHECK(!BadGuid.IsValid());

		//Guid Constructed with Strings return the correct values
		int A, B, C, D;
		Guid.GetGuidValues(A, B, C, D);
		CHECK(A == -1412571974);
		CHECK(B == 305414945);
		CHECK(C == 324516504);
		CHECK(D == 1985229328);
		CHECK(Guid.IsValid());

		//Guid Constructor using int
		WwiseDBGuid NumberGuid(A, B, C, D);
		CHECK(NumberGuid.ToString() == WwiseDBString("{ABCDDCBA-1234-4321-1357-BA9876543210}"));

		//Checking Guids with at least one 0 but not all 4 is valid. 
		NumberGuid = WwiseDBGuid(0, 1, 2, 3);
		CHECK(NumberGuid.IsValid());

		//Checking Guids with only 0 is not valid.
		NumberGuid = WwiseDBGuid(0, 0, 0, 0);
		CHECK(!NumberGuid.IsValid());

		//Checking Guids with multiple 0 at the start
		Guid = WwiseDBGuid(WwiseDBString("{00C0DC0A-1234-4321-1357-BA9876543210}"));
		CHECK(Guid.ToString() == WwiseDBString("{00C0DC0A-1234-4321-1357-BA9876543210}"));
	}

	SECTION("ParallelFor")
	{
		WwiseDBArray<int> Values;
		WwiseDBArray<int> ValuesParallel;
		ValuesParallel.Reserve(1000);
		
		//Setting values Asynchronously in an array.
		for(int i = 0; i < 1000; i++)
		{
			Values.Add(i);
			ValuesParallel.Add(i);
		}
		WwiseDBAsync::WwiseParallelFor(
			Values,
			[&ValuesParallel, &Values](const auto& n)
		    {
				ValuesParallel[Values(n)] = Values(n);
			}
		);

		//Doing modifications Asynchronously
		WwiseDBAsync::WwiseParallelFor(
			Values,
			[&ValuesParallel, &Values](const auto& n)
			{
				Values[Values(n)] = Values(n) + 1;
			}
		);

		for(int i = 0; i < 1000; i++)
		{
			CHECK(ValuesParallel[i] == i);
			CHECK(Values[i] == i + 1);
		}

		WwiseDBMap<int, int> MapValues;
		WwiseDBMap<int, int> MapValuesParallel;
		//Setting values Asynchronously in a map.
		for(int i = 0; i < 1000; i++)
		{
			MapValues.Add(i, i);
			MapValuesParallel.Add(i, 0);
		}
		WwiseDBAsync::WwiseParallelFor(
			MapValues,
			[&MapValuesParallel, &MapValues](const auto& n)
			{
				WwiseDBPair<int, int> Pair(MapValues(n));
				MapValuesParallel.FindChecked(Pair.GetFirst()) = Pair.GetSecond();
			}
		);
		
		for(int i = 0; i < 1000; i++)
		{
			CHECK(*MapValuesParallel.Find(i) == i);
		}
	}

	SECTION("Map")
	{
		WwiseDBMap<int, int> Map;
		//Adding a Key to a Map
		Map.Add(1, 1);
		CHECK(Map[1] == 1);
		
		//Adding an already existing Key to a Map. This will not replace the value.
		Map.Add(1, 2);
		CHECK(Map[1] == 1);

		//Adding or replacing an already existing Key to a Map. This will replace the value.
		Map.AddOrReplace(1, 2);
		CHECK(Map[1] == 2);

		//Searching for a key that does not exist. Returns nullptr.
		CHECK(!Map.Find(2));

		//Finding a value from a key and editing it.
		Map.FindChecked(1) = 3;
		CHECK(Map[1] == 3);

		//Emptying the Map.
		Map.Empty();
		CHECK(Map.Size() == 0);

		//Searching for a key that does not exist. No Crash
		Map[1];

		Map.Add(1, 2);
		Map.Add(2, 2);
		int count = 0;
		//Iterator Test. It passes through the whole map.
		for(auto& Pair : Map)
		{
			count++;
		}
		CHECK(count == 2);
	}
	
	SECTION("MultiMap")
	{
		WwiseDBMultiMap<int, int> MultiMap;
		
		//Adding multiple values for a key.
		MultiMap.Add(1, 1);
		MultiMap.Add(1, 2);
		WwiseDBArray<int> OutValues;
		MultiMap.MultiFind(1, OutValues);
		CHECK(OutValues.Size() == 2);
		for(unsigned int i = 0; i < OutValues.Size(); i++)
		{
			CHECK(OutValues[i] == i + 1);
		}

		//Iterator Test. It passes through the whole multimap.
		int count = 0;
		for(auto& Pair : MultiMap)
		{
			count++;
		}
		CHECK(count == 2);

		//Searching for a Key that does not exist. Returns empty array.
		OutValues.Empty();
		MultiMap.MultiFind(2, OutValues);
		CHECK(OutValues.Size() == 0);

		//Emptying the MultiMap.
		MultiMap.Empty();
		CHECK(MultiMap.Size() == 0);

		//Searching with Pointers.
		MultiMap.Add(1, 1);
		WwiseDBArray<const int*> OutPointerValues;
		MultiMap.MultiFindPointer(1, OutPointerValues);
		CHECK(OutPointerValues.Size() == 1);

		OutPointerValues.Empty();
		MultiMap.Empty();
		MultiMap.MultiFindPointer(1, OutPointerValues);
		CHECK(OutValues.Size() == 0);
	}

	SECTION("Array")
	{
		WwiseDBArray<int> Array;
		//Adding Values to the Array
		Array.Add(1);
		CHECK(Array[0] == 1);
		Array.Add(2);
		CHECK(Array[1] == 2);
		CHECK(Array.Size() == 2);

		//Iterator Test. It passes through the whole array.
		int count = 0;
		for(auto& Pair : Array)
		{
			count++;
		}
		CHECK(count == 2);

		//GetData returns the array as pointer.
		CHECK(Array.GetData()[0] == 1);

		//GetData of an empty array returns nullptr
		Array.Empty();
		CHECK(!Array.GetData());
		
		//Emptying the Array.
		Array.Empty();
		CHECK(Array.Size() == 0);
	}

	SECTION("Set")
	{
		WwiseDBSet<int> Set1;
		WwiseDBSet<int> Set2;
		Set1.Add(1);
		Set1.Add(2);
		Set2.Add(1);

		//Iterator Test. It passes through the whole set.
		int count = 0;
		for(auto& Object : Set1)
		{
			count++;
		}
		CHECK(count == 2);
		
		CHECK(Set1.Size() == 2);

		//Difference returns the values that are exclusively in one of the two sets
		auto Diff = Set1.Difference(Set2);
		CHECK(Diff.Size() == 1);
		CHECK(Diff.GetFirst() == 2);

		//Convert Set as Array
		auto Array = Set1.AsArray();
		CHECK(Array.Size() == 2);
		CHECK(Array[0] == 1);
		CHECK(Array[1] == 2);

		//Emptying the Set.
		Set1.Empty();
		CHECK(Set1.Size() == 0);
	}

	SECTION("Pair")
	{
		WwiseDBPair<int, int> Pair(1, 2);
		WwiseDBPair<int, int> Pair2(2, 2);
		WwiseDBPair<int, int> Pair3(1, 3);
		WwiseDBPair<int, int> Pair4(1, 2);
		WwiseDBPair<int, int> Pair5(0, 4);


		//Pair Values
		CHECK(Pair.GetFirst() == 1);
		CHECK(Pair.GetSecond() == 2);

		//Operator == returns true only when both values of the pair are the same
		CHECK(Pair != Pair2);
		CHECK(Pair != Pair3);
		CHECK(Pair == Pair4);
		CHECK(Pair != Pair5);

		//Operator < returns true if the first value is smaller.
		//If the first values are equal, check the second value as well.
		CHECK(Pair < Pair2);
		CHECK(Pair < Pair3);
		CHECK(!(Pair < Pair4));
		CHECK(!(Pair < Pair5));
	}

	SECTION("String")
	{
		WwiseDBString String("Test");
		WwiseDBString Path("Path/To/../To/Test.exe");

		CHECK(!String.Equals(Path, EStringCompareType::CaseSensitive));
		CHECK(!String.Equals(Path, EStringCompareType::IgnoreCase));

		//CaseSensitive returns true if the strings are identical
		//IgnoreCase returns true if the characters are the same regardless of Cases.
		WwiseDBString String2("tESt");
		CHECK(!String.Equals(String2, EStringCompareType::CaseSensitive));
		CHECK(String.Equals(String2, EStringCompareType::IgnoreCase));

		//Number of characters in the String
		CHECK(String.Size() == 4);

		//The extension of the path. Returns an empty string if there is no extension.
		CHECK(Path.GetExtension() == WwiseDBString("exe"));
		CHECK(String.GetExtension() == WwiseDBString(""));

		//The path before the file. Returns an empty string if there is no path before the file.
		CHECK(Path.GetPath() == WwiseDBString("Path/To/../To"));
		CHECK(String.GetPath() == WwiseDBString(""));

		//The name of the file in the path (with extension). Returns the string if there is path or extension.
		CHECK(Path.GetFileName() == WwiseDBString("Test.exe"));
		CHECK(String.GetFileName() == WwiseDBString("Test"));

		//CollapseRelativeDirectories fuses ../ with the parent folder above.
		Path.CollapseRelativeDirectories();
		CHECK(Path == WwiseDBString("Path/To/Test.exe"));

		WwiseDBString Path2 = WwiseDBString("Path/../To/To/Test.exe");
		Path2.CollapseRelativeDirectories();
		CHECK(Path2 == WwiseDBString("/To/To/Test.exe"));

		//Collapsing with no ../
		String.CollapseRelativeDirectories();
		CHECK(String == WwiseDBString("Test"));

		//Collapsing multiple ../
		WwiseDBString Path3("Path/To/../../To/Test.exe");
		Path3.CollapseRelativeDirectories();
		CHECK(Path3 == WwiseDBString("/To/Test.exe"));

		//Collapsing two instances of ../
		WwiseDBString Path4("Path/../To/../To/Test.exe");
		Path4.CollapseRelativeDirectories();
		CHECK(Path4 == WwiseDBString("/To/Test.exe"));

		//Adding too much ../ to be removed. Remove the possibles ../ while keeping the others
		WwiseDBString Path5("Path/../../To/Test.exe");
		Path5.CollapseRelativeDirectories();
		CHECK(Path5 == WwiseDBString("/../To/Test.exe"));

		//operator / pairs two string with a '/'
		CHECK(String / WwiseDBString("Test2") == WwiseDBString("Test/Test2"));


		WwiseDBString RelativePath("C:/Path");
#ifdef WIN32
		CHECK(!RelativePath.IsRelative());
#else
		CHECK(RelativePath.IsRelative());
#endif

		WwiseDBString RelativePath2("/Path/To/Test");
#ifdef WIN32
#ifdef WWISE_DB_UNREAL_TYPES
		CHECK(!RelativePath2.IsRelative());
#else
		CHECK(RelativePath2.IsRelative());
#endif
#else
		CHECK(!RelativePath2.IsRelative());
#endif
		WwiseDBString RelativePath3("~/Path");
#ifdef WIN32
		CHECK(RelativePath3.IsRelative());
#else
		CHECK(!RelativePath3.IsRelative());
#endif

		WwiseDBString RelativePath4("\\\\Path/To/External");
#ifdef WIN32
		CHECK(!RelativePath4.IsRelative());
#else
		CHECK(RelativePath4.IsRelative());
#endif

		WwiseDBString RelativePath5("/");
#ifdef WIN32
		CHECK(!RelativePath5.IsRelative());
#else
		CHECK(!RelativePath5.IsRelative());
#endif

		WwiseDBString RelativePath6("Path/To/Test");
#ifdef WIN32
		CHECK(RelativePath6.IsRelative());
#else
		CHECK(RelativePath6.IsRelative());
#endif

		WwiseDBString RelativePath7("C:RelativePath");
#ifdef WIN32
#ifdef WWISE_DB_UNREAL_TYPES
		CHECK(!RelativePath7.IsRelative());
#else
		CHECK(RelativePath7.IsRelative());
#endif
#else
#ifdef WWISE_DB_UNREAL_TYPES
		CHECK(!RelativePath7.IsRelative());
#else
		CHECK(RelativePath7.IsRelative());
#endif
#endif
	}
}