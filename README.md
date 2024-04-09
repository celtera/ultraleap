# Ultraleap Leap Motion Controller Max external

Compatible with the Leap Motion Controller 1 & 2, Ultraleap 3Di Stereo,  IR 170 Evaluation Kit.

Requires [Gemini: Ultraleap Hand Tracking Software](https://leap2.ultraleap.com/gemini-downloads/)


Build for [Max 8](https://cycling74.com/products/max) for Windows, Mac intel & arm64.



This external returns :
```
frame_info : frame_id, left & right hands tracking status, device_framerate
hands (palms) : position(xyz), orientation(quat), velocity(xyz), pinch, grab
Fingers (tips) : position(xyz), orientation(quat), velocity(xyz), fingerExtended, fingerLength
Bones : prev_joint(xyz), bone_orientation(quat), next_joint(xyz), bone_width, bone_length
```
Multiple devices support : select device by index or serial number.

![ultraleap screenshot](/docs/ultraleap-screenshot.png?raw=true)
![ultraleap screenshot](/docs/ultraleap-screenshot2.png?raw=true)

This external is built thanks to the [Avendish](https://github.com/celtera/avendish) library, which allows (among other amazing things) automatically generation of Max/MSP and PureData objects.



## TODO

- Output stereo camera images (jitter matrix / texture)

