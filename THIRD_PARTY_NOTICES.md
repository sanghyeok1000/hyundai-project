# Third-party notices

## Unreal Engine · Epic Games

Engine: Unreal Engine 5.8.

Unreal Engine is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere. Unreal Engine, Copyright 1998–2026, Epic Games, Inc. All rights reserved.

The vehicle assets in `Content/Vehicles/SportsCar/` originate from Epic's installed vehicle template at `Templates/TemplateResources/Standard/Vehicles/Content/SportsCar`. Their use and distribution follow the Unreal Engine EULA, including the provisions for Examples. Built-in engine materials, basic meshes and fonts are referenced from the installed engine.

[Unreal Engine EULA](https://www.unrealengine.com/eula/unreal).

## Poly Haven

| Asset | Source | License |
|---|---|---|
|Rock Boulder Cracked · 2K diffuse / DirectX normal / roughness|[Poly Haven](https://polyhaven.com/a/rock_boulder_cracked)|[CC0](https://polyhaven.com/license)|

Authors: Dario Barresi (processing), Dimitrios Savva (photography).

Source files and provenance: [`SourceArt/AlpineV2/`](unreal/BeyondPhysics/SourceArt/AlpineV2). Imported textures: `Content/Driving/Textures/T_AlpineRock_*`.

The mountain geometry, ramp geometry, lane markings, lamp assemblies, ice source textures and synthesized driving sounds are project-generated assets.

## Software dependencies

| Component | Use | Terms |
|---|---|---|
|Pleos Vehicle SDK2.3.2|Vehicle inputs and cabin outputs|Pleos provider terms; resolved from the [Pleos Maven repository](https://nexus-playground.pleos.ai/repository/maven-releases/)|
|Android SDK / NDK / AAOS|Platform, toolchain and vehicle state|Android provider terms|
|Kotlin standard library2.2.10|Android bridge runtime|[Apache License2.0](https://github.com/JetBrains/kotlin/blob/master/license/LICENSE.txt)|
|Google Gson|SDK capability serialization|[Apache License2.0](https://github.com/google/gson/blob/main/LICENSE)|
|Gradle Wrapper|Reproducible library build|[Gradle license](docs/licenses/Gradle-LICENSE.txt)|
|JUnit4.13.2|Unit tests|[Eclipse Public License1.0](https://junit.org/junit4/license.html)|

The engine, editor plugins and Android toolchains are installed separately. Runtime dependencies are resolved by the build configuration. SDK source extraction and local build caches are excluded from this repository.
