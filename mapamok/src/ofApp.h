#pragma once

#include "ofMain.h"
#include "ofxAutoControlPanel.h"
#include "Mapamok.h"
class ofApp : public ofBaseApp {
public:
	void setb(string name, bool value);
	void seti(string name, int value);
	void setf(string name, float value);
	bool getb(string name);
	int geti(string name);
	float getf(string name);
	
	void setup();
	void update();
	void draw();	
	void keyPressed(int key);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	
	void setupControlPanel();
	
	ofxAutoControlPanel panel;
    Mapamok model;
};
