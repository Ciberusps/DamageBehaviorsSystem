// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehaviorsSystemBlueprintLibrary.generated.h"


UCLASS()
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehaviorsSystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "UnrealHelperLibrary")
    static AActor* GetTopmostAttachedActor(AActor* Actor_In);
};
