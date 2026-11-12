#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ChaosDirector.generated.h"

class ADrona;
class ABratRobotic;
class ACutie;
class APiesa;

/** O piesa care leviteaza deasupra cutiei ei, cu pozitia initiala salvata. */
struct FPiesaLevitanta
{
	TWeakObjectPtr<APiesa> Piesa;
	TWeakObjectPtr<ACutie> Cutie;
	FTransform TransformRelativ;
	/** Decalaj de faza, ca sa nu pluteasca toate sincron. */
	float Faza = 0.0f;
};

/** Mutatorii care pot fi declansati la fiecare 10 secunde din countdown. */
UENUM(BlueprintType)
enum class EChaosMutator : uint8
{
	None		UMETA(DisplayName = "Fara Mutator"),
	Scramble	UMETA(DisplayName = "A - Comenzi Inversate"),
	AntiGravity	UMETA(DisplayName = "B - Anti-Gravitatie + Drona"),
	Overdrive	UMETA(DisplayName = "C - Overdrive")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMutatorActivated, EChaosMutator, Mutator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMutatorDeactivated, EChaosMutator, Mutator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownUpdated, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownFinished);

/**
 * Regizorul de haos. Un WorldSubsystem: se creeaza automat cu lumea, moare cu ea,
 * nu trebuie plasat in nivel si e accesibil din orice clasa (si din Blueprint)
 * fara sa mai facem Cast la GameMode peste tot.
 *
 * Observer Pattern: sistemele existente (Spawner, BratRobotic, UI) se aboneaza la
 * delegatii de mai jos in BeginPlay. Regizorul nu stie nimic despre ele.
 */
// BlueprintType e OBLIGATORIU: fara el, UK2Node_GetSubsystem::GetMenuActions
// sare clasa (IsAllowableBlueprintVariableType) si nodul "Get Chaos Director"
// nu apare niciodata in Blueprint.
UCLASS(BlueprintType)
class SIMULATOR_API UChaosDirector : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- USubsystem / FTickableGameObject -------------------------------
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UChaosDirector, STATGROUP_Tickables);
	}

	// ---- Acces rapid din Blueprint / UMG --------------------------------
	/** Rezerva: daca nodul generat de motor lipseste, cauta "Get Chaos Director (Static)". */
	UFUNCTION(BlueprintPure, Category = "Chaos", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Chaos Director (Static)"))
	static UChaosDirector* Get(const UObject* WorldContextObject);

	// ---- Control -------------------------------------------------------
	/** Apelat din ASimulatorGameMode::IncepeJoc(). */
	UFUNCTION(BlueprintCallable, Category = "Chaos")
	void StartCountdown();

	/** Apelat din ASimulatorGameMode::OpresteJoc(). Revine la starea normala. */
	UFUNCTION(BlueprintCallable, Category = "Chaos")
	void StopCountdown();

	/** Pentru debug: forteaza un mutator anume imediat. */
	UFUNCTION(BlueprintCallable, Category = "Chaos|Debug")
	void ForceMutator(EChaosMutator Mutator);

	// ---- Interogari (UI) -----------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Chaos")
	float GetTimeRemaining() const { return TimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "Chaos")
	bool IsCountdownRunning() const { return bCountdownRunning; }

	UFUNCTION(BlueprintPure, Category = "Chaos")
	EChaosMutator GetActiveMutator() const { return ActiveMutator; }

	/**
	 * Textul pentru banner. Intoarce "NORMAL" cand nu ruleaza niciun mutator,
	 * deci se poate lega direct pe proprietatea Text a unui TextBlock
	 * (property binding), fara evenimente si fara noduri Select.
	 */
	UFUNCTION(BlueprintPure, Category = "Chaos")
	FText GetNumeMutatorCurent() const;

	/** True doar cat timp ruleaza un mutator. Util pentru culoarea banner-ului. */
	UFUNCTION(BlueprintPure, Category = "Chaos")
	bool EsteMutatorActiv() const { return ActiveMutator != EChaosMutator::None; }

	UFUNCTION(BlueprintPure, Category = "Chaos")
	float GetMutatorTimeRemaining() const { return MutatorTimeRemaining; }

	/** Secunde ramase pana la urmatorul mutator (pentru bara de tensiune din UI). */
	UFUNCTION(BlueprintPure, Category = "Chaos")
	float GetTimeUntilNextMutator() const;

	// ---- Evenimente (Observer) -----------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "Chaos|Events")
	FOnMutatorActivated OnMutatorActivated;

	UPROPERTY(BlueprintAssignable, Category = "Chaos|Events")
	FOnMutatorDeactivated OnMutatorDeactivated;

	UPROPERTY(BlueprintAssignable, Category = "Chaos|Events")
	FOnCountdownUpdated OnCountdownUpdated;

	/** Se trimite cand incepe runda. Spawner-ele o folosesc ca sa scoata prima piesa imediat. */
	UPROPERTY(BlueprintAssignable, Category = "Chaos|Events")
	FOnCountdownStarted OnCountdownStarted;

	UPROPERTY(BlueprintAssignable, Category = "Chaos|Events")
	FOnCountdownFinished OnCountdownFinished;

	// ---- Reglaje --------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float CountdownDuration = 180.0f;

	/** 20s intre mutatori, care tin 6s => ~70% din runda e joc normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float MutatorInterval = 20.0f;

	/** Secunde de joc curat la inceputul rundei, inainte de primul mutator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float IntarziereInitiala = 20.0f;

	/**
	 * ATENTIE: cu MutatorDuration == MutatorInterval (20 == 20) haosul e CONTINUU -
	 * un mutator nou porneste exact cand se termina precedentul, deci nu mai exista
	 * timp de joc normal. Pentru alternanta 20s normal / 20s haos, pune Interval = 40.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float MutatorDuration = 20.0f;

	/** Gravitatia Z folosita in timpul mutatorului Anti-Gravitatie (pozitiv = in sus). */
	/**
	 * Sansa ca un ciclu sa fie NORMAL (fara mutator). 0.0 = haos continuu,
	 * 1.0 = niciodata haos. La 0.25, in medie 1 din 4 ferestre de 20s e liniste,
	 * si nu stii dinainte care - ceea ce mentine tensiunea.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float SansaModNormal = 0.25f;

	/**
	 * Cat de sus fata de pozitia din cutie plutesc piesele.
	 * Peste ~150 ajung in structurile de deasupra benzii si devin
	 * imposibil de prins cu bratul robotic.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float InaltimeLevitatie = 110.0f;

	/** Cat de repede urca pana la inaltimea de levitatie. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float VitezaRidicare = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float OverdriveTimeDilation = 1.5f;

	/** Multiplicator aplicat peste SpawnDelay-ul de baza al Spawner-elor in Overdrive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	float OverdriveSpawnMultiplier = 0.5f;

	/** Muta drona langa brat cand o posedam fortat, ca jucatorul sa nu se piarda. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	bool bTeleportDroneToArm = true;

	/** Unde apare drona fata de brat. Z prea mare o arunca in tavan. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos|Config")
	FVector OffsetTeleportDrona = FVector(0.0f, 0.0f, 220.0f);

private:
	void ActivateMutator(EChaosMutator Mutator);
	void DeactivateCurrentMutator();
	EChaosMutator PickNextMutator() const;

	// Implementarile concrete pentru starea GLOBALA (gravitatie, time dilation, posedare).
	// Starea detinuta de actori (inversarea input-ului pe brat, SpawnDelay-ul Spawner-ului)
	// NU se face aici: acei actori se aboneaza singuri la delegatii de mai sus.
	// bEnable == false => revert exact.
	void ApplyAntiGravity(bool bEnable);
	void ApplyOverdrive(bool bEnable);
	void ApplyForcedPossession(bool bEnable);

	/** Retine piesele din cutii si porneste levitatia. */
	void PornesteLevitatia();
	/** Le aseaza inapoi exact unde erau, in cutia lor. */
	void OpresteLevitatia();
	/** Ridica si balanseaza piesele cat timp mutatorul e activ. */
	void ActualizeazaLevitatia(float DeltaTime);

	ABratRobotic* FindArm() const;
	ADrona* FindDrone() const;

	UPROPERTY()
	EChaosMutator ActiveMutator = EChaosMutator::None;

	EChaosMutator LastMutator = EChaosMutator::None;

	float TimeRemaining = 0.0f;
	float MutatorTimeRemaining = 0.0f;
	float TimeSinceLastMutator = 0.0f;
	bool bCountdownRunning = false;

	/** Pawn-ul detinut inainte de posedarea fortata, ca sa il putem returna. */
	TWeakObjectPtr<APawn> PawnBeforePossession;

	/**
	 * True cand mutatorul s-a terminat dar drona inca duce o piesa: amanam
	 * revenirea la brat pana cand jucatorul apuca sa livreze piesa.
	 */
	bool bRevenireDronaInAsteptare = false;

	/** Piesele care leviteaza acum deasupra cutiilor lor. */
	TArray<FPiesaLevitanta> PieseLevitante;

	/** Cronometru pentru balansul piesei in aer. */
	float TimpLevitatie = 0.0f;
};
