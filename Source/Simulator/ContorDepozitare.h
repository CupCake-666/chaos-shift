#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ContorDepozitare.generated.h"

UCLASS()
class SIMULATOR_API AContorDepozitare : public AActor
{
	GENERATED_BODY()
public:	
	AContorDepozitare();
protected:
	virtual void BeginPlay() override;
public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente")
	UStaticMeshComponent* ContainerMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente")
	UBoxComponent* TriggerBox;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setari Sortare")
	bool bEsteContainerPentruDefecte;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scor")
	int32 ContorSortateCorect;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scor")
	int32 ContorGreseli;

	UPROPERTY(EditAnywhere, Category = "Sunet")
	class USoundBase* SunetCorect;

	UPROPERTY(EditAnywhere, Category = "Sunet")
	class USoundBase* SunetGresit;
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
						UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
						bool bFromSweep, const FHitResult& SweepResult);
};