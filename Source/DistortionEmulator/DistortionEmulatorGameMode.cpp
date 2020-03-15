// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "DistortionEmulatorGameMode.h"
#include "DistortionEmulatorPawn.h"
#include "DistortionEmulatorHud.h"

ADistortionEmulatorGameMode::ADistortionEmulatorGameMode()
{
	DefaultPawnClass = ADistortionEmulatorPawn::StaticClass();
	HUDClass = ADistortionEmulatorHud::StaticClass();
}
