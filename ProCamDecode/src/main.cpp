#include "ofApp.h"
#include "ofAppGlutWindow.h"

int main() {
	ofAppGlutWindow window;
	ofSetupOpenGL(&window, 1024, 480 + 768, OF_WINDOW);
	ofRunApp(new ofApp());
}
