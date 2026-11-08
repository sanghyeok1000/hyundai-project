#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FogHighwayScene.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMeshComponent;
class ULocalFogVolumeComponent;
UCLASS()
class BEYONDPHYSICS_API AFogHighwayScene : public AActor
{
    GENERATED_BODY()
public:
    AFogHighwayScene();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    UPROPERTY(EditAnywhere,Category="Fog Appearance",meta=(ClampMin="0")) float RadialDensity=.27f;
    UPROPERTY(EditAnywhere,Category="Fog Appearance",meta=(ClampMin="100")) float FogRadiusMeters=450;
    UPROPERTY(EditAnywhere,Category="Fog Appearance") float FogCenterXMeters=220;
    UPROPERTY(EditAnywhere,Category="Fog Appearance") FLinearColor FogGlow=FLinearColor(300,330,360);
private:
    UPROPERTY() TObjectPtr<USceneComponent> Origin;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GeometryInstances;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LeadBody;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LeadGlass;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> LeadWheels;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> TrafficBodies;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> TrafficGlass;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> TrafficWheels;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sky;
    UPROPERTY(VisibleAnywhere) TObjectPtr<ULocalFogVolumeComponent> FogBank;
    void BuildScene();
};
