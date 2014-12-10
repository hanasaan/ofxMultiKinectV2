//
//  ofxMultiKinectV2
//
//  Created by Yuya Hanai on 10/16/14.
//
#include "ofxMultiKinectV2.h"
#include "ofProtonect2.h"

using namespace libfreenect2;

class DepthProcessor {
protected:
    void updateShaderParameters(ofShader& program){
        if (!params_need_update) return;
        
        program.setUniform1f("Params.ab_multiplier", params.ab_multiplier);
        program.setUniform3fv("Params.ab_multiplier_per_frq", params.ab_multiplier_per_frq);
        program.setUniform1f("Params.ab_output_multiplier", params.ab_output_multiplier);
        
        program.setUniform3fv("Params.phase_in_rad", params.phase_in_rad);
        
        program.setUniform1f("Params.joint_bilateral_ab_threshold", params.joint_bilateral_ab_threshold);
        program.setUniform1f("Params.joint_bilateral_max_edge", params.joint_bilateral_max_edge);
        program.setUniform1f("Params.joint_bilateral_exp", params.joint_bilateral_exp);
        
        {
            GLint idx = glGetUniformLocation(program.getProgram(), "Params.gaussian_kernel");
            if(idx != -1) {
                glUniformMatrix3fv(idx, 1, false, params.gaussian_kernel);
            }
        }
        
        program.setUniform1f("Params.phase_offset", params.phase_offset);
        program.setUniform1f("Params.unambigious_dist", params.unambigious_dist);
        program.setUniform1f("Params.individual_ab_threshold", params.individual_ab_threshold);
        program.setUniform1f("Params.ab_threshold", params.ab_threshold);
        program.setUniform1f("Params.ab_confidence_slope", params.ab_confidence_slope);
        program.setUniform1f("Params.ab_confidence_offset", params.ab_confidence_offset);
        program.setUniform1f("Params.min_dealias_confidence", params.min_dealias_confidence);
        program.setUniform1f("Params.max_dealias_confidence", params.max_dealias_confidence);
        
        program.setUniform1f("Params.edge_ab_avg_min_value", params.edge_ab_avg_min_value);
        program.setUniform1f("Params.edge_ab_std_dev_threshold", params.edge_ab_std_dev_threshold);
        program.setUniform1f("Params.edge_close_delta_threshold", params.edge_close_delta_threshold);
        program.setUniform1f("Params.edge_far_delta_threshold", params.edge_far_delta_threshold);
        program.setUniform1f("Params.edge_max_delta_threshold", params.edge_max_delta_threshold);
        program.setUniform1f("Params.edge_avg_delta_threshold", params.edge_avg_delta_threshold);
        program.setUniform1f("Params.max_edge_count", params.max_edge_count);
        
        program.setUniform1f("Params.min_depth", params.min_depth);
        program.setUniform1f("Params.max_depth", params.max_depth);
    }
    
    ofBuffer x;
    ofBuffer z;
    ofBuffer lut;
    
    ofTexture lut11to16;
    ofTexture p0table[3];
    ofTexture x_table;
    ofTexture z_table;
    
    ofTexture input_data_1;
    ofTexture input_data_2;
    
    ofTexture stage1_infrared;
    ofFbo stage1_fbo;
    ofFbo filter1_fbo;
    
    ofFbo stage2_fbo;
    ofFbo filter2_fbo;
    
    ofMesh quad;
    
    ofShader stage1, stage2, filter1, filter2;
    
    ofShortPixels inputPix1;
    ofShortPixels inputPix2;
    
    uint64_t ts;
    uint64_t t_acc;
    int t_acc_n;
    
    bool params_need_update;
    
    DepthPacketProcessor::Parameters params;
    
    bool bEnableBilateralFilter;
    bool bEnableEdgeAwareFilter;
    bool bSetup;
    
public:
    DepthProcessor() {
        params_need_update = true;
        bEnableBilateralFilter = true;
        bEnableEdgeAwareFilter = false;
        bSetup = false;
    }
    
    void setup() {
        // load bins
        ofBuffer lut = ofBufferFromFile("11to16.bin");
        unsigned short* ushortBuffer = new unsigned short[2048];
        for (int i=0; i<2048; ++i) {
            short* ptr = &reinterpret_cast<short*>(lut.getBinaryBuffer())[i];
            int intval = *ptr;
            intval += 32768;
            ushortBuffer[i] = (unsigned short) intval;
        }
        
        lut11to16.loadData(ushortBuffer, 2048, 1, GL_LUMINANCE);
        lut11to16.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        
        delete [] ushortBuffer;
        
        x = ofBufferFromFile("xTable.bin");
        x_table.loadData(reinterpret_cast<float*>(x.getBinaryBuffer()), 512, 424, GL_LUMINANCE);
        x_table.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        
        z = ofBufferFromFile("zTable.bin");
        z_table.loadData(reinterpret_cast<float*>(z.getBinaryBuffer()), 512, 424, GL_LUMINANCE);
        z_table.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        
        input_data_1.allocate(352, 424 * 5, GL_R16);
        input_data_1.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        
        input_data_2.allocate(352, 424 * 5, GL_R16);
        input_data_2.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        
		stage1_infrared.allocate(512, 424, GL_RGB, ofGetUsingArbTex(), GL_LUMINANCE, GL_UNSIGNED_SHORT);
		
        ofFbo::Settings settings;
        settings.width = 512;
        settings.height = 424;
        settings.numColorbuffers = 3;
        settings.internalformat = GL_RGB32F_ARB;
        settings.maxFilter = GL_NEAREST;
        settings.minFilter = GL_NEAREST;
        stage1_fbo.allocate(settings);
        stage1_fbo.attachTexture(stage1_infrared, GL_R32F, 3);
        
        stage2_fbo.allocate(512, 424, GL_R16);
        stage2_fbo.getTextureReference().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
        stage2_fbo.createAndAttachTexture(GL_RG32F, 1);
        
        settings.numColorbuffers = 2;
        filter1_fbo.allocate(settings);
        filter1_fbo.createAndAttachTexture(GL_R8, 2);
        
        filter2_fbo.allocate(512, 424, GL_R16);
        
        stage1.load("", "shader/stage1.fs");
        stage2.load("", "shader/stage2.fs");
        filter1.load("", "shader/filter1.fs");
        filter2.load("", "shader/filter2.fs");
        
        quad.getVertices().resize(4);
        quad.getTexCoords().resize(4);
        quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
        
        GLfloat px0 = 0;
        GLfloat py0 = 0;
        GLfloat px1 = 512;
        GLfloat py1 = 424;
        
        GLfloat tx0 = 0;
        GLfloat ty0 = 0;
        GLfloat tx1 = 512;
        GLfloat ty1 = 424;
        
        quad.getVertices()[0].set(px0, py0, 0);
        quad.getVertices()[1].set(px1, py0, 0);
        quad.getVertices()[2].set(px1, py1, 0);
        quad.getVertices()[3].set(px0, py1, 0);
        
        quad.getTexCoords()[0].set(tx0, ty0);
        quad.getTexCoords()[1].set(tx1, ty0);
        quad.getTexCoords()[2].set(tx1, ty1);
        quad.getTexCoords()[3].set(tx0, ty1);
        
        inputPix1.allocate(352, 424 * 5, 1);
        inputPix2.allocate(352, 424 * 5, 1);
        
        stage1_fbo.begin();
        stage1_fbo.activateAllDrawBuffers();
        stage1_fbo.end();
        
        stage2_fbo.begin();
        stage2_fbo.activateAllDrawBuffers();
        stage2_fbo.end();
        
        filter1_fbo.begin();
        filter1_fbo.activateAllDrawBuffers();
        filter1_fbo.end();
        
        stage1.begin();
        stage1.setUniformTexture("Lut11to16", lut11to16, 13);
        stage1.setUniformTexture("ZTable", z_table, 15);
        stage1.end();
        
        stage2.begin();
        stage2.setUniformTexture("XTable", x_table, 14);
        stage2.setUniformTexture("ZTable", z_table, 15);
        stage2.end();
        
        bSetup = true;
    }
    
    bool isSetup() {
        return bSetup;
    }
    
    ofTexture* getP0Texture() {
        return p0table;
    }
    
    
    void update(const char* buffer) {
        // Work around for Intel HD Graphics
        inputPix1.setFromPixels(reinterpret_cast<const unsigned short*>(buffer), 352, 424 * 5, 1);
        inputPix2.setFromPixels(reinterpret_cast<const unsigned short*>(buffer) + (352 * 424 * 5), 352, 424 * 5, 1);
        
        input_data_1.loadData(inputPix1);
        input_data_2.loadData(inputPix2);
        
        
        // stage 1
        stage1_fbo.begin();
        ofClear(0);
        stage1.begin();
        {
            updateShaderParameters(stage1);
            stage1.setUniformTexture("P0Table0", p0table[0], 0);
            stage1.setUniformTexture("P0Table1", p0table[1], 1);
            stage1.setUniformTexture("P0Table2", p0table[2], 2);
            stage1.setUniformTexture("Data1", input_data_1, 4);
            stage1.setUniformTexture("Data2", input_data_2, 5);
            quad.draw();
        }
        stage1.end();
        stage1_fbo.end();
        
        // filter 1
        if (bEnableBilateralFilter) {
            filter1_fbo.begin();
            ofClear(0);
            filter1.begin();
            {
                updateShaderParameters(filter1);
                filter1.setUniformTexture("A", stage1_fbo.getTextureReference(0), 0);
                filter1.setUniformTexture("B", stage1_fbo.getTextureReference(1), 1);
                filter1.setUniformTexture("Norm", stage1_fbo.getTextureReference(2), 2);
                quad.draw();
            }
            filter1.end();
            filter1_fbo.end();
        }
        
        // stage 2
        stage2_fbo.begin();
        ofClear(0);
        stage2.begin();
        {
            updateShaderParameters(stage2);
            if (bEnableBilateralFilter) {
                stage2.setUniformTexture("A", filter1_fbo.getTextureReference(0), 0);
                stage2.setUniformTexture("B", filter1_fbo.getTextureReference(1), 1);
            } else {
                stage2.setUniformTexture("A", stage1_fbo.getTextureReference(0), 0);
                stage2.setUniformTexture("B", stage1_fbo.getTextureReference(1), 1);
            }
            quad.draw();
        }
        stage2.end();
        stage2_fbo.end();
        
        // filter 2
        if (bEnableEdgeAwareFilter) {
            filter2_fbo.begin();
            ofClear(0);
            filter2.begin();
            {
                updateShaderParameters(filter2);
                filter2.setUniformTexture("DepthAndIrSum", stage2_fbo.getTextureReference(1), 0);
                filter2.setUniformTexture("MaxEdgeTest", filter1_fbo.getTextureReference(2), 1);
                quad.draw();
            }
            filter2.end();
            filter2_fbo.end();
        }
        
        params_need_update = false;
    }
    
    ofTexture& getDepthTexture() {
        if (bEnableEdgeAwareFilter) {
            return filter2_fbo.getTextureReference();
        } else {
            return stage2_fbo.getTextureReference();
        }
    }
    
    ofTexture& getIrTexture() {
        return stage1_infrared;
    }
};

//------------------------------------------
ofxMultiKinectV2::ofxMultiKinectV2()
{
    protonect2 = new ofProtonect2();
    depthProcessor = new DepthProcessor();
    
    bEnableJpegDecode = true;
    bOpened = false;
    bNewBuffer = false;
    bNewFrame = false;
    
    lastFrameNo = -1;
}

ofxMultiKinectV2::~ofxMultiKinectV2()
{
    close();
    delete depthProcessor;
    delete protonect2;
}

int ofxMultiKinectV2::getDeviceCount()
{
    return protonect2->getDeviceCount();
}

void ofxMultiKinectV2::open(bool enableColor, bool enableIr, int deviceIndex)
{
    close();
    
    if (!depthProcessor->isSetup()) {
        depthProcessor->setup();
    }
    
    bNewFrame  = false;
    bNewBuffer = false;
    bOpened    = false;
    
    int mode = 0;
    mode |= enableColor ? libfreenect2::Frame::Color : 0;
    mode |= enableIr ? libfreenect2::Frame::Ir : 0;
    
    bool ret = protonect2->open(deviceIndex, mode);
    
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
        irBack = protonect2->getIrBuffer();
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
        irFront.swap(irBack);
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
        ir = irFront;
        if (bEnableJpegDecode) {
            colorPix = colorPixFront;
        }
        unlock();
        
        protonect2->loadP0Texture(depthProcessor->getP0Texture());
        if (ir.size()) {
            depthProcessor->update(&ir.front());
        }
        
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

ofTexture& ofxMultiKinectV2::getDepthTextureRef() {
    return depthProcessor->getDepthTexture();
}


ofTexture& ofxMultiKinectV2::getIrTextureRef() {
    return depthProcessor->getIrTexture();
}

const vector<char>& ofxMultiKinectV2::getJpegBuffer() {
    return jpeg;
}