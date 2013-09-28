#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

void ofApp::setb(string name, bool value) {
	panel.setValueB(name, value);
}
void ofApp::seti(string name, int value) {
	panel.setValueI(name, value);
}
void ofApp::setf(string name, float value) {
	panel.setValueF(name, value);
}
bool ofApp::getb(string name) {
	return panel.getValueB(name);
}
int ofApp::geti(string name) {
	return panel.getValueI(name);
}
float ofApp::getf(string name) {
	return panel.getValueF(name);
}

void ofApp::setup() {
	ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
	ofSetVerticalSync(true);
	calibrationReady = false;
	setupMesh();	
	setupControlPanel();
}

void ofApp::update() {
	ofSetWindowTitle("mapamok");
}



void ofApp::draw() {
	ofBackground(geti("backgroundColor"));
    if(getb("loadCalibration")) {
		loadCalibration();
		setb("loadCalibration", false);
	}
	if(getb("saveCalibration")) {
		saveCalibration();
		setb("saveCalibration", false);
	}
    ofPushStyle();
    ofSetColor(255, 0, 255);
	if(getb("selectionMode")) {
		drawSelectionMode();
	} else {
		drawRenderMode();
	}
    ofPopStyle();
	if(!getb("validShader")) {
		ofPushStyle();
		ofSetColor(magentaPrint);
		ofSetLineWidth(8);
		ofLine(0, 0, ofGetWidth(), ofGetHeight());
		ofLine(ofGetWidth(), 0, 0, ofGetHeight());
		string message = "Shader failed to compile.";
		ofVec2f center(ofGetWidth(), ofGetHeight());
		center /= 2;
		center.x -= message.size() * 8 / 2;
		center.y -= 8;
		drawHighlightString(message, center);
		ofPopStyle();
	}
}

void ofApp::keyPressed(int key) {
	if(key == OF_KEY_LEFT || key == OF_KEY_UP || key == OF_KEY_RIGHT|| key == OF_KEY_DOWN){
		int choice = geti("selectionChoice");
		setb("arrowing", true);
		if(choice > 0){
			Point2f& cur = imagePoints[choice];
			switch(key) {
				case OF_KEY_LEFT: cur.x -= 1; break;
				case OF_KEY_RIGHT: cur.x += 1; break;
				case OF_KEY_UP: cur.y -= 1; break;
				case OF_KEY_DOWN: cur.y += 1; break;
			}
		}
	} else {
		setb("arrowing",false);
	}
	if(key == OF_KEY_BACKSPACE) { // delete selected
		if(getb("selected")) {
			setb("selected", false);
			int choice = geti("selectionChoice");
			referencePoints[choice] = false;
			imagePoints[choice] = Point2f();
		}
	}
	if(key == '\n') { // deselect
		setb("selected", false);
	}
	if(key == ' ') { // toggle render/select mode
		setb("selectionMode", !getb("selectionMode"));
	}
}

void ofApp::mousePressed(int x, int y, int button) {
	setb("selected", getb("hoverSelected"));
	seti("selectionChoice", geti("hoverChoice"));
	if(getb("selected")) {
		setb("dragging", true);
	}
}

void ofApp::mouseReleased(int x, int y, int button) {
	setb("dragging", false);
}


void ofApp::setupControlPanel() {
	panel.setup();
	panel.msg = "tab hides the panel, space toggles render/selection mode, 'f' toggles fullscreen.";
	
	panel.addPanel("Interaction");
	panel.addToggle("setupMode", true);
	panel.addSlider("scale", 1, .1, 125);
	panel.addSlider("backgroundColor", 0, 0, 255, true);
	panel.addMultiToggle("drawMode", 3, variadic("faces")("fullWireframe")("outlineWireframe")("occludedWireframe"));
	panel.addMultiToggle("shading", 0, variadic("none")("lights")("shader"));
	panel.addToggle("loadCalibration", false);
	panel.addToggle("saveCalibration", false);
	
	panel.addPanel("Highlight");
	panel.addToggle("highlight", false);
	panel.addSlider("highlightPosition", 0, 0, 1);
	panel.addSlider("highlightOffset", .1, 0, 1);
	
	panel.addPanel("Calibration");
	panel.addSlider("aov", 80, 50, 100);
	panel.addToggle("CV_CALIB_FIX_ASPECT_RATIO", true);
	panel.addToggle("CV_CALIB_FIX_K1", true);
	panel.addToggle("CV_CALIB_FIX_K2", true);
	panel.addToggle("CV_CALIB_FIX_K3", true);
	panel.addToggle("CV_CALIB_ZERO_TANGENT_DIST", true);
	panel.addToggle("CV_CALIB_FIX_PRINCIPAL_POINT", false);
	
	panel.addPanel("Rendering");
	panel.addSlider("lineWidth", 2, 1, 8, true);
	panel.addToggle("useSmoothing", false);
	panel.addToggle("useFog", false);
	panel.addSlider("fogNear", 200, 0, 1000);
	panel.addSlider("fogFar", 1850, 0, 2500);
	panel.addSlider("screenPointSize", 2, 1, 16, true);
	panel.addSlider("selectedPointSize", 8, 1, 16, true);
	panel.addSlider("selectionRadius", 12, 1, 32);
	panel.addSlider("lightX", 200, -1000, 1000);
	panel.addSlider("lightY", 400, -1000, 1000);
	panel.addSlider("lightZ", 800, -1000, 1000);
	panel.addToggle("randomLighting", false);
	
	panel.addPanel("Internal");
	panel.addToggle("validShader", true);
	panel.addToggle("selectionMode", true);
	panel.addToggle("hoverSelected", false);
	panel.addSlider("hoverChoice", 0, 0, objectPoints.size(), true);
	panel.addToggle("selected", false);
	panel.addToggle("dragging", false);
	panel.addToggle("arrowing", false);
	panel.addSlider("selectionChoice", 0, 0, objectPoints.size(), true);
	panel.addSlider("slowLerpRate", .001, 0, .01);
	panel.addSlider("fastLerpRate", 1, 0, 1);
}

