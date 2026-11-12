#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "ChaosDirector.h"
#include "BratRobotic.generated.h"

class ACutie;

UCLASS()
class SIMULATOR_API ABratRobotic : public APawn
{
	GENERATED_BODY()

public:
	ABratRobotic();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* SphereCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPointLightComponent* PointLightComponent;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LedMesh1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LedMesh2;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* RobotCamera;

	// ---- Sunet -----------------------------------------------------------
	/** Bucla de servomotor. Pune sunetul pe componenta, in Details -> Sound. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunet")
	class UAudioComponent* SunetServo;

	UPROPERTY(EditAnywhere, Category = "Sunet")
	class USoundBase* SunetPrindere;

	UPROPERTY(EditAnywhere, Category = "Sunet")
	class USoundBase* SunetEliberare;
	UPROPERTY(EditAnywhere, Category = "LED Settings")
	UMaterialInterface* MaterialLedRosu;
	UPROPERTY(EditAnywhere, Category = "LED Settings")
	UMaterialInterface* MaterialLedVerde;
	UPROPERTY(EditAnywhere, Category = "LED Settings")
	UMaterialInterface* MaterialLedGalben;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA2_Min = -25.0f;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA2_Max = 25.0f;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA3_Min = -75.0f;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA3_Max = 20.0f;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA5_Min = -90.0f;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA5_Max = 90.0f;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA6_Min = -90.0f;
	UPROPERTY(EditAnywhere, Category = "Limites")
	float Limit_RA6_Max = 90.0f;
	void SwitchToDrone();

	// ---- Integrare Chaos Director ------------------------------------
	/** -1.0f cat timp mutatorul Scramble e activ, altfel 1.0f. */
	UPROPERTY(BlueprintReadOnly, Category = "Chaos")
	float InputScrambleSign = 1.0f;

	/** Cat timp e true, tasta SwitchVehicle e ignorata (posedare fortata). */
	UPROPERTY(BlueprintReadOnly, Category = "Chaos")
	bool bControlSwitchLocked = false;

	/** Lasa imediat piesa din cleste si opreste animatiile in curs. */
	void ForceReleaseHeldObject();
private:
	UFUNCTION()
	void HandleMutatorActivated(EChaosMutator Mutator);
	UFUNCTION()
	void HandleMutatorDeactivated(EChaosMutator Mutator);

	UPROPERTY(EditAnywhere)
	USceneComponent* SimRoot;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA0;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA1;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA2;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA3;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA4;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA5;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA6;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA7;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* RA8;
	UPROPERTY()
	UMaterialInterface* MaterialSalvat;
	UPROPERTY(EditAnywhere, Category = "Robot Limits")
	float GripperOpenAngle = 45.0f;
	UPROPERTY(EditAnywhere, Category = "Robot Limits")
	bool bInvertGripperDirection = false;
	UPROPERTY(EditAnywhere, Category = "Robot Limits")
	float GripperClosedAngle = 0.0f;
	float CurrentGripperValue;
	float TargetGripperValue;
	bool bIsGripperMoving;
	bool bIsSnapping;
	void UpdateObjectSnapping(float DeltaTime);
private:
	TArray<UStaticMeshComponent*> RoboticParts;
	int32 CurrentPartIndex;
	UPROPERTY(EditAnywhere)
	float RotationSpeed;
	void SelectNextPart(); 
	void SelectPrevPart();
	void RotateRight(float AxisValue);
	void RotateUp(float AxisValue);
	void RotateRoll(float AxisValue);
	UFUNCTION()
	void OnPiesaBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnPiesaEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex);
	UPROPERTY()
	AActor* m_CurrentInteractObject;
	UPROPERTY()
	AActor* HeldObject;
	void ToggleGrab();
	void UpdateGripperAnimation(float DeltaTime);
	UPROPERTY()
	class ACutie* m_HoveredBox;
	bool bIsReturningToBox;
	void UpdateReturnToBoxAnimation(float DeltaTime);
	FTimerHandle TimerHandle_CameraReturn;
	void ResetCameraToRobot();

	/** Ultima secunda in care s-a rotit o articulatie. Porneste/opreste servo-ul. */
	float UltimulMomentRotatie = -100.0f;
	void ActualizeazaSunetServo();
};
