# Third-party notices and asset provenance

## Unreal Engine / Epic Games

Unreal Engine5.8 is installed separately and is not included as engine source or editor binaries in this repository. The compiled APK incorporates Unreal runtime object code. Unreal Engine is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere. Unreal Engine, Copyright 1998–2026, Epic Games, Inc. All rights reserved.

`unreal/BeyondPhysics/Content/Vehicles/SportsCar/` was copied from the installed Epic `Templates/TemplateResources/Standard/Vehicles/Content/SportsCar` vehicle template, not an unidentified third-party vehicle download. Epic's Unreal EULA defines template/sample content as Examples and addresses their distribution in section5(b). These assets remain subject to Epic's terms; no CC0/MIT claim is made for them. Built-in Engine materials, basic meshes and fonts are referenced through the installed engine and are not copied as an Engine source tree.

Terms: https://www.unrealengine.com/eula/unreal

## Poly Haven

Rock Boulder Cracked, 2K diffuse / DirectX normal / roughness: https://polyhaven.com/a/rock_boulder_cracked
Authors: Dario Barresi (processing), Dimitrios Savva (photography).
CC0: https://polyhaven.com/license
Original source records and textures: `unreal/BeyondPhysics/SourceArt/AlpineV2/ASSET_SOURCES.md` and adjacent files. Used in `Content/Driving/Textures/T_AlpineRock_*`. Poly Haven branding and preview images are not bundled as game assets.

Mountains, ramps, lane markings, lamp assemblies, older ice source textures and synthesized driving sounds are project-generated. Mountain shapes are procedural and are not survey data; the ramp is not a replica of a named real road.

## Build/runtime dependencies

- Pleos Vehicle SDK2.3.2: resolved by Gradle from https://nexus-playground.pleos.ai/repository/maven-releases/ . No extracted SDK source files, SDK credentials or standalone SDK JARs/AARs are included in the source handoff. SDK usage follows the provider's terms; no new license is granted here.
- Android SDK / NDK / AAOS `android.car`: installed separately under their respective Android terms.
- Kotlin standard library2.2.10: JetBrains, Apache License2.0, https://github.com/JetBrains/kotlin/blob/master/license/LICENSE.txt . Resolved dependency, not vendored source.
- Gradle Wrapper: included to reproduce the build, Gradle Inc., Apache License2.0, https://github.com/gradle/gradle/blob/master/LICENSE . The distribution is downloaded by the wrapper.
- JUnit4.13.2: test-only dependency, Eclipse Public License1.0, https://junit.org/junit4/license.html . Not an application feature.

No Fab marketplace asset pack or installed MCP editor plugin is being redistributed as plugin source/binaries. Third-party terms apply independently of this project's RIGHTS.md. Consult the linked primary sources for the full terms.
