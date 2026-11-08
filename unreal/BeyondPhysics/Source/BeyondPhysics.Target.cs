using UnrealBuildTool;

public class BeyondPhysicsTarget : TargetRules
{
    public BeyondPhysicsTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("BeyondPhysics");
    }
}
