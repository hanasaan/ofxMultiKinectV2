ofxMultiKinectV2
================

Connecting more than one Kinect For Windows V2 sensor to one Supports only OSX. Based on the excellent work by

- The [libfreenect2](https://github.com/OpenKinect/libfreenect2) team ( @JoshBlake @floe and @christiankerl plus others )
- The [ofxKinectV2](https://github.com/ofTheo/ofxKinectV2) addon by [Theo Watson](https://github.com/ofTheo)

### Compatibility
- oF 0.9.0 or newer
- example projects are generated with oF 0.9.3 project generator

### Features
- More than one Kinect V2 to one Mac
- 30fps depth decoding using GPU (OpenCL)
- OpenGL depth to color registration (experimental)

### Xcode Project Setup

Add these libraries/frameworks to **Build Phases > Linked Frameworks and Libraries**

- `OpenCL.framework`
- `usb-1.0.0-superspeed.a` from `ofxMultiKinectV2/libs/libusb/lib/osx`
- `libturbojpeg.dylib` from `ofxTurboJpeg/libs/turbo-jpeg/lib/osx`

Add these libraries/frameworks to **Build Phases > Copy Files** and make sure the Desination is set to Frameworks

- `libturbojpeg.dylib`

### Dependencies

- [ofxTurboJpeg](https://github.com/armadillu/ofxTurboJpeg)
  - Now we can use original master instead of Satoru Higa's fork (Apr 2016)	

  `#define USE_OFX_TURBO_JPEG`

### Notes

- The requirement is almost same as ofxKinectV2 https://github.com/ofTheo/ofxKinectV2
  - Check the notes above first.
- Tested environments: 
  - MacBookProRetina (Mid2012) + OSX Yosemite (v 10.10.1) + oF v0.8.4 + Kinect V2 x2 (retail version)  
	  - Some depth packets are dropped when connecting two Kinect V2 to built-in USB3 ports.
	  - Works well when connecting one Kinect V2 to built-in port meanwhile another Kinect V2 to CalDigit.
  - MacBookProRetina (Mid2014) + OSX Yosemite (v 10.10.5) + oF pre-0.9 + KinectV2 x2 (retail + dev version)
  - Mac Pro (Late2013) + OSX Mavericks (v 10.9.5) + oF v 0.9.0 + Kinect V2 x5
  	- On Mac Pro + Yosemite or El Capitan, currently this addon will not work! Please downgrade OS to Mavericks if you want to use this addon on Mac Pro.
- If you're not seeing data out of the Kinect and see `Failed to reset Kinect` messages in the console,
  see a workaround in [this issue from libfreenect2](https://github.com/OpenKinect/libfreenect2/issues/31#issuecomment-58154847)
