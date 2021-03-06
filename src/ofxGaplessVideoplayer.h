#ifndef _THREADED_VIDEOPLAYER
#define _THREADED_VIDEOPLAYER

#include "ofMain.h"

#ifdef GSTREAMER_ON_OSX
#include "ofGstVideoPlayer.h"
#endif



#define MAX_VIDEOS 2

class ofxGaplessVideoPlayer : public ofThread{
    
private:
		
    int currentMovie, pendingMovie;
    
    enum PStatus { empty, ready, appended, waiting, switching, switched, forceappended };
    PStatus state;
    const string state_string[7] = {"empty", "ready", "appended", "waiting", "switching", "switched", "forceappended"};

    bool hasPreview, receivedVolumeChange;
    
    /* Command Queue */
    struct command {
        string c;
        string n;
        bool i;
        bool o;
    };
    deque<command> queue;

    /* Fades */
    struct fade {
        bool in;
        bool out;
    };


    /* Players */
    struct player {
        ofVideoPlayer video;
        fade          fades;
        int           loadTime;
        int           actionTimeout;
        float         maxVol;
    };
    player players[MAX_VIDEOS];

    void _loadMovie(string _name, bool _in, bool _out);
    void _appendMovie(string _name, bool _in, bool _out);
    void _triggerMovie(string _name);
    
public:
    
    ofxGaplessVideoPlayer();
    ~ofxGaplessVideoPlayer();

    void start();
    void stop();
    
    int getCurrentMovie() {return currentMovie;}
    int getState() {return state;}
    int getLoadTime() {return players[currentMovie].video.isLoaded() ? players[currentMovie].loadTime : 0;}
    
    int getCurrentFrame() {return players[currentMovie].video.isLoaded() ? players[currentMovie].video.getCurrentFrame() : 0;}
    int getTotalNumFrames() {return players[currentMovie].video.isLoaded() ? players[currentMovie].video.getTotalNumFrames() : 0;}
    
    float getWidth() {return players[currentMovie].video.isLoaded() ? players[currentMovie].video.getWidth() : 0;}
    float getHeight() {return players[currentMovie].video.isLoaded() ? players[currentMovie].video.getHeight() : 0;}
    
    void setPreview(bool p);
    void togglePreview();
    
    void loadMovie(string _name, bool _in, bool _out);
    void appendMovie(string _name, bool _in, bool _out);
    void triggerMovie(string _name);
    
    void setVolume(float _volume);

    void update();
    bool draw(int x, int y, int w, int h);
    bool draw() {return draw(0,0,ofGetWidth(),ofGetHeight());}
  	
};

#endif
