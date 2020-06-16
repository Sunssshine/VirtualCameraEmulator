// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "DistortionEmulatorHud.generated.h"


UCLASS(config = Game)
class ADistortionEmulatorHud : public AHUD
{
	GENERATED_BODY()

public:
	ADistortionEmulatorHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
