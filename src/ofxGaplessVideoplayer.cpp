#include "ofxGaplessVideoplayer.h"

// Constructor
ofxGaplessVideoPlayer::ofxGaplessVideoPlayer() {
    hasPreview      = false;
    currentMovie    = 0;
    pendingMovie    = 1;
	players[0].actionTimeout   = 0;
	players[1].actionTimeout   = 0;
    state           = empty;

	
    
	#ifdef TARGET_LINUX
	players[0].video.getPlayer<ofGstVideoPlayer>()->setAsynchronousLoad(true);
	players[1].video.getPlayer<ofGstVideoPlayer>()->setAsynchronousLoad(true);
	#endif

}

// Desonstructor
ofxGaplessVideoPlayer::~ofxGaplessVideoPlayer() {
    
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::setPreview(bool p){
    hasPreview = p;
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::togglePreview(){
    hasPreview = !hasPreview;
}


//--------------------------------------------------------------
// Enqueue Function: triggered from another thread
//--------------------------------------------------------------
void ofxGaplessVideoPlayer::loadMovie(string _name, bool _in, bool _out){
    ofLogVerbose() << "+ [net] loadMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "loadMovie";
    c.n = _name;
    c.i = _in;
    c.o = _out;
    if (lock() && state != empty) {
        queue.push_back(c);
        unlock();
    }
    else {
        ofLogVerbose() << "x loadMovie: no lock!";
    }
}


//--------------------------------------------------------------
void ofxGaplessVideoPlayer::appendMovie(string _name, bool _in, bool _out){
    ofLogVerbose() << "+ [net] appendMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "appendMovie";
    c.n = _name;
    c.i = _in;
    c.o = _out;
    if (lock() && state != empty) {
        queue.push_back(c);
        unlock();
    }
    else {
        ofLogVerbose() << "x appendMovie: no lock!";
    }
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::triggerMovie(string _name){
    ofLogVerbose() << "+ [net] triggerMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "triggerMovie";
    c.n = _name;
    c.i = false;
    c.o = false;
    if (lock() && state != empty) {
        queue.push_back(c);
        unlock();
    }
    else {
        ofLogVerbose() << "x triggerMovie: no lock!";
    }
}


//--------------------------------------------------------------
// Dequeue Function: triggered from the update func(main thread)
//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_loadMovie(string _name, bool _in, bool _out){
    ofLogVerbose() << "        " << _name << " loaded";
    appendMovie(_name,_in,_out);
    triggerMovie(_name);
}


//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_appendMovie(string _name, bool _in, bool _out){
    ofLogVerbose() << "        " << _name << " appended";
    players[pendingMovie].actionTimeout = ofGetElapsedTimeMillis();
    players[pendingMovie].loadTime = players[pendingMovie].actionTimeout;
//    players[pendingMovie].video.setVolume(0);
    players[pendingMovie].video.close();
#ifdef TARGET_OSX
//	players[pendingMovie].video.getPlayer<ofAVFoundationPlayer>()->videoPlayer =  [[ofAVFoundationVideoPlayer alloc] init]
#endif
    players[pendingMovie].video.loadAsync(_name);
//    players[pendingMovie].video.setPaused(true);
    players[pendingMovie].fades.in  = _in;
    players[pendingMovie].fades.out = _out;
    players[pendingMovie].loadTime = ofGetElapsedTimeMillis() - players[pendingMovie].loadTime;
    
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_triggerMovie(string _name){
    ofLogVerbose() << "        " << _name << " triggered";

    if(players[pendingMovie].video.isLoaded()) {
        players[pendingMovie].video.play();
    }
    else {
        _appendMovie(_name, false, false);
        players[pendingMovie].video.play();
    }
    state = switching;
}



//--------------------------------------------------------------
void ofxGaplessVideoPlayer::update(){

    if (state==empty) state = ready;
    
    if (queue.size()>0) {
        ofxGaplessVideoPlayer::command next_command;
        int t = ofGetElapsedTimeMillis();
        if (lock()) {
            next_command = queue.front();
            queue.pop_front();
            unlock();
        }
        else {
            ofLogVerbose() << "x update: no lock!";
        }

        if (next_command.c == "loadMovie") {
            _loadMovie(next_command.n,next_command.i,next_command.o);
        }
        if (next_command.c == "appendMovie") {
            _appendMovie(next_command.n,next_command.i,next_command.o);
        }
        if (next_command.c == "triggerMovie") {
            _triggerMovie(next_command.n);
        }
    }
//    if (players[0].video.isPlaying())
        players[0].video.update();
//    if (players[1].video.isPlaying())
        players[1].video.update();
    
    if (state == switched) {
        players[pendingMovie].video.setVolume(0.0f);
        players[pendingMovie].video.setPaused(true);
        state = ready;
    }
    
    if (state == switching) {
        if(players[pendingMovie].video.isPlaying() && players[pendingMovie].video.getCurrentFrame() > 1) {
            pendingMovie = currentMovie;
            currentMovie = pendingMovie==1?0:1;
            state = switched;
            if (!players[currentMovie].fades.in) {
                players[currentMovie].video.setVolume(1.0f);
                ofLogVerbose() << "Set Audio to 1";
            }
        }
    }
}


//--------------------------------------------------------------
bool ofxGaplessVideoPlayer::draw(int x, int y, int w, int h){

    
    static bool isDrawing = false;
//	static bool force_hide = false;

    


    int current_pos = players[currentMovie].video.getCurrentFrame();
    int total_pos = players[currentMovie].video.getTotalNumFrames();

    if (players[currentMovie].video.isPlaying()) {
        ofPushStyle();
        if (players[currentMovie].fades.out || players[currentMovie].fades.in) {
            float fade = 1.0f;
            static float old_fade = 0.0f;
            if (players[currentMovie].fades.in && current_pos<25) {
                fade = 1.0f / 25.0f * CLAMP(current_pos-1, 0, 25);
                ofLogVerbose() << "Fade in " << fade;
            }
            if (players[currentMovie].fades.out && (total_pos-current_pos)<25) {
                fade = 1.0f / 25.0f * CLAMP(total_pos-current_pos, 0, 25);
                ofLogVerbose() << "Fade out " << fade;
            }
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            ofSetColor(255 * fade, 255 * fade, 255 * fade, 255 * fade);
            if (fade != old_fade) {
                players[currentMovie].video.setVolume(fade);
                ofLogVerbose() << "Fade Audio: " << fade;
            }
            old_fade = fade;
        }
        else {
            ofSetColor(255,255,255,255);
        }
        players[currentMovie].video.draw(x, y, w, h);
      	ofDisableBlendMode();
        ofPopStyle();
        isDrawing = true;
    }
    
    if (hasPreview && players[pendingMovie].video.isLoaded()) {
        players[pendingMovie].video.draw(w-w/4-2, 2, w/4, h/4);
    }
    
    if (hasPreview) {
        ofPushStyle();
        ofNoFill();
        ofSetColor(255, 255, 255);
        ofRect(w-w/4-2, 2, w/4, h/4);
        ofSetColor(255, 0, 0);
        ostringstream os;
        os << "Activ : " << getCurrentMovie() << endl;
        os << "State : " << getState() << endl;
        os << "Load  : " << getLoadTime() << endl;
        os << "Frame : " << getCurrentFrame() << "/" << getTotalNumFrames() << endl;
        os << "FPS   : " << ofGetFrameRate() << endl;
        ofDrawBitmapString(os.str(), w-w/4+2, 15);
        ofPopStyle();
    }

/*    if (players[currentMovie].video.getCurrentFrame() == players[currentMovie].video.getTotalNumFrames()) {
        force_hide = true;
        ofLogVerbose() << "Force HIDE";
    }
*/

    return isDrawing;
}

void ofxGaplessVideoPlayer::start() {

}

void ofxGaplessVideoPlayer::stop() {
    players[0].video.close();
    players[1].video.close();
}

