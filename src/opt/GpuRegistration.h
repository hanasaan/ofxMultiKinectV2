//
//  Created by Yuya Hanai, https://github.com/hanasaan/
//
#pragma once

#include "ofMain.h"
#include "ofxMultiKinectV2.h"
#include "ofProtonect2.h"


class GpuRegistration
{
protected:
    ofFloatPixels depth_to_color_map;
    ofTexture depth_to_color_map_tex;
    ofTexture depth_tex;
    ofTexture color_tex;
    
    ofFbo ldepth_work_fbo;
    ofFbo ldepth_fbos[2];
    int ldepth_index;
    ofFbo regist_fbo;
    ofFbo masked_regist_fbo;
    ofShader regist_shader;
    
    ofVboMesh m;
    ofShader c_vert_shader;
    ofShader c_mask_shader;
    ofShader depth_fill_hole_shader;
public:
    // this must be called after kinect.start();
    void setup(ofProtonect2* protonect, float scale = 1.0f);

    void update(const ofFloatPixels& depth_pix, const ofPixels& color_pix, bool process_occlusion_mask = true);
    void update(ofTexture& depth_tex, ofTexture& color_tex, bool process_occlusion_mask = true);
    
    ofTexture& getRegisteredTexture() { return regist_fbo.getTextureReference(0); }
    ofTexture& getRegisteredTexture(bool occlusion_mask) {
        return occlusion_mask ? masked_regist_fbo.getTextureReference() : getRegisteredTexture();
    }
    
protected:
    void undistort_depth(float x, float y, float& mx, float& my);
    void depth_to_color(float mx, float my, float& rx, float& ry);
    
    libfreenect2::Freenect2Device::IrCameraParams depth;
    libfreenect2::Freenect2Device::ColorCameraParams color;
};