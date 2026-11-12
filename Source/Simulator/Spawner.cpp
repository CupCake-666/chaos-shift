#include "Spawner.h"
#include "Cutie.h"
#include "SimulatorGameMode.h" // <--- 1. FOARTE IMPORTANT: Includem GameMode-ul nostru!
#include "ChaosDirector.h"
#include "TimerManager.h"
#include "Engine/World.h"

ASpawner::ASpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnDelay = 5.0f;
}

void ASpawner::BeginPlay()
{
	Super::BeginPlay();

	if (CutieClassToSpawn)
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ASpawner::SpawnCutie, SpawnDelay, true, 0.0f);
	}

	// Observer: spawner-ul isi ajusteaza singur ritmul cand se activeaza Overdrive.
	// Regizorul nu tine referinte catre spawnere, deci merge cu oricate.
	if (UChaosDirector* Director = UChaosDirector::Get(this))
	{
		Director->OnCountdownStarted.AddUniqueDynamic(this, &ASpawner::HandleCountdownStarted);
		Director->OnMutatorActivated.AddUniqueDynamic(this, &ASpawner::HandleMutatorActivated);
		Director->OnMutatorDeactivated.AddUniqueDynamic(this, &ASpawner::HandleMutatorDeactivated);
	}
}

void ASpawner::HandleCountdownStarted()
{
	// Fara asta, dupa START trebuia sa astepti pana la urmatorul tick al
	// timer-ului pornit in BeginPlay - adica pana la SpawnDelay secunde de gol.
	RestartSpawnTimer(1.0f, true);
}

void ASpawner::SpawnCutie()
{
	ASimulatorGameMode* GameMode = Cast<ASimulatorGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode != nullptr && GameMode->bJocInceput == true)
	{
		if (CutieClassToSpawn && GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<ACutie>(CutieClassToSpawn, GetActorLocation(), GetActorRotation(), SpawnParams);
		}
	}
}

void ASpawner::RestartSpawnTimer(float Multiplier, bool bImediat)
{
	if (!CutieClassToSpawn) return;

	const float NewDelay = FMath::Max(SpawnDelay * Multiplier, 0.25f);
	const float PrimaDeclansare = bImediat ? 0.0f : NewDelay;
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ASpawner::SpawnCutie, NewDelay, true, PrimaDeclansare);
}

void ASpawner::HandleMutatorActivated(EChaosMutator Mutator)
{
	if (Mutator != EChaosMutator::Overdrive) return;

	// ATENTIE: timer-ele sunt DEJA accelerate de Global Time Dilation.
	// La 1.5x dilatare + multiplicator 0.5 rezulta ~3x rata de spawn, nu 2x.
	// Regleaza OverdriveSpawnMultiplier daca e prea mult.
	if (const UChaosDirector* Director = UChaosDirector::Get(this))
	{
		RestartSpawnTimer(Director->OverdriveSpawnMultiplier);
	}
}

void ASpawner::HandleMutatorDeactivated(EChaosMutator Mutator)
{
	if (Mutator == EChaosMutator::Overdrive)
	{
		RestartSpawnTimer(1.0f);
	}
}
