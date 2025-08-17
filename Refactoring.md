This is a work in progress state of how to structure this project

What do we have:
IniReader / FFBConfig

Inputs: TelemetryReader
Processing: FFBProcessor
Output: FFBOutput

FFBOutput consists of a FFBDevice, that handles the DirectInput API - so each device (if there where multiple) tries to initialize DirectInput if not done already,
The FFBConfig gets passed around by references so the "modules" can read data from it, assuming no changes can be done to it at runtime

The seperation between FFBProcessor, FFBOutput and FFBDevice and effect processing is still not good! DirectInput API move is done, but where effects reside either in FFBProcessor or FFBOutput needs to be structured better. Or remove FFBOtput and move all its functionality into FFBProcessor

Code style:
There are likely some unnecessary includes and IWYU is not applied yet. windows.h should only be included via - currently named "project_dependencies.h" to get a consistent set of funtionality with NOMINMAX and WIN32_LEAN_AND_MEAN macros set
