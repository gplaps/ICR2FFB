This is a work in progress state of how to structure this project

What do we have:
IniReader / FFBConfig

Inputs: TelemetryReader
Processing: FFBProcessor
Output: FFBOutput

FFBOutput consists of a DI_Device, that needs the DirectInput API to be setup, so each device tries to initialize DirectInput if not done already
Then pass around references to the FFBConfig object so the "modules" can read data from it ... as its unlikely that they change any, keep it const ref

The seperation between FFBProcessor, FFBOutput and FFBDevice and effects is still not good! DirectInput API move is done, but where effects reside either in FFBProcessor or FFBOutput ... not sure yet. Or remove FFBOtput and move its functionality into FFBProcessor also

Code style:
There are likely some unnecessary includes. be cautious when including windows api headers to prefer project_dependencies.h to get a consistent set with NOMINMAX and WIN32_LEAN_AND_MEAN
Reducing braces around if/else with only one line of content is a matter of taste, I like this better, but opionions are different. Same with preferring the tenarity operator, but this depends on the situation and may reduce readability
