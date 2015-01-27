ofxMultiKinectV2
================

Connecting more than one Kinect For Windows V2 sensors to one Mac.

- Based on the excellent work by the https://github.com/OpenKinect/libfreenect2 team ( @JoshBlake @floe and @christiankerl plus others )
- Also based on the excellent work by Theo Watson's ofxKinectV2 https://github.com/ofTheo/ofxKinectV2
- Supports only OSX.

New Features (Jan 2015):
- Ported OpenCL implementation, and all old APIs are removed.
- No need to copy assets to bin/data.

Features:
- More than one Kinect V2 to one Mac
- 30fps depth decoding using GPU

Xcode Project setup:
- Add `OpenCL.framework` to Linked Frameworks and Libraries

Depends:
- ofxTurboJpeg
  - Satoru Higa's version: https://github.com/satoruhiga/ofxTurboJpeg
  - To disable ofxTurboJpeg, comment out (it might cause low fps) :
  
  `#define USE_OFX_TURBO_JPEG`

Notes:
- The requirement is almost same as ofxKinectV2 https://github.com/ofTheo/ofxKinectV2
  - Check the notes above first.
- Tested environment : 
  - MacBookProRetina (Mid2012) + OSX Yosemite (v 10.10.1) + oF v0.8.4 + Kinect V2 x2 (retail version)
  - Some depth packets are dropped when connecting two Kinect V2 to built-in USB3 ports.
  - Works well when connecting one Kinect V2 to built-in port meanwhile another Kinect V2 to CalDigit.
