//
//  Created by Yuya Hanai on 10/16/14.
//

#pragma once

#include "ofMain.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/packet_pipeline.h>

class ofProtonect2 {
public:
    ofProtonect2();
    bool open(const std::string &serial, int mode = libfreenect2::Frame::Depth | libfreenect2::Frame::Ir | libfreenect2::Frame::Color, int clindex = -1);
    void start(); // controllig this is important for avoiding interference...?
    void update();
    void close();
    bool isOpen() {return bOpen;}
    const vector<char>& getJpegBuffer() const {
        return jpeg;
    }
    const vector<char>& getIrBuffer() const {
        return rawir;
    }
    const vector<char>& getDepthBuffer() const {
        return rawdepth;
    }
    int getDeviceCount() {return freenect2.enumerateDevices();}
    libfreenect2::Freenect2Device::ColorCameraParams getColorCameraParams() {
        if (!dev) {return;}
        return dev->getColorCameraParams();
    }
    libfreenect2::Freenect2Device::IrCameraParams getIrCameraParams() {
        if (!dev) {return;}
        return dev->getIrCameraParams();
    }
protected:
    libfreenect2::FrameMap frames;
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *dev;
    libfreenect2::SyncMultiFrameListener *listener;
    libfreenect2::PacketPipeline *pipeline;
    bool bOpen;
    vector<char> jpeg;
    vector<char> rawir;
    vector<char> rawdepth;
};