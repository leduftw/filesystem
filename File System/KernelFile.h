#pragma once

#include <string>

#include "part.h"
#include "Disk.h"
#include "LRUCache.h"

using namespace std;

typedef unsigned long BytesCnt;

class KernelFile {
public:

	/*
		Creates file object. Initializes all necessary data members.
	*/
	KernelFile(Disk *d, Cache *c, ClusterNo firstLevel, ClusterNo dir, int entry, char mode, BytesCnt sz);

	/*
		Called upon closing.
	*/
	~KernelFile();

	/*
		Returns 1 on success and 0 on failure.
	*/
	char write(BytesCnt, char *buffer);

	/*
		Returns 0 on failure or actual number of bytes read from file.
	*/
	BytesCnt read(BytesCnt, char *buffer);

	/*
		Sets new absolute position of file cursor. Returns 1 on success and 0 on failure.
	*/
	char seek(BytesCnt);

	/*
		Returns current position of file cursor.
	*/
	BytesCnt filePos();

	/*
		Checks if filePos() == END_OF_FILE.
		Returns:
			0 - False,
			1 - Error,
			2 - True.
	*/
	char eof();

	/*
		Returns current size of file in bytes.
		Only data clusters of file are included.
	*/
	BytesCnt getFileSize();

	/*
		Deletes file from filePos() to END_OF_FILE. Returns 1 on success and 0 on failure.
	*/
	char truncate();

private:

	friend class KernelFS;

	bool isClosed;
	BytesCnt cursor, fileSize;

	Disk *disk;
	Cache *cache;

	ClusterNo first;  // first-level index cluster number for this file

	ClusterNo dir;  // directory data cluster number which contains descriptor for this file
	int entryNo;  // number of entry which contains descriptor for this file

	char mode;

	string path;

};
