#include "SimulatorGameMode.h"
#include "ChaosDirector.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

ASimulatorGameMode::ASimulatorGameMode()
{
	ScorCurent = 0;
	GreseliCurente = 0;
	bJocInceput = false; // Cand pornim, asteptam ca operatorul sa apese START
}

void ASimulatorGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Stinge TOATE mesajele AddOnScreenDebugMessage din proiect dintr-un loc.
	// UEngine::AddOnScreenDebugMessage iese imediat cand flag-ul e false, deci
	// apelurile raman in cod pentru depanare, dar nu mai apar pe ecran.
	if (GEngine)
	{
		GEngine->bEnableOnScreenDebugMessages = bAfiseazaMesajeDebug;
	}

	// La pornire suntem in meniul Start: cursorul trebuie sa se vada.
	ModMeniu();
}

void ASimulatorGameMode::ModMeniu()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	PC->bShowMouseCursor = true;

	FInputModeGameAndUI Mod;
	Mod.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mod.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mod);
}

void ASimulatorGameMode::ModJoc()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	// GameOnly capteaza mouse-ul instant si ascunde cursorul - fara click.
	// Asta rezolva si "cursorul ramas pe ecran dupa Continue".
	PC->bShowMouseCursor = false;
	FInputModeGameOnly Mod;
	PC->SetInputMode(Mod);
}

void ASimulatorGameMode::IncepeJoc()
{
	bJocInceput = true;
	ScorCurent = 0;
	GreseliCurente = 0;

	// Pornim countdown-ul de 90s si pulsul de haos la fiecare 10s.
	UltimaSecundaTicTac = -1;

	if (UChaosDirector* Director = UChaosDirector::Get(this))
	{
		// AddUnique: fara asta, un restart ar lega evenimentul de doua ori
		// si runda s-ar termina de doua ori.
		Director->OnCountdownFinished.AddUniqueDynamic(this, &ASimulatorGameMode::HandleCountdownFinished);
		Director->OnCountdownUpdated.AddUniqueDynamic(this, &ASimulatorGameMode::HandleCountdownUpdated);
		Director->StartCountdown();
	}

	// Redam controlul, in caz ca runda precedenta l-a blocat.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* PawnCurent = PC->GetPawn())
		{
			PawnCurent->EnableInput(PC);
		}
	}

	// Ascunde cursorul si capteaza mouse-ul pentru camera, fara click.
	ModJoc();

	PornesteMuzica();

	if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("SIMULARE INCEPUTA! Mult succes!"));
}

void ASimulatorGameMode::PornesteMuzica()
{
	if (!MuzicaFundal)
	{
		return;
	}

	// bAutoDestroy = false: pastram referinta ca sa o putem opri la final.
	if (!ComponentaMuzica)
	{
		ComponentaMuzica = UGameplayStatics::SpawnSound2D(
			this, MuzicaFundal, VolumMuzica, 1.0f, 0.0f, nullptr, false, false);
		return;
	}

	// La o runda noua refolosim aceeasi componenta.
	ComponentaMuzica->SetVolumeMultiplier(VolumMuzica);
	if (!ComponentaMuzica->IsPlaying())
	{
		ComponentaMuzica->Play();
	}
}

void ASimulatorGameMode::OpresteMuzica()
{
	if (ComponentaMuzica)
	{
		ComponentaMuzica->FadeOut(1.5f, 0.0f);
	}
}

void ASimulatorGameMode::OpresteJoc()
{
	bJocInceput = false;

	// Opreste cronometrul si face revert la orice mutator ramas activ
	// (gravitatie, time dilation, posedare).
	if (UChaosDirector* Director = UChaosDirector::Get(this))
	{
		Director->StopCountdown();
	}

	OpresteMuzica();

	// Oprim efectiv jocul in C++. Nu ne bazam doar pe ArataEcranFinal(), care e
	// un BlueprintImplementableEvent si poate fi neimplementat.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* PawnCurent = PC->GetPawn())
		{
			PawnCurent->DisableInput(PC);
		}
	}

	// Ecranul final e un meniu: aratam cursorul.
	ModMeniu();

	if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow, FString::Printf(TEXT("TIMPUL A EXPIRAT! Scor final: %d | Greseli: %d"), ScorCurent, GreseliCurente));

	// Declansam evenimentul care va deschide widget-ul de final din Blueprint
	ArataEcranFinal();
}

void ASimulatorGameMode::HandleCountdownFinished()
{
	OpresteJoc();
}

void ASimulatorGameMode::HandleCountdownUpdated(float TimeRemaining)
{
	if (!SunetTicTac)
	{
		return;
	}

	// Ceil: la 9.4 secunde ramase, "secunda" e 10, la 8.9 e 9, etc.
	const int32 SecundaCurenta = FMath::CeilToInt(TimeRemaining);

	// O singura data pe secunda, doar in fereastra finala, si nu la zero.
	if (SecundaCurenta == UltimaSecundaTicTac
		|| SecundaCurenta > SecundeTicTac
		|| SecundaCurenta <= 0)
	{
		return;
	}

	UltimaSecundaTicTac = SecundaCurenta;

	// Ultimele secunde suna mai ascutit, ca sa creasca tensiunea.
	const float Pitch = (SecundaCurenta <= SecundePitchInalt) ? 1.4f : 1.0f;

	UGameplayStatics::PlaySound2D(this, SunetTicTac, VolumTicTac, Pitch);
}

float ASimulatorGameMode::GetTimpRamas() const
{
	const UChaosDirector* Director = UChaosDirector::Get(this);
	return Director ? Director->GetTimeRemaining() : 0.0f;
}

void ASimulatorGameMode::AdaugaPunct(int32 Puncte)
{
	if (bJocInceput)
	{
		ScorCurent += Puncte;
	}
}

void ASimulatorGameMode::AdaugaGreseala(int32 Penalizare)
{
	if (bJocInceput)
	{
		// GreseliCurente ramane un NUMAR de greseli (1, 2, 3...), nu puncte -
		// asta afiseaza HUD-ul tau. Penalizarea se aplica pe scor.
		GreseliCurente++;
		ScorCurent = FMath::Max(0, ScorCurent - Penalizare);
	}
}
