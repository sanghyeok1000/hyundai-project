#include "DrivingGameMode.h"
#include "DrivingPawn.h"
#include "UI/DrivingHUD.h"
ADrivingGameMode::ADrivingGameMode()
{
    DefaultPawnClass=ADrivingPawn::StaticClass();
    HUDClass=ADrivingHUD::StaticClass();
}
