# Static SDF shapes for Box2D v3
https://telegra.ph/SDF-From-Graphics-to-Physics-06-07

This is an experiment in integrating SDF (signed distance field) into Box2D 3.1.1 (tagged release). The collisions work quite well for circles: they do not fall through the ground; they can be stacked into a stable tower; they slide if friction is low, or roll if friction is high; bodies go to sleep. The functionality that requires calculating the distance between shapes does not work: OverlapShape, CastShape, continuous collision, sensors, and who knows what else.

I recommend reading added code (in GitHub Desktop, for example, select all commits from 'SDF terrain shape' to the most recent to see the combined changes).

The SDF.h file contains functions I use in my game. It requires the GLM library and at least C++20. The ParseSVG.js file contains a standalone JavaScript script for parsing an SVG file into a format compatible with SDF::svg.
# TODO
- Shape cast of circle (circle cast) vs. exact SDF is simply a raycast with `d - radius`.
- Making the gradient computation a user-defined callback can be useful https://iquilezles.org/articles/distgradfunctions2d/. Also bounding boxes https://iquilezles.org/articles/bboxes2d/
- Implement kinematic SDF bodies. To do this, first ensure that the body transform is not ignored.
- bump box2d to latest tagged release
- https://www.shadertoy.com/view/WstcW4 moving distance field (using dt on gradiant)

![Box2D Logo](https://box2d.org/images/logo.svg)

# Build Status
[![Build Status](https://github.com/erincatto/box2d/actions/workflows/build.yml/badge.svg)](https://github.com/erincatto/box2d/actions)

# Box2D 
Box2D is a 2D physics engine for games.

[![Box2D Version 3.0 Release Demo](https://img.youtube.com/vi/dAoM-xjOWtA/0.jpg)](https://www.youtube.com/watch?v=dAoM-xjOWtA)

## Features

### Collision
- Continuous collision detection
- Contact events
- Convex polygons, capsules, circles, rounded polygons, segments, and chains
- Multiple shapes per body
- Collision filtering
- Ray casts, shape casts, and overlap queries
- Sensor system

### Physics
- Robust _Soft Step_ rigid body solver
- Continuous physics for fast translations and rotations
- Island based sleep
- Revolute, prismatic, distance, mouse joint, weld, and wheel joints
- Joint limits, motors, springs, and friction
- Joint and contact forces
- Body movement events and sleep notification

### System
- Data-oriented design
- Written in portable C17
- Extensive multithreading and SIMD
- Optimized for large piles of bodies

### Samples
- OpenGL with GLFW and enkiTS
- Graphical user interface with imgui
- Many samples to demonstrate features and performance

## Building for Visual Studio
- Install [CMake](https://cmake.org/)
- Ensure CMake is in the user `PATH`
- Run `create_sln.bat`
- Open and build `build/box2d.sln`

## Building for Linux
- Run `build.sh` from a bash shell
- Results are in the build sub-folder

## Building for Xcode
- Install [CMake](https://cmake.org)
- Add Cmake to the path in .zprofile (the default Terminal shell is zsh)
    - export PATH="/Applications/CMake.app/Contents/bin:$PATH"
- mkdir build
- cd build
- cmake -G Xcode ..
- Open `box2d.xcodeproj`
- Select the samples scheme
- Build and run the samples

## Building and installing
- mkdir build
- cd build
- cmake ..
- cmake --build . --config Release
- cmake --install . (might need sudo)

## Compatibility
The Box2D library and samples build and run on Windows, Linux, and Mac.

You will need a compiler that supports C17 to build the Box2D library.

You will need a compiler that supports C++20 to build the samples.

Box2D uses SSE2 and Neon SIMD math to improve performance. This can be disabled by defining `BOX2D_DISABLE_SIMD`.

## Documentation
- [Manual](https://box2d.org/documentation/)
- [Migration Guide](https://github.com/erincatto/box2d/blob/main/docs/migration.md)

## Community
- [Discord](https://discord.gg/NKYgCBP)

## Contributing
Please do not submit pull requests. Instead, please file an issue for bugs or feature requests. For support, please visit the Discord server.

# Giving Feedback
Please file an issue or start a chat on discord. You can also use [GitHub Discussions](https://github.com/erincatto/box2d/discussions).

## License
Box2D is developed by Erin Catto and uses the [MIT license](https://en.wikipedia.org/wiki/MIT_License).

## Sponsorship
Support development of Box2D through [Github Sponsors](https://github.com/sponsors/erincatto).

Please consider starring this repository and subscribing to my [YouTube channel](https://www.youtube.com/@erin_catto).

## External ports, wrappers, and bindings (unsupported)
- Beef bindings - https://github.com/EnokViking/Box2DBeef
- C++ bindings - https://github.com/HolyBlackCat/box2cpp
- WASM - https://github.com/Birch-san/box2d3-wasm