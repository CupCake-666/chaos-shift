#include "BratRobotic.h"

#include "Cutie.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Math/UnrealMathUtility.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Drona.h"
#include "ChaosDirector.h"
#include "Components/AudioComponent.h"

ABratRobotic::ABratRobotic()
{
    PrimaryActorTick.bCanEverTick = true;

    SimRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SimRoot"));
    SetRootComponent(SimRoot);

    RA0 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh0"));
    RA0->SetupAttachment(SimRoot);
    RA1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh1"));
    RA1->SetupAttachment(RA0);
    RA2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh2"));
    RA2->SetupAttachment(RA1);
    RA3 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh3"));
    RA3->SetupAttachment(RA2);
    RA4 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh4"));
    RA4->SetupAttachment(RA3);
    RA5 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh5"));
    RA5->SetupAttachment(RA4);
    RA6 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh6"));
    RA6->SetupAttachment(RA5);
    RA7 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh7"));
    RA7->SetupAttachment(RA6);
    RA8 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh8"));
    RA8->SetupAttachment(RA6);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RA1);
    CameraBoom->TargetArmLength = 800.0f;
    CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));
    CameraBoom->SetRelativeRotation(FRotator(-25.0f, 0.0f, 0.0f));
    CameraBoom->bUsePawnControlRotation = false;

    RobotCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("RobotCamera"));
    RobotCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

    LedMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LedMesh1"));
    LedMesh1->SetupAttachment(RA5);
    LedMesh1->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
    LedMesh1->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));

    LedMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LedMesh2"));
    LedMesh2->SetupAttachment(RA5);
    LedMesh2->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
    LedMesh2->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));

    CurrentPartIndex = 1;
    RotationSpeed = 100.0f;

    GripperOpenAngle = 50.0f; 
    GripperClosedAngle = 0.0f;

    CurrentGripperValue = GripperOpenAngle;
    TargetGripperValue = GripperOpenAngle;
    bIsGripperMoving = false;
    bIsSnapping = false;

    SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
    SphereCollision->SetupAttachment(RA6);
    SphereCollision->InitSphereRadius(40.0f);
    
    PointLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
    PointLightComponent->SetupAttachment(RA8);

    SunetServo = CreateDefaultSubobject<UAudioComponent>(TEXT("SunetServo"));
    SunetServo->SetupAttachment(RA3);
    SunetServo->bAutoActivate = false;

    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;                      
    bUseControllerRotationRoll = false;                                              
}

void ABratRobotic::BeginPlay()                
{
    Super::BeginPlay();

    // Starea cursorului/input-ului e gestionata central de ASimulatorGameMode
    // (ModMeniu / ModJoc). Nu o mai setam aici, ca sa nu se contrazica.

    RoboticParts.Add(RA0); // Index 0
    RoboticParts.Add(RA1); // Index 1
    RoboticParts.Add(RA2); // Index 2
    RoboticParts.Add(RA3); // Index 3
    RoboticParts.Add(RA4); // Index 4
    RoboticParts.Add(RA5); // Index 5
    RoboticParts.Add(RA6); // Index 6
    RoboticParts.Add(RA7); // Index 7 (Cleste)
    RoboticParts.Add(RA8); // Index 8 (Cleste)
        
    if(GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Brat Robotic Initializat. Selecteaza cu W/S, Roteste cu A/D"));

    if (RoboticParts.IsValidIndex(CurrentPartIndex))
    {
        MaterialSalvat = RoboticParts[CurrentPartIndex]->GetMaterial(0);
        RoboticParts[CurrentPartIndex]->SetMaterial(0, MaterialLedGalben);
    }
    
    PointLightComponent->SetAttenuationRadius(150.0f); 
    PointLightComponent->SetIntensity(1500.0f); 
    PointLightComponent->SetLightColor(FColor::Red); 

    SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &ABratRobotic::OnPiesaBeginOverlap);
    SphereCollision->OnComponentEndOverlap.AddDynamic(this, &ABratRobotic::OnPiesaEndOverlap);

    if (LedMesh1 && MaterialLedRosu) LedMesh1->SetMaterial(0, MaterialLedRosu);
    if (LedMesh2 && MaterialLedRosu) LedMesh2->SetMaterial(0, MaterialLedRosu);

    // Observer: bratul se aboneaza singur la regizorul de haos.
    if (UChaosDirector* Director = UChaosDirector::Get(this))
    {
        Director->OnMutatorActivated.AddUniqueDynamic(this, &ABratRobotic::HandleMutatorActivated);
        Director->OnMutatorDeactivated.AddUniqueDynamic(this, &ABratRobotic::HandleMutatorDeactivated);
    }
}

void ABratRobotic::HandleMutatorActivated(EChaosMutator Mutator)
{
    if (Mutator == EChaosMutator::Scramble)
    {
        InputScrambleSign = -1.0f;
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("COMENZI INVERSATE!"));
    }
}

void ABratRobotic::HandleMutatorDeactivated(EChaosMutator Mutator)
{
    if (Mutator == EChaosMutator::Scramble)
    {
        InputScrambleSign = 1.0f;
    }
}

void ABratRobotic::ForceReleaseHeldObject()
{
    // Oprim si animatia de intoarcere in cutie, si timer-ul de camera:
    // altfel ne trage view target-ul inapoi la brat cat timp pilotam drona.
    GetWorldTimerManager().ClearTimer(TimerHandle_CameraReturn);
    bIsSnapping = false;
    bIsReturningToBox = false;

    if (IsValid(HeldObject))
    {
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(HeldObject->GetComponentByClass(UStaticMeshComponent::StaticClass()));
        if (MeshComp)
        {
            MeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
            MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            MeshComp->SetSimulatePhysics(true);
            MeshComp->WakeAllRigidBodies();
        }
    }

    HeldObject = nullptr;
    m_CurrentInteractObject = nullptr;
    TargetGripperValue = GripperOpenAngle;
}

void ABratRobotic::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateGripperAnimation(DeltaTime);
    UpdateObjectSnapping(DeltaTime);
    UpdateReturnToBoxAnimation(DeltaTime);
    ActualizeazaSunetServo();
}

void ABratRobotic::ActualizeazaSunetServo()
{
    if (!SunetServo || !SunetServo->Sound) return;

    // Marcajul de timp e mai sigur decat un flag: nu depinde de ordinea in care
    // ruleaza input-ul fata de Tick.
    const bool bRoteste = (GetWorld()->GetTimeSeconds() - UltimulMomentRotatie) < 0.12f;

    if (bRoteste && !SunetServo->IsPlaying())
    {
        SunetServo->FadeIn(0.05f);
    }
    else if (!bRoteste && SunetServo->IsPlaying())
    {
        SunetServo->FadeOut(0.15f, 0.0f);
    }
}

void ABratRobotic::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("SelectUp", IE_Pressed, this, &ABratRobotic::SelectNextPart);
    PlayerInputComponent->BindAction("SelectDown", IE_Pressed, this, &ABratRobotic::SelectPrevPart);
    PlayerInputComponent->BindAxis("RotateRight", this, &ABratRobotic::RotateRight);
    PlayerInputComponent->BindAxis("RotateUp", this, &ABratRobotic::RotateUp);
    PlayerInputComponent->BindAxis("RotateRoll", this, &ABratRobotic::RotateRoll);
    PlayerInputComponent->BindAction("Grab", IE_Pressed, this, &ABratRobotic::ToggleGrab);
    PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
    PlayerInputComponent->BindAction("SwitchVehicle", IE_Pressed, this, &ABratRobotic::SwitchToDrone);
}

void ABratRobotic::SelectNextPart()
{
    if (RoboticParts.IsValidIndex(CurrentPartIndex))
    {
        RoboticParts[CurrentPartIndex]->SetMaterial(0, MaterialSalvat);
    }
    
    CurrentPartIndex++;
    if (CurrentPartIndex >= 7 || CurrentPartIndex >= RoboticParts.Num()) CurrentPartIndex = 1;
    
    if (RoboticParts.IsValidIndex(CurrentPartIndex))
    {
        MaterialSalvat = RoboticParts[CurrentPartIndex]->GetMaterial(0);
        RoboticParts[CurrentPartIndex]->SetMaterial(0, MaterialLedGalben);
        
        if(GEngine) 
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
                FString::Printf(TEXT("Controlam: %s"), *RoboticParts[CurrentPartIndex]->GetName()));
    }
}

void ABratRobotic::SelectPrevPart()
{
    if (RoboticParts.IsValidIndex(CurrentPartIndex))
    {
        RoboticParts[CurrentPartIndex]->SetMaterial(0, MaterialSalvat);
    }
    
    CurrentPartIndex--;
    if (CurrentPartIndex < 1)
    {
        CurrentPartIndex = 6;
    }
    
    if (RoboticParts.IsValidIndex(CurrentPartIndex))
    {
        MaterialSalvat = RoboticParts[CurrentPartIndex]->GetMaterial(0);
        RoboticParts[CurrentPartIndex]->SetMaterial(0, MaterialLedGalben);
        
        if(GEngine) 
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Controlam: %s"), *RoboticParts[CurrentPartIndex]->GetName()));
    }
}

void ABratRobotic::RotateRight(float AxisValue)
{
    AxisValue *= InputScrambleSign; // Mutator A: Scramble
    if (AxisValue == 0.0f || RoboticParts.Num() == 0) return;
    UltimulMomentRotatie = GetWorld()->GetTimeSeconds();
    float RotateAmount = AxisValue * RotationSpeed * GetWorld()->GetDeltaSeconds();
    
    if (CurrentPartIndex == 1) 
    {
       FRotator NewRotation = FRotator::ZeroRotator;
       NewRotation.Yaw = RotateAmount;
       RoboticParts[CurrentPartIndex]->AddLocalRotation(NewRotation);
    }
}

void ABratRobotic::RotateUp(float AxisValue)
{
    AxisValue *= InputScrambleSign; // Mutator A: Scramble
    if (AxisValue == 0.0f || RoboticParts.Num() == 0) return;
    UltimulMomentRotatie = GetWorld()->GetTimeSeconds();
    float RotateAmount = AxisValue * RotationSpeed * GetWorld()->GetDeltaSeconds();
    
    if (CurrentPartIndex == 4) 
    {
        FRotator NewRotation = FRotator::ZeroRotator;
        NewRotation.Pitch = RotateAmount;
        RoboticParts[CurrentPartIndex]->AddLocalRotation(NewRotation);
    }
}

void ABratRobotic::RotateRoll(float AxisValue)
{
    AxisValue *= InputScrambleSign; // Mutator A: Scramble
    if (AxisValue == 0.0f || RoboticParts.Num() == 0) return;
    UltimulMomentRotatie = GetWorld()->GetTimeSeconds();
    float Delta = AxisValue * RotationSpeed * GetWorld()->GetDeltaSeconds();
    
    if (CurrentPartIndex == 2 || CurrentPartIndex == 3 || CurrentPartIndex == 5 || CurrentPartIndex == 6)
    {
        UStaticMeshComponent* CurrentMesh = RoboticParts[CurrentPartIndex];
        FRotator CurrentRot = CurrentMesh->GetRelativeRotation();
        float NewRoll = CurrentRot.Roll + Delta;
        
        if (CurrentPartIndex == 2) NewRoll = FMath::Clamp(NewRoll, Limit_RA2_Min, Limit_RA2_Max);
        if (CurrentPartIndex == 3) NewRoll = FMath::Clamp(NewRoll, Limit_RA3_Min, Limit_RA3_Max);
        if (CurrentPartIndex == 5) NewRoll = FMath::Clamp(NewRoll, Limit_RA5_Min, Limit_RA5_Max);
        if (CurrentPartIndex == 6) NewRoll = FMath::Clamp(NewRoll, Limit_RA6_Min, Limit_RA6_Max);
        
        CurrentMesh->SetRelativeRotation(FRotator(CurrentRot.Pitch, CurrentRot.Yaw, NewRoll));
    }
}

void ABratRobotic::OnPiesaBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!IsValid(OtherActor)) return;

    if (OtherActor->ActorHasTag("PiesaBanda"))
    {
       m_CurrentInteractObject = OtherActor;
       PointLightComponent->SetLightColor(FColor::Green);
       if (LedMesh1 && MaterialLedVerde) LedMesh1->SetMaterial(0, MaterialLedVerde);
       if (LedMesh2 && MaterialLedVerde) LedMesh2->SetMaterial(0, MaterialLedVerde);
    }
    else if (OtherActor->ActorHasTag("CutieBanda"))
    {
        m_HoveredBox = Cast<ACutie>(OtherActor);
    }
}

void ABratRobotic::OnPiesaEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!IsValid(OtherActor)) return;

    if (OtherActor->ActorHasTag("PiesaBanda"))
    {
       if (m_CurrentInteractObject == OtherActor) m_CurrentInteractObject = nullptr;
       PointLightComponent->SetLightColor(FColor::Red);
       if (LedMesh1 && MaterialLedRosu) LedMesh1->SetMaterial(0, MaterialLedRosu);
       if (LedMesh2 && MaterialLedRosu) LedMesh2->SetMaterial(0, MaterialLedRosu);
    }
    else if (OtherActor->ActorHasTag("CutieBanda"))
    {
        if (m_HoveredBox == OtherActor) m_HoveredBox = nullptr;
    }
}

void ABratRobotic::ToggleGrab()
{
    if (bIsReturningToBox) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (HeldObject)
    {
        TargetGripperValue = GripperOpenAngle; 
        bIsSnapping = false; 
        if (m_HoveredBox)
        {
            bIsReturningToBox = true; 
            if (PC) PC->SetViewTargetWithBlend(m_HoveredBox, 0.5f);
            UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(HeldObject->GetComponentByClass(UStaticMeshComponent::StaticClass()));
            if (MeshComp)
            {
                MeshComp->AttachToComponent(m_HoveredBox->DropArea, FAttachmentTransformRules::KeepWorldTransform);
            }
            if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Punem piesa inapoi in cutie..."));
        }
        else
        {
            if (PC) PC->SetViewTargetWithBlend(this, 0.5f);
            UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(HeldObject->GetComponentByClass(UStaticMeshComponent::StaticClass()));
            if (MeshComp)
            {
                MeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
                MeshComp->SetSimulatePhysics(true);
                MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
                MeshComp->WakeAllRigidBodies();
            }
            HeldObject = nullptr;
            if (SunetEliberare) UGameplayStatics::PlaySoundAtLocation(this, SunetEliberare, GetActorLocation());
            if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Am eliberat piesa!"));
        }
        return;
    }
    if (m_CurrentInteractObject)
    {
        TargetGripperValue = GripperClosedAngle; 
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(m_CurrentInteractObject->GetComponentByClass(UStaticMeshComponent::StaticClass()));
        if (MeshComp)
        {
            MeshComp->SetSimulatePhysics(false);
            MeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
            MeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
            MeshComp->AttachToComponent(RA7, FAttachmentTransformRules::KeepWorldTransform);
            bIsSnapping = true;
            if (PC)
            {
                PC->SetViewTargetWithBlend(m_CurrentInteractObject, 0.5f);
            }
        }
        HeldObject = m_CurrentInteractObject;
        if (SunetPrindere) UGameplayStatics::PlaySoundAtLocation(this, SunetPrindere, GetActorLocation());
        if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Am prins piesa!"));
    }
    else 
    {
        if (FMath::IsNearlyEqual(TargetGripperValue, GripperOpenAngle, 1.0f))
            TargetGripperValue = GripperClosedAngle;
        else
            TargetGripperValue = GripperOpenAngle;
    }
}

void ABratRobotic::UpdateGripperAnimation(float DeltaTime)
{
    if (!FMath::IsNearlyEqual(CurrentGripperValue, TargetGripperValue, 0.1f))
    {
        CurrentGripperValue = FMath::FInterpTo(CurrentGripperValue, TargetGripperValue, DeltaTime, 10.0f);
        FRotator Rot7 = RA7->GetRelativeRotation();
        RA7->SetRelativeRotation(FRotator(Rot7.Pitch, Rot7.Yaw, CurrentGripperValue));
        FRotator Rot8 = RA8->GetRelativeRotation();
        float Angle8 = bInvertGripperDirection ? -CurrentGripperValue : CurrentGripperValue;
        RA8->SetRelativeRotation(FRotator(Rot8.Pitch, Rot8.Yaw, Angle8));
    }
}

void ABratRobotic::UpdateObjectSnapping(float DeltaTime)
{
    if (bIsSnapping)
    {
        if (!IsValid(HeldObject))
        {
            HeldObject = nullptr;
            m_CurrentInteractObject = nullptr;
            bIsSnapping = false;
            return;
        }
        
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(HeldObject->GetComponentByClass(UStaticMeshComponent::StaticClass()));
        if (MeshComp)
        {
            FVector CurrentLoc = MeshComp->GetRelativeLocation();
            FVector TargetLoc = FVector(0.0f, 50.0f, -5.0f); 
            FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, 5.0f);
            MeshComp->SetRelativeLocation(NewLoc);
            FRotator CurrentRot = MeshComp->GetRelativeRotation();
            FRotator TargetRot = FRotator::ZeroRotator;
            FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 5.0f);
            MeshComp->SetRelativeRotation(NewRot);
            if (NewLoc.Equals(TargetLoc, 0.1f) && NewRot.Equals(TargetRot, 1.0f))
            {
                bIsSnapping = false;
                MeshComp->SetRelativeLocation(TargetLoc);
                MeshComp->SetRelativeRotation(TargetRot);
            }
        }
    }
}

void ABratRobotic::UpdateReturnToBoxAnimation(float DeltaTime)
{
    if (bIsReturningToBox)
    {
        if (!IsValid(HeldObject))
        {
            bIsReturningToBox = false;
            HeldObject = nullptr;
            ResetCameraToRobot();
            return;
        }
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(HeldObject->GetComponentByClass(UStaticMeshComponent::StaticClass()));
        if (!MeshComp)
        {
            bIsReturningToBox = false;
            ResetCameraToRobot();
            return;
        }
        FVector CurrentLoc = MeshComp->GetRelativeLocation();
        FRotator CurrentRot = MeshComp->GetRelativeRotation();
        FVector TargetLoc = FVector::ZeroVector;
        FRotator TargetRot = FRotator::ZeroRotator; 
        FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, 15.0f);
        FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 15.0f);
        MeshComp->SetRelativeLocationAndRotation(NewLoc, NewRot);
        if (CurrentLoc.Size() < 2.0f)
        {
            bIsReturningToBox = false; 
            MeshComp->SetRelativeLocationAndRotation(TargetLoc, TargetRot); 
            HeldObject = nullptr;
            if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Piesa s-a reintors perfect la locul ei!"));
            GetWorldTimerManager().SetTimer(TimerHandle_CameraReturn, this, &ABratRobotic::ResetCameraToRobot, 0.5f, false);
        }
    }
}

void ABratRobotic::ResetCameraToRobot()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC && PC->GetPawn() == this) 
    {
        PC->SetViewTargetWithBlend(this, 0.5f);
    }
}

void ABratRobotic::SwitchToDrone()
{
    // In timpul posedarii fortate jucatorul nu poate anula mutatorul.
    if (bControlSwitchLocked) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADrona::StaticClass());
        ADrona* Drona = Cast<ADrona>(FoundActor);
        
        if (Drona)
        {
            PC->Possess(Drona);
            if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Sistem de control transferat catre: DRONA"));
        }
    }
}

