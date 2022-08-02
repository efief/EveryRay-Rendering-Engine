# EveryRay-Rendering-Engine

![picture](screenshots/EveryRay_testScene_simple.png)

Rendering engine/framework written in C++/DirectX 11. 
https://youtu.be/_htQNxesr9U

***
**Note (will be removed with v1.0)**: after years of development and increased popularity on Github in recent months, I decided to start preparing the release of the first proper version of this project (v1.0). I would like to: add a wiki page, license, refactored structure and, most importantly, clean all the previous functionalities and make them ready to be used by the public. In addition, since I started working on this project, I've learned a lot of concepts that are used in the industry and I would like to bring some of them to v1.0: i.e., graphics API abstraction which does not exist here at the moment.  Currently, if you pull master, things will work and you will be able to run the project and do whatever you want, however, a lot will change with v1.0 that should come out around summer 2022. The engine grew a lot with years and became a bit messy, although it still is (and will be) a rendering engine (not a game engine). 
***

# Some of the rendering features
- Deferred + Forward Rendering
- Local illumination w/ Physically Based Rendering
- Global illumination w/:
- - Static: Image Based Lighting from light probes:
- - - diffuse (2nd order spherical harmonics)
- - - specular (cubemaps)
- - Dynamic: Cascaded Voxel Cone Tracing (AO, diffuse, specular)
- Cascaded Shadow Mapping
- Parallax-Occlusion Mapping w/ soft self-shadowing
- Separable Subsurface Scattering
- Terrain w/ GPU tessellation
- Foliage
- Volumetric clouds
- Volumetric fog
- Post Processing: Linear Fog, SSR, Tonemap, LUT color grading, Vignette, FXAA

# Some of the engine concepts/features
- Concept of an RHI (aka "Rendering Hardware Interface"): graphics API is abstracted from the general code (systems, etc.)
- Concept of a simplified generic game object - "ER_RenderingObject":
- - supports model loading (.obj, .fbx and etc.) with Assimp Library
- - supports multiple meshes
- - supports texture loading (.png, .jpg, .dds, .tga)
- - supports materials
- - supports GPU instancing
- - supports LOD groups
- - supports AABB (with visualization)
- - supports on-terrain procedural placement & instances distribution
- - customizable via "Object editor" (with instancing support)
- Concept of a generic scene, which contains "ER_RenderingObject" elements + scene data (lights, terrain, GI and other info):
- - supports loading from & saving to JSON scene files
- CPU frustum culling
- ImGUI, ImGuizmo
 
# Roadmap (big architectural engine tasks)
 * [X] <del>remove DX11 "Effects" library, all .fx shaders and refactor the material system (DONE)</del> (https://github.com/steaklive/EveryRay-Rendering-Engine/pull/51)
 * [X] <del>remove all low-level DX11 code and put it into the abstracted RHI (DONE)</del> (https://github.com/steaklive/EveryRay-Rendering-Engine/pull/56)
 * [ ] add support for DX12
 * [ ] remove DirectXMath and its usages (maybe come up with a custom math lib)
 * [ ] add cross-API shader compiler
 * [ ] add simple job-system (i.e. for Update(), CPU culling, etc.)
 * [ ] add simple memory management system (for now CPU memory; at least linear, pool allocators)

# Roadmap (big graphics tasks)
 * [ ] Order Independent Transparency (in Forward pass)
 * [ ] Atmospheric Scattering (Mie, etc.)
 * [ ] Better Anti-Aliasing (SMAA, TAA, explore FSR/DLSS)

# Screenshots

![picture](screenshots/EveryRayTerrain.png)
![picture](screenshots/EveryRayMaterials.png)
![picture](screenshots/EveryRayWater.png)
![picture](screenshots/EveryRayCollisionDetection.png)
![picture](screenshots/EveryRayTestScene.png)

# Controls
- Mouse + Right Click - camera rotation
- WASD - camera side movement
- E/Q - camera up/down movement
- Backspace - enable editor
- R/T/Y - scale/translate/rotate object in the editor mode

# Notes
The framework is NOT API-agnostic and, thus, has been tighly bound to DX11 since the beginning of its development. Hopefully, I will be changing that in the future and adding DX12 support for it. It will undeniably improve the perfomance in many scenarios. 

You might increase TDR time of your GPU driver (explained here https://docs.substance3d.com/spdoc/gpu-drivers-crash-with-long-computations-128745489.html).

# External Dependencies
- DirectXTK (https://github.com/Microsoft/DirectXTK)
- ImGui (https://github.com/ocornut/imgui)
- Assimp 5.0.1 (https://github.com/assimp/assimp)
- JsonCpp (https://github.com/open-source-parsers/jsoncpp/)

# References
- "Real-Time 3D Rendering with DirectX and HLSL: A Practical Guide to Graphics Programming" by Paul Varcholik.
- "Real-Time Rendering" 3rd/4th ed. by Tomas Möller, Naty Hoffman, Eric Haines
- "GPU Gems" series from NVIDIA
- "GPU Pro" series by Wolfgang Engel
- numerous SIGGRAPH, GDC papers and blogposts by fellow graphics geeks and vendors :)
 
# Requirements
- Visual Studio 2017
- Windows 10 + SDK
- DirectX 11 supported hardware
