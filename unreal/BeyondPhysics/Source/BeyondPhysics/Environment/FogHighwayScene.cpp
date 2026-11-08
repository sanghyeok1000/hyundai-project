#include "FogHighwayScene.h"
#include "Physics/FogHighway.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFogHighwayScene::AFogHighwayScene()
{
    PrimaryActorTick.bCanEverTick=false;
    Origin=CreateDefaultSubobject<USceneComponent>(TEXT("FogHighwayOrigin"));SetRootComponent(Origin);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar.SM_SportsCar"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GlassMesh(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Glass.SM_SportsCar_Glass"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> WheelMesh(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Wheel.SM_SportsCar_Wheel"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SkyMesh(TEXT("/Engine/EngineSky/SM_SkySphere.SM_SkySphere"));
    const TCHAR* Names[]={TEXT("Road"),TEXT("Shoulder"),TEXT("Markings"),TEXT("Barriers"),TEXT("LampSteel"),TEXT("LampHeads"),TEXT("Reflectors"),TEXT("Verge"),TEXT("LeadTailLamps")};
    for(const TCHAR* Name:Names){
        auto* Layer=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
        Layer->SetupAttachment(Origin);Layer->SetStaticMesh(Cube.Object);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);Layer->SetCastShadow(false);
        Layer->SetCanEverAffectNavigation(false);Layer->bAutoRebuildTreeOnInstanceChanges=false;GeometryInstances.Add(Layer);
    }
    LeadBody=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationaryLeadBody"));LeadBody->SetupAttachment(Origin);LeadBody->SetStaticMesh(BodyMesh.Object);
    LeadBody->SetRelativeLocation(FVector(BPPhysics::Fog::LeadX*100,0,0));LeadBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeadGlass=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationaryLeadGlass"));LeadGlass->SetupAttachment(LeadBody);LeadGlass->SetStaticMesh(GlassMesh.Object);LeadGlass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    for(int32 I=0;I<4;++I){
        auto* W=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("LeadWheel%d"),I));
        W->SetupAttachment(LeadBody);W->SetStaticMesh(WheelMesh.Object);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        W->SetRelativeLocation(FVector(I<2?145:-145,I%2?94:-94,39.4));W->SetRelativeRotation(FRotator(0,I%2?180:0,0));LeadWheels.Add(W);
    }
    for(int32 Car=1;Car<3;++Car){
        auto* B=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Traffic%dBody"),Car));
        B->SetupAttachment(Origin);B->SetStaticMesh(BodyMesh.Object);B->SetCollisionEnabled(ECollisionEnabled::NoCollision);TrafficBodies.Add(B);
        auto* G=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Traffic%dGlass"),Car));
        G->SetupAttachment(B);G->SetStaticMesh(GlassMesh.Object);G->SetCollisionEnabled(ECollisionEnabled::NoCollision);TrafficGlass.Add(G);
        for(int32 I=0;I<4;++I){
            auto* W=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Traffic%dWheel%d"),Car,I));
            W->SetupAttachment(B);W->SetStaticMesh(WheelMesh.Object);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            W->SetRelativeLocation(FVector(I<2?145:-145,I%2?94:-94,39.4));W->SetRelativeRotation(FRotator(0,I%2?180:0,0));TrafficWheels.Add(W);
        }
    }
    Sky=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloudySkyBackdrop"));Sky->SetupAttachment(Origin);Sky->SetStaticMesh(SkyMesh.Object);
    Sky->SetRelativeScale3D(FVector(20000));Sky->SetCastShadow(false);Sky->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FogBank=CreateDefaultSubobject<ULocalFogVolumeComponent>(TEXT("SpatialFogBank"));FogBank->SetupAttachment(Origin);
}
void AFogHighwayScene::OnConstruction(const FTransform& Transform){Super::OnConstruction(Transform);BuildScene();}
void AFogHighwayScene::BeginPlay(){Super::BeginPlay();if(GeometryInstances[0]->GetInstanceCount()==0)BuildScene();}
void AFogHighwayScene::BuildScene()
{
    const TCHAR* Materials[]={TEXT("/Game/Driving/Fog/MI_FogRoad"),TEXT("/Game/Driving/Fog/MI_FogShoulder"),TEXT("/Game/Driving/Fog/MI_FogPaint"),TEXT("/Game/Driving/Fog/MI_FogConcrete"),TEXT("/Game/Driving/Fog/MI_FogLampSteel"),TEXT("/Game/Driving/Fog/MI_FogLampWarm"),TEXT("/Game/Driving/Fog/MI_FogAmber"),TEXT("/Game/Driving/Fog/MI_FogVerge"),TEXT("/Game/Driving/Fog/MI_FogTailLamp")};
    for(int32 I=0;I<GeometryInstances.Num();++I){GeometryInstances[I]->ClearInstances();if(auto* M=LoadObject<UMaterialInterface>(nullptr,Materials[I],nullptr,LOAD_NoWarn))GeometryInstances[I]->SetMaterial(0,M);}
    auto Add=[&](int32 Layer,double X,double Y,double Z,double SX,double SY,double SZ){GeometryInstances[Layer]->AddInstance(FTransform(FRotator::ZeroRotator,FVector(X*100,Y*100,Z*100),FVector(SX,SY,SZ)));};
    // Ten-metre tiles avoid giant per-vertex fog interpolation across the whole road.
    for(double X=-1995;X<2000;X+=10){
        Add(0,X,0,-.05,10,13.5,.1);
        for(int32 Side:{-1,1}){
            Add(1,X,Side*6,-.015,10,1.5,.04);Add(2,X,Side*5.25,.018,10,.15,.012);
            Add(3,X,Side*6.95,.6,10,.4,1.2);
        }
    }
    for(double X=-1998;X<2000;X+=9)for(int32 Side:{-1,1})Add(2,X,Side*1.75,.02,3,.15,.012);
    for(double X=-1975;X<2000;X+=50)Add(7,X,0,-.22,50,400,.25);
    for(double X=-1980;X<2000;X+=40)for(int32 Side:{-1,1}){
        Add(4,X,Side*7.6,4.5,.14,.14,9);Add(4,X,Side*6.5,8.92,.16,2.8,.16);
        Add(5,X,Side*5.05,8.75,.95,.6,.18);Add(6,X,Side*6.735,.75,.12,.025,.25);
    }
    for(const auto& Vehicle:BPPhysics::Fog::Vehicles)for(int32 Side:{-1,1})Add(8,Vehicle.X-2.47,-Vehicle.Y+Side*.67,.78,.07,.40,.16);
    for(const auto& Layer:GeometryInstances)Layer->BuildTreeIfOutdated(false,true);
    if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Driving/Fog/MI_FogLeadPaint"),nullptr,LOAD_NoWarn))LeadBody->SetMaterial(1,M);
    for(int32 Car=0;Car<3;++Car){
        auto* B=Car==0?LeadBody.Get():TrafficBodies[Car-1].Get();
        auto* G=Car==0?LeadGlass.Get():TrafficGlass[Car-1].Get();
        const auto& Vehicle=BPPhysics::Fog::Vehicles[Car];
        B->SetRelativeLocation(FVector(Vehicle.X*100-15,-Vehicle.Y*100,0));B->SetRelativeScale3D(FVector(2.7/2.9,1,1));
        if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Driving/Fog/MI_FogLeadPaint"),nullptr,LOAD_NoWarn))B->SetMaterial(1,M);
        if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Driving/Materials/MI_Charcoal")))G->SetMaterial(0,M);
    }
    // Opaque tinted glass prevents unfogged translucent silhouettes on mobile.
    if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Driving/Materials/MI_Charcoal")))LeadGlass->SetMaterial(0,M);
    if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Driving/Fog/M_FogCloudySky"),nullptr,LOAD_NoWarn))Sky->SetMaterial(0,M);
    FogBank->SetRelativeLocation(FVector(FogCenterXMeters*100,0,0));FogBank->SetRelativeScale3D(FVector(FogRadiusMeters/5.f));
    // UE combines radial/height coverage multiplicatively: zero height density
    // cancels radial fog. Saturate the height mask; radial density sets visibility.
    FogBank->SetRadialFogExtinction(RadialDensity);FogBank->SetHeightFogExtinction(1000);
    FogBank->SetFogPhaseG(.2f);FogBank->SetFogAlbedo(FLinearColor(.93f,.96f,1));FogBank->SetFogEmissive(FogGlow);FogBank->SetFogStartDistance(0);
}

