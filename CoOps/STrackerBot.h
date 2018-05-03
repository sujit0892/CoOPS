// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class UStaticMeshComponent; 
class SHealthComponent;
class UParticleSystem;
class USphereComponent;
class USoundCue;

UCLASS()
class COOPS_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "Mesh")
		UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Health")
		USHealthComponent* HealthComp;

	FVector GetNextPathPoint();

	FVector NextPathPoint;

	float DistanceToTarget;
	
	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float ForceToMovement;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	bool bVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float RequiredDistancetToTarget;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
		float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
		float BaseDamage;

		UMaterialInstanceDynamic* MatIns;

		bool isExploded;

	UFUNCTION()
		void HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta,
			const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void SelfDestruct();

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
		UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
		USoundCue* ExplosionSoundEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
		USoundCue* SelfDestructSound;

	UPROPERTY(VisibleDefaultsOnly, Category = "TrackerBot")
		USphereComponent* SphereComponent;

	FTimerHandle Timer_selfDamage;

	void DamageSelf();

	bool bStartSelfDestruct;
	void OnCheckNearbyBots();

	// the power boost of the bot, affects damaged caused to enemies and color of the bot (range: 1 to 4)
	int32 PowerLevel;

	FTimerHandle TimerHandle_RefreshPath;

	void RefreshPath();
	


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	
	
};
