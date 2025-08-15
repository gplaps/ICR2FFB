This is a work in progress state of how to structure this project more towards what C++ was made for ... structs and classes

What do we have:
IniReader / FFBConfig

Inputs: TelemetryReader
Processing: FFBProcessor
Output: FFBOutput

FFBOutput consists of a DI_Device, that needs the DirectInput API to be setup, so each device tries to initialize DirectInput if not done already

Then pass around references to the FFBConfig object so the "modules" can read data from it ... as its unlikely that they change any, keep it const ref

The seperation between FFBOutput and FFBDevice and effects is still not good!
