// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/AutomationTest.h"

namespace MoqAutomationTestFlags
{
	inline constexpr EAutomationTestFlags ApplicationContextMask =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::ServerContext |
		EAutomationTestFlags::CommandletContext |
		EAutomationTestFlags::ProgramContext;

	inline constexpr EAutomationTestFlags DefaultFlags = ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}
