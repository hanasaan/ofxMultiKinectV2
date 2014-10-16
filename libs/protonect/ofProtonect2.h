//
//  Created by Yuya Hanai on 10/16/14.
//

#pragma once

#include "ofMain.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/threading.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/protocol/response.h>

class Freenect2Instance {
public:
    static libfreenect2::Freenect2& getFreenect2() {
        static libfreenect2::Freenect2 freenect2;
        return freenect2;
    }
};

class ofProtonect2 {
public:
    ofProtonect2() :
    dev(NULL), listener(NULL), bOpen(false), pipeline(NULL), freenect2(&Freenect2Instance::getFreenect2()) {}
    bool open(int deviceIndex = 0, int mode = libfreenect2::Frame::Ir | libfreenect2::Frame::Color);
    void update();
    void close();
    bool isOpen() {return bOpen;}
    const vector<char>& getJpegBuffer() const {
        return jpeg;
    }
    const vector<char>& getIrBuffer() const {
        return rawir;
    }
    void loadP0Texture(ofTexture* tex) const;
protected:
    libfreenect2::FrameMap frames;
    libfreenect2::Freenect2* freenect2;
    libfreenect2::Freenect2Device *dev;
    libfreenect2::SyncMultiFrameListener *listener;
    libfreenect2::PacketPipeline *pipeline;
    bool bOpen;
    vector<char> jpeg;
    vector<char> rawir;
};