This is an attempt at a structure for this project

What do we have:
Config:     FFBConfig
Inputs:     TelemetryReader
Processing: FFBProcessor
Output:     FFBOutput and FFBDevice

FFBOutput contains the FFBDevice, that handles the DirectInput API - so each device (if there where multiple) tries to initialize DirectInput if not done already,
The FFBConfig gets passed around by references so the "modules" can read data from it, assuming no changes can be done to it at runtime

The seperation between FFBProcessor, FFBOutput and effect processing is still not good! FFBDevice and DirectInput API move is done, but where effects reside either in FFBProcessor or FFBOutput needs to be structured better.

Code style:
- windows.h should only be included via - currently named "project_dependencies.h" - to get a consistent set of funtionality with NOMINMAX and WIN32_LEAN_AND_MEAN macros set and handling of compiler differences
- As this is a project for an old game this refactor prefers compatibility to C++ closer to how it was available back then, so it could be possible to build for old Windows versions that the game runs on
- Its not tested with ancient compiler versions though, "only" with current version in C++98 mode. 
- Some expressions are admittedly more compact and elegant with current versions of the language, e.g. auto types and braced initializers. Also initializing in declarations instead of (every) constructors is more sane and more robust against bugs due to a missing initialization statement 
