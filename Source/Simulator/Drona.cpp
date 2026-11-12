// Fill out your copyright notice in the Description page of Project Settings.


#include "Drona.h"
#include "Kismet/GameplayStatics.h"
#include "BratRobotic.h"
#include "ChaosDirector.h"
#include "SimulatorGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"

// Sets default values
ADrona::ADrona()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DronaMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DronaMesh"));
	SetRootComponent(DronaMesh);
	
	// Fizica trebuie OPRITA: miscarea se face cu AddActorWorldOffset(..., bSweep=true),
	// iar pe un corp care simuleaza fizica sweep-ul e ignorat - de aia trecea prin pereti.
	DronaMesh->SetSimulatePhysics(false);
	DronaMesh->SetEnableGravity(false);

	// Blocam doar geometria fixa. Daca am bloca si PhysicsBody, drona ar impinge
	// piesele in loc sa le prinda.
	DronaMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DronaMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	DronaMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	DronaMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(DronaMesh);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	// Camera se retrage cand bratul intra intr-un perete, in loc sa ramana in geometrie.
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 15.0f;

	DronaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DronaCamera"));
	DronaCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	DronaCamera->bUsePawnControlRotation = false;

	SferaColectare = CreateDefaultSubobject<USphereComponent>(TEXT("SferaColectare"));
	SferaColectare->SetupAttachment(DronaMesh);
	SferaColectare->InitSphereRadius(RazaColectare);
	SferaColectare->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	SunetRotor = CreateDefaultSubobject<UAudioComponent>(TEXT("SunetRotor"));
	SunetRotor->SetupAttachment(DronaMesh);
	SunetRotor->bAutoActivate = false;
}

// Called when the game starts or when spawned
void ADrona::BeginPlay()
{
	Super::BeginPlay();

	if (SferaColectare)
	{
		SferaColectare->SetSphereRadius(RazaColectare);
		SferaColectare->OnComponentBeginOverlap.AddDynamic(this, &ADrona::OnColectareOverlap);
		SferaColectare->OnComponentEndOverlap.AddDynamic(this, &ADrona::OnColectareEndOverlap);
	}

	// Bucla de rotoare porneste doar daca i-ai atribuit un sunet in BP_Drona.
	if (SunetRotor && SunetRotor->Sound)
	{
		SunetRotor->SetVolumeMultiplier(VolumRotorMin);
		SunetRotor->SetPitchMultiplier(PitchRotorMin);
		SunetRotor->Play();
	}
}

void ADrona::OnColectareOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Overlap-ul doar MARCHEAZA candidatul. Apucarea se face cu tasta E,
	// ca sa alegi tu ce piesa iei, nu sa o "aspire" drona la atingere.
	if (!IsValid(OtherActor) || !OtherActor->ActorHasTag("PiesaBanda"))
	{
		return;
	}

	if (OtherActor == PiesaTinuta)
	{
		return;
	}

	PiesaInApropiere = OtherActor;
}

void ADrona::OnColectareEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (PiesaInApropiere == OtherActor)
	{
		PiesaInApropiere = nullptr;
	}
}

void ADrona::ApucaSauElibereaza()
{
	if (IsValid(PiesaTinuta))
	{
		ElibereazaPiesa();
	}
	else
	{
		ApucaPiesa();
	}
}

void ADrona::ApucaPiesa()
{
	if (!IsValid(PiesaInApropiere))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Silver,
				TEXT("Nicio piesa in raza. Apropie-te de ea."));
		}
		return;
	}

	// Nu furam piesa din clestele bratului robotic.
	if (const AActor* Parinte = PiesaInApropiere->GetAttachParentActor())
	{
		if (Parinte->IsA(ABratRobotic::StaticClass()))
		{
			return;
		}
	}

	UStaticMeshComponent* MeshPiesa = Cast<UStaticMeshComponent>(
		PiesaInApropiere->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (!MeshPiesa)
	{
		return;
	}

	// Acelasi tipar ca la bratul robotic: oprim fizica si atasam, apoi o tragem
	// lin la pozitie in Tick (nu o teleportam, ca sa para ca o cara).
	MeshPiesa->SetSimulatePhysics(false);
	MeshPiesa->SetPhysicsLinearVelocity(FVector::ZeroVector);
	MeshPiesa->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	MeshPiesa->AttachToComponent(DronaMesh, FAttachmentTransformRules::KeepWorldTransform);

	PiesaTinuta = PiesaInApropiere;
	PiesaInApropiere = nullptr;
	bAtragePiesa = true;

	if (SunetPrindere)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SunetPrindere, GetActorLocation());
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan,
			TEXT("Piesa prinsa! E ca sa-i dai drumul in container."));
	}
}

void ADrona::ActualizeazaAtragereaPiesei(float DeltaTime)
{
	// Rulam cat timp DUCEM piesa, nu doar in timpul atragerii initiale.
	// Daca ne opream dupa ce ajungea la pozitie, orice altceva care ii atinge
	// transformul (levitatia, banda) o facea sa se departeze de drona.
	if (!IsValid(PiesaTinuta))
	{
		bAtragePiesa = false;
		PiesaTinuta = nullptr;
		return;
	}

	UStaticMeshComponent* MeshPiesa = Cast<UStaticMeshComponent>(
		PiesaTinuta->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (!MeshPiesa)
	{
		bAtragePiesa = false;
		return;
	}

	if (bAtragePiesa)
	{
		// Faza de atragere: alunecare lina catre pozitia de transport.
		const FVector NouaLocatie = FMath::VInterpTo(
			MeshPiesa->GetRelativeLocation(), OffsetPiesaTinuta, DeltaTime, VitezaAtragere);
		const FRotator NouaRotatie = FMath::RInterpTo(
			MeshPiesa->GetRelativeRotation(), FRotator::ZeroRotator, DeltaTime, VitezaAtragere);

		MeshPiesa->SetRelativeLocation(NouaLocatie);
		MeshPiesa->SetRelativeRotation(NouaRotatie);

		if (NouaLocatie.Equals(OffsetPiesaTinuta, 1.0f) && NouaRotatie.Equals(FRotator::ZeroRotator, 1.0f))
		{
			bAtragePiesa = false;
		}
		return;
	}

	// Faza de transport: fixam pozitia in fiecare frame, ca sa fie lipita rigid.
	MeshPiesa->SetRelativeLocation(OffsetPiesaTinuta);
	MeshPiesa->SetRelativeRotation(FRotator::ZeroRotator);
}

void ADrona::ElibereazaPiesa()
{
	if (!IsValid(PiesaTinuta))
	{
		return;
	}

	UStaticMeshComponent* MeshPiesa = Cast<UStaticMeshComponent>(
		PiesaTinuta->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (MeshPiesa)
	{
		MeshPiesa->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		MeshPiesa->SetCollisionProfileName(TEXT("PhysicsActor"));
		MeshPiesa->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshPiesa->SetSimulatePhysics(true);
		MeshPiesa->WakeAllRigidBodies();
	}

	if (SunetEliberare)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SunetEliberare, GetActorLocation());
	}

	PiesaTinuta = nullptr;
	bAtragePiesa = false;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, TEXT("Piesa eliberata!"));
	}
}

// Called every frame
void ADrona::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ActualizeazaAtragereaPiesei(DeltaTime);

	FVector DirectieDorita = InputVector.GetSafeNormal();
	FVector TargetVelocity = DirectieDorita * (double)VitezaMaxima;
	float CurrentInterpSpeed = DirectieDorita.IsNearlyZero() ? RataDeceleratie : RataAcceleratie;
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime, CurrentInterpSpeed);

	// Rotoarele urca in turatie odata cu viteza.
	if (SunetRotor && SunetRotor->IsPlaying())
	{
		const float Fractie = FMath::Clamp(CurrentVelocity.Size() / FMath::Max(VitezaMaxima, 1.0f), 0.0f, 1.0f);
		SunetRotor->SetVolumeMultiplier(FMath::Lerp(VolumRotorMin, VolumRotorMax, Fractie));
		SunetRotor->SetPitchMultiplier(FMath::Lerp(PitchRotorMin, PitchRotorMax, Fractie));
	}

	// Coliziune manuala. Nu ne bazam pe componenta: DronaMesh e un SkeletalMesh
	// fara Physics Asset, deci nu are geometrie de coliziune si orice sweep pe el
	// trece prin tot, indiferent de profil sau de canale.
	FVector DeplasareRamasa = CurrentVelocity * (double)DeltaTime;

	FCollisionObjectQueryParams ObiecteDeTestat;
	ObiecteDeTestat.AddObjectTypesToQuery(ECC_WorldStatic);
	ObiecteDeTestat.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams Parametri;
	Parametri.AddIgnoredActor(this);
	if (IsValid(PiesaTinuta))
	{
		Parametri.AddIgnoredActor(PiesaTinuta);
	}

	const FCollisionShape Forma = FCollisionShape::MakeSphere(RazaColiziune);

	// Mai multe incercari: dupa ce alunecam pe un perete, restul deplasarii
	// poate lovi alt perete (colturi).
	for (int32 Incercare = 0; Incercare < 3 && !DeplasareRamasa.IsNearlyZero(); ++Incercare)
	{
		const FVector Start = GetActorLocation();
		const FVector End = Start + DeplasareRamasa;

		FHitResult Impact;
		const bool bLovit = GetWorld()->SweepSingleByObjectType(
			Impact, Start, End, FQuat::Identity, ObiecteDeTestat, Forma, Parametri);

		if (!bLovit)
		{
			SetActorLocation(End);
			break;
		}

		if (Impact.bStartPenetrating)
		{
			// Suntem deja infipti in geometrie: iesim pe normala si reincercam.
			SetActorLocation(Start + Impact.Normal * (Impact.PenetrationDepth + MarjaColiziune));
			DeplasareRamasa = FVector::VectorPlaneProject(DeplasareRamasa, Impact.Normal);
			CurrentVelocity = FVector::VectorPlaneProject(CurrentVelocity, Impact.Normal);
			continue;
		}

		// Ne oprim cu MarjaColiziune INAINTE de suprafata. Fara marja, sweep-ul
		// din frame-ul urmator porneste lipit de perete, raporteaza penetrare
		// si drona ramane blocata definitiv.
		SetActorLocation(Impact.Location + Impact.Normal * MarjaColiziune);

		// Ce a ramas din deplasare, proiectat pe suprafata => alunecare.
		DeplasareRamasa = FVector::VectorPlaneProject(DeplasareRamasa * (1.0f - Impact.Time), Impact.Normal);
		CurrentVelocity = FVector::VectorPlaneProject(CurrentVelocity, Impact.Normal);
	}
	if (Controller != nullptr)
	{
		FRotator ControlRot = Controller->GetControlRotation();
		FVector LocalVelocity = DronaMesh->GetComponentTransform().InverseTransformVectorNoScale(CurrentVelocity);
		float PitchTilt = FMath::Clamp(LocalVelocity.X * -0.015f, -35.0f, 35.0f);
		float RollTilt = FMath::Clamp(LocalVelocity.Y * 0.015f, -35.0f, 35.0f);
		FRotator TargetRotation = FRotator(PitchTilt, ControlRot.Yaw, RollTilt);
		FRotator CurrentRotation = DronaMesh->GetComponentRotation();
		FRotator SmoothRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f);
		DronaMesh->SetWorldRotation(SmoothRotation);
	}
	InputVector = FVector::ZeroVector;
}

// Called to bind functionality to input
void ADrona::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ADrona::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADrona::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ADrona::MoveUp);
	
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("SwitchVehicle", IE_Pressed, this, &ADrona::SwitchToArm);
	PlayerInputComponent->BindAction("ApucaPiesaDrona", IE_Pressed, this, &ADrona::ApucaSauElibereaza);
}

void ADrona::SwitchToArm()
{
	// In timpul mutatorului Anti-Gravitatie jucatorul nu poate scapa de drona.
	if (bControlSwitchLocked) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ABratRobotic::StaticClass());
		ABratRobotic* Brat = Cast<ABratRobotic>(FoundActor);
		if (Brat)
		{
			PC->Possess(Brat);
			if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Sistem de control transferat catre: BRAT ROBOTIC"));
		}
	}
}

void ADrona::MoveForward(float Val)
{
	if (Val != 0.0f && Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		InputVector += Direction * Val; 
	}
}

void ADrona::MoveRight(float Val)
{
	if (Val != 0.0f && Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		InputVector += Direction * Val; 
	}
}

void ADrona::MoveUp(float Val)
{
	if (Val != 0.0f)
	{
		InputVector += FVector::UpVector * Val;
	}
}
