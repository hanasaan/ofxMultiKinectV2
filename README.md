ofxMultiKinectV2
================

Connecting more than one Kinect For Windows V2 sensor to one Supports only OSX. Based on the excellent work by

- The [libfreenect2](https://github.com/OpenKinect/libfreenect2) team ( @JoshBlake @floe and @christiankerl plus others )
- The [ofxKinectV2](https://github.com/ofTheo/ofxKinectV2) addon by [Theo Watson](https://github.com/ofTheo)


### New Features (Jan 2015)

- Ported OpenCL implementation, and all old APIs are removed.
- No need to copy assets to bin/data.

### Features

- More than one Kinect V2 to one Mac
- 30fps depth decoding using GPU

### Xcode Project Setup

Add these libraries/frameworks to **Build Phases > Linked Frameworks and Libraries**

- `OpenCL.framework`
- `usb-1.0.0-superspeed.a` from `ofxMultiKinectV2/libs/libusb/lib/osx`
- `libturbojpeg.dylib` from `ofxTurboJpeg/libs/turbo-jpeg/lib/osx`

Add these libraries/frameworks to **Build Phases > Copy Files** and make sure the Desination is set to Frameworks

- `libturbojpeg.dylib`

### Dependencies

- [ofxTurboJpeg](https://github.com/satoruhiga/ofxTurboJpeg)
  - This is using [Satoru Higa's](https://github.com/satoruhiga) fork
  - To disable ofxTurboJpeg, comment out (it might cause low fps) :
  
  `#define USE_OFX_TURBO_JPEG`

### Notes

- The requirement is almost same as ofxKinectV2 https://github.com/ofTheo/ofxKinectV2
  - Check the notes above first.
- Tested environments: 
  - MacBookProRetina (Mid2012) + OSX Yosemite (v 10.10.1) + oF v0.8.4 + Kinect V2 x2 (retail version)  
	  - Some depth packets are dropped when connecting two Kinect V2 to built-in USB3 ports.
	  - Works well when connecting one Kinect V2 to built-in port meanwhile another Kinect V2 to CalDigit.
  - Also tested on MacBookProRetina (Mid2014) + OSX Yosemite (v 10.10.5) + oF pre-0.9 + KinectV2 x2 (retail + dev version)  
- If you're not seeing data out of the Kinect and see `Failed to reset Kinect` messages in the console,
  see a workaround in [this issue from libfreenect2](https://github.com/OpenKinect/libfreenect2/issues/31#issuecomment-58154847)