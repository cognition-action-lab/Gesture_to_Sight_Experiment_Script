#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <istream>
#include <windows.h>
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
#include "FTDI.h"

#include "MouseInput.h"
#include "TrackBird.h"

#include "Circle.h"
#include "DataWriter.h"
#include "HandCursor.h"
#include "Object2D.h"
#include "Path2D.h"
#include "Region2D.h"
#include "Sound.h"
#include "Timer.h"
#include "Image.h"

#include "config.h"

#include <gl/GL.h>
#include <gl/GLU.h>

/*
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_mixer.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
#pragma comment(lib, "Bird.lib")
#pragma comment(lib, "ftd2xx.lib")
#pragma comment(lib, "ATC3DG.lib")
*/
#pragma push(1)

//state machine
enum GameState
{
	Idle = 0x01,       //00001
	Instruct = 0x02,   //00010
	WaitStim = 0x03,   //00011
	ShowStim = 0x04,   //00100
	Active = 0x06,     //00110
	EndTrial = 0x08, //01000
	Finished = 0x10    //10000
};



SDL_Event event;
SDL_Window *screen = NULL;
SDL_GLContext glcontext = NULL;

HandCursor* curs[BIRDCOUNT + 1];
HandCursor* player = NULL;
Circle* startCircle = NULL;  //circle to keep track of the "home" position
//Circle* handCircle = NULL;
Object2D* items[NIMAGES];
Object2D* instructimages[NINSTRUCT];
Region2D blackRegion;
Image* endtext = NULL;
Image* readytext = NULL;
Image* trialinstructtext = NULL;
Image* stoptext = NULL;
Image* holdtext = NULL;
Image* redotext = NULL;
Image* trialnum = NULL;
Image* recordtext = NULL;
Image* proceedtext = NULL;
Image* returntext = NULL;
Image* mousetext = NULL;
Sound* startbeep = NULL;
Sound* stopbeep = NULL;
Sound* scorebeep = NULL;
Sound* errorbeep = NULL;
Sound* itemsounds[NIMAGES];
SDL_Color textColor = {0, 0, 0, 1};
SDL_Color textGrayColor = {128, 128, 128, 255};
DataWriter* writer = NULL;
GameState state;
Timer* trialTimer;
Timer* hoverTimer;
Timer* movTimer;

//Uint32 gameTimer;
//Uint32 hoverTimer;


//FTDI variables (to trigger LCD glasses)
FT_HANDLE ftHandle;
bool ftdiActive;

//tracker variables
int trackstatus;
TrackSYSCONFIG sysconfig;
TrackDATAFRAME dataframe[BIRDCOUNT+1];
Uint32 DataStartTime = 0;
bool recordData = false;
bool didrecord = false;

//colors
float redColor[3] = {1.0f, 0.0f, 0.0f};
float greenColor[3] = {0.0f, 1.0f, 0.0f};
float blueColor[3] = {0.0f, 0.0f, 1.0f};
float cyanColor[3] = {0.0f, 0.5f, 1.0f};
float grayColor[3] = {0.6f, 0.6f, 0.6f};
float blkColor[3] = {0.0f, 0.0f, 0.0f};
float whiteColor[3] = {1.0f, 1.0f, 1.0f};
float orangeColor[3] = {1.0f, 0.5f, 0.0f};
float *cursColor = blueColor;


// Trial table structure, to keep track of all the parameters for each trial of the experiment
typedef struct {
	//int TrialType;		// Flag 1-for trial type
	int vision;			// flag if we should shut the shutter glasses on this trial; can be extended to specify when if desired
	int item;			//item number, corresonding to an image and audio file
	int dur;				//duration to display the image to the screen, in ms
	int practice;			//flag if the block is a practice block or not (changes the instructions disiplayed at the start of the block)
} TRTBL;

#define TRTBL_SIZE 100
TRTBL trtbl [TRTBL_SIZE];

int NTRIALS = 0;
int CurTrial = -1;

#define curtr trtbl[CurTrial]

//target structure; keep track of the target and other parameters, for writing out to data stream
TargetFrame Target;

//varibles that let the experimenter control the experiment flow
bool nextstateflag = false;
bool redotrialflag = false;
int numredos = 0;
//int dotrialflag = 0;

// Initializes everything and returns true if there were no errors
bool init();
// Sets up OpenGL
void setup_opengl();
// Performs closing operations
void clean_up();
// Draws objects on the screen
void draw_screen();
//file to load in trial table
int LoadTrFile(char *filename);
// Update loop (state machine)
void game_update();

bool quit = false;  //flag to cue exit of program



int main(int argc, char* args[])
{
	int a = 0;
	int flagoffsetkey = -1;
	Target.key = ' ';

	//redirect stderr output to a file
	freopen( "./Debug/errorlog.txt", "w", stderr); 

	std::cerr << "Start main." << std::endl;

	SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);
	//HIGH_PRIORITY_CLASS
	std::cerr << "Promote process priority to Above Normal." << std::endl;

	if (!init())
	{
		// There was an error during initialization
		std::cerr << "Initialization error." << std::endl;
		return 1;
	}

	DataStartTime = SDL_GetTicks();

	while (!quit)
	{
		int inputs_updated = 0;

		// Retrieve Flock of Birds data
		if (trackstatus>0)
		{
			// Update inputs from Flock of Birds
			inputs_updated = TrackBird::GetUpdatedSample(&sysconfig,dataframe);
		}

		// Handle SDL events
		while (SDL_PollEvent(&event))
		{
			// See http://www.libsdl.org/docs/html/sdlevent.html for list of event types
			if (event.type == SDL_MOUSEMOTION)
			{
				if (trackstatus <= 0)
				{
					MouseInput::ProcessEvent(event);
					inputs_updated = MouseInput::GetFrame(dataframe);

				}
			}
			else if (event.type == SDL_KEYDOWN)
			{
				// See http://www.libsdl.org/docs/html/sdlkey.html for Keysym definitions
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else if (event.key.keysym.sym == SDLK_0)
				{
					flagoffsetkey = 1;
					Target.key = '0';
					//std::cerr << "Zero requested" << std::endl;
				}
				else if (event.key.keysym.sym == SDLK_r)
				{
					redotrialflag = true;
					Target.key = 'r';

					//std::cerr << "Redo requested" << std::endl;
				}
				/*
				else if (event.key.keysym.sym == SDLK_o)
				{
					flagoffsetkey = 0;
					Target.key = 'O';
					//std::cerr << "Offsets requested" << std::endl;
				}
				*/
				else if (event.key.keysym.sym == SDLK_SPACE)
				{
					nextstateflag = true;
					Target.key = 's';
					//std::cerr << "Advance requested" << std::endl;
				}
				else //if( event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0 )
				{
					Target.key = *SDL_GetKeyName(event.key.keysym.sym);  //(char)event.key.keysym.unicode;
					//std::cerr << Target.flag << std::endl;
				}
			}
			else if (event.type == SDL_KEYUP)
			{
				Target.key = '-1';
			}
			else if (event.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		if ((CurTrial >= NTRIALS) && (state == Finished) && (trialTimer->Elapsed() >= 10000))
			quit = true;

		// Get data from input devices
		if (inputs_updated > 0) // if there is a new frame of data
		{

			//updatedisplay = true;
			for (int a = ((trackstatus>0) ? 1 : 0); a <= ((trackstatus>0) ? BIRDCOUNT : 0); a++)
			{
				if (dataframe[a].ValidInput)
				{
					curs[a]->UpdatePos(float(dataframe[a].x),float(dataframe[a].y),float(dataframe[a].z));
					dataframe[a].vel = curs[a]->GetVel3D();

					//std::cerr << "Curs" << a << ": " << curs[a]->GetX() << " , " << curs[a]->GetY() << " , " << curs[a]->GetZ() << std::endl;
					//std::cerr << "Data" << a << ": " << dataframe[a].x << " , " << dataframe[a].y << " , " << dataframe[a].z << std::endl;

					if (recordData)  //only write out if we need to
						writer->Record(a, dataframe[a], Target);
					else
						Target.starttime = dataframe[a].time;
				}
			}

		} //end if inputs updated

		if (flagoffsetkey == 0) //set the offsets, if requested
		{
			if (trackstatus > 0)
			{
				sysconfig.PosOffset[0] = dataframe[HAND].x;
				sysconfig.PosOffset[1] = dataframe[HAND].y;
				sysconfig.PosOffset[2] = dataframe[HAND].z;
				std::cerr << "Data Pos Offsets set: " << sysconfig.PosOffset[0] << " , " << sysconfig.PosOffset[1] << " , "<< sysconfig.PosOffset[2] << std::endl;
			}
			else
				std::cerr << "Data Pos Requested but mouse is being used, request is ignored." << std::endl;
			flagoffsetkey = -1;
			Target.key = ' ';

		}
		else if (flagoffsetkey == 1) //set the startCircle position
		{	
			if (trackstatus > 0)
			{
				//startCircle->SetPos(curs[HAND]->GetMeanX(),curs[HAND]->GetMeanY(),curs[HAND]->GetMeanZ());
				startCircle->SetPos(curs[HAND]->GetX(),curs[HAND]->GetY(),curs[HAND]->GetZ());
				//std::cerr << "Curs " << HAND << ": " << curs[HAND]->GetX() << " , " << curs[HAND]->GetY() << " , " << curs[HAND]->GetZ() << std::endl;
			}
			else //mouse mode
			{
				//startCircle->SetPos(curs[0]->GetMeanX(),curs[0]->GetMeanY(),0.0f);
				startCircle->SetPos(curs[0]->GetX(),curs[0]->GetY(),0.0f);
				//std::cerr << "Curs0: " << curs[0]->GetX() << " , " << curs[0]->GetY() << " , " << curs[0]->GetZ() << std::endl;
			}

			//std::cerr << "Player: " << player->GetX() << " , " << player->GetY() << " , " << player->GetZ() << std::endl;
			std::cerr << "StartPos Offsets set: " << startCircle->GetX() << " , " << startCircle->GetY() << " , "<< startCircle->GetZ() << std::endl;

			flagoffsetkey = -1;
			Target.key = ' ';
		}

		if (!quit)
		{

			game_update(); // Run the game loop (state machine update)

			//if (updatedisplay)  //reduce number of calls to draw_screen -- does this speed up display/update?
			draw_screen();
		}

	}

	std::cerr << "Exiting..." << std::endl;

	clean_up();
	return 0;
}



//function to read in the name of the trial table file, and then load that trial table
int LoadTrFile(char *fname)
{

	//std::cerr << "LoadTrFile begin." << std::endl;

	char tmpline[100] = ""; 
	int ntrials = 0;

	//read in the trial file name
	std::ifstream trfile(fname);

	if (!trfile)
	{
		std::cerr << "Cannot open input file." << std::endl;
		return(-1);
	}
	else
		std::cerr << "Opened TrialFile " << TRIALFILE << std::endl;

	trfile.getline(tmpline,sizeof(tmpline),'\n');  //get the first line of the file, which is the name of the trial-table file

	while(!trfile.eof())
	{
		sscanf(tmpline, "%d %d %d %d", 
			&trtbl[ntrials].vision,
			&trtbl[ntrials].item,
			&trtbl[ntrials].dur,
			&trtbl[ntrials].practice);

		ntrials++;
		trfile.getline(tmpline,sizeof(tmpline),'\n');
	}

	trfile.close();
	if(ntrials == 0)
	{
		std::cerr << "Empty input file." << std::endl;
		//exit(1);
		return(-1);
	}
	return ntrials;
}


//initialization function - set up the experimental environment and load all relevant parameters/files
bool init()
{

	// Initialize Flock of Birds
	/* The program will run differently if the birds fail to initialize, so we
	* store it in a bool.
	*/

	int a;
	char tmpstr[80];
	char fname[50] = TRIALFILE;
	//char dataPath[50] = DATA_OUTPUT_PATH;

	//std::cerr << "Start init." << std::endl;

	std::cerr << std::endl;
	std::cout << "Initializing the tracker... " ;

	Target.starttime = 0;
	trackstatus = TrackBird::InitializeBird(&sysconfig);
	if (trackstatus <= 0)
	{
		std::cerr << "Tracker failed to initialize. Mouse Mode." << std::endl;
		std::cout << "failed. Switching to mouse mode." << std::endl;
	}
	else
		std::cout << "completed" << std::endl;

	std::cerr << std::endl;

	std::cout << "Initializing SDL and loading files... ";

	// Initialize SDL, OpenGL, SDL_mixer, and SDL_ttf
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "SDL failed to intialize."  << std::endl;
		return false;
	}
	else

		std::cerr << "SDL initialized." << std::endl;


	screen = SDL_CreateWindow("Code Base SDL2",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | (WINDOWED ? 0 : SDL_WINDOW_FULLSCREEN)); //SCREEN_BPP,
	//note, this call is missing the request to set to 32 bpp. unclear if this is going to be a problem

	if (screen == NULL)
	{
		std::cerr << "Screen failed to build." << std::endl;
		return false;
	}
	else
	{
		glcontext = SDL_GL_CreateContext(screen);
		std::cerr << "Screen built." << std::endl;
	}

	SDL_GL_SetSwapInterval(0); //ask for immediate updates rather than syncing to vertical retrace

	setup_opengl();

	//a = Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 512);  //initialize SDL_mixer
	a = Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024);  //initialize SDL_mixer, may have to play with the chunksize parameter to tune this a bit
	if (a != 0)
	{
		std::cerr << "Audio failed to initialize." << std::endl;
		return false;
	}
	else
		std::cerr << "Audio initialized." << std::endl;


	//initialize SDL_TTF (text handling)
	if (TTF_Init() == -1)
	{
		std::cerr << "SDL_TTF failed to initialize." << std::endl;
		return false;
	}
	else
		std::cerr << "SDL_TTF initialized." << std::endl;

	//turn off the computer cursor
	//SDL_ShowCursor(0);

	std::cerr << std::endl;

	// Load files and initialize pointers

	Image* tgtimages[NIMAGES];

	//load all the image files
	for (a = 0; a < NIMAGES; a++)
	{
		sprintf(tmpstr,"%s/%d.jpg",IMAGEPATH,a+1);
		tgtimages[a] = Image::LoadFromFile(tmpstr);
		if (tgtimages[a] == NULL)
			std::cerr << "Image Trace" << a << " did not load." << std::endl;
		else
		{
			items[a] = new Object2D(tgtimages[a]);
			std::cerr << "   Image " << a+1 << " loaded." << std::endl;
			items[a]->SetPos(PHYSICAL_WIDTH / 2, PHYSICAL_HEIGHT / 2);
		}
	}

	Image* instructim[NIMAGES];

	for (a = 0; a < NINSTRUCT; a++)
	{
		sprintf(tmpstr,"%s/Instruct%d.png",INSTRUCTPATH,a);
		instructim[a] = Image::LoadFromFile(tmpstr);
		if (instructim[a] == NULL)
			std::cerr << "Instruction " << a << " did not load." << std::endl;
		else
		{
			instructimages[a] = new Object2D(instructim[a]);
			std::cerr << "   Instruction " << a << " loaded." << std::endl;
			instructimages[a]->SetPos(PHYSICAL_WIDTH / 2, PHYSICAL_HEIGHT / 2);
		}
	}

	std::cerr << "Images loaded." << std::endl;


	//set up the start position
	startCircle = new Circle(PHYSICAL_WIDTH/2.0f, PHYSICAL_HEIGHT/2.0f, 0.0f, START_RADIUS*2, blkColor);
	startCircle->SetBorderWidth(0.001f);
	startCircle->SetBorderColor(blkColor);
	startCircle->BorderOff();
	startCircle->Off();
	//startCircle->On();
	std::cerr << "Start Circle: " << startCircle->GetX() << " , " << startCircle->GetY() << " : " << startCircle->drawState() << std::endl;
	

	//set up screen blackout rectangle
	blackRegion.SetNSides(4);
	blackRegion.SetCenteredRectDims(PHYSICAL_WIDTH,PHYSICAL_HEIGHT);
	blackRegion.SetRegionCenter(PHYSICAL_WIDTH/2,PHYSICAL_HEIGHT/2);
	blackRegion.SetRegionColor(blkColor);
	blackRegion.SetBorderColor(blkColor);
	blackRegion.BorderOff();
	blackRegion.Off();


	//initialize the LCD glasses control lines
	int status = -5;
	int devNum = 0;

	UCHAR Mask = 0xff;  
	//the bits in the upper nibble should be set to 1 to be output lines and 0 to be input lines (only used 
	//  in SetSensorBitBang() ). The bits in the lower nibble should be set to 1 initially to be active lines.

	status = Ftdi::InitFtdi(devNum,&ftHandle,1,Mask);
	std::cerr << "LCD Glasses (FTDI): " << status << std::endl;

	//set all lines low = lenses clear (signal passes through an inverter). Set the line low to make the lens translucent
	Ftdi::SetFtdiBitBang(ftHandle,Mask,4,0);
	Ftdi::SetFtdiBitBang(ftHandle,Mask,3,0);
	Ftdi::SetFtdiBitBang(ftHandle,Mask,2,0);
	Ftdi::SetFtdiBitBang(ftHandle,Mask,1,0);
	Target.lensstatus[0] = 1;
	Target.lensstatus[1] = 1;
	Target.visfdbk = -1;

	UCHAR dataBit;

	FT_GetBitMode(ftHandle, &dataBit);

	std::cerr << "DataByte: " << std::hex << dataBit << std::dec << std::endl;

	if (status==0)
	{
		printf("FTDI found and opened.\n");
		ftdiActive = true;
	}
	else
	{
		if (status == 1)
			std::cerr << "   Failed to create device list." << std::endl;
		else if (status == 2)
			std::cerr << "   FTDI ID=" << devNum << " not found." << std::endl;
		else if (status == 3)
			std::cerr << "   FTDI " << devNum << " failed to open." << std::endl;
		else if (status == 4)
			std::cerr << "   FTDI " << devNum << " failed to start in BitBang mode." << std::endl;
		else
			std::cerr << "UNDEFINED ERROR!" << std::endl;

		ftdiActive = false;
	}

	std::cerr << std::endl;



	//load trial table from file
	NTRIALS = LoadTrFile(fname);
	//std::cerr << "Filename: " << fname << std::endl;

	if(NTRIALS == -1)
	{
		std::cerr << "Trial File did not load." << std::endl;
		return false;
	}
	else
		std::cerr << "Trial File loaded: " << NTRIALS << " trials found." << std::endl;

	//assign the data-output file name based on the trial-table name 
	std::string savfile;
	savfile.assign(fname);
	savfile.insert(savfile.rfind("."),"_data");

	std::strcpy(fname,savfile.c_str());

	std::cerr << "SavFileName: " << fname << std::endl;

	//writer = new DataWriter(&sysconfig,Target,"InstructFile");  //set up the data-output file
	//recordData = true;
	didrecord = false;
	recordData = false;


	// set up the cursors
	if (trackstatus > 0)
	{
		/* Assign birds to the same indices of controller and cursor that they use
		* for the Flock of Birds
		*/
		for (a = 1; a <= BIRDCOUNT; a++)
		{
			curs[a] = new HandCursor(0.0f, 0.0f, CURSOR_RADIUS*2, cursColor);
			curs[a]->BorderOff();
			curs[a]->SetOrigin(0.0f, 0.0f);
		}

		player = curs[HAND];  //this is the cursor that represents the hand
		std::cerr << "Player = " << HAND << std::endl;
	}
	else
	{
		// Use mouse control
		curs[0] = new HandCursor(0.0f, 0.0f, CURSOR_RADIUS*2, cursColor);
		curs[0]->SetOrigin(0.0f, 0.0f);
		player = curs[0];
		std::cerr << "Player = 0"  << std::endl;
	}

	//player->On();
	player->Off();


	//load sound files
	startbeep = new Sound("Resources/startbeep.wav");
	stopbeep = new Sound("Resources/beep.wav");
	scorebeep = new Sound("Resources/coin.wav");
	errorbeep = new Sound("Resources/errorbeep1.wav");

	for (a = 0; a < NIMAGES; a++)
	{
		sprintf(tmpstr,"%s/%d.wav",AUDIOPATH,a+1);
		itemsounds[a] = new Sound(tmpstr);
		if (itemsounds[a] == NULL)
			std::cerr << "   Audiofile " << a+1 << " did not load." << std::endl;
		else
			std::cerr << "   Audiofile " << a+1 << " loaded." << std::endl;
	}
	
	std::cerr << "Audio loaded: " << a-1 << "." << std::endl;


	//set up placeholder text
	endtext = Image::ImageText(endtext, "Block ended.","arial.ttf", 28, textColor);
	endtext->Off();

	readytext = Image::ImageText(readytext, "Get ready...","arial.ttf", 28, textColor);
	readytext->Off();
	stoptext = Image::ImageText(stoptext, "STOP!","arial.ttf", 32, textGrayColor);
	stoptext->Off();
	holdtext = Image::ImageText(holdtext, "Wait until the go signal!","arial.ttf", 28, textColor);
	holdtext->Off();
	proceedtext = Image::ImageText(proceedtext, "Zero the tracker and proceed when ready.","arial.ttf", 28, textColor);
	proceedtext->Off();
	returntext = Image::ImageText(returntext, "Please return to start position.","arial.ttf", 28, textColor);
	returntext->Off();

	trialinstructtext = Image::ImageText(trialinstructtext, "Press (space) to advance or (r) to repeat trial.","arial.ttf", 12, textGrayColor);
	trialinstructtext->Off();
	redotext = Image::ImageText(redotext, "Press (r) to repeat trial.","arial.ttf", 12, textGrayColor);
	redotext->Off();
	recordtext = Image::ImageText(recordtext, "Recording...","arial.ttf", 12, textGrayColor);
	recordtext->Off();
	mousetext = Image::ImageText(mousetext, "Trackers not found! Mouse-emulation mode.","arial.ttf", 12, textGrayColor);
	if (trackstatus > 0)
		mousetext->Off();
	else
		mousetext->On();

	//set up trial number text image
	trialnum = Image::ImageText(trialnum,"0_0","arial.ttf", 12,textGrayColor);
	trialnum->On();

	hoverTimer = new Timer();
	trialTimer = new Timer();
	movTimer = new Timer();

	// Set the initial game state
	state = Idle; 

	std::cerr << "Initialization complete." << std::endl;
	std::cout << "completed." << std::endl;

	return true;
}


static void setup_opengl()
{
	glClearColor(1, 1, 1, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* The default coordinate system has (0, 0) at the bottom left. Width and
	* height are in meters, defined by PHYSICAL_WIDTH and PHYSICAL_HEIGHT
	* (config.h). If MIRRORED (config.h) is set to true, everything is flipped
	* horizontally.
	*/
	glOrtho(MIRRORED ? PHYSICAL_WIDTH : 0, MIRRORED ? 0 : PHYSICAL_WIDTH,
		0, PHYSICAL_HEIGHT, -1.0f, 1.0f);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);

}


//end the program; clean up everything neatly.
void clean_up()
{
	std::cout << "Shutting down." << std::endl;

	delete startbeep;
	delete scorebeep;
	delete errorbeep;

	for (int a = 0; a < NIMAGES; a++)
	{
		delete items[a];
		delete itemsounds[a];
	}

	for (int a = 0; a < NINSTRUCT; a++)
		delete instructimages[a];
	
	delete endtext;
	delete readytext;
	delete trialinstructtext;
	delete stoptext;
	delete holdtext;
	delete redotext;
	delete trialnum;
	delete recordtext;
	delete proceedtext;
	delete returntext;
	delete mousetext;

	//std::cerr << "Deleted all objects." << std::endl;

	int status = Ftdi::CloseFtdi(ftHandle,1);

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(screen);
	Mix_CloseAudio();
	TTF_Quit();
	SDL_Quit();

	std::cerr << "Shut down SDL." << std::endl;

	if (trackstatus > 0)
	{
		TrackBird::ShutDownBird(&sysconfig);
		std::cerr << "Shut down tracker." << std::endl;
	}

	freopen( "CON", "w", stderr );

}

//control what is drawn to the screen
static void draw_screen()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//draw the image specified
	Target.trace = -1;
	for (int a = 0; a < NIMAGES; a++)
	{
		items[a]->Draw();
		if (items[a]->DrawState())
			Target.trace = a;
	}
	//draw the instructions specified
	for (int a = 0; a < NINSTRUCT; a++)
	{
		instructimages[a]->Draw();
		if (instructimages[a]->DrawState())
			Target.instruct = a;
	}

	blackRegion.Draw();

	startCircle->Draw();
	//if (startCircle->drawState())
		//std::cerr << " Start circle drawn at " << startCircle->GetX() << " , " << startCircle->GetY() << std::endl;

	player->Draw();


	// Draw text
	endtext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);

	proceedtext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	readytext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	stoptext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	holdtext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	returntext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*1.0f/2.0f);

	trialinstructtext->DrawAlign(PHYSICAL_WIDTH*0.5f/24.0f,PHYSICAL_HEIGHT*0.5f/24.0f,3);
	redotext->DrawAlign(PHYSICAL_WIDTH*0.5f/24.0f,PHYSICAL_HEIGHT*0.5f/24.0f,3);
	recordtext->DrawAlign(PHYSICAL_WIDTH*0.5f/24.0f,PHYSICAL_HEIGHT*0.5f/24.0f,3);
	mousetext->DrawAlign(PHYSICAL_WIDTH*23.5f/24.0f,PHYSICAL_HEIGHT*23.5f/24.0f,1);
	//write the trial number
	trialnum->Draw(PHYSICAL_WIDTH*23.0f/24.0f, PHYSICAL_HEIGHT*0.5f/24.0f);

	SDL_GL_SwapWindow(screen);
	glFlush();

}


//game update loop - state machine controlling the status of the experiment
bool mvtStarted = false;
bool falsestart = false;
bool cuestop = false;

bool reachedvelmin = false;
bool reachedvelmax = false;

bool mvmtEnded = false;
bool hitTarget = false;
bool hitRegion = false;
bool hitPath = false;

float LastPeakVel = 0;
bool returntostart = true;

bool writefinalscore;

void game_update()
{

	switch (state)
	{
	case Idle:
		// This state only happens at the start of the experiment, and is just a null pause until the experimenter is ready to begin.

		Target.trial = -1;
		Target.trace = -1;
		Target.instruct = -1;
		Target.lat = -999;
		Target.redo = -1;
		Target.trial = -1;
		Target.visfdbk = -1;

		endtext->Off();
		readytext->Off();
		stoptext->Off();
		trialinstructtext->Off();
		holdtext->Off();
		recordtext->Off();
		proceedtext->On();

		blackRegion.Off();

		//startCircle->On();

		if (ftdiActive)
		{
			UCHAR Mask = 0xff; 

			//set the lines low to be clear
			Ftdi::SetFtdiBitBang(ftHandle,Mask,4,0);
			Ftdi::SetFtdiBitBang(ftHandle,Mask,3,0);
			Ftdi::SetFtdiBitBang(ftHandle,Mask,2,0);
			Ftdi::SetFtdiBitBang(ftHandle,Mask,1,0);
			Target.lensstatus[0] = 1;
			Target.lensstatus[1] = 1;
		}
		else
		{
			Target.lensstatus[0] = -99;
			Target.lensstatus[1] = -99;
		}

		if (!returntostart)
		{
			//if we haven't yet gotten back to the start target yet
			if (player->Distance(startCircle) < START_RADIUS)
				returntostart = true;
		}

		//shut off all images
		for (int a = 0; a < NIMAGES; a++)
			items[a]->Off();

		for (int a = 0; a < NINSTRUCT; a++)
			instructimages[a]->Off();


		if( returntostart && nextstateflag)  //hand is in the home position and the experimenter asked to advance the experiment
		{
			hoverTimer->Reset();
			trialTimer->Reset();

			proceedtext->Off();

			nextstateflag = false;

			Target.key = ' ';

			std::cerr << "Leaving IDLE state." << std::endl;

			state = Instruct;
		}
		break;

	case Instruct: 
		//this state is just for displaying instructions, and only happens at the start of the experiment.

		//display instructions
		//std::cerr << " Instruction " << trtbl[0].practice*2+trtbl[0].vision << std::endl;
		instructimages[trtbl[0].practice*2+trtbl[0].vision]->On();
		Target.instruct = trtbl[0].practice*2+trtbl[0].vision;

		// If experimenter hits advance button, move on (after delay to make sure not triggered from previous keypress)
		if (hoverTimer->Elapsed() > 1000 && nextstateflag)
		{
			nextstateflag = false;

			redotrialflag = false;

			instructimages[trtbl[0].practice*2+trtbl[0].vision]->Off();

			falsestart = false;
			mvtStarted = false;

			//Target.trial = CurTrial+1;

			Target.key = ' ';

			hoverTimer->Reset();
			returntext->Off();

			state = WaitStim;
			std::cerr << "Leaving INSTRUCT state." << std::endl;

		}
		break;

	case WaitStim:
		//this state is just a "pause" state between trials.
		
		trialnum->Off();  //is it confusing that we show the "last" trial here, since we don't know if we want to redo or advance yet?

		returntext->Off();
		Target.item = -1;

		blackRegion.Off();

		//show "pause" text, and instructions to experimenter at the bottom of the screen
		readytext->On();
		holdtext->Off();
		if (falsestart && hoverTimer->Elapsed() < 1000)
		{
			readytext->Off();
			holdtext->On();
		}
		cuestop = false;

		//recordtext->Off();

		if (!falsestart)
			trialinstructtext->On();
		else
			redotext->On();
		

		if (hoverTimer->Elapsed() > 1000 && (nextstateflag || redotrialflag) )
		{
			
			if (!falsestart && (nextstateflag || (redotrialflag && CurTrial < 0) )) //if accidentally hit "r" before the first trial, ignore this error!
			{
				nextstateflag = false;
				redotrialflag = false;
				numredos = 0;
				CurTrial++;  //we started the experiment with CurTrial = -1, so now we are on the "next" trial (or first trial)
				Target.trial = CurTrial+1;
				Target.redo = 0;
				Target.visfdbk = -1;
				Target.visfdbk = curtr.vision;
				Target.item = curtr.item;
				Target.key = ' ';
			}
			else if ( (!falsestart && (redotrialflag && CurTrial >= 0)) ||  (falsestart && (nextstateflag || redotrialflag)) )
			{
				redotrialflag = false;
				falsestart = false;
				numredos++;
				Target.trial = CurTrial+1; //we do not update the trial number
				Target.item = curtr.item;
				Target.redo = numredos; //count the number of redos
				Target.key = ' ';
			}
			
			readytext->Off();
			redotext->Off();
			holdtext->Off();
			trialinstructtext->Off();

			//if we have reached the end of the trial table, quit
			if (CurTrial >= NTRIALS)
			{
				std::cerr << "Going to FINISHED state." << std::endl;
				trialTimer->Reset();
				writer->Close();
				state = Finished;
			}
			else
			{
				std::stringstream texttn;
				texttn << CurTrial+1 << "_" << numredos+1;  //CurTrial starts from 0, so we add 1 for convention.
				trialnum = Image::ImageText(trialnum,texttn.str().c_str(),"arial.ttf", 12,textColor);
				std::cerr << "Trial " << CurTrial+1 << " Redo " << numredos+1 << " started at " << SDL_GetTicks() << std::endl;

				//set up the new data file and start recording
				recordData = true;
				recordtext->On();
				//Target.starttime = SDL_GetTicks();
				if (!didrecord)  //detect if a file has ever been opened; if so shut it. if not, skip this.
					didrecord = true;
				else
					writer->Close();  //this will this cause an error if no writer is open
				std::string savfname;
				savfname.assign(TRIALFILE);
				savfname = savfname.substr(savfname.rfind("/")+1);  //cut off the file extension
				savfname = savfname.replace(savfname.rfind(".txt"),4,"");  //cut off the file extension
				std::stringstream datafname;
				//datafname << savfname.c_str() << "_" << SUBJECT_ID << "_Item" << curtr.item << "-" << numredos+1 << "_" << (curtr.vision == 1 ? "VF" : "NVF");
				datafname << DATAPATH << savfname.c_str() << "_" << SUBJECT_ID << "_Item" << curtr.item << "-" << numredos+1;

				writer = new DataWriter(&sysconfig,Target,datafname.str().c_str());  //create the data-output file

				//reset the trial timers
				hoverTimer->Reset();
				trialTimer->Reset();

				//show the stimulus and play the audio
				items[curtr.item-1]->On();
				itemsounds[curtr.item-1]->Play();
				Target.trace = curtr.item;

				mvtStarted = false;
				mvmtEnded = false;
				
				//move to the next state
				state = ShowStim;
				std::cerr << "Leaving WAITSTATE state." << std::endl;

			}

		}
		break;


	case ShowStim:
		//show the stimulus and wait

		//std::cerr << "Item: " << curtr.item-1 << " drawn." << std::endl;
		//items[curtr.item-1]->On();

		if (player->Distance(startCircle) > START_RADIUS)
		{	//detected movement too early; 
			//std::cerr << "Player: " << player->GetX() << " , " << player->GetY() << " , " << player->GetZ() << std::endl;
			//std::cerr << "Circle: " << startCircle->GetX() << " , " << startCircle->GetY() << " , " << startCircle->GetZ() << std::endl;
			//std::cerr << "Distance: " << player->Distance(startCircle) << std::endl;
			mvtStarted = true;
			items[curtr.item-1]->Off();
			holdtext->On();
			hoverTimer->Reset();
		}

		if (mvtStarted && !falsestart && (hoverTimer->Elapsed() > 1000) )  //moved too soon!
		{
			//state = WaitStim;
			returntext->On();
			nextstateflag = false;
			redotrialflag = false;
			falsestart = true;
			Target.key = ' ';
			trialTimer->Reset();
		}

		if (falsestart && (player->Distance(startCircle) < START_RADIUS)) //(falsestart && (trialTimer->Elapsed() > 1000))
		{
			nextstateflag = false;
			redotrialflag = false;
			Target.key = ' ';
			recordtext->Off();
			hoverTimer->Reset();
			state = WaitStim;
			std::cerr << "False start; returning to WAITSTIM state." << std::endl;
		}

		if (!mvtStarted && (trialTimer->Elapsed() > curtr.dur) )  //prompt start signal
		{
			items[curtr.item-1]->Off(); //shut off visual display

			//Target.visfdbk = curtr.vision;

			//shut off vision, if desired
			if (curtr.vision == 0)
			{
				if (ftdiActive)
				{
					UCHAR Mask = 0xff; 

					//set the lines high to be translucent
					Ftdi::SetFtdiBitBang(ftHandle,Mask,4,1);
					Ftdi::SetFtdiBitBang(ftHandle,Mask,3,1);
					Ftdi::SetFtdiBitBang(ftHandle,Mask,2,1);
					Ftdi::SetFtdiBitBang(ftHandle,Mask,1,1);
					Target.lensstatus[0] = 0;
					Target.lensstatus[1] = 0;
				}
				else
				{
					Target.lensstatus[0] = -99;
					Target.lensstatus[1] = -99;
				}

				blackRegion.On();
			}


			//play start tone
			startbeep->Play();
			mvtStarted = false;
			mvmtEnded = false;
			movTimer->Reset();
			hoverTimer->Reset();
			trialTimer->Reset();

			recordData = true;
			recordtext->On();

			readytext->Off();
			holdtext->Off();

			std::cerr << "Leaving SHOWSTIM state." << std::endl;

			nextstateflag = false;

			state = Active;
		}

		break;

	case Active:

		//detect the onset of hand movement, for calculating latency
		if (!mvtStarted && (player->Distance(startCircle) > START_RADIUS))
		{
			mvtStarted = true;
			Target.lat = movTimer->Elapsed();
			movTimer->Reset();
		}


		//detect movement offset
		if (!mvmtEnded && mvtStarted && (player->GetVel3D() < VEL_MVT_TH) && (movTimer->Elapsed()>200))
		{
			mvmtEnded = true;
			//Target.dur = movTimer->Elapsed();
			hoverTimer->Reset();
			//std::cerr << "Mvmt Ended: " << float(SDL_GetTicks()) << std::endl;
		}

		if (mvmtEnded && (hoverTimer->Elapsed() >= VEL_END_TIME))
		{
			Target.dur = movTimer->Elapsed()-VEL_END_TIME;  //if the hand was still long enough, call this the "end" of the movement!
		}
		else if (mvmtEnded && (hoverTimer->Elapsed() < VEL_END_TIME) && (player->GetVel3D() > VEL_MVT_TH))
			mvmtEnded = false;  //just kidding, the movement is still going...

		//if trial duration is exceeded, display a "stop" signal but do not change state until experimenter propmts
		if (!nextstateflag && !cuestop && (trialTimer->Elapsed() > TRIAL_DURATION) )
		{
			stoptext->On();
			stopbeep->Play();
			cuestop = true;
		}

		//if the experimenter ends the trial
		if (nextstateflag)
		{
			nextstateflag = false;
			Target.key = ' ';

			if (!mvmtEnded)
			{
				//if the movement hasn't ended yet, we will call this the "duration"
				Target.dur = movTimer->Elapsed();
			}

			//unblind the participant
			Target.visfdbk = -1;
			if (ftdiActive)
			{
				UCHAR Mask = 0xff; 

				//set the lines low to be transparent
				Ftdi::SetFtdiBitBang(ftHandle,Mask,4,0);
				Ftdi::SetFtdiBitBang(ftHandle,Mask,3,0);
				Ftdi::SetFtdiBitBang(ftHandle,Mask,2,0);
				Ftdi::SetFtdiBitBang(ftHandle,Mask,1,0);
				Target.lensstatus[0] = 1;
				Target.lensstatus[1] = 1;
			}
			else
			{
				Target.lensstatus[0] = -99;
				Target.lensstatus[1] = -99;
			}

			blackRegion.Off();

			returntostart = false;

			stoptext->Off();

			//stop recording data
			recordtext->Off();
			recordData = false;

			//go to mext state
			trialTimer->Reset();// = SDL_GetTicks();
			state = EndTrial;

		}

		break;

	case EndTrial:

		returntext->On();

		if (player->Distance(startCircle) > START_RADIUS)
			returntostart = false;
		else
			returntostart = true;

		//wait for participant to return to start before allowing entry into next trial
		if (returntostart)
		{
			returntext->Off();
			
			nextstateflag = false;
			std::cerr << "Ending Trial." << std::endl;
			state = WaitStim;

		}

		break;

	case Finished:
		// Trial table ended, wait for program to quit

		endtext->On();

		if (trialTimer->Elapsed() > 5000)
			quit = true;


		break;

	}
}

