#include "SoccerGoal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

ASoccerGoal::ASoccerGoal()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    const float HalfGoalW = SoccerField::GoalWidth / 2.0f;
    const float GoalH = SoccerField::GoalHeight;
    const float PostR = SoccerField::GoalPostRadius;
    const float Depth = SoccerField::GoalDepth;

    LeftPost = CreatePostMesh(TEXT("LeftPost"),
        FVector(0.0f, -HalfGoalW, GoalH / 2.0f),
        FVector(PostR * 2.0f / 100.0f, PostR * 2.0f / 100.0f, GoalH / 100.0f));

    RightPost = CreatePostMesh(TEXT("RightPost"),
        FVector(0.0f, HalfGoalW, GoalH / 2.0f),
        FVector(PostR * 2.0f / 100.0f, PostR * 2.0f / 100.0f, GoalH / 100.0f));

    Crossbar = CreatePostMesh(TEXT("Crossbar"),
        FVector(0.0f, 0.0f, GoalH),
        FVector(PostR * 2.0f / 100.0f, SoccerField::GoalWidth / 100.0f, PostR * 2.0f / 100.0f));

    NetBack = CreateNetMesh(TEXT("NetBack"),
        FVector(-Depth, 0.0f, GoalH / 2.0f),
        FVector(0.02f, SoccerField::GoalWidth / 100.0f, GoalH / 100.0f));

    NetLeft = CreateNetMesh(TEXT("NetLeft"),
        FVector(-Depth / 2.0f, -HalfGoalW, GoalH / 2.0f),
        FVector(Depth / 100.0f, 0.02f, GoalH / 100.0f));

    NetRight = CreateNetMesh(TEXT("NetRight"),
        FVector(-Depth / 2.0f, HalfGoalW, GoalH / 2.0f),
        FVector(Depth / 100.0f, 0.02f, GoalH / 100.0f));

    GoalTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("GoalTrigger"));
    GoalTrigger->SetupAttachment(Root);
    GoalTrigger->SetRelativeLocation(FVector(-Depth / 2.0f, 0.0f, GoalH / 2.0f));
    GoalTrigger->SetBoxExtent(FVector(Depth / 2.0f - 10.0f, HalfGoalW - 10.0f, GoalH / 2.0f - 5.0f));
    GoalTrigger->SetCollisionProfileName(TEXT("GoalTrigger"));
    GoalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GoalTrigger->SetGenerateOverlapEvents(true);
    GoalTrigger->SetHiddenInGame(true);
}

void ASoccerGoal::BeginPlay()
{
    Super::BeginPlay();

    UPhysicalMaterial* PostMat = NewObject<UPhysicalMaterial>(this);
    PostMat->Friction = PostFriction;
    PostMat->Restitution = PostRestitution;
    PostMat->RestitutionCombineMode = EFrictionCombineMode::Max;
    LeftPost->SetPhysMaterialOverride(PostMat);
    RightPost->SetPhysMaterialOverride(PostMat);
    Crossbar->SetPhysMaterialOverride(PostMat);

    // Net visual: use NetMaterialPath if valid (e.g. translucent grid), else light grey dynamic material
    UMaterialInterface* NetMat = nullptr;
    if (NetMaterialPath.IsValid())
    {
        NetMat = Cast<UMaterialInterface>(NetMaterialPath.TryLoad());
    }
    if (!NetMat)
    {
        UMaterialInterface* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultLitMaterial.DefaultLitMaterial"));
        if (BaseMat)
        {
            UMaterialInstanceDynamic* NetMID = UMaterialInstanceDynamic::Create(BaseMat, this);
            if (NetMID)
            {
                NetMID->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.75f, 0.75f, 0.78f));
                NetMat = NetMID;
            }
        }
    }
    if (NetMat)
    {
        if (NetBack) NetBack->SetMaterial(0, NetMat);
        if (NetLeft) NetLeft->SetMaterial(0, NetMat);
        if (NetRight) NetRight->SetMaterial(0, NetMat);
    }

    UPhysicalMaterial* NetPhysMat = NewObject<UPhysicalMaterial>(this);
    NetPhysMat->Friction = NetFriction;
    NetPhysMat->Restitution = NetRestitution;
    NetBack->SetPhysMaterialOverride(NetPhysMat);
    NetLeft->SetPhysMaterialOverride(NetPhysMat);
    NetRight->SetPhysMaterialOverride(NetPhysMat);

    GoalTrigger->OnComponentBeginOverlap.AddDynamic(this, &ASoccerGoal::OnGoalTriggerOverlap);

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerGoal initialized for team %d at %s"),
        static_cast<int32>(TeamId), *GetActorLocation().ToString());
}

UStaticMeshComponent* ASoccerGoal::CreatePostMesh(FName Name, FVector Location, FVector Scale)
{
    UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Mesh->SetupAttachment(Root);
    Mesh->SetRelativeLocation(Location);
    Mesh->SetRelativeScale3D(Scale);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetCollisionObjectType(ECC_WorldStatic);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }

    return Mesh;
}

UStaticMeshComponent* ASoccerGoal::CreateNetMesh(FName Name, FVector Location, FVector Scale)
{
    return CreatePostMesh(Name, Location, Scale);
}

void ASoccerGoal::OnGoalTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                        bool bFromSweep, const FHitResult& SweepResult)
{
    ASoccerBall* Ball = Cast<ASoccerBall>(OtherActor);
    if (!Ball) return;

    UE_LOG(LogSoccerSim, Log, TEXT("GOAL detected in %s goal!"),
        TeamId == ETeamId::Home ? TEXT("Home") : TEXT("Away"));

    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        ETeamId ScoringTeam = (TeamId == ETeamId::Home) ? ETeamId::Away : ETeamId::Home;
        GM->HandleGoalScored(ScoringTeam);
    }
}
