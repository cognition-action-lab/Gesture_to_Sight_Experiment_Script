#ifndef DATAWRITER_H
#define DATAWRITER_H
#pragma once

#include <fstream>
#include "TargetFrame.h"
#include "InputFrame.h"
#include "TrackBird.h"

// Records data
class DataWriter
{
private:
	std::ofstream file;
	int iswriting;
public:
	/* filename defines a custom name to save the file as. If filename is NULL,
	 * a name will be generated automatically.
	 */
	DataWriter(TrackSYSCONFIG *sysconfig, TargetFrame Target, const char* filename = NULL);
	DataWriter(TargetFrame Target, const char* filename = NULL);
	DataWriter();
	~DataWriter();
	// Record a new line of data
	// deviceNo is a number identifying the device sending the data
	// frame is the frame of input data
	void Record(int deviceNo, TrackDATAFRAME frame, TargetFrame Target);
	void Close();

	void Record(TargetFrame Target);

};

#endif
