#include "Cutie.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Piesa.h"

// Sets default values
ACutie::ACutie()
{
    PrimaryActorTick.bCanEverTick = true;
    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);
    CutieMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CutieMesh"));
    CutieMesh->SetupAttachment(RootScene);
    CutieMesh->SetSimulatePhysics(false); 
    CutieMesh->SetCollisionProfileName(TEXT("BlockAll"));
    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(CutieMesh);
    BoxCollision->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f)); 
    BoxCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    DropArea = CreateDefaultSubobject<UBoxComponent>(TEXT("DropArea"));
    DropArea->SetupAttachment(CutieMesh);
    DropArea->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
    DropArea->SetBoxExtent(FVector(40.0f, 40.0f, 5.0f)); 
    DropArea->SetCollisionProfileName(TEXT("NoCollision")); 
    CutieCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CutieCamera"));
    CutieCamera->SetupAttachment(CutieMesh);
    CutieCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
    CutieCamera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    Tags.Add("CutieBanda");
}

void ACutie::BeginPlay()
{
    Super::BeginPlay();
    GenerarePiesa();
}

void ACutie::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACutie::GenerarePiesa()
{
    if (ClasaPiesaDeGenerat && GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        FVector SpawnLocation = CutieMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 20.0f);
        FRotator SpawnRotation = CutieMesh->GetComponentRotation();
        PiesaDinInterior = GetWorld()->SpawnActor<APiesa>(ClasaPiesaDeGenerat, SpawnLocation, SpawnRotation, SpawnParams);
        if (PiesaDinInterior)
        {
            if (PiesaDinInterior->PiesaMesh)
            {
                PiesaDinInterior->PiesaMesh->SetSimulatePhysics(false);
            }
            PiesaDinInterior->AttachToComponent(CutieMesh, FAttachmentTransformRules::KeepWorldTransform);
        }
    }
}