#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SimulatorGameMode.generated.h"

UCLASS()
class SIMULATOR_API ASimulatorGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ASimulatorGameMode();
protected:
	virtual void BeginPlay() override;
public:
	/**
	 * Mesajele galbene/verzi de debug de pe ecran. False pentru build-ul de jam,
	 * true daca ai nevoie de ele cand vanezi un bug.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bAfiseazaMesajeDebug = false;

	// ---- Muzica ---------------------------------------------------------
	/** Melodia de fundal. Bifeaza Looping pe asset, altfel canta o singura data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sunet")
	class USoundBase* MuzicaFundal;

	/** 0.15-0.30 = fundal discret, sub efecte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sunet")
	float VolumMuzica = 0.22f;

	/** Tic-tacul din ultimele secunde. Un singur one-shot, NU pune Looping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sunet")
	class USoundBase* SunetTicTac;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sunet")
	float VolumTicTac = 1.0f;

	/** Sub cate secunde ramase incepe tic-tacul. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sunet")
	int32 SecundeTicTac = 10;

	/** Sub cate secunde tic-tacul devine mai ascutit (tensiune). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sunet")
	int32 SecundePitchInalt = 4;

	UFUNCTION(BlueprintCallable, Category = "Sunet")
	void PornesteMuzica();

	UFUNCTION(BlueprintCallable, Category = "Sunet")
	void OpresteMuzica();

	UPROPERTY(Transient)
	class UAudioComponent* ComponentaMuzica = nullptr;

	/** Ultima secunda intreaga la care s-a auzit tic-tacul, ca sa nu se repete pe frame. */
	int32 UltimaSecundaTicTac = -1;
	UPROPERTY(BlueprintReadWrite, Category = "Stare Joc")
	bool bJocInceput;
	UPROPERTY(BlueprintReadOnly, Category = "Scor")
	int32 ScorCurent;
	UPROPERTY(BlueprintReadOnly, Category = "Scor")
	int32 GreseliCurente;
	UFUNCTION(BlueprintCallable, Category = "Control Joc")
	void IncepeJoc();
	UFUNCTION(BlueprintCallable, Category = "Control Joc")
	void OpresteJoc();

	/** Cursor vizibil, input UI. Pentru meniuri (Start, Pauza, Final). */
	UFUNCTION(BlueprintCallable, Category = "Control Joc")
	void ModMeniu();

	/**
	 * Cursor ascuns, mouse-ul captat imediat pentru camera - fara sa fie
	 * nevoie de un click. Pentru joc.
	 */
	UFUNCTION(BlueprintCallable, Category = "Control Joc")
	void ModJoc();
	void AdaugaPunct(int32 Puncte = 100);
	/** Numara greseala SI scade puncte, ca sortarea gresita sa doara. */
	void AdaugaGreseala(int32 Penalizare = 100);
	UFUNCTION(BlueprintImplementableEvent, Category = "Control Joc")
	void ArataEcranFinal();

	// ---- Countdown (delegat catre UChaosDirector) -----------------------
	/** Secundele ramase din runda. Pentru binding direct in UMG. */
	UFUNCTION(BlueprintPure, Category = "Scor")
	float GetTimpRamas() const;

	/** Legat la UChaosDirector::OnCountdownFinished. */
	UFUNCTION()
	void HandleCountdownFinished();

	/** Legat la UChaosDirector::OnCountdownUpdated. Reda tic-tacul o data pe secunda. */
	UFUNCTION()
	void HandleCountdownUpdated(float TimeRemaining);
};