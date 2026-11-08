#include "DrivingHUD.h"
#include "Driving/DrivingPawn.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void ADrivingHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* P=Cast<ADrivingPawn>(GetOwningPlayerController()->GetPawn()); if(!P || !Canvas) return;
    const float S=Canvas->ClipX/1600.f,H=Canvas->ClipY/S;
    const FLinearColor White(.9f,.96f,1),Muted(.46f,.61f,.68f),Cyan(.2f,.88f,1),Amber(1,.63f,.19f),Panel(.008f,.022f,.034f,.9f);
    auto Rect=[&](float X,float Y,float W,float Z,FLinearColor C){ DrawRect(C,X*S,Y*S,W*S,Z*S); };
    auto Text=[&](FString V,float X,float Y,float Size,FLinearColor C){ DrawText(V,C,X*S,Y*S,GEngine->GetMediumFont(),Size*S,false); };
    auto Bar=[&](float X,float Y,float W,double Value,FLinearColor C){ Rect(X,Y,W,5,FLinearColor(.06f,.14f,.18f,.9f)); Rect(X,Y,W*FMath::Clamp(Value,0.,1.),5,C); };
    const auto& T=P->GetTelemetry(); const auto& State=P->GetPhysicsState(); const auto& I=P->GetControls();
    Rect(30,27,455,78,Panel); Rect(30,27,3,78,Cyan);
    Text(TEXT("BEYOND PHYSICS"),52,39,1.7f,White);
    Text(P->IsFog()?TEXT("FOG HIGHWAY   /   03"):(P->IsRamp()?TEXT("VIRTUAL EXIT RAMP   /   02"):TEXT("WINTER PROVING GROUND   /   01")),54,77,.75f,Muted);
    Rect(1170,27,400,78,Panel);
    Text(P->IsIce()?TEXT("ASPHALT / LOW GRIP"):TEXT("ASPHALT / NORMAL GRIP"),1192,40,1.05f,P->IsIce()?Cyan:Amber);
    Text(FString::Printf(TEXT("MU %.2f   |   g 9.81 m/s2"),P->GetFriction()),1192,75,.85f,White);
    const float Y=H-184;
    Rect(30,Y,1540,151,Panel); Rect(30,Y,1540,1,FLinearColor(.2f,.5f,.6f,.6f));
    Text(FString::Printf(TEXT("%03.0f"),T.Speed*3.6),53,Y+10,3.7f,White);
    Text(TEXT("KM/H"),215,Y+70,.8f,Cyan);
    Text(I.Brake>0?TEXT("BRAKING / DRIVE CUT"):(P->HasVehicleInput()?TEXT("PLEOS INPUT / AUTO DRIVE"):TEXT("AUTO DRIVE / HOLD S TO BRAKE")),55,Y+113,.65f,I.Brake>0?Amber:Muted);
    Text(TEXT("LATERAL / LONGITUDINAL"),345,Y+18,.7f,Muted);
    Text(FString::Printf(TEXT("%+.2f g   /   %+.2f g"),T.Ay/9.81,T.Ax/9.81),345,Y+49,1.3f,White);
    Text(FString::Printf(TEXT("YAW %+.1f deg/s   STEER %+.1f deg"),State.R*180/PI,P->GetSteeringRadians()*180/PI),345,Y+93,.8f,Cyan);
    Text(TEXT("CONTACT FORCE USAGE"),810,Y+18,.7f,Muted);
    Text(TEXT("FRONT"),810,Y+49,.75f,White); Bar(885,Y+56,175,T.Front.Utilization,T.Front.Utilization>.95?Amber:Cyan);
    Text(TEXT("REAR"),810,Y+86,.75f,White); Bar(885,Y+93,175,T.Rear.Utilization,T.Rear.Utilization>.95?Amber:Cyan);
    Text(TEXT("AXLE SLIP ANGLE"),1155,Y+18,.7f,Muted);
    Text(FString::Printf(TEXT("F  %+.1f deg"),T.Front.SlipAngle*180/PI),1155,Y+49,1,White);
    Text(FString::Printf(TEXT("R  %+.1f deg"),T.Rear.SlipAngle*180/PI),1155,Y+87,1,White);
    Text(P->HasVehicleInput()?TEXT("PLEOS STEERING + BRAKE    AUTO ACCEL    PARK ONLY    EMULATOR CALIBRATION    TAP RESTART OR R"):
        TEXT("AUTO ACCEL    S BRAKE    A/D STEER    1/2 NORMAL/LOW GRIP    V CAMERA    TAB CLUSTER    B BRAKE TEST    T TURN TEST    E RECORD    ENTER REPLAY    R RESTART    SPACE PAUSE"),36,H-24,.62f,White);
    Rect(30,118,660,37,Panel); Text(P->GetStatus(),46,128,.8f,P->IsPaused()?Amber:Cyan);
    Rect(970,118,355,37,Panel);
    Text(P->IsFog()?(P->HasVehicleInput()?TEXT("MAP 01: STRAIGHT / TAP"):TEXT("MAP 01: STRAIGHT / M")):
        (P->IsRamp()?(P->HasVehicleInput()?TEXT("MAP 03: FOG / TAP"):TEXT("MAP 03: FOG / M")):
        (P->HasVehicleInput()?TEXT("MAP 02: CURVE / TAP"):TEXT("MAP 02: CURVE / M"))),985,128,.85f,Cyan);
    Rect(1340,118,230,37,Panel);Text(TEXT("EFFECTS / F"),1360,128,.85f,Cyan);
    const auto& Feedback=P->GetFeedbackStatus();const auto& Effect=P->GetFeedbackOutput();
    Rect(30,160,1540,23,Panel);
    const FColor Color((Effect.ColorArgb>>16)&255,(Effect.ColorArgb>>8)&255,Effect.ColorArgb&255);
    Rect(38,166,10,10,FLinearColor(Color));
    Text(FString::Printf(TEXT("LIGHT %s / %s    AIR %s / %s    SOUND %s    VIRTUAL EXPERIENCE"),
        P->HasLightFeedback()?TEXT("ON"):TEXT("OFF"),*Feedback.Lights,P->HasClimateFeedback()?TEXT("ON"):TEXT("OFF"),*Feedback.Climate,
        P->HasSoundFeedback()?TEXT("ON"):TEXT("OFF")),60,166,.62f,White);
    if(P->IsFog())
    {
        const auto& Trial=P->GetFogTrial();
        auto Number=[](double Value,const TCHAR* Unit){return Value<0?FString(TEXT("--")):FString::Printf(TEXT("%.2f %s"),Value,Unit);};
        Text(TEXT("START 80 -> 100 KM/H / SHORT FOG APPROACH / 3 VEHICLES"),46,213,.75f,Muted);
        Text(FString::Printf(TEXT("RACK 18:1  HAND %+.0f / FRONT L %+.1f  R %+.1f DEG"),P->GetHandwheelDegrees(),P->GetWheelSteeringRadians(0)*180/PI,P->GetWheelSteeringRadians(1)*180/PI),46,238,.72f,Cyan);
        Rect(1180,240,390,290,Panel);Text(TEXT("FOG BRAKING LAB"),1198,257,.95f,White);
        Text(FString::Printf(TEXT("REF SPEED    %s"),*Number(Trial.ReferenceSpeed<0?-1:Trial.ReferenceSpeed*3.6,TEXT("KM/H"))),1198,293,.8f,Cyan);
        Text(FString::Printf(TEXT("REF -> BRAKE  %s"),*Number(Trial.ReactionTime(),TEXT("S"))),1198,329,.8f,White);
        Text(FString::Printf(TEXT("REACTION PATH %s"),*Number(Trial.ReactionDistance,TEXT("M"))),1198,365,.8f,White);
        Text(FString::Printf(TEXT("AFTER BRAKE   %s"),*Number(Trial.BrakingDistance(),TEXT("M"))),1198,401,.8f,White);
        Text(FString::Printf(TEXT("TARGET GAP    %s"),*Number(Trial.ReferenceTime<0 && Trial.IsRunning()?-1:Trial.FinalGap,TEXT("M"))),1198,437,.8f,Cyan);
        Text(FString::Printf(TEXT("30 M REFERENCE / TARGET %d / HIT %d"),Trial.ReferenceVehicle<0?0:Trial.ReferenceVehicle+1,Trial.HitVehicle<0?0:Trial.HitVehicle+1),1198,482,.62f,Muted);
        Text(Trial.BrakingAtReference?TEXT("ALREADY BRAKING AT REFERENCE"):(Trial.PreBraked?TEXT("PRIOR BRAKING / NONSTANDARD REACTION"):(Trial.InterruptedBraking?TEXT("DISTANCE INCLUDES BRAKE RELEASE"):TEXT("FIRST BRAKE TO END / PATH DISTANCE"))),1198,505,.62f,Muted);
        if(P->IsPaused() && !Trial.IsRunning()){
            const TCHAR* Result=TEXT("INTERRUPTED / RESTART REQUIRED");
            switch(Trial.Outcome){
                case BPPhysics::FogOutcome::Stopped:Result=Trial.ReferenceTime<0?TEXT("EARLY PRECAUTIONARY STOP / BEFORE REFERENCE"):TEXT("STOPPED BEFORE THE VEHICLE");break;
                case BPPhysics::FogOutcome::Collision:Result=TEXT("LEAD VEHICLE CONTACT");break;
                case BPPhysics::FogOutcome::Wall:Result=TEXT("BARRIER CONTACT");break;
                case BPPhysics::FogOutcome::Passed:Result=TEXT("PASSED WITHOUT STOPPING");break;
                case BPPhysics::FogOutcome::StoppedAlongside:Result=TEXT("STOPPED ALONGSIDE / LATERAL AVOIDANCE");break;
                default:break;
            }
            Rect(410,H*.34f,770,221,FLinearColor(.012f,.018f,.024f,.97f));Rect(410,H*.34f,770,3,Amber);
            Text(Result,440,H*.34f+20,1.2f,Trial.Outcome==BPPhysics::FogOutcome::Stopped?Cyan:Amber);
            Text(FString::Printf(TEXT("REFERENCE -> BRAKE %s / REACTION PATH %s"),*Number(Trial.ReactionTime(),TEXT("S")),*Number(Trial.ReactionDistance,TEXT("M"))),440,H*.34f+65,.84f,White);
            Text(FString::Printf(TEXT("DISTANCE AFTER FIRST BRAKE %s"),*Number(Trial.BrakingDistance(),TEXT("M"))),440,H*.34f+103,.9f,Cyan);
            if(Trial.FirstBrakeSpeed>=0)Text(FString::Printf(TEXT("IDEAL STRAIGHT BENCHMARK %.2f M"),Trial.FirstBrakeSpeed*Trial.FirstBrakeSpeed/(2*BPPhysics::Fog::Friction*9.81)),440,H*.34f+139,.75f,Muted);
            Text(P->HasVehicleInput()?TEXT("TAP RESTART TO REPEAT"):TEXT("R TO RESTART / BRAKE CANCELS AUTO DRIVE"),440,H*.34f+183,.86f,Amber);
        }
    }
    if(P->IsRamp()) Text(TEXT("FICTIONAL DESIGN / R150m / BANK 4% / GRADE -3%"),46,213,.7f,Muted);
    if(P->IsRamp())
    {
        const auto& Lesson=P->GetRampLesson();
        auto Entry=[](double Speed){ return Speed<0?FString(TEXT("--")):FString::Printf(TEXT("%.1f"),Speed*3.6); };
        auto Outcome=[](BPPhysics::RampOutcome Value){
            switch(Value) {
                case BPPhysics::RampOutcome::Wall:return TEXT("WALL CONTACT");
                case BPPhysics::RampOutcome::Complete:return TEXT("COMPLETED");
                case BPPhysics::RampOutcome::Restarted:return TEXT("RESTARTED");
                default:return TEXT("IN PROGRESS");
            }
        };
        Text(TEXT("BRAKE BEFORE THE BEND / DOWNHILL CAN ADD SPEED"),46,235,.7f,Cyan);
        Rect(1180,240,390,331,Panel);
        Text(TEXT("BEND ENTRY / ATTEMPT COMPARISON"),1198,255,.72f,White);
        Text(FString::Printf(TEXT("THIS  %s KM/H / MU %.2f"),*Entry(Lesson.Current.EntrySpeed),P->GetFriction()),1198,282,.9f,Cyan);
        Text(FString::Printf(TEXT("PREV  %s KM/H / MU %.2f / %s"),*Entry(Lesson.Previous.EntrySpeed),Lesson.Previous.Friction,
            Lesson.Previous.Distance>=1?Outcome(Lesson.Previous.Outcome):TEXT("NO ATTEMPT")),1198,308,.72f,Amber);
        const auto& Nodes=BPPhysics::Ramp::Nodes();
        const double MapScale=175./FMath::Max(Nodes.back().X+20.,-Nodes.back().Y+20.);
        auto MapPoint=[&](double X,double Y){return FVector2D(1270+(X+10)*MapScale,340+(-Y+10)*MapScale);};
        auto Line=[&](FVector2D A,FVector2D B,FLinearColor C,float Width){
            DrawLine(static_cast<float>(A.X)*S,static_cast<float>(A.Y)*S,static_cast<float>(B.X)*S,static_cast<float>(B.Y)*S,C,Width*S);
        };
        for(size_t N=8;N<Nodes.size();N+=8) Line(MapPoint(Nodes[N-8].X,Nodes[N-8].Y),MapPoint(Nodes[N].X,Nodes[N].Y),FLinearColor(.15f,.23f,.27f),5.f);
        auto Trace=[&](const BPPhysics::RampAttempt& Attempt,FLinearColor Color){
            if(Attempt.Trace.size()<2) return;
            size_t Last=0;
            for(size_t N=2;N<Attempt.Trace.size();N+=2){
                Line(MapPoint(Attempt.Trace[Last].X,Attempt.Trace[Last].Y),MapPoint(Attempt.Trace[N].X,Attempt.Trace[N].Y),Color,1.8f);Last=N;
            }
            Line(MapPoint(Attempt.Trace[Last].X,Attempt.Trace[Last].Y),MapPoint(Attempt.Trace.back().X,Attempt.Trace.back().Y),Color,1.8f);
        };
        Trace(Lesson.Previous,Amber);Trace(Lesson.Current,Cyan);
        Text(TEXT("CYAN THIS / AMBER PREVIOUS"),1198,522,.62f,Muted);
        Text(FString::Printf(TEXT("DRIVE TARGET %.0f KM/H / NO AUTO BRAKE"),P->GetRampDriveTarget()*3.6),1198,542,.67f,Cyan);
        if(P->IsPaused() && Lesson.Current.Outcome!=BPPhysics::RampOutcome::Running)
        {
            Rect(420,H*.34f,760,186,FLinearColor(.012f,.018f,.024f,.97f));Rect(420,H*.34f,760,3,Amber);
            Text(FString::Printf(TEXT("RAMP RESULT / %s"),Outcome(Lesson.Current.Outcome)),450,H*.34f+20,1.25f,White);
            Text(FString::Printf(TEXT("ENTRY  THIS %s / PREVIOUS %s KM/H"),*Entry(Lesson.Current.EntrySpeed),*Entry(Lesson.Previous.EntrySpeed)),450,H*.34f+64,1.f,Cyan);
            Text(FString::Printf(TEXT("MAX OFFSET %.2f M / COMPARE PATHS ON THE RIGHT"),Lesson.Current.MaxOffset),450,H*.34f+101,.8f,Muted);
            Text(P->HasVehicleInput()?TEXT("TAP RESTART TO TRY AGAIN"):TEXT("R TO TRY AGAIN / SAME ROAD AND GRIP"),450,H*.34f+143,.9f,Amber);
        }
    }
    const double Course=State.Yaw+FMath::Atan2(State.V,State.U);
    Text(FString::Printf(TEXT("BODY %+.1f   COURSE %+.1f   FRONT WHEEL %+.1f deg"),
        FMath::RadiansToDegrees(State.Yaw),FMath::RadiansToDegrees(Course),
        FMath::RadiansToDegrees(State.Yaw+P->GetSteeringRadians())),46,163,.7f,Muted);
    if(P->HasVehicleInput())
    {
        Rect(715,118,230,37,Panel); Text(TEXT("RESTART / TAP"),737,128,.85f,Cyan);
        Text(P->GetVehicleInputText(),46,191,.8f,P->IsVehicleInputBlocked()?Amber:Cyan);
        if(P->IsVehicleInputBlocked())
        {
            Rect(400,H*.36f,800,135,FLinearColor(.012f,.018f,.024f,.97f));
            Text(TEXT("VEHICLE INPUT / DRIVE PAUSED"),435,H*.36f+22,1.2f,Amber);
            Text(P->GetStatus(),435,H*.36f+76,1.f,White);
        }
    }
    if(P->IsPaused()) { Rect(670,28,260,47,Panel); Text(TEXT("SIMULATION PAUSED"),690,41,.9f,Amber); }
    if(P->IsCrashed() && !P->IsRamp() && !P->IsFog())
    {
        Rect(460,H*.34f,680,135,FLinearColor(.012f,.018f,.024f,.97f));
        Rect(460,H*.34f,680,3,Amber);
        Text(TEXT("WALL CONTACT / DRIVE STOPPED"),500,H*.34f+25,1.35f,Amber);
        Text(TEXT("R TO RESTART / AUTO ACCEL RESUMES"),500,H*.34f+78,1.f,White);
    }
    if(P->IsClusterVisible())
    {
        const float X=430,Z=H-411;
        Rect(X,Z,740,193,FLinearColor(.004f,.012f,.02f,.97f));
        Rect(X,Z,740,2,Cyan);
        Text(TEXT("CLUSTER PREVIEW / SHARED VEHICLE STATE"),X+25,Z+14,.7f,Muted);
        Text(FString::Printf(TEXT("%.0f"),T.Speed*3.6),X+310,Z+38,3.2f,White);
        Text(TEXT("KM/H"),X+321,Z+123,.7f,Cyan);
        Text(FString::Printf(TEXT("MU %.2f"),P->GetFriction()),X+34,Z+64,1.35f,Cyan);
        Text(FString::Printf(TEXT("%+.2f g"),T.Ay/9.81),X+557,Z+64,1.35f,White);
        Bar(X+32,Z+156,220,T.Front.Utilization,Cyan); Bar(X+490,Z+156,220,T.Rear.Utilization,Amber);
    }
    if(P->GetExperiment().bRecording)
        Text(FString::Printf(TEXT("REC  %.2f s  /  %.2f m"),P->GetExperiment().Elapsed,P->GetExperiment().Distance),1050,164,.85f,Amber);
    if(P->IsFeedbackMenuOpen())
    {
        Rect(0,0,1600,H,FLinearColor(.003f,.008f,.015f,.84f));
        Rect(320,215,960,385,FLinearColor(.014f,.03f,.045f,.99f));Rect(320,215,960,3,Cyan);
        Text(TEXT("CABIN EXPERIENCE"),350,235,1.8f,White);
        auto Toggle=[&](float Row,const TCHAR* Label,bool Enabled,const FString& Value){
            Rect(350,Row,900,44,FLinearColor(.04f,.08f,.10f));
            Text(Label,368,Row+12,.95f,White);Text(Enabled?TEXT("ON"):TEXT("OFF"),750,Row+12,.95f,Enabled?Cyan:Muted);
            Text(Value,835,Row+14,.7f,Muted);
        };
        Toggle(285,TEXT("AMBIENT LIGHT / L"),P->HasLightFeedback(),Feedback.Lights);
        Toggle(345,TEXT("GENTLE CLIMATE / H"),P->HasClimateFeedback(),Feedback.Climate);
        Toggle(405,TEXT("DRIVING AUDIO / K"),P->HasSoundFeedback(),TEXT("APP MIX / NO MASTER VOLUME CHANGE"));
        FString Hint=TEXT("CHOOSE OPTIONAL EFFECTS / SOME SUPPORTED SEAT ZONES ARE LINKED");
        if(Feedback.Lights==TEXT("RESTORING")||Feedback.Climate==TEXT("RESTORING"))
            Hint=TEXT("RESTORING YOUR CABIN SETTINGS / UNAVAILABLE DEVICES WILL RETRY");
        else if(Feedback.Detail.Contains(TEXT("USER OVERRIDE")))
            Hint=TEXT("YOUR MANUAL SETTINGS TAKE PRIORITY / SWITCH OFF THEN ON TO RE-ENABLE");
        else if(Feedback.Detail.Contains(TEXT("AC OFF")))
            Hint=TEXT("A/C IS OFF / GENTLE FAN ONLY / TEMPERATURE STAYS UNCHANGED");
        else if(Feedback.Climate==TEXT("UNSUPPORTED"))
            Hint=TEXT("CLIMATE UNAVAILABLE / USE SUPPORTED MANUAL CLIMATE WITH POWER ON");
        else if(Feedback.Lights==TEXT("ERROR")||Feedback.Climate==TEXT("ERROR"))
            Hint=TEXT("VEHICLE EFFECT UNAVAILABLE / SWITCH OFF AND RECONNECT TO RETRY");
        else if(!P->HasVehicleInput())Hint=TEXT("PC PREVIEW / VEHICLE LIGHTING AND CLIMATE ARE SIMULATED");
        Text(Hint,350,465,.73f,Muted);
        Text(TEXT("GENTLE OPTIONAL EFFECTS / APP CHANGES RESTORED / YOUR OVERRIDES KEPT"),350,493,.68f,Muted);
        Rect(350,525,900,45,FLinearColor(.07f,.21f,.27f));Text(TEXT("APPLY AND RESTART / F"),595,538,1.f,Cyan);
    }
}
