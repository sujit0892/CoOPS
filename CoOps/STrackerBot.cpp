// Fill out your copyright notice in the Description page of Project Settings.

#include "STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "AI/Navigation//NavigationPath.h"
#include "GameFramework/Character.h"
#include "SHealthComponent.h"
#include "Components/SphereComponent.h"
#include "SCharacter.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundCue.h"


// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	RootComponent = MeshComp;
	MeshComp->SetSimulatePhysics(true);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("Health Component"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collison"));
	SphereComponent->SetSphereRadius(200);
	SphereComponent->SetupAttachment(MeshComp);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	
	bVelocityChange = false;
	
	ForceToMovement = 1000.0f;
	RequiredDistancetToTarget = 100.0f;

	BaseDamage = 20.0f;
	ExplosionRadius = 100.0f;

	isExploded = false;
	
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority)
	{
		NextPathPoint = GetNextPathPoint();
		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearbyBots, 1.0f, true);

	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this,0);
	UNavigationPath* NavPath = UNavigationSystem::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);

	if (NavPath->PathPoints.Num() > 1)
		return NavPath->PathPoints[1];

	return GetActorLocation();


}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatIns == nullptr)
	{
		MatIns=MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
		UE_LOG(LogTemp, Log, TEXT("Health %s of %s %s"), *FString::SanitizeFloat(Health), *GetName(),MatIns);
	}
	if (MatIns)
	{
		MatIns->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
		
	}

	if (Health <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(Timer_selfDamage);
		SelfDestruct();
	}
}

void ASTrackerBot::SelfDestruct()
{

	if (isExploded)
		return;
	isExploded = true;
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),ExplosionEffect,GetActorLocation());

	UGameplayStatics::PlaySoundAtLocation(this, ExplosionSoundEffect, GetActorLocation());
	MeshComp->SetVisibility(false, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Role == ROLE_Authority)
	{
		TArray<AActor*> IgnoredActors;
		float ActualDamage = BaseDamage + (BaseDamage * PowerLevel);

		// Apply Damage!
		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);

		SetLifeSpan(2.0f);
	}
	

}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20.0f, GetInstigatorController(), this, nullptr);

}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Role == ROLE_Authority && !isExploded)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequiredDistancetToTarget)
		{
			NextPathPoint = GetNextPathPoint();
		}
		else
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();
			ForceDirection *= ForceToMovement;

			MeshComp->AddForce(ForceDirection, NAME_None, bVelocityChange);
		}

		DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 0.0f, 1.0f);
	}
	
}

// Called to bind functionality to input
void ASTrackerBot::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (!bStartSelfDestruct && !isExploded)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn)
		{
			if(Role == ROLE_Authority)
			     GetWorldTimerManager().SetTimer(Timer_selfDamage, this, &ASTrackerBot::DamageSelf, 0.5f, true, 0.0f);

			bStartSelfDestruct = true;

			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}
}

void ASTrackerBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}

void ASTrackerBot::OnCheckNearbyBots()
{

	const float Radius = 600;

	
	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	FCollisionObjectQueryParams QueryParams;
	
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	
	

	int32 NrOfBots = 0;

	for (FOverlapResult Result : Overlaps)
	{
	
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());

		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;

	// Clamp between min=0 and max=4
	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);

	UE_LOG(LogTemp, Log, TEXT("powerlevel Changed: %s"), *FString::SanitizeFloat(PowerLevel));
	// Update the material color
	if (MatIns == nullptr)
	{
		MatIns = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}
	if (MatIns)
	{
		float Alpha = PowerLevel / (float)MaxPowerLevel;


		MatIns->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}

	
		// Draw on the bot location
		DrawDebugString(GetWorld(), FVector(0, 0, 0), FString::FromInt(PowerLevel), this, FColor::White, 1.0f, true);
	
}
