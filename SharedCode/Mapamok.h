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
#include "ofApp.h"
class Mapamok {
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
    ofApp * mParent;
    string modelFile;
public:
    Mapamok();
    void setup(ofApp * parent, string filename);
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
};
