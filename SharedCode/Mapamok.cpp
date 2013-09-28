//
//  Mapamok.cpp
//  mapamok
//
//  Created by Dan Moore on 9/28/13.
//
//

#include "Mapamok.h"

using namespace ofxCv;
using namespace cv;

Mapamok::Mapamok(){
    modelFile = "model.dae";
}


void Mapamok::setup(ofApp * parent, string filename){
    mParent = parent;
    modelFile = filename;
}

void Mapamok::setupMesh() {
	model.loadModel(modelFile);
	objectMesh = model.getMesh(0);
	int n = objectMesh.getNumVertices();
	objectPoints.resize(n);
	imagePoints.resize(n);
	referencePoints.resize(n, false);
	for(int i = 0; i < n; i++) {
		objectPoints[i] = toCv(objectMesh.getVertex(i));
	}
}

void Mapamok::updateRenderMode() {
	// generate camera matrix given aov guess
	float aov = mParent->getf("aov");
	Size2i imageSize(ofGetWidth(), ofGetHeight());
	float f = imageSize.width * ofDegToRad(aov); // i think this is wrong, but it's optimized out anyway
	Point2f c = Point2f(imageSize) * (1. / 2);
	Mat1d cameraMatrix = (Mat1d(3, 3) <<
                          f, 0, c.x,
                          0, f, c.y,
                          0, 0, 1);
    
	// generate flags
#define getFlag(flag) (mParent->panel.getValueB((#flag)) ? flag : 0)
	int flags =
    CV_CALIB_USE_INTRINSIC_GUESS |
    getFlag(CV_CALIB_FIX_PRINCIPAL_POINT) |
    getFlag(CV_CALIB_FIX_ASPECT_RATIO) |
    getFlag(CV_CALIB_FIX_K1) |
    getFlag(CV_CALIB_FIX_K2) |
    getFlag(CV_CALIB_FIX_K3) |
    getFlag(CV_CALIB_ZERO_TANGENT_DIST);
	
	vector<Mat> rvecs, tvecs;
	Mat distCoeffs;
	vector<vector<Point3f> > referenceObjectPoints(1);
	vector<vector<Point2f> > referenceImagePoints(1);
	int n = referencePoints.size();
	for(int i = 0; i < n; i++) {
		if(referencePoints[i]) {
			referenceObjectPoints[0].push_back(objectPoints[i]);
			referenceImagePoints[0].push_back(imagePoints[i]);
		}
	}
	const static int minPoints = 6;
	if(referenceObjectPoints[0].size() >= minPoints) {
		calibrateCamera(referenceObjectPoints, referenceImagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, flags);
		rvec = rvecs[0];
		tvec = tvecs[0];
		intrinsics.setup(cameraMatrix, imageSize);
		modelMatrix = makeMatrix(rvec, tvec);
		calibrationReady = true;
	} else {
		calibrationReady = false;
	}
}

void Mapamok::setupMesh(string filename) {
	model.loadModel(filename);
	objectMesh = model.getMesh(0);
	int n = objectMesh.getNumVertices();
	objectPoints.resize(n);
	imagePoints.resize(n);
	referencePoints.resize(n, false);
	for(int i = 0; i < n; i++) {
		objectPoints[i] = toCv(objectMesh.getVertex(i));
	}
}

void Mapamok::drawLabeledPoint(int label, ofVec2f position, ofColor color, ofColor bg, ofColor fg) {
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_DEPTH_TEST);
	ofVec2f tooltipOffset(5, -25);
	ofSetColor(color);
	float w = ofGetWidth();
	float h = ofGetHeight();
	ofSetLineWidth(1.5);
	ofLine(position - ofVec2f(w,0), position + ofVec2f(w,0));
	ofLine(position - ofVec2f(0,h), position + ofVec2f(0,h));
	ofCircle(position, mParent->geti("selectedPointSize"));
	drawHighlightString(ofToString(label), position + tooltipOffset, bg, fg);
	glPopAttrib();
}

void Mapamok::update(){
    if(mParent->getb("randomLighting")) {
		mParent->setf("lightX", ofSignedNoise(ofGetElapsedTimef(), 1, 1) * 1000);
		mParent->setf("lightY", ofSignedNoise(1, ofGetElapsedTimef(), 1) * 1000);
		mParent->setf("lightZ", ofSignedNoise(1, 1, ofGetElapsedTimef()) * 1000);
	}
	light.setPosition(mParent->getf("lightX"), mParent->getf("lightY"), mParent->getf("lightZ"));
    
	if(mParent->getb("selectionMode")) {
		cam.enableMouseInput();
	} else {
		updateRenderMode();
		cam.disableMouseInput();
	}
}

void Mapamok::drawRenderMode() {
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	if(calibrationReady) {
		intrinsics.loadProjectionMatrix(10, 2000);
        
        // this apply Mtatrix should allow us to use the same tranformation
        // matrix on multiple files that match the calibrated file.  so if
        // the worlds are the same in the file then the calibration *should*
        // just work across multiple 3D meshes.
        
		applyMatrix(modelMatrix);
		render();
		if(mParent->getb("setupMode")) {
			imageMesh = getProjectedMesh(objectMesh);
		}
	}
	
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	if(mParent->getb("setupMode")) {
		// draw all reference points cyan
		int n = referencePoints.size();
		for(int i = 0; i < n; i++) {
			if(referencePoints[i]) {
				drawLabeledPoint(i, toOf(imagePoints[i]), cyanPrint);
			}
		}
		
		// move points that need to be dragged
		// draw selected yellow
		int choice = mParent->geti("selectionChoice");
		if(mParent->getb("selected")) {
			referencePoints[choice] = true;
			Point2f& cur = imagePoints[choice];
			if(cur == Point2f()) {
				if(calibrationReady) {
					cur = toCv(ofVec2f(imageMesh.getVertex(choice)));
				} else {
					cur = Point2f(mParent->mouseX, mParent->mouseY);
				}
			}
		}
		if(mParent->getb("dragging")) {
			Point2f& cur = imagePoints[choice];
			float rate = ofGetMousePressed(0) ? mParent->getf("slowLerpRate") : mParent->getf("fastLerpRate");
			cur = Point2f(ofLerp(cur.x, mParent->mouseX, rate), ofLerp(cur.y, mParent->mouseY, rate));
			drawLabeledPoint(choice, toOf(cur), yellowPrint, ofColor::white, ofColor::black);
			ofSetColor(ofColor::black);
			ofRect(toOf(cur), 1, 1);
		} else if(mParent->getb("arrowing")) {
			Point2f& cur = imagePoints[choice];
			drawLabeledPoint(choice, toOf(cur), yellowPrint, ofColor::white, ofColor::black);
			ofSetColor(ofColor::black);
			ofRect(toOf(cur), 1, 1);
        } else {
			// check to see if anything is selected
			// draw hover magenta
			float distance;
			ofVec2f selected = toOf(getClosestPoint(imagePoints, mParent->mouseX, mParent->mouseY, &choice, &distance));
			if(!ofGetMousePressed() && referencePoints[choice] && distance < mParent->getf("selectionRadius")) {
				mParent->seti("hoverChoice", choice);
				mParent->setb("hoverSelected", true);
				drawLabeledPoint(choice, selected, magentaPrint);
			} else {
				mParent->setb("hoverSelected", false);
			}
		}
	}
}


void Mapamok::render() {
    ofPushStyle();
    ofSetLineWidth(mParent->geti("lineWidth"));
    if(mParent->getb("useSmoothing")) {
        ofEnableSmoothing();
    } else {
        ofDisableSmoothing();
    }
    int shading = mParent->geti("shading");
    bool useLights = shading == 1;
    bool useShader = shading == 2;
    if(useLights) {
        light.enable();
        ofEnableLighting();
        glShadeModel(GL_SMOOTH);
        glEnable(GL_NORMALIZE);
    }
    
    if(mParent->getb("highlight")) {
        objectMesh.clearColors();
        int n = objectMesh.getNumVertices();
        float highlightPosition = mParent->getf("highlightPosition");
        float highlightOffset = mParent->getf("highlightOffset");
        for(int i = 0; i < n; i++) {
            int lower = ofMap(highlightPosition - highlightOffset, 0, 1, 0, n);
            int upper = ofMap(highlightPosition + highlightOffset, 0, 1, 0, n);
            ofColor cur = (lower < i && i < upper) ? ofColor::white : ofColor::black;
            objectMesh.addColor(cur);
        }
    }
    
    ofSetColor(255);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_DEPTH_TEST);
    if(useShader) {
        ofFile fragFile("shader.frag"), vertFile("shader.vert");
        Poco::Timestamp fragTimestamp = fragFile.getPocoFile().getLastModified();
        Poco::Timestamp vertTimestamp = vertFile.getPocoFile().getLastModified();
        if(fragTimestamp != lastFragTimestamp || vertTimestamp != lastVertTimestamp) {
            bool validShader = shader.load("shader");
            mParent->setb("validShader", validShader);
        }
        lastFragTimestamp = fragTimestamp;
        lastVertTimestamp = vertTimestamp;
        
        shader.begin();
        shader.setUniform1f("elapsedTime", ofGetElapsedTimef());
        shader.end();
    }
    ofColor transparentBlack(0, 0, 0, 0);
    switch(mParent->geti("drawMode")) {
        case 0: // faces
            if(useShader) shader.begin();
            //glEnable(GL_CULL_FACE);
            //glCullFace(GL_BACK);
            objectMesh.drawFaces();
            if(useShader) shader.end();
            break;
        case 1: // fullWireframe
            if(useShader) shader.begin();
            objectMesh.drawWireframe();
            if(useShader) shader.end();
            break;
        case 2: // outlineWireframe
            LineArt::draw(objectMesh, true, transparentBlack, useShader ? &shader : NULL);
            break;
        case 3: // occludedWireframe
            LineArt::draw(objectMesh, false, transparentBlack, useShader ? &shader : NULL);
            break;
    }
    glPopAttrib();
    if(useLights) {
        ofDisableLighting();
    }
    ofPopStyle();
}

void Mapamok::saveCalibration() {
	string dirName = "calibration-" + ofGetTimestampString() + "/";
	ofDirectory dir(dirName);
	dir.create();
	
	FileStorage fs(ofToDataPath(dirName + "calibration-advanced.yml"), FileStorage::WRITE);
	
	Mat cameraMatrix = intrinsics.getCameraMatrix();
	fs << "cameraMatrix" << cameraMatrix;
	
	double focalLength = intrinsics.getFocalLength();
	fs << "focalLength" << focalLength;
	
	Point2d fov = intrinsics.getFov();
	fs << "fov" << fov;
	
	Point2d principalPoint = intrinsics.getPrincipalPoint();
	fs << "principalPoint" << principalPoint;
	
	cv::Size imageSize = intrinsics.getImageSize();
	fs << "imageSize" << imageSize;
	
	fs << "translationVector" << tvec;
	fs << "rotationVector" << rvec;
    
	Mat rotationMatrix;
	Rodrigues(rvec, rotationMatrix);
	fs << "rotationMatrix" << rotationMatrix;
	
	double rotationAngleRadians = norm(rvec, NORM_L2);
	double rotationAngleDegrees = ofRadToDeg(rotationAngleRadians);
	Mat rotationAxis = rvec / rotationAngleRadians;
	fs << "rotationAngleRadians" << rotationAngleRadians;
	fs << "rotationAngleDegrees" << rotationAngleDegrees;
	fs << "rotationAxis" << rotationAxis;
	
	ofVec3f axis(rotationAxis.at<double>(0), rotationAxis.at<double>(1), rotationAxis.at<double>(2));
	ofVec3f euler = ofQuaternion(rotationAngleDegrees, axis).getEuler();
	Mat eulerMat = (Mat_<double>(3,1) << euler.x, euler.y, euler.z);
	fs << "euler" << eulerMat;
	
	ofFile basic("calibration-basic.txt", ofFile::WriteOnly);
	ofVec3f position( tvec.at<double>(1), tvec.at<double>(2));
	basic << "position (in world units):" << endl;
	basic << "\tx: " << ofToString(tvec.at<double>(0), 2) << endl;
	basic << "\ty: " << ofToString(tvec.at<double>(1), 2) << endl;
	basic << "\tz: " << ofToString(tvec.at<double>(2), 2) << endl;
	basic << "axis-angle rotation (in degrees):" << endl;
	basic << "\taxis x: " << ofToString(axis.x, 2) << endl;
	basic << "\taxis y: " << ofToString(axis.y, 2) << endl;
	basic << "\taxis z: " << ofToString(axis.z, 2) << endl;
	basic << "\tangle: " << ofToString(rotationAngleDegrees, 2) << endl;
	basic << "euler rotation (in degrees):" << endl;
	basic << "\tx: " << ofToString(euler.x, 2) << endl;
	basic << "\ty: " << ofToString(euler.y, 2) << endl;
	basic << "\tz: " << ofToString(euler.z, 2) << endl;
	basic << "fov (in degrees):" << endl;
	basic << "\thorizontal: " << ofToString(fov.x, 2) << endl;
	basic << "\tvertical: " << ofToString(fov.y, 2) << endl;
	basic << "principal point (in screen units):" << endl;
	basic << "\tx: " << ofToString(principalPoint.x, 2) << endl;
	basic << "\ty: " << ofToString(principalPoint.y, 2) << endl;
	basic << "image size (in pixels):" << endl;
	basic << "\tx: " << ofToString(principalPoint.x, 2) << endl;
	basic << "\ty: " << ofToString(principalPoint.y, 2) << endl;
	
	saveMat(Mat(objectPoints), dirName + "objectPoints.yml");
	saveMat(Mat(imagePoints), dirName + "imagePoints.yml");
}

void enableFog(float nearFog, float farFog) {
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	GLfloat fogColor[4]= {0, 0, 0, 1};
	glFogfv(GL_FOG_COLOR, fogColor);
	glHint(GL_FOG_HINT, GL_FASTEST);
	glFogf(GL_FOG_START, nearFog);
	glFogf(GL_FOG_END, farFog);
}

void disableFog() {
	glDisable(GL_FOG);
}

void Mapamok::drawSelectionMode() {
	ofSetColor(255, 0, 255);
	cam.begin();
	float scale = mParent->getf("scale");
	ofScale(scale, scale, scale);
	if(mParent->getb("useFog")) {
		enableFog(mParent->getf("fogNear"), mParent->getf("fogFar"));
	}
	render();
	if(mParent->getb("useFog")) {
		disableFog();
	}
	if(mParent->getb("setupMode")) {
		imageMesh = getProjectedMesh(objectMesh);
	}
	cam.end();
	
	if(mParent->getb("setupMode")) {
		// draw all points cyan small
		glPointSize(mParent->geti("screenPointSize"));
		glEnable(GL_POINT_SMOOTH);
		ofSetColor(cyanPrint);
		imageMesh.drawVertices();
        
		// draw all reference points cyan
		int n = referencePoints.size();
		for(int i = 0; i < n; i++) {
			if(referencePoints[i]) {
				drawLabeledPoint(i, imageMesh.getVertex(i), cyanPrint);
			}
		}
		
		// check to see if anything is selected
		// draw hover point magenta
		int choice;
		float distance;
		ofVec3f selected = getClosestPointOnMesh(imageMesh, mParent->mouseX, mParent->mouseY, &choice, &distance);
		if(!ofGetMousePressed() && distance < mParent->getf("selectionRadius")) {
			mParent->seti("hoverChoice", choice);
			mParent->setb("hoverSelected", true);
			drawLabeledPoint(choice, selected, magentaPrint);
		} else {
			mParent->setb("hoverSelected", false);
		}
		
		// draw selected point yellow
		if(mParent->getb("selected")) {
			int choice = mParent->geti("selectionChoice");
			ofVec2f selected = imageMesh.getVertex(choice);
			drawLabeledPoint(choice, selected, yellowPrint, ofColor::white, ofColor::black);
		}
	}
}

void Mapamok::loadCalibration() {
    
    // retrieve advanced calibration folder
    
    string calibPath;
    ofFileDialogResult result = ofSystemLoadDialog("Select a calibration folder", true, ofToDataPath("", true));
    calibPath = result.getPath();
    
    // load objectPoints and imagePoints
    
    cout<<calibPath<<endl;
    
    Mat objPointsMat, imgPointsMat;
    loadMat( objPointsMat, calibPath + "/objectPoints.yml");
    loadMat( imgPointsMat, calibPath + "/imagePoints.yml");
    
    int numVals;
    float x, y, z;
    cv::Point3f oP;
    
    const float* objVals = objPointsMat.ptr<float>(0);
    numVals = objPointsMat.cols * objPointsMat.rows;
    
    for(int i = 0; i < numVals; i+=3) {
        oP.x = objVals[i];
        oP.y = objVals[i+1];
        oP.z = objVals[i+2];
        objectPoints[i/3] = oP;
    }
    
    cv::Point2f iP;
    
    referencePoints.resize( (imgPointsMat.cols * imgPointsMat.rows ) / 2, false);
    
    const float* imgVals = imgPointsMat.ptr<float>(0);
    numVals = objPointsMat.cols * objPointsMat.rows;
    
    for(int i = 0; i < numVals; i+=2) {
        iP.x = imgVals[i];
        iP.y = imgVals[i+1];
        if(iP.x != 0 && iP.y != 0) {
            referencePoints[i/2] = true;
        }
        imagePoints[i/2] = iP;
    }
    
    
    // load the calibration-advanced yml
    
    FileStorage fs(calibPath + "/calibration-advanced.yml", FileStorage::READ);
    
    Mat cameraMatrix;
    Size2i imageSize;
    fs["cameraMatrix"] >> cameraMatrix;
    fs["imageSize"][0] >> imageSize.width;
    fs["imageSize"][1] >> imageSize.height;
    fs["rotationVector"] >> rvec;
    fs["translationVector"] >> tvec;
    
    intrinsics.setup(cameraMatrix, imageSize);
    modelMatrix = makeMatrix(rvec, tvec);
    
    calibrationReady = true;
}