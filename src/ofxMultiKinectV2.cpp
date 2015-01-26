//
//  ofxMultiKinectV2
//
//  Created by Yuya Hanai on 10/16/14.
//
#include "ofxMultiKinectV2.h"
#include "ofProtonect2.h"

using namespace libfreenect2;

//------------------------------------------
ofxMultiKinectV2::ofxMultiKinectV2()
{
    protonect2 = new ofProtonect2();
	
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

int ofxMultiKinectV2::getDeviceCount()
{
	ofProtonect2* protonect2 = new ofProtonect2();
	int cnt = protonect2->getDeviceCount();
	delete protonect2;
	return cnt;
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
    
    bool ret = protonect2->open(deviceIndex, mode, oclDeviceIndex);
    
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
		
		if (protonect2->getIrBuffer().size() == 512 * 424 * 4) {
			irPixBack.setFromPixels(reinterpret_cast<const float*>(&protonect2->getIrBuffer().front()), 512, 424, 1);
		}
		if (protonect2->getDepthBuffer().size() == 512 * 424 * 4) {
			depthPixBack.setFromPixels(reinterpret_cast<const float*>(&protonect2->getDepthBuffer().front()), 512, 424, 1);
		}
        if (bEnableJpegDecode && jpegBack.size()) {
            ofBuffer tmp;
            tmp.set(&jpegBack.front(), jpegBack.size());
#ifdef USE_OFX_TURBO_JPEG
            turbo.load(tmp, colorPixBack);
#else
            ofLoadImage(colorPixBack, tmp);
#endif
        }
        
        lock();
        jpegFront.swap(jpegBack);
        irPixFront.swap(irPixBack);
		depthPixFront.swap(depthPixBack);
        if (bEnableJpegDecode && jpegBack.size()) {
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