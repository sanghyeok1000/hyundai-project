using UnrealBuildTool;

public class BeyondPhysicsEditorTarget : TargetRules
{
    public BeyondPhysicsEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("BeyondPhysics");
    }
}
