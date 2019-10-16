//Modified to add target inputs, 9/10/2012 AW

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "DataWriter.h"


using namespace std;

DataWriter::DataWriter(TrackSYSCONFIG *sysconfig, TargetFrame Target, const char* filename)
{

	//record current date/time
	time_t current_time = time(0);
	tm* ltm = localtime(&current_time);
	stringstream ss1, ss2, ss3;
	
	iswriting = 0;

	ss1 << setw(4) << setfill('0') << ltm->tm_year + 1900;
	ss1 << setw(2) << setfill('0') << ltm->tm_mon + 1;
	ss1 << setw(2) << setfill('0') << ltm->tm_mday;
	
	ss2 << setw(2) << setfill('0') << ltm->tm_hour;
	ss2 << setw(2) << setfill('0') << ltm->tm_min;
	ss2 << setw(2) << setfill('0') << ltm->tm_sec;


	// If no filename was supplied, use the date and time
	if (filename == NULL)
	{
		ss3 << ss1.str() << "_" << ss2.str() << ".dat";
		file.open(ss3.str(), ios::out);
	}
	else
	{
		//append timestamp to prevent overwrites
		ss3 << filename << "_" << ss1.str() << ss2.str() << ".dat";
		file.open(ss3.str(), ios::out);
	}

	// Write headers to file
	if (file.is_open())
	{

		//write current date
		file << "Date " << ss1.str() << endl;
		file << "Time " << ss2.str() << endl;
		//write trial parameters of interest
		file << "SubjectID " << SUBJECT_ID << endl;
		file << "Visual_Feedback " << Target.visfdbk << endl;
		file << "Trial " << Target.trial << endl;
		file << "Redo " <<	Target.redo << endl;
		file << "Item " << Target.item << endl;
		//write bird config parameters of interest
		file << "Tracker " << (sysconfig->trackType ? "TrakStar" : "FOB") << endl;
		file << "Number_of_Birds " << sysconfig->birdCount << endl;
		file << "Sampling_Rate " << sysconfig->measureRate << endl;
		file << "Report_Rate " << sysconfig->reportRate << endl;
		

		file << "--" << endl;
		
		file << "Device_Num "
			 << "TrackerTime "
			 << "Trial "
			 << "Redo "
			 << "Stimulus "
			 << "HandX "
			 << "HandY "
			 << "HandZ "
			 << "HandAzim "
			 << "HandElev "
			 << "HandRoll "
			 << "HandRotMat11 "
			 << "HandRotMat12 "
			 << "HandRotMat13 "
			 << "HandRotMat21 "
			 << "HandRotMat22 "
			 << "HandRotMat23 "
			 << "HandRotMat31 "
			 << "HandRotMat32 "
			 << "HandRotMat33 "
			 << "Velocity "
			 << "Latency "
			 << "Duration "
			 << "Vision "
			 << "Lens1Status "
			 << "Lens2Status "
			 << endl;

		file << "-----" << endl;  //flag designator for finding start of the data stream.  everything above is header

		iswriting = 1;

	}
}

DataWriter::~DataWriter()
{
	iswriting = 0;
	file.close();
}
 
void DataWriter::Record(int deviceNo, TrackDATAFRAME frame, TargetFrame Target)
{
	std::streamsize ss;
	ss = std::cout.precision();

	// Write data
	if (file.is_open())
	{
		file << deviceNo << " "
			<< std::fixed << showpoint << std::setprecision(5) 
			<< frame.time-Target.starttime << " "
			<< std::resetiosflags( std::ios::fixed | std::ios::showpoint )
			<< Target.trial << " "
			<< Target.redo << " "
			<< Target.trace << " "
			<< std::fixed << showpoint << std::setprecision(8) 
			<< frame.x << " "
			<< frame.y << " "
			<< frame.z << " "
			<< frame.azimuth << " "
			<< frame.elevation << " "
			<< frame.roll << " "
			<< frame.anglematrix[0][0] << " "
			<< frame.anglematrix[0][1] << " "
			<< frame.anglematrix[0][2] << " "
			<< frame.anglematrix[1][0] << " "
			<< frame.anglematrix[1][1] << " "
			<< frame.anglematrix[1][2] << " "
			<< frame.anglematrix[2][0] << " "
			<< frame.anglematrix[2][1] << " "
			<< frame.anglematrix[2][2] << " "
			<< std::resetiosflags( std::ios::fixed | std::ios::showpoint ) << std::setprecision(ss)
			<< frame.vel << " "
			<< Target.lat << " "
			<< Target.dur << " "
			<< Target.visfdbk << " "
			<< Target.lensstatus[0] << " "
			<< Target.lensstatus[1]
			<< endl;
	}
}

void DataWriter::Close()
{
	if (iswriting == 1)
		file.close();
	
	iswriting = 0;
}