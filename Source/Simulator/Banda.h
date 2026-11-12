#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Banda.generated.h"

UCLASS()
class SIMULATOR_API ABanda : public AActor
{
	GENERATED_BODY()
	
public:	
	ABanda();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente")
	UStaticMeshComponent* BandaMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente")
	UBoxComponent* SuprafataRulare;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setari Banda")
	float Viteza;

	/** Bucla de banda. Pune sunetul pe componenta, in Details -> Sound. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunet")
	class UAudioComponent* SunetBanda;
private:
	/** Banda se opreste cand jocul nu ruleaza. Cache-uit, nu cautat la fiecare frame. */
	UPROPERTY()
	class ASimulatorGameMode* GameMode = nullptr;
};