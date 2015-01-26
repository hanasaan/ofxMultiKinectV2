ofxMultiKinectV2
================

An addon for multiple Kinect For Windows V2 sensors.

- Based on the excellent work by the https://github.com/OpenKinect/libfreenect2 team ( @JoshBlake @floe and @christiankerl plus others )
- Also based on the excellent work by Theo Watson's ofxKinectV2 https://github.com/ofTheo/ofxKinectV2

New Features (Jan 2015):
- Ported OpenCL implementation, and all old APIs are removed.
- No need to copy assets to bin/data.

Features:
- More than one Kinect V2 to one Mac
- 30fps depth decoding using GPU

Notes:
- The requirement is almost same as ofxKinectV2 https://github.com/ofTheo/ofxKinectV2
- Tested two Kinect V2 to MBPR (Mid 2012).
  - Some depth packets are dropped when connecting two Kinect V2 to built-in USB3 ports.
  - Works well when connecting one Kinect V2 to built-in port meanwhile another Kinect V2 to CalDigit.
  
