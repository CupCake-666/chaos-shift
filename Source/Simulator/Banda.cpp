#include "Banda.h"
#include "SimulatorGameMode.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"

ABanda::ABanda()
{
	PrimaryActorTick.bCanEverTick = true;
	BandaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BandaMesh"));
	SetRootComponent(BandaMesh);
	SuprafataRulare = CreateDefaultSubobject<UBoxComponent>(TEXT("SuprafataRulare"));
	SuprafataRulare->SetupAttachment(BandaMesh);
	SuprafataRulare->SetBoxExtent(FVector(200.0f, 50.0f, 10.0f));
	SuprafataRulare->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f)); 
	SuprafataRulare->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Viteza = 100.0f;

	SunetBanda = CreateDefaultSubobject<UAudioComponent>(TEXT("SunetBanda"));
	SunetBanda->SetupAttachment(BandaMesh);
	SunetBanda->bAutoActivate = false;
}

void ABanda::BeginPlay()
{
	Super::BeginPlay();
	GameMode = Cast<ASimulatorGameMode>(GetWorld()->GetAuthGameMode());
}

void ABanda::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Cand runda s-a terminat (sau nu a inceput), banda sta. Altfel piesele
	// continuau sa circule si dupa expirarea celor 180 de secunde.
	const bool bRuleaza = GameMode && GameMode->bJocInceput;

	// Sunetul urmeaza starea benzii, cu fade ca sa nu pocneasca.
	if (SunetBanda && SunetBanda->Sound)
	{
		if (bRuleaza && !SunetBanda->IsPlaying())
		{
			SunetBanda->FadeIn(0.4f);
		}
		else if (!bRuleaza && SunetBanda->IsPlaying())
		{
			SunetBanda->FadeOut(0.6f, 0.0f);
		}
	}

	if (!bRuleaza)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	SuprafataRulare->GetOverlappingActors(OverlappingActors);
	FVector DirectieMiscare = GetActorForwardVector();
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && Actor != this)
		{
			if (Actor->ActorHasTag("CutieBanda") || Actor->ActorHasTag("PiesaBanda"))
			{
				FVector Deplasare = DirectieMiscare * Viteza * DeltaTime;
				Actor->AddActorWorldOffset(Deplasare, true); 
			}
		}
	}
}