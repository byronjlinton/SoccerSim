#include "SoccerField.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/ConfigCacheIni.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ASoccerField::ASoccerField()
{
    PrimaryActorTick.bCanEverTick = false;

    PitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PitchMesh"));
    SetRootComponent(PitchMesh);

    FieldMarkings = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("FieldMarkings"));
    FieldMarkings->SetupAttachment(PitchMesh);
    FieldMarkings->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PitchMesh->SetStaticMesh(CubeMesh.Object);
        float ScaleX = SoccerField::PitchLength / 100.0f;
        float ScaleY = SoccerField::PitchWidth / 100.0f;
        float ScaleZ = 0.1f;
        PitchMesh->SetRelativeScale3D(FVector(ScaleX, ScaleY, ScaleZ));
        PitchMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
    }

    PitchMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PitchMesh->SetCollisionResponseToAllChannels(ECR_Block);
    PitchMesh->SetCollisionObjectType(ECC_WorldStatic);

    // Extended ground plane — larger than the pitch so the ball doesn't fall off edges
    GroundPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundPlane"));
    GroundPlane->SetupAttachment(PitchMesh);
    if (CubeMesh.Succeeded())
    {
        GroundPlane->SetStaticMesh(CubeMesh.Object);
        // 200m x 200m x 2m — much larger than the 105m x 68m pitch
        GroundPlane->SetRelativeScale3D(FVector(200.0f, 200.0f, 0.02f));
        GroundPlane->SetRelativeLocation(FVector(0.0f, 0.0f, -0.5f));
    }
    GroundPlane->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GroundPlane->SetCollisionResponseToAllChannels(ECR_Block);
    GroundPlane->SetCollisionObjectType(ECC_WorldStatic);
    GroundPlane->SetVisibility(false); // hidden — only for physics

    float TriggerThickness = 200.0f;
    float TriggerHeight = 500.0f;
    float Offset = TriggerThickness / 2.0f;

    CreateBoundaryTrigger(LeftTouchline, TEXT("LeftTouchline"),
        FVector(0.0f, -(SoccerField::HalfWidth + Offset), TriggerHeight / 2.0f),
        FVector(SoccerField::HalfLength, TriggerThickness / 2.0f, TriggerHeight / 2.0f));

    CreateBoundaryTrigger(RightTouchline, TEXT("RightTouchline"),
        FVector(0.0f, SoccerField::HalfWidth + Offset, TriggerHeight / 2.0f),
        FVector(SoccerField::HalfLength, TriggerThickness / 2.0f, TriggerHeight / 2.0f));

    CreateBoundaryTrigger(HomeGoalLine, TEXT("HomeGoalLine"),
        FVector(-(SoccerField::HalfLength + Offset), 0.0f, TriggerHeight / 2.0f),
        FVector(TriggerThickness / 2.0f, SoccerField::HalfWidth, TriggerHeight / 2.0f));

    CreateBoundaryTrigger(AwayGoalLine, TEXT("AwayGoalLine"),
        FVector(SoccerField::HalfLength + Offset, 0.0f, TriggerHeight / 2.0f),
        FVector(TriggerThickness / 2.0f, SoccerField::HalfWidth, TriggerHeight / 2.0f));
}

void ASoccerField::BeginPlay()
{
    Super::BeginPlay();

    // Optional default material path from config (DefaultGame.ini [/Script/SoccerSim.SoccerField] DefaultPitchMaterialPath="...")
    if (!PitchMaterialPath.IsValid() && GConfig && GGameIni.Len() > 0)
    {
        FString ConfigPath;
        if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerField"), TEXT("DefaultPitchMaterialPath"), ConfigPath, GGameIni) && !ConfigPath.IsEmpty())
        {
            PitchMaterialPath.SetPath(ConfigPath);
        }
    }

    // Pitch material: use PitchMaterialPath if valid (e.g. Megascans grass), else green dynamic material
    if (PitchMesh)
    {
        UMaterialInterface* PitchMat = nullptr;
        if (PitchMaterialPath.IsValid())
        {
            PitchMat = Cast<UMaterialInterface>(PitchMaterialPath.TryLoad());
        }
        if (!PitchMat)
        {
            UMaterialInterface* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Art/M_DynamicColor.M_DynamicColor"));
            if (!BaseMat)
                BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultLitMaterial.DefaultLitMaterial"));
            if (BaseMat)
            {
                UMaterialInstanceDynamic* GrassMID = UMaterialInstanceDynamic::Create(BaseMat, this);
                if (GrassMID)
                {
                    GrassMID->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.12f, 0.42f, 0.15f));
                    PitchMat = GrassMID;
                }
            }
        }
        if (PitchMat)
        {
            PitchMesh->SetMaterial(0, PitchMat);
        }
    }

    UPhysicalMaterial* GrassPhysMat = NewObject<UPhysicalMaterial>(this);
    GrassPhysMat->Friction = GrassFriction;
    GrassPhysMat->Restitution = GrassRestitution;
    GrassPhysMat->FrictionCombineMode = EFrictionCombineMode::Average;
    GrassPhysMat->RestitutionCombineMode = EFrictionCombineMode::Average;
    PitchMesh->SetPhysMaterialOverride(GrassPhysMat);

    if (LeftTouchline)
        LeftTouchline->OnComponentBeginOverlap.AddDynamic(this, &ASoccerField::OnTouchlineOverlap);
    if (RightTouchline)
        RightTouchline->OnComponentBeginOverlap.AddDynamic(this, &ASoccerField::OnTouchlineOverlap);
    if (HomeGoalLine)
        HomeGoalLine->OnComponentBeginOverlap.AddDynamic(this, &ASoccerField::OnGoalLineOverlap);
    if (AwayGoalLine)
        AwayGoalLine->OnComponentBeginOverlap.AddDynamic(this, &ASoccerField::OnGoalLineOverlap);

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerField initialized: %.0f x %.0f cm"),
        SoccerField::PitchLength, SoccerField::PitchWidth);

    GenerateFieldMarkings();
}

void ASoccerField::CreateBoundaryTrigger(TObjectPtr<UBoxComponent>& OutComp, FName Name, FVector Location, FVector Extent)
{
    OutComp = CreateDefaultSubobject<UBoxComponent>(Name);
    OutComp->SetupAttachment(RootComponent);
    OutComp->SetRelativeLocation(Location);
    OutComp->SetBoxExtent(Extent);
    OutComp->SetCollisionProfileName(TEXT("FieldBoundary"));
    OutComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OutComp->SetGenerateOverlapEvents(true);
    OutComp->SetHiddenInGame(true);
}

void ASoccerField::OnTouchlineOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                       bool bFromSweep, const FHitResult& SweepResult)
{
    ASoccerBall* Ball = Cast<ASoccerBall>(OtherActor);
    if (!Ball) return;

    UE_LOG(LogSoccerSim, Log, TEXT("Ball crossed touchline -> Throw-In"));

    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        GM->HandleBallOutOfPlay(EBallState::OutForThrowIn, Ball->LastTouchTeam);
    }
}

void ASoccerField::OnGoalLineOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                      bool bFromSweep, const FHitResult& SweepResult)
{
    ASoccerBall* Ball = Cast<ASoccerBall>(OtherActor);
    if (!Ball) return;

    bool bIsHomeEnd = (OverlappedComp == HomeGoalLine);

    UE_LOG(LogSoccerSim, Log, TEXT("Ball crossed goal line at %s end"),
        bIsHomeEnd ? TEXT("Home") : TEXT("Away"));

    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        ETeamId DefendingTeam = bIsHomeEnd ? ETeamId::Home : ETeamId::Away;

        if (Ball->LastTouchTeam == DefendingTeam)
        {
            GM->HandleBallOutOfPlay(EBallState::OutForCorner, Ball->LastTouchTeam);
        }
        else
        {
            GM->HandleBallOutOfPlay(EBallState::OutForGoalKick, Ball->LastTouchTeam);
        }
    }
}

// ========== Dynamic Mesh Field Markings ==========

void ASoccerField::GenerateFieldMarkings()
{
    if (!FieldMarkings) return;

    using namespace UE::Geometry;

    FDynamicMesh3 Mesh;
    Mesh.EnableVertexNormals(FVector3f(0.0f, 0.0f, 1.0f));

    const float Z = 1.5f;
    const float LW = SoccerField::LineWidth;
    const float HL = SoccerField::HalfLength;
    const float HW = SoccerField::HalfWidth;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;

    // Touchlines
    AddLineQuad(Vertices, Triangles, FVector(-HL, -HW, Z), FVector(HL, -HW, Z), LW, Z);
    AddLineQuad(Vertices, Triangles, FVector(-HL,  HW, Z), FVector(HL,  HW, Z), LW, Z);

    // Goal lines
    AddLineQuad(Vertices, Triangles, FVector(-HL, -HW, Z), FVector(-HL, HW, Z), LW, Z);
    AddLineQuad(Vertices, Triangles, FVector( HL, -HW, Z), FVector( HL, HW, Z), LW, Z);

    // Halfway line
    AddLineQuad(Vertices, Triangles, FVector(0.0f, -HW, Z), FVector(0.0f, HW, Z), LW, Z);

    // Center circle
    AddArcStrip(Vertices, Triangles, FVector::ZeroVector, SoccerField::CenterCircleRadius, LW, Z, 0.0f, 360.0f, 48);

    // Center spot
    AddFilledCircle(Vertices, Triangles, FVector::ZeroVector, 15.0f, Z, 12);

    // Penalty areas + goal areas (both ends)
    for (int32 Side = -1; Side <= 1; Side += 2)
    {
        float GoalX = Side * HL;
        float PenX = GoalX - Side * SoccerField::PenaltyBoxLength;
        float PenHalfW = SoccerField::PenaltyBoxWidth / 2.0f;

        AddLineQuad(Vertices, Triangles, FVector(GoalX, -PenHalfW, Z), FVector(PenX, -PenHalfW, Z), LW, Z);
        AddLineQuad(Vertices, Triangles, FVector(PenX, -PenHalfW, Z), FVector(PenX, PenHalfW, Z), LW, Z);
        AddLineQuad(Vertices, Triangles, FVector(PenX, PenHalfW, Z), FVector(GoalX, PenHalfW, Z), LW, Z);

        float GoalAreaX = GoalX - Side * SoccerField::GoalBoxLength;
        float GoalAreaHalfW = SoccerField::GoalBoxWidth / 2.0f;

        AddLineQuad(Vertices, Triangles, FVector(GoalX, -GoalAreaHalfW, Z), FVector(GoalAreaX, -GoalAreaHalfW, Z), LW, Z);
        AddLineQuad(Vertices, Triangles, FVector(GoalAreaX, -GoalAreaHalfW, Z), FVector(GoalAreaX, GoalAreaHalfW, Z), LW, Z);
        AddLineQuad(Vertices, Triangles, FVector(GoalAreaX, GoalAreaHalfW, Z), FVector(GoalX, GoalAreaHalfW, Z), LW, Z);

        // Penalty spot
        float PenSpotX = GoalX - Side * SoccerField::PenaltySpotDistance;
        AddFilledCircle(Vertices, Triangles, FVector(PenSpotX, 0.0f, Z), 15.0f, Z, 12);

        // Penalty arc (portion outside the penalty box)
        float ArcDistFromEdge = SoccerField::PenaltyBoxLength - SoccerField::PenaltySpotDistance;
        float HalfAngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(ArcDistFromEdge / SoccerField::CenterCircleRadius, -1.0f, 1.0f)));

        if (Side == -1)
        {
            AddArcStrip(Vertices, Triangles, FVector(PenSpotX, 0.0f, Z),
                SoccerField::CenterCircleRadius, LW, Z, -HalfAngleDeg, HalfAngleDeg, 24);
        }
        else
        {
            AddArcStrip(Vertices, Triangles, FVector(PenSpotX, 0.0f, Z),
                SoccerField::CenterCircleRadius, LW, Z, 180.0f - HalfAngleDeg, 180.0f + HalfAngleDeg, 24);
        }
    }

    // Corner arcs
    AddArcStrip(Vertices, Triangles, FVector(-HL, -HW, Z), SoccerField::CornerArcRadius, LW, Z, 0.0f, 90.0f, 8);
    AddArcStrip(Vertices, Triangles, FVector(-HL,  HW, Z), SoccerField::CornerArcRadius, LW, Z, 270.0f, 360.0f, 8);
    AddArcStrip(Vertices, Triangles, FVector( HL, -HW, Z), SoccerField::CornerArcRadius, LW, Z, 90.0f, 180.0f, 8);
    AddArcStrip(Vertices, Triangles, FVector( HL,  HW, Z), SoccerField::CornerArcRadius, LW, Z, 180.0f, 270.0f, 8);

    // Transfer vertex data to FDynamicMesh3
    const FVector3f UpNormal(0.0f, 0.0f, 1.0f);
    for (int32 i = 0; i < Vertices.Num(); i++)
    {
        Mesh.AppendVertex(FVertexInfo(
            FVector3d(Vertices[i].X, Vertices[i].Y, Vertices[i].Z),
            UpNormal));
    }

    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        Mesh.AppendTriangle(Triangles[i], Triangles[i + 1], Triangles[i + 2]);
    }

    FieldMarkings->SetMesh(MoveTemp(Mesh));

    // White unlit material
    UMaterialInterface* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Art/M_DynamicColor.M_DynamicColor"));
    if (!BaseMat)
        BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultUnlitMaterial.DefaultUnlitMaterial"));
    if (BaseMat)
    {
        UMaterialInstanceDynamic* LineMat = UMaterialInstanceDynamic::Create(BaseMat, this);
        if (LineMat)
        {
            LineMat->SetVectorParameterValue(FName("BaseColor"), FLinearColor::White);
            FieldMarkings->SetMaterial(0, LineMat);
        }
    }

    UE_LOG(LogSoccerSim, Log, TEXT("Field markings generated: %d verts, %d tris"),
        Vertices.Num(), Triangles.Num() / 3);
}

void ASoccerField::AddLineQuad(TArray<FVector>& Vertices, TArray<int32>& Triangles,
                                FVector Start, FVector End, float Width, float Z)
{
    FVector Dir = (End - Start).GetSafeNormal();
    FVector Perp = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal() * (Width * 0.5f);

    int32 BaseIdx = Vertices.Num();
    Vertices.Add(Start + Perp);
    Vertices.Add(Start - Perp);
    Vertices.Add(End + Perp);
    Vertices.Add(End - Perp);

    Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1);
    Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);
}

void ASoccerField::AddArcStrip(TArray<FVector>& Vertices, TArray<int32>& Triangles,
                                FVector Center, float Radius, float Width, float Z,
                                float StartAngleDeg, float EndAngleDeg, int32 Segments)
{
    float AngleStep = (EndAngleDeg - StartAngleDeg) / Segments;
    float InnerR = Radius - Width * 0.5f;
    float OuterR = Radius + Width * 0.5f;

    for (int32 i = 0; i < Segments; i++)
    {
        float A1 = FMath::DegreesToRadians(StartAngleDeg + i * AngleStep);
        float A2 = FMath::DegreesToRadians(StartAngleDeg + (i + 1) * AngleStep);

        int32 BaseIdx = Vertices.Num();
        Vertices.Add(Center + FVector(FMath::Cos(A1) * InnerR, FMath::Sin(A1) * InnerR, Z));
        Vertices.Add(Center + FVector(FMath::Cos(A1) * OuterR, FMath::Sin(A1) * OuterR, Z));
        Vertices.Add(Center + FVector(FMath::Cos(A2) * InnerR, FMath::Sin(A2) * InnerR, Z));
        Vertices.Add(Center + FVector(FMath::Cos(A2) * OuterR, FMath::Sin(A2) * OuterR, Z));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 3);
    }
}

void ASoccerField::AddFilledCircle(TArray<FVector>& Vertices, TArray<int32>& Triangles,
                                    FVector Center, float Radius, float Z, int32 Segments)
{
    int32 CenterIdx = Vertices.Num();
    Vertices.Add(Center);

    for (int32 i = 0; i <= Segments; i++)
    {
        float Angle = (2.0f * PI * i) / Segments;
        Vertices.Add(Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z));
    }

    for (int32 i = 0; i < Segments; i++)
    {
        Triangles.Add(CenterIdx);
        Triangles.Add(CenterIdx + 1 + i);
        Triangles.Add(CenterIdx + 2 + i);
    }
}
