#include "AnimNotify_KickContact.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"

void UAnimNotify_KickContact::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp) return;

    AActor* Owner = MeshComp->GetOwner();
    ASoccerPlayerPawn* Pawn = Cast<ASoccerPlayerPawn>(Owner);
    if (Pawn)
    {
        Pawn->ExecuteKickFromNotify();
    }
}
