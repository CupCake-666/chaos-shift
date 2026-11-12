#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Piesa.generated.h"

UCLASS()
class SIMULATOR_API APiesa : public AActor
{
	GENERATED_BODY()
public: 
	APiesa();
protected:
	virtual void BeginPlay() override;
public: 
	virtual void Tick(float DeltaTime) override;
private:
	void DetermineQuality();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float MasaPiesei = 5.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mesh")
	UStaticMeshComponent* PiesaMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Control")
	UMaterialInterface* MaterialBun;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Control")
	UMaterialInterface* MaterialDefect;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quality Control")
	bool bIsDefective;
	UPROPERTY(EditAnywhere, Category = "Quality Control")
	float DefectChance = 0.3f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* PiesaSpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* PiesaCamera;
};