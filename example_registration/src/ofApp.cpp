#include "ofMain.h"
#include "ofxMultiKinectV2.h"

#define STRINGIFY(x) #x

static string depthFragmentShader =
STRINGIFY(
          uniform sampler2DRect tex;
          void main()
          {
              vec4 col = texture2DRect(tex, gl_TexCoord[0].xy);
              float value = col.r;
              float low1 = 500.0;
              float high1 = 5000.0;
              float low2 = 1.0;
              float high2 = 0.0;
              float d = clamp(low2 + (value - low1) * (high2 - low2) / (high1 - low1), 0.0, 1.0);
              if (d == 1.0) {
                  d = 0.0;
              }
              gl_FragColor = vec4(vec3(d), 1.0);
          }
          );

static string irFragmentShader =
STRINGIFY(
          uniform sampler2DRect tex;
          void main()
          {
              vec4 col = texture2DRect(tex, gl_TexCoord[0].xy);
              float value = col.r / 65535.0;
              gl_FragColor = vec4(vec3(value), 1.0);
          }
          );

#include "GpuRegistration.h"

//========================================================================
class ofApp : public ofBaseApp{
    ofShader depthShader;
    ofShader irShader;
    
    ofxMultiKinectV2 kinect0;
    ofTexture colorTex0;
    ofTexture depthTex0;
    ofTexture irTex0;
    GpuRegistration gr;
    
    bool process_occlusion;
public:
    
    void setup()
    {
        process_occlusion = true;
        
        ofSetVerticalSync(true);
        ofSetFrameRate(60);
        
        depthShader.setupShaderFromSource(GL_FRAGMENT_SHADER, depthFragmentShader);
        depthShader.linkProgram();
        
        irShader.setupShaderFromSource(GL_FRAGMENT_SHADER, irFragmentShader);
        irShader.linkProgram();
        
        kinect0.open(true, true, 0);
        // Note :
        // Default OpenCL device might not be optimal.
        // e.g. Intel HD Graphics will be chosen instead of GeForce.
        // To avoid it, specify OpenCL device index manually like following.
        // kinect1.open(true, true, 0, 2); // GeForce on MacBookPro Retina
        
        kinect0.start();
        
        gr.setup(kinect0.getProtonect(), 2);
    }
    
    void update() {
        kinect0.update();
        if (kinect0.isFrameNew()) {
            colorTex0.loadData(kinect0.getColorPixelsRef());
            depthTex0.loadData(kinect0.getDepthPixelsRef());
            irTex0.loadData(kinect0.getIrPixelsRef());
            
            depthTex0.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            gr.update(depthTex0, colorTex0, process_occlusion);
        }
    }
    
    void draw()
    {
        ofClear(0);
        
        if (colorTex0.isAllocated()) {
            colorTex0.draw(0, 0, 640, 360);
        }
        if (depthTex0.isAllocated()) {
            
            if (ofGetKeyPressed(' ')) {
                gr.getRegisteredTexture(process_occlusion).draw(640, 0, 1024, 848);
            } else {
                irShader.begin();
                irTex0.draw(640, 0, 1024, 848);
                irShader.end();
                ofPushStyle();
                ofEnableAlphaBlending();
                ofSetColor(255, 80);
                gr.getRegisteredTexture(process_occlusion).draw(640, 0, 1024, 848);
                ofPopStyle();
            }
            
            depthShader.begin();
            depthTex0.draw(0, 424, 512, 424);
            depthShader.end();
        }
        
        ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate()), 10, 20);
        ofDrawBitmapStringHighlight("Device Count : " + ofToString(ofxMultiKinectV2::getDeviceCount()), 10, 40);
        if (process_occlusion) {
            ofDrawBitmapStringHighlight("Process Occlusion Mask Enable", 10, 60);
        }
    }

    void keyPressed(int key)
    {
        if (key == 'd') {
            process_occlusion =  !process_occlusion;
        }
    }
};

//#include "ofAppGLFWWindow.h"
//========================================================================
int main( ){
    ofSetupOpenGL(1920,1080,OF_WINDOW);            // <-------- setup the GL context
    
    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    ofRunApp(new ofApp());
    
}
