// Fill out your copyright notice in the Description page of Project Settings.

#include "VRTestGameModeBase.h"
#include "EngineUtils.h"
#include "VRCharacter.h"
#include "UObject/ConstructorHelpers.h"

AVRTestGameModeBase::AVRTestGameModeBase()
	: Super()
{
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/VRTest/Blueprints/VRCharacter"));
	//DefaultPawnClass = PlayerPawnClassFinder.Class;
}
