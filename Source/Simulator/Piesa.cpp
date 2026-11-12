#include "Piesa.h"
 #include "Math/UnrealMathUtility.h"
 #include "GameFramework/SpringArmComponent.h"
 #include "Camera/CameraComponent.h"
 #include "Kismet/GameplayStatics.h"
 
 APiesa::APiesa()
 {
     PrimaryActorTick.bCanEverTick = true;
     
     // PiesaMesh este din nou Seful (Root Component)
     PiesaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PiesaMesh"));
     SetRootComponent(PiesaMesh);
 
     // Fizica ramane activata implicit pentru cand o lasa bratul din mana
     PiesaMesh->SetSimulatePhysics(true);
     PiesaMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
 
     PiesaSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("PiesaSpringArm"));
     PiesaSpringArm->SetupAttachment(PiesaMesh);
     PiesaSpringArm->TargetArmLength = 250.0f;
     PiesaSpringArm->bUsePawnControlRotation = false;
     PiesaSpringArm->SetAbsolute(false, true, false);
     PiesaSpringArm->bEnableCameraLag = true;
     PiesaSpringArm->CameraLagSpeed = 10.0f;
     PiesaSpringArm->bEnableCameraRotationLag = true;
     PiesaSpringArm->CameraRotationLagSpeed = 15.0f;
 
     PiesaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PiesaCamera"));
     PiesaCamera->SetupAttachment(PiesaSpringArm, USpringArmComponent::SocketName);
     PiesaCamera->bUsePawnControlRotation = false;
 
     Tags.Add("PiesaBanda");
     bIsDefective = false;
 }
 
 void APiesa::BeginPlay()
 {
     Super::BeginPlay();
     if (PiesaMesh)
     {
        PiesaMesh->SetMassOverrideInKg(NAME_None, MasaPiesei, true);
     }
     DetermineQuality();
 }
 
 void APiesa::Tick(float DeltaTime)
 {
     Super::Tick(DeltaTime);
     APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
     if (PC)
     {
        if (PC->GetViewTarget() == this)
        {
           FRotator ControlRot = PC->GetControlRotation();
           PiesaSpringArm->SetWorldRotation(ControlRot);
        }
     }
 }
 
 void APiesa::DetermineQuality()
 {
     float RandomValue = FMath::RandRange(0.0f, 1.0f);
     if (RandomValue <= DefectChance)
     {
        bIsDefective = true;
        Tags.Add("Defect");
        if (PiesaMesh && MaterialDefect)
        {
           PiesaMesh->SetMaterial(0, MaterialDefect);
        }
     }
     else
     {
        bIsDefective = false;
        Tags.Add("Bun");
        if (PiesaMesh && MaterialBun)
        {
           PiesaMesh->SetMaterial(0, MaterialBun);
        }
     }
 }