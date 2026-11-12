#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Cutie.generated.h"

class APiesa;
class UBoxComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USceneComponent;

UCLASS()
class SIMULATOR_API ACutie : public AActor
{
	GENERATED_BODY()
public:	
	ACutie();
protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CutieMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* BoxCollision;
	UPROPERTY(EditAnywhere, Category = "Configuratie")
	TSubclassOf<APiesa> ClasaPiesaDeGenerat;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stare")
	APiesa* PiesaDinInterior;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DropLocation;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CutieCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente")
	class UBoxComponent* DropArea;
private:
	void GenerarePiesa();
};