#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChaosDirector.h"
#include "Spawner.generated.h"

UCLASS()
class SIMULATOR_API ASpawner : public AActor
{
	GENERATED_BODY()
public:
	ASpawner();
protected:
	virtual void BeginPlay() override;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<class ACutie> CutieClassToSpawn;

	/** Intervalul de baza. Nu se suprascrie niciodata: mutatorii doar il inmultesc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnDelay;
private:
	void SpawnCutie();
	FTimerHandle SpawnTimerHandle;

	/** Reporneste timer-ul cu SpawnDelay * Multiplier. bImediat = prima piesa acum. */
	void RestartSpawnTimer(float Multiplier, bool bImediat = false);

	UFUNCTION()
	void HandleCountdownStarted();

	UFUNCTION()
	void HandleMutatorActivated(EChaosMutator Mutator);
	UFUNCTION()
	void HandleMutatorDeactivated(EChaosMutator Mutator);
};
