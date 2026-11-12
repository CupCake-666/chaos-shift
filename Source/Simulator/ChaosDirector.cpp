#include "ChaosDirector.h"

#include "BratRobotic.h"
#include "Drona.h"
#include "Piesa.h"
#include "Cutie.h"
#include "Banda.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

bool UChaosDirector::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Nu vrem regizorul in lumile de editor / preview, doar in joc si in PIE.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UChaosDirector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Ca HUD-ul sa arate durata completa in meniu, nu 0, inainte de START.
	TimeRemaining = CountdownDuration;
}

void UChaosDirector::Deinitialize()
{
	// Plasa de siguranta: daca lumea moare cu un mutator activ (Stop PIE in mijlocul
	// haosului) nu lasam gravitatia sau time dilation-ul stricate.
	if (ActiveMutator != EChaosMutator::None)
	{
		DeactivateCurrentMutator();
	}
	Super::Deinitialize();
}

UChaosDirector* UChaosDirector::Get(const UObject* WorldContextObject)
{
	if (const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		return World->GetSubsystem<UChaosDirector>();
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// Countdown
// ---------------------------------------------------------------------------

void UChaosDirector::StartCountdown()
{
	TimeRemaining = CountdownDuration;
	MutatorTimeRemaining = 0.0f;
	LastMutator = EChaosMutator::None;
	bCountdownRunning = true;

	// Primul mutator apare dupa IntarziereInitiala secunde, nu dupa MutatorInterval:
	// pornim contorul "avansat" cu diferenta dintre cele doua.
	TimeSinceLastMutator = MutatorInterval - IntarziereInitiala;

	OnCountdownUpdated.Broadcast(TimeRemaining);
	OnCountdownStarted.Broadcast();
}

void UChaosDirector::StopCountdown()
{
	bCountdownRunning = false;
	TimeRemaining = 0.0f;
	TimeSinceLastMutator = 0.0f;

	if (ActiveMutator != EChaosMutator::None)
	{
		DeactivateCurrentMutator();
	}

	// La finalul rundei nu mai are cine sa livreze: fortam revenirea la brat.
	if (bRevenireDronaInAsteptare)
	{
		bRevenireDronaInAsteptare = false;
		if (ADrona* Drone = FindDrone())
		{
			Drone->ElibereazaPiesa();
		}
		ApplyForcedPossession(false);
	}
}

FText UChaosDirector::GetNumeMutatorCurent() const
{
	switch (ActiveMutator)
	{
	case EChaosMutator::Scramble:
		return FText::FromString(TEXT("CONTROLS SCRAMBLED!"));
	case EChaosMutator::AntiGravity:
		return FText::FromString(TEXT("ANTI-GRAVITY - FLY THE DRONE!"));
	case EChaosMutator::Overdrive:
		return FText::FromString(TEXT("OVERDRIVE!"));
	default:
		return FText::FromString(TEXT("ALL SYSTEMS NORMAL"));
	}
}

float UChaosDirector::GetTimeUntilNextMutator() const
{
	return bCountdownRunning ? FMath::Max(0.0f, MutatorInterval - TimeSinceLastMutator) : 0.0f;
}

void UChaosDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Revenirea amanata la brat: se face imediat ce drona a livrat piesa.
	// Verificata inaintea countdown-ului, ca sa functioneze si dupa terminarea rundei.
	if (bRevenireDronaInAsteptare)
	{
		const ADrona* Drone = FindDrone();
		if (!Drone || !Drone->TinePiesa())
		{
			bRevenireDronaInAsteptare = false;
			ApplyForcedPossession(false);
		}
	}

	if (!bCountdownRunning)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// IMPORTANT: DeltaTime primit aici este DILATAT. Daca l-am folosi direct,
	// mutatorul Overdrive ar consuma cele 90 de secunde cu 1.5x viteza si
	// cronometrul afisat n-ar mai corespunde cu timpul real. Impartim la
	// dilatare ca sa obtinem secunde reale.
	const float Dilation = FMath::Max(World->GetWorldSettings()->GetEffectiveTimeDilation(), KINDA_SMALL_NUMBER);
	const float RealDelta = DeltaTime / Dilation;

	// --- animatia de levitatie, cat timp mutatorul e activ ---
	if (ActiveMutator == EChaosMutator::AntiGravity)
	{
		ActualizeazaLevitatia(RealDelta);
	}

	// --- durata mutatorului activ ---
	if (ActiveMutator != EChaosMutator::None)
	{
		MutatorTimeRemaining -= RealDelta;
		if (MutatorTimeRemaining <= 0.0f)
		{
			DeactivateCurrentMutator();
		}
	}

	// --- pulsul de 10 secunde ---
	TimeSinceLastMutator += RealDelta;
	if (TimeSinceLastMutator >= MutatorInterval)
	{
		TimeSinceLastMutator -= MutatorInterval;

		// Nu suprapunem mutatori: daca unul e inca activ, il inchidem intai.
		if (ActiveMutator != EChaosMutator::None)
		{
			DeactivateCurrentMutator();
		}
		ActivateMutator(PickNextMutator());
	}

	// --- countdown-ul global ---
	TimeRemaining -= RealDelta;
	OnCountdownUpdated.Broadcast(FMath::Max(TimeRemaining, 0.0f));

	if (TimeRemaining <= 0.0f)
	{
		TimeRemaining = 0.0f;
		bCountdownRunning = false;

		if (ActiveMutator != EChaosMutator::None)
		{
			DeactivateCurrentMutator();
		}

		OnCountdownFinished.Broadcast();
	}
}

// ---------------------------------------------------------------------------
// Mutatori
// ---------------------------------------------------------------------------

EChaosMutator UChaosDirector::PickNextMutator() const
{
	// Fereastra asta poate fi pur si simplu normala. ActivateMutator(None) iese
	// imediat, deci raman 20 de secunde de joc curat, fara banner.
	if (FMath::FRand() < SansaModNormal)
	{
		return EChaosMutator::None;
	}

	TArray<EChaosMutator, TInlineAllocator<3>> Candidates;
	Candidates.Add(EChaosMutator::Scramble);
	Candidates.Add(EChaosMutator::Overdrive);

	// Anti-gravitatia are sens doar daca exista o drona in nivel de posedat.
	if (FindDrone() != nullptr)
	{
		Candidates.Add(EChaosMutator::AntiGravity);
	}

	// Evitam sa repetam acelasi mutator de doua ori la rand.
	if (Candidates.Num() > 1)
	{
		Candidates.Remove(LastMutator);
	}

	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}

void UChaosDirector::ForceMutator(EChaosMutator Mutator)
{
	if (ActiveMutator != EChaosMutator::None)
	{
		DeactivateCurrentMutator();
	}
	ActivateMutator(Mutator);
}

void UChaosDirector::ActivateMutator(EChaosMutator Mutator)
{
	if (Mutator == EChaosMutator::None)
	{
		return;
	}

	ActiveMutator = Mutator;
	LastMutator = Mutator;
	MutatorTimeRemaining = MutatorDuration;

	switch (Mutator)
	{
	case EChaosMutator::Scramble:
		// Nimic global de facut: bratul se aboneaza singur si isi inverseaza axele.
		break;

	case EChaosMutator::AntiGravity:
		ApplyAntiGravity(true);
		ApplyForcedPossession(true);
		break;

	case EChaosMutator::Overdrive:
		ApplyOverdrive(true);
		// SpawnDelay-ul e treaba Spawner-ului: se ajusteaza singur pe delegat.
		break;

	default:
		break;
	}

	// Se trimite DUPA aplicarea starii globale, ca ascultatorii sa vada o lume coerenta.
	OnMutatorActivated.Broadcast(Mutator);
}

void UChaosDirector::DeactivateCurrentMutator()
{
	const EChaosMutator Finished = ActiveMutator;
	if (Finished == EChaosMutator::None)
	{
		return;
	}

	switch (Finished)
	{
	case EChaosMutator::AntiGravity:
		ApplyForcedPossession(false);
		ApplyAntiGravity(false);
		break;

	case EChaosMutator::Overdrive:
		ApplyOverdrive(false);
		break;

	default:
		break;
	}

	ActiveMutator = EChaosMutator::None;
	MutatorTimeRemaining = 0.0f;

	OnMutatorDeactivated.Broadcast(Finished);
}

// ---------------------------------------------------------------------------
// Mutator B: Anti-Gravitatie
// ---------------------------------------------------------------------------

void UChaosDirector::ApplyAntiGravity(bool bEnable)
{
	// NU mai atingem gravitatia globala. Cu ea inversata, orice piesa libera
	// pleca spre tavan si se pierdea definitiv din joc. Levitatia se face
	// cinematic, cu piesa ramasa atasata de cutia ei.
	if (bEnable)
	{
		PornesteLevitatia();
	}
	else
	{
		OpresteLevitatia();
	}
}

void UChaosDirector::PornesteLevitatia()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	PieseLevitante.Reset();
	TimpLevitatie = 0.0f;

	for (TActorIterator<ACutie> It(World); It; ++It)
	{
		ACutie* Cutie = *It;
		if (!IsValid(Cutie) || !IsValid(Cutie->PiesaDinInterior))
		{
			continue;
		}

		APiesa* Piesa = Cutie->PiesaDinInterior;

		// Daca bratul o tine deja in cleste, nu i-o smulgem.
		if (Piesa->GetAttachParentActor() != Cutie)
		{
			continue;
		}

		UStaticMeshComponent* Mesh = Piesa->PiesaMesh;
		if (!Mesh)
		{
			continue;
		}

		FPiesaLevitanta Info;
		Info.Piesa = Piesa;
		Info.Cutie = Cutie;
		Info.TransformRelativ = Mesh->GetRelativeTransform();
		Info.Faza = FMath::FRandRange(0.0f, 2.0f * PI);
		PieseLevitante.Add(Info);
	}
}

void UChaosDirector::ActualizeazaLevitatia(float DeltaTime)
{
	TimpLevitatie += DeltaTime;

	for (const FPiesaLevitanta& Info : PieseLevitante)
	{
		APiesa* Piesa = Info.Piesa.Get();
		ACutie* Cutie = Info.Cutie.Get();

		if (!IsValid(Piesa) || !IsValid(Cutie))
		{
			continue;
		}

		// A apucat-o bratul sau drona intre timp - nu ne mai atingem de ea.
		if (Piesa->GetAttachParentActor() != Cutie)
		{
			continue;
		}

		UStaticMeshComponent* Mesh = Piesa->PiesaMesh;
		if (!Mesh)
		{
			continue;
		}

		// Piesa ramane COPIL al cutiei, deci se deplaseaza automat cu banda.
		// Noi modificam doar inaltimea relativa.
		const float Balans = FMath::Sin(TimpLevitatie * 2.0f + Info.Faza) * 10.0f;
		const FVector Tinta = Info.TransformRelativ.GetLocation()
			+ FVector(0.0f, 0.0f, InaltimeLevitatie + Balans);

		Mesh->SetRelativeLocation(
			FMath::VInterpTo(Mesh->GetRelativeLocation(), Tinta, DeltaTime, VitezaRidicare));

		// Rotatie lenta, ca sa se vada ca pluteste liber.
		Mesh->AddLocalRotation(FRotator(0.0f, 35.0f * DeltaTime, 0.0f));
	}
}

void UChaosDirector::OpresteLevitatia()
{
	for (const FPiesaLevitanta& Info : PieseLevitante)
	{
		APiesa* Piesa = Info.Piesa.Get();
		ACutie* Cutie = Info.Cutie.Get();

		if (!IsValid(Piesa) || !IsValid(Cutie))
		{
			continue;
		}

		// Prinsa de brat sau de drona => ramane acolo unde e.
		if (Piesa->GetAttachParentActor() != Cutie)
		{
			continue;
		}

		if (UStaticMeshComponent* Mesh = Piesa->PiesaMesh)
		{
			Mesh->SetRelativeTransform(Info.TransformRelativ);
		}
	}

	PieseLevitante.Reset();
}

// ---------------------------------------------------------------------------
// Mutator B: Posedare fortata (Brat -> Drona si inapoi)
// ---------------------------------------------------------------------------

void UChaosDirector::ApplyForcedPossession(bool bEnable)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
	}

	if (bEnable)
	{
		ADrona* Drone = FindDrone();
		if (!Drone)
		{
			return;
		}

		APawn* CurrentPawn = PC->GetPawn();
		if (CurrentPawn == Drone)
		{
			// Jucatorul e deja pe drona; nu avem ce prelua, dar blocam schimbarea.
			Drone->bControlSwitchLocked = true;
			PawnBeforePossession = nullptr;
			return;
		}

		PawnBeforePossession = CurrentPawn;

		// Bratul PASTREAZA piesa din cleste. E o dificultate in plus placuta:
		// te intorci la ea dupa mutator. ResetCameraToRobot() verifica singur
		// daca mai e pawn-ul controlat, deci nu ne fura camera cat pilotam drona.
		if (ABratRobotic* Arm = Cast<ABratRobotic>(CurrentPawn))
		{
			Arm->bControlSwitchLocked = true;
		}

		if (bTeleportDroneToArm && CurrentPawn)
		{
			const FVector DropIn = CurrentPawn->GetActorLocation() + OffsetTeleportDrona;
			Drone->SetActorLocation(DropIn, false, nullptr, ETeleportType::TeleportPhysics);
		}

		// Possess() face si UnPossess pe pawn-ul curent, dar il apelam explicit
		// ca sa fie clar in cod si ca sa rulam RestartLevel-safe.
		PC->UnPossess();
		PC->Possess(Drone);
		PC->SetViewTargetWithBlend(Drone, 0.4f);

		// Blocam tasta SwitchVehicle: altfel jucatorul anuleaza mutatorul instant.
		Drone->bControlSwitchLocked = true;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Magenta,
				TEXT("ANTI-GRAVITATIE! Preia drona si prinde piesele!"));
		}
	}
	else
	{
		ADrona* Drone = FindDrone();

		// Daca inca duce o piesa, NU il smulgem de pe drona: il lasam sa termine
		// livrarea. Revenirea se face automat cand da drumul piesei (vezi Tick).
		if (Drone && Drone->TinePiesa())
		{
			Drone->bControlSwitchLocked = false;
			bRevenireDronaInAsteptare = true;

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan,
					TEXT("Du piesa la container! Revii la brat dupa ce ii dai drumul."));
			}
			return;
		}

		if (Drone)
		{
			Drone->bControlSwitchLocked = false;
		}

		APawn* ReturnTo = PawnBeforePossession.Get();
		if (IsValid(ReturnTo))
		{
			PC->UnPossess();
			PC->Possess(ReturnTo);
			PC->SetViewTargetWithBlend(ReturnTo, 0.4f);
		}

		if (ABratRobotic* Arm = FindArm())
		{
			Arm->bControlSwitchLocked = false;
		}

		PawnBeforePossession = nullptr;
	}
}

// ---------------------------------------------------------------------------
// Mutator C: Overdrive
// ---------------------------------------------------------------------------

void UChaosDirector::ApplyOverdrive(bool bEnable)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Atentie: WorldSettings clampeaza intre MinGlobalTimeDilation si
	// MaxGlobalTimeDilation (implicit 0.0001 .. 20), deci 1.5 e in siguranta.
	UGameplayStatics::SetGlobalTimeDilation(World, bEnable ? OverdriveTimeDilation : 1.0f);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

ABratRobotic* UChaosDirector::FindArm() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ABratRobotic> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

ADrona* UChaosDirector::FindDrone() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ADrona> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
