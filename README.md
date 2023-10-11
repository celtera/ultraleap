# Ultraleap Leap Motion Controller Max external

Compatible with the Leap Motion Controller 1 & 2, Ultraleap 3Di Stereo,  IR 170 Evaluation Kit.

Requires [Gemini: Ultraleap Hand Tracking Software](https://leap2.ultraleap.com/gemini-downloads/)


Build for [Max 8](https://cycling74.com/products/max) for Windows, Mac intel & arm64.



This external returns :
```
frame_info : frame_id, left & right hands tracking status, device_framerate
hands (palms) : position(xyz), orientation(quat), velocity(xyz), pinch, grab
Fingers (tips) : position(xyz), orientation(quat), velocity(xyz), fingerExtended, fingerLength
```
![ultraleap screenshot](/docs/ultraleap-screenshot.png?raw=true)

This external is built thanks to the [Avendish](https://github.com/celtera/avendish) library, which allows (among other amazing things) automatically generation of Max/MSP and PureData objects.



## TODO

- Add a « poll » mechanism : send a bang to return latest frame (to sync data to jitter rendering / VR / limit frame rate…)
- Output all fingers joints / bones position & orientation (maybe as a dict, or jitter matrices ?…)
- Output stereo camera images (jitter matrix / texture)
- Support Multiple devices : Gemini allow multiple devices connected to the same computer. Select device by id.

## Known issues
Fingers velocity is currently not working (returns 0. 0. 0.)
Ultraleap sdk doesn’t returns fingers velocity, so it should be computed internally.
