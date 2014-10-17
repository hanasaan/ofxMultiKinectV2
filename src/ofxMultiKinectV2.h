//
//  ofxMultiKinectV2
//
//  Created by Yuya Hanai on 10/16/14.
//

#pragma once
#include "ofMain.h"

//#define USE_OFX_TURBO_JPEG

#ifdef USE_OFX_TURBO_JPEG
#include "ofxTurboJpeg.h"
#endif

class DepthProcessor;
class ofProtonect2;

class ofxMultiKinectV2 : public ofThread
{
public:
    ofxMultiKinectV2();
    ~ofxMultiKinectV2();
    
    int getDeviceCount();
    void open(bool enableColor = true, bool enableIr = true, int deviceIndex = 0);
    void start();
    void update();
    void close();
    
    bool isFrameNew();

    ofPixels& getColorPixelsRef();
    ofTexture& getDepthTextureRef();
    ofTexture& getIrTextureRef();
    const vector<char>& getJpegBuffer();
    
    void setEnableJpegDecode(bool b) {bEnableJpegDecode = b;}
    bool isEnableJpegDecode() {return bEnableJpegDecode;}
protected:
    void threadedFunction();
    
    bool bEnableJpegDecode;
    bool bOpened;
    bool bNewBuffer;
    bool bNewFrame;

    ofPixels colorPix;
    ofPixels colorPixFront;
    ofPixels colorPixBack;
    
    vector<char> jpeg;
    vector<char> jpegFront;
    vector<char> jpegBack;
    
    vector<char> ir;
    vector<char> irFront;
    vector<char> irBack;
    
    ofProtonect2* protonect2;
    DepthProcessor* depthProcessor;
    
    int lastFrameNo;
    
#ifdef USE_OFX_TURBO_JPEG
    ofxTurboJpeg turbo;
#endif

};