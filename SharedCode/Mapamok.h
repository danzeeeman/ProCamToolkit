//
//  Mapamok.h
//  mapamok
//
//  Created by Dan Moore on 9/28/13.
//
//

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxAssimpModelLoader.h"
#include "ofxProCamToolkit.h"
#include "LineArt.h"
#include "ofxAutoControlPanel.h"

class Mapamok : public ofNode{
private:
    ofxAssimpModelLoader model;
	ofEasyCam cam;
	ofVboMesh objectMesh;
	ofMesh imageMesh;
	ofLight light;
	vector<cv::Point3f> objectPoints;
	vector<cv::Point2f> imagePoints;
	vector<bool> referencePoints;
	cv::Mat rvec, tvec;
	ofMatrix4x4 modelMatrix;
	ofxCv::Intrinsics intrinsics;
	bool calibrationReady;
	Poco::Timestamp lastFragTimestamp, lastVertTimestamp;
	ofShader shader;
    string modelFile;
    ofxAutoControlPanel * panel;
public:
    Mapamok();
    void setup(ofxAutoControlPanel * control, string filename);
    void loadCalibration();
    void saveCalibration();
    void drawRenderMode();
	void drawLabeledPoint(int label, ofVec2f position, ofColor color, ofColor bg = ofColor::black, ofColor fg = ofColor::white);
    void render();
    void setupMesh();
    void updateRenderMode();
    void drawSelectionMode();
    void setupMesh(string filename);
    void update();
    
    void setb(string name, bool value);
	void seti(string name, int value);
	void setf(string name, float value);
	bool getb(string name);
	int geti(string name);
	float getf(string name);
    
    void clearPoint(int choice);
    int getObjectSize();
    cv::Point2f & getImagePoint(int index);
    
};
