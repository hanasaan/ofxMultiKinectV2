//
//  ofxMultiKinectV2
//
//  Created by Yuya Hanai on 10/16/14.
//
#include "ofxMultiKinectV2.h"
#include "ofProtonect2.h"

using namespace libfreenect2;

class KinectV2DeviceManager
{
public:
    static KinectV2DeviceManager& getManager()
    {
        static KinectV2DeviceManager o;
        if (!o.initialized) {
            o.init();
        }
        
        return o;
    }
    
    void forceInit() {
        init();
    }
    
    int getDeviceCount() const {
        return device_count;
    }
    
    const vector<string>& getSerials() const {
        return serials;
    }

private:
    vector<string> serials;
    int device_count = -1;
    bool initialized = false;
    KinectV2DeviceManager() {}
    
    void init()
    {
        libfreenect2::Freenect2 freenect2;
        device_count = freenect2.enumerateDevices();
        serials.clear();
        for (int i = 0; i < device_count; i++){
            string serial = freenect2.getDeviceSerialNumber(i);
            serials.push_back(serial);
        }
        std::sort(serials.begin(), serials.end());
        for (auto&p : serials) {
            ofLogVerbose("KinectV2DeviceManager") << "sorted device id : " << p;
        }
        initialized = true;
    }
};

//------------------------------------------
ofxMultiKinectV2::ofxMultiKinectV2()
{
    protonect2 = new ofProtonect2();
    
    bEnableFlipBuffer = false;
    bEnableJpegDecode = true;
    bOpened = false;
    bNewBuffer = false;
    bNewFrame = false;
    
    lastFrameNo = -1;
}

ofxMultiKinectV2::~ofxMultiKinectV2()
{
    close();
    delete protonect2;
}

bool ofxMultiKinectV2::isOpen()
{
   if (protonect2 != NULL)
   {
       return protonect2->isOpen();
   }
    
    return false;
}
int ofxMultiKinectV2::getDeviceCount()
{
    return KinectV2DeviceManager::getManager().getDeviceCount();
}

void ofxMultiKinectV2::open(bool enableColor, bool enableIr, int deviceIndex, int oclDeviceIndex)
{
    close();
    
    bNewFrame  = false;
    bNewBuffer = false;
    bOpened    = false;
    
    int mode = 0;
    mode |= enableColor ? libfreenect2::Frame::Color : 0;
    mode |= enableIr ? libfreenect2::Frame::Ir | libfreenect2::Frame::Depth : 0;
    
    bool ret = protonect2->open(KinectV2DeviceManager::getManager().getSerials()[deviceIndex],
                                mode, oclDeviceIndex);
    
    if (!ret) {
        return;
    }
    
    lastFrameNo = -1;
    
    
    bOpened = true;
}

void ofxMultiKinectV2::start()
{
    if (!bOpened) {
        return;
    }
    protonect2->start();
    
    startThread();
}

void ofxMultiKinectV2::threadedFunction()
{
    while(isThreadRunning()){
        protonect2->update();
        jpegBack = protonect2->getJpegBuffer();
        
//        float ts = ofGetElapsedTimef();
        if (protonect2->getIrBuffer().size() == 512 * 424 * 4) {
            irPixBack.setFromPixels(reinterpret_cast<const float*>(&protonect2->getIrBuffer().front()), 512, 424, 1);
            if (bEnableFlipBuffer) {
                irPixBack.mirror(false, true);
            }
        }
        if (protonect2->getDepthBuffer().size() == 512 * 424 * 4) {
            depthPixBack.setFromPixels(reinterpret_cast<const float*>(&protonect2->getDepthBuffer().front()), 512, 424, 1);
            if (bEnableFlipBuffer) {
                depthPixBack.mirror(false, true);
            }
        }
        if (bEnableJpegDecode && jpegBack.size()) {
            ofBuffer tmp;
            tmp.set(&jpegBack.front(), jpegBack.size());
#ifdef USE_OFX_TURBO_JPEG
            turbo.load(colorPixBack, tmp);
#else
            ofLoadImage(colorPixBack, tmp);
#endif
            if (bEnableFlipBuffer) {
                colorPixBack.mirror(false, true);
            }
        }
//        float t = ofGetElapsedTimef() - ts;
//        cerr << 1000.0*t << "ms" << endl;
        
        lock();
        jpegFront.swap(jpegBack);
        irPixFront.swap(irPixBack);
        depthPixFront.swap(depthPixBack);
        if (bEnableJpegDecode && colorPixBack.isAllocated()) {
            colorPixFront.swap(colorPixBack);
        }
        bNewBuffer = true;
        unlock();
        
        ofSleepMillis(2);
    }
}

void ofxMultiKinectV2::update()
{
    if( ofGetFrameNum() != lastFrameNo ){
        bNewFrame = false;
        lastFrameNo = ofGetFrameNum();
    }
    if( bNewBuffer ){
        lock();
        bNewBuffer = false;
        jpeg = jpegFront;
        irPix = irPixFront;
        depthPix = depthPixFront;
        if (bEnableJpegDecode) {
            colorPix = colorPixFront;
        }
        unlock();
        bNewFrame = true;
    }

}

void ofxMultiKinectV2::close()
{
    if( bOpened ){
        waitForThread(true, 3000);
        protonect2->close();
        bOpened = false;
    }
}

bool ofxMultiKinectV2::isFrameNew()
{
    return bNewFrame;
}


ofPixels& ofxMultiKinectV2::getColorPixelsRef() {
    return colorPix;
}

ofFloatPixels& ofxMultiKinectV2::getDepthPixelsRef() {
    return depthPix;
}

ofFloatPixels& ofxMultiKinectV2::getIrPixelsRef() {
    return irPix;
}

const vector<char>& ofxMultiKinectV2::getJpegBuffer() {
    return jpeg;
}

float ofxMultiKinectV2::getDistanceAt(int x, int y) {
    if (!depthPix.isAllocated()) {
        return 0.0f;
    }
    return depthPix[x + y * depthPix.getWidth()] * 0.1; // mm to cm
}

// TODO: use undistorted
ofVec3f ofxMultiKinectV2::getWorldCoordinateAt(int x, int y) {
    return getWorldCoordinateAt(x, y, getDistanceAt(x, y));
}

ofVec3f ofxMultiKinectV2::getWorldCoordinateAt(int x, int y, float z) {
    libfreenect2::Freenect2Device::IrCameraParams p;
    ofVec3f world;
    if (this->getProtonect()) {
        p = this->getProtonect()->getIrCameraParams();
        world.z = z;
        world.x = (x - p.cx) * z / p.fx;
        world.y = -(y - p.cy) * z / p.fy;
    }
    return world;
}
