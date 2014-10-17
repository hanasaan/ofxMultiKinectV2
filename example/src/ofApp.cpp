#include "ofMain.h"
#include "ofxMultiKinectV2.h"

#define STRINGIFY(x) #x

static string depthFragmentShader =
STRINGIFY(
    uniform sampler2DRect tex;
    void main()
    {
      vec4 col = texture2DRect(tex, gl_TexCoord[0].xy);
        float value = col.r * 65535.0;
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

//========================================================================
class ofApp : public ofBaseApp{
    ofShader depthShader;
    ofxMultiKinectV2 kinect0;
    ofxMultiKinectV2 kinect1;
    ofTexture colorTex0;
    ofTexture colorTex1;
public:
    void setup()
    {
        ofSetVerticalSync(true);
        ofSetFrameRate(30);
        
        depthShader.setupShaderFromSource(GL_FRAGMENT_SHADER, depthFragmentShader);
        depthShader.linkProgram();
        
        kinect0.open(true, true, 0);
        kinect1.open(true, true, 1);
        kinect0.start();
        kinect1.start();
    }
    
    void update() {
        kinect0.update();
        if (kinect0.isFrameNew()) {
            colorTex0.loadData(kinect0.getColorPixelsRef());
        }
        kinect1.update();
        if (kinect1.isFrameNew()) {
            colorTex1.loadData(kinect1.getColorPixelsRef());
        }
    }
    
    
    void draw()
    {
        ofClear(0);

        if (colorTex0.isAllocated()) {
            colorTex0.draw(0, 0, 640, 360);
        }
        if (kinect0.getDepthTextureRef().isAllocated()) {
            depthShader.begin();
            kinect0.getDepthTextureRef().draw(0, 360, 512, 424);
            depthShader.end();
        }
        if (colorTex1.isAllocated()) {
            colorTex1.draw(640, 0, 640, 360);
        }
        if (kinect1.getDepthTextureRef().isAllocated()) {
            depthShader.begin();
            kinect1.getDepthTextureRef().draw(640, 360, 512, 424);
            depthShader.end();
        }
        
        ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate()), 10, 20);
    }
    
    void keyPressed(int key)
    {
    }
    void keyReleased(int key) {}
    void mouseMoved(int x, int y ) {}
    void mouseDragged(int x, int y, int button) {}
    void mousePressed(int x, int y, int button) {}
    void mouseReleased(int x, int y, int button) {}
    void windowResized(int w, int h) {}
    void dragEvent(ofDragInfo dragInfo) {}
    void gotMessage(ofMessage msg) {}
    
};

//========================================================================
int main( ){
	ofSetupOpenGL(1280,800,OF_WINDOW);			// <-------- setup the GL context
    
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());
    
}
