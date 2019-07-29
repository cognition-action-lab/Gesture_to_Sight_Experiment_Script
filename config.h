#ifndef CONFIG_H
#define CONFIG_H
#pragma once

#include "SDL_opengl.h"

////////// Set these parameters only! //////////////////////////

//set this to the data path in which the current set of data for this block should be stored.

//set the subject number
#define SUBJECT_ID "9999"

//define the file that contains the name of the trial table
#define TRIALFILE "./TrialTables/pair9/tbl9-2_listA_NVF.txt"  

//define the folder where the data will go (this folder must exist!)
#define DATAPATH "C:/Users/MRRI/Desktop/nabdata/"


//define paths
#define IMAGEPATH "./Resources/BN_Images"
#define NIMAGES 45

#define AUDIOPATH "./Resources/BN_Sounds"

#define INSTRUCTPATH "./Resources/Instructions"
#define NINSTRUCT 4



// */

//set this parameter to determine the primary sensor -- should correspond to the index finger
#define HAND 2

//set BLINDED to 1 if this block is blinded, otherwise 0
//#define BLINDED 1


////////////////////////////////////////////////////////////////




// Configurable constants

//TRACKTYPE: type of tracker being used: FOB (0) or trakStar (1)
#define TRACKTYPE 1
#define BIRDCOUNT 8
//SAMPRATE: sampling rate (for 1 full measurement cycle, or time to activate all 3 axes.  for trakSTAR, we get samples returned at SAMPRATE*3 frequency.
#define SAMPRATE 140
//1 out of every REPORTRATE samples will be acquired from the trakSTAR, when in Synchronous mode
#define REPORTRATE 1
#define FILTER_WIDE true
#define FILTER_NARROW false
#define FILTER_DC 1.0f
#define HEMIFIELD FRONT
#define RANGE 72.0f
#define DOSYNC true


//screen dimensions
//#define SCREEN_WIDTH  1920
//#define SCREEN_HEIGHT  1080
#define SCREEN_WIDTH  1600
#define SCREEN_HEIGHT  900

// Physical dimensions of the screen in meters
#define PHYSICAL_WIDTH  0.307
#define PHYSICAL_HEIGHT  0.173
//#define PHYSICAL_HEIGHT  0.192

#define IMAGEX SCREEN_WIDTH/2
#define IMAGEY SCREEN_HEIGHT/2

#define SCREEN_BPP  32
//switch WINDOWED and MIRRORED for kinereach/simulation runs
#define WINDOWED  false
#define MIRRORED  false
// screen ratio, in meters per pixel
#define PHYSICAL_RATIO  (PHYSICAL_WIDTH / SCREEN_WIDTH)
//#define PHYSICAL_RATIOI  (SCREEN_WIDTH/PHYSICAL_WIDTH)

#define CURSOR_RADIUS 0.006f
#define START_RADIUS 0.05f

//wait times
#define WAITTIME 1200
#define FIXTIME 1800 
#define DISPTIME 800
#define INTROTIME 8000

//define pi as a float
#define PI 3.14159265358979f

//calibration parameters -- set to zero here, we will set them dynamically later!
#define CALxOFFSET 0.0f   //meters
#define CALyOFFSET 0.0f   //meters
#define CALzOFFSET 0.0f   //meters
#define CALthetaROTANG 0.0f  //radians
#define CALphiROTANG 0.0f  //radians

//define the velocity threshold for detecting movement onset/offset, in m/sec
#define VEL_MVT_TH 0.05f
//define the time that vel must be below thresh to define movement end, in msec
#define VEL_END_TIME 2000 //hold still for 2 sec

#define TRIAL_DURATION 10000 //10 sec, in msec
//#define MAX_TRIAL_DURATION 600000 //10 min, in msec

#endif
