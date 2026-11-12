#include "ContorDepozitare.h"
#include "Cutie.h"
#include "Piesa.h"
#include "SimulatorGameMode.h"
#include "ChaosDirector.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

namespace
{
	/**
	 * Penalizarea NU se dubleaza in haos. Haosul e exact momentul in care
	 * gresesti cel mai usor; dublarea ar transforma mutatorii din oportunitate
	 * in pedeapsa. Recompensa se dubleaza, riscul nu.
	 */
	constexpr int32 PenalizareGreseala = 50;

	/** Punctele pentru o sortare corecta, x2 daca un mutator e activ. */
	int32 CalculeazaPuncte(const UObject* WorldContext)
	{
		int32 Puncte = 100;
		if (const UChaosDirector* Director = UChaosDirector::Get(WorldContext))
		{
			if (Director->GetActiveMutator() != EChaosMutator::None)
			{
				Puncte *= 2;
			}
		}
		return Puncte;
	}
}

AContorDepozitare::AContorDepozitare()
{
	PrimaryActorTick.bCanEverTick = false;

	ContorSortateCorect = 0;
	ContorGreseli = 0;
	bEsteContainerPentruDefecte = false;

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	SetRootComponent(ContainerMesh);
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(ContainerMesh);
	TriggerBox->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AContorDepozitare::OnOverlapBegin);
}

void AContorDepozitare::BeginPlay()
{
	Super::BeginPlay();
}

void AContorDepozitare::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
       ASimulatorGameMode* GameMode = Cast<ASimulatorGameMode>(GetWorld()->GetAuthGameMode());
       if (OtherActor->ActorHasTag("PiesaBanda"))
       {
          // Daca piesa e inca atasata de cutie, cutia intra si ea in trigger si
          // o conteaza ramura "CutieBanda" de mai jos. Fara verificarea asta,
          // aceeasi piesa e punctata (sau penalizata) de DOUA ori.
          if (OtherActor->GetAttachParentActor() != nullptr)
          {
             return;
          }

          bool bPiesaEsteDefecta = OtherActor->ActorHasTag("Defect");
          if (bPiesaEsteDefecta == bEsteContainerPentruDefecte)
          {
             ContorSortateCorect++;
             const int32 Puncte = CalculeazaPuncte(this);
             if(GameMode) GameMode->AdaugaPunct(Puncte);
             if(SunetCorect) UGameplayStatics::PlaySoundAtLocation(this, SunetCorect, GetActorLocation());
             if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Sortare Corecta (Piesa Simpla)! +%d"), Puncte));
          }
          else
          {
             ContorGreseli++;
             const int32 Penalizare = PenalizareGreseala;
             if(GameMode) GameMode->AdaugaGreseala(Penalizare);
             if(SunetGresit) UGameplayStatics::PlaySoundAtLocation(this, SunetGresit, GetActorLocation());
             if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("GRESEALA! Piesa Simpla in container gresit! -%d"), Penalizare));
          }
          OtherActor->Destroy(); 
       }
       else if (OtherActor->ActorHasTag("CutieBanda"))
       {
          ACutie* Cutie = Cast<ACutie>(OtherActor);
          if (Cutie != nullptr && IsValid(Cutie->PiesaDinInterior))
          {
             if (Cutie->PiesaDinInterior->GetAttachParentActor() == Cutie)
             {
                 bool bPiesaEsteDefecta = Cutie->PiesaDinInterior->ActorHasTag("Defect");
                 if (bPiesaEsteDefecta == bEsteContainerPentruDefecte)
                 {
                     ContorSortateCorect++;
                     const int32 Puncte = CalculeazaPuncte(this);
                     if(GameMode) GameMode->AdaugaPunct(Puncte);
                     if(SunetCorect) UGameplayStatics::PlaySoundAtLocation(this, SunetCorect, GetActorLocation());
                     if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Sortare Corecta (Piesa in Cutie)! +%d"), Puncte));
                 }
                 else
                 {
                     ContorGreseli++;
                     const int32 Penalizare = PenalizareGreseala;
                     if(GameMode) GameMode->AdaugaGreseala(Penalizare);
                     if(SunetGresit) UGameplayStatics::PlaySoundAtLocation(this, SunetGresit, GetActorLocation());
                     if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("GRESEALA! Cutie cu piesa dusa gresit! -%d"), Penalizare));
                 }
                 Cutie->PiesaDinInterior->Destroy();
             }
          }
          OtherActor->Destroy();
       }
    }
}