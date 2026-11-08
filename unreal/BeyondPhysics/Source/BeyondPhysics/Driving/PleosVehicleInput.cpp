#include "PleosVehicleInput.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidJavaEnv.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#endif

FPleosVehicleInput ReadPleosVehicleInput()
{
    FPleosVehicleInput Input;
#if PLATFORM_ANDROID
    Input.bEnabled=true;
    Input.Reason=TEXT("SDK BRIDGE UNAVAILABLE");
    JNIEnv* Env=FAndroidApplication::GetJavaEnv();
    if(!Env || !FJavaWrapper::GameActivityThis) return Input;
    static jmethodID Method=FJavaWrapper::FindMethod(Env,FJavaWrapper::GameActivityClassID,
        "AndroidThunkJava_PleosSnapshot","()Ljava/lang/String;",true);
    if(!Method) return Input;
    auto Value=static_cast<jstring>(Env->CallObjectMethod(FJavaWrapper::GameActivityThis,Method));
    if(Env->ExceptionCheck()) { Env->ExceptionClear(); if(Value) Env->DeleteLocalRef(Value); return Input; }
    if(!Value) return Input;
    const FString Json=FJavaHelper::FStringFromLocalRef(Env,Value);
    if(Json.Len()>4096) return Input;
    TSharedPtr<FJsonObject> Root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root.IsValid()) return Input;
    bool Allowed=false;
    double Schema=0,Steer=0,Brake=0,Raw=0,Loss=0;
    Root->TryGetStringField(TEXT("reason"),Input.Reason);
    if(!Root->TryGetNumberField(TEXT("schema"),Schema) || Schema!=1 ||
       !Root->TryGetBoolField(TEXT("allowed"),Allowed) || !Root->TryGetNumberField(TEXT("lossEpoch"),Loss) ||
       !FMath::IsFinite(Loss) || Loss<0 || Loss>9007199254740991.0 || FMath::FloorToDouble(Loss)!=Loss) return Input;
    Input.LossEpoch=static_cast<uint64>(Loss);
    Root->TryGetNumberField(TEXT("rawSteering"),Input.RawSteering);
    if(!Allowed) return Input;
    if(!Root->TryGetNumberField(TEXT("steering"),Steer) || !Root->TryGetNumberField(TEXT("brake"),Brake) ||
       !Root->TryGetNumberField(TEXT("rawSteering"),Raw) || !FMath::IsFinite(Steer) || !FMath::IsFinite(Brake) ||
       !FMath::IsFinite(Raw) || FMath::Abs(Steer)>1 || FMath::Abs(Raw)>540 || (Brake!=0 && Brake!=1))
    { Input.Reason=TEXT("INVALID SDK INPUT"); return Input; }
    Input.bAllowed=true; Input.Steering=Steer; Input.Brake=Brake; Input.RawSteering=Raw;
#endif
    return Input;
}

FPleosFeedbackStatus SendPleosFeedback(const FString& Command)
{
    FPleosFeedbackStatus Status;
#if PLATFORM_ANDROID
    Status.Lights=Status.Climate=TEXT("UNAVAILABLE");Status.Detail=TEXT("SDK OUTPUT BRIDGE UNAVAILABLE");
    JNIEnv* Env=FAndroidApplication::GetJavaEnv();
    if(!Env || !FJavaWrapper::GameActivityThis) return Status;
    static jmethodID Method=FJavaWrapper::FindMethod(Env,FJavaWrapper::GameActivityClassID,
        "AndroidThunkJava_PleosFeedback","(Ljava/lang/String;)Ljava/lang/String;",true);
    if(!Method) return Status;
    jstring Argument=Env->NewStringUTF(TCHAR_TO_UTF8(*Command));
    auto Value=static_cast<jstring>(Env->CallObjectMethod(FJavaWrapper::GameActivityThis,Method,Argument));
    Env->DeleteLocalRef(Argument);
    if(Env->ExceptionCheck()){Env->ExceptionClear();if(Value)Env->DeleteLocalRef(Value);return Status;}
    if(!Value)return Status;
    const FString Json=FJavaHelper::FStringFromLocalRef(Env,Value);
    TSharedPtr<FJsonObject> Root;
    if(Json.Len()<=4096 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) && Root.IsValid()){
        Root->TryGetStringField(TEXT("lights"),Status.Lights);
        Root->TryGetStringField(TEXT("climate"),Status.Climate);
        Root->TryGetStringField(TEXT("detail"),Status.Detail);
    }
#endif
    return Status;
}
