#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Drona.generated.h"

UCLASS()
class SIMULATOR_API ADrona : public APawn
{
	GENERATED_BODY()
public:
	ADrona();
protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void SwitchToArm();

	/** Cat timp e true, tasta SwitchVehicle e ignorata (mutatorul Anti-Gravitatie). */
	UPROPERTY(BlueprintReadOnly, Category = "Chaos")
	bool bControlSwitchLocked = false;

	/** E: prinde piesa din apropiere, sau o elibereaza pe cea din "gheara". */
	void ApucaSauElibereaza();

	/** Da drumul piesei acolo unde e drona. Public: o apeleaza si ChaosDirector. */
	UFUNCTION(BlueprintCallable, Category = "Colectare")
	void ElibereazaPiesa();

	/** True cat timp drona cara o piesa. */
	UFUNCTION(BlueprintPure, Category = "Colectare")
	bool TinePiesa() const { return IsValid(PiesaTinuta); }
private:
	void ApucaPiesa();
	/** Trage piesa lin catre pozitia de transport, ca la clestele bratului. */
	void ActualizeazaAtragereaPiesei(float DeltaTime);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* DronaMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* DronaCamera;

	/** Zona in care drona "prinde" piesele care plutesc. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componente", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* SferaColectare;

	// ---- Sunet -----------------------------------------------------------
	/** Bucla de rotoare. Pune sunetul direct pe componenta, in Details -> Sound. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunet", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* SunetRotor;

	UPROPERTY(EditAnywhere, Category = "Sunet")
	class USoundBase* SunetPrindere;

	UPROPERTY(EditAnywhere, Category = "Sunet")
	class USoundBase* SunetEliberare;

	/** Volumul rotorului cand drona sta pe loc, respectiv la viteza maxima. */
	UPROPERTY(EditAnywhere, Category = "Sunet")
	float VolumRotorMin = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Sunet")
	float VolumRotorMax = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Sunet")
	float PitchRotorMin = 0.85f;
	UPROPERTY(EditAnywhere, Category = "Sunet")
	float PitchRotorMax = 1.30f;

	UFUNCTION()
	void OnColectareOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnColectareEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Piesa aflata in raza, candidata pentru apucare cu E. */
	UPROPERTY()
	AActor* PiesaInApropiere = nullptr;

	/** True cat timp piesa inca e trasa spre pozitia de transport. */
	bool bAtragePiesa = false;

	/** Raza in care poti apuca o piesa cu E. */
	UPROPERTY(EditAnywhere, Category = "Colectare")
	float RazaColectare = 90.0f;

	/** Cat de repede e trasa piesa langa drona. */
	UPROPERTY(EditAnywhere, Category = "Colectare")
	float VitezaAtragere = 8.0f;

	/** Piesa prinsa in prezent. Se elibereaza cu Enter. */
	UPROPERTY(BlueprintReadOnly, Category = "Colectare", meta = (AllowPrivateAccess = "true"))
	AActor* PiesaTinuta = nullptr;

	/** Unde sta piesa fata de drona cat timp e prinsa - lipita dedesubt. */
	UPROPERTY(EditAnywhere, Category = "Colectare")
	FVector OffsetPiesaTinuta = FVector(0.0f, 0.0f, -15.0f);

	/**
	 * Raza sferei folosite pentru coliziunea manuala.
	 * Mesh-ul skeletal nu are Physics Asset, deci nu ofera geometrie de coliziune -
	 * de aia raspunsurile pe canale nu erau suficiente si drona trecea prin tot.
	 */
	UPROPERTY(EditAnywhere, Category = "Fizica Zbor")
	float RazaColiziune = 70.0f;

	/** Spatiul pastrat fata de suprafete, ca sa nu ramana lipita si blocata. */
	UPROPERTY(EditAnywhere, Category = "Fizica Zbor")
	float MarjaColiziune = 3.0f;
	FVector InputVector;
	FVector CurrentVelocity;
	UPROPERTY(EditAnywhere, Category = "Fizica Zbor")
	float VitezaMaxima = 600.0f;
	UPROPERTY(EditAnywhere, Category = "Fizica Zbor")
	float RataAcceleratie = 4.0f;
	/** Mare = se opreste prompt cand lasi tastele. Mic = pluteste mult in inertie. */
	UPROPERTY(EditAnywhere, Category = "Fizica Zbor")
	float RataDeceleratie = 6.0f;
	void MoveForward(float Val);
	void MoveRight(float Val);
	void MoveUp(float Val);
};
