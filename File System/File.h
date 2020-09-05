#pragma once

typedef unsigned long BytesCnt;
typedef unsigned long ClusterNo;

#include "KernelFile.h"

// class KernelFile;
class Partition;

class Disk;

/*
	Wrapper for KernelFile class.
*/
class File {
public:
	~File();

	char write(BytesCnt, char *buffer);

	BytesCnt read(BytesCnt, char *buffer);

	char seek(BytesCnt);

	BytesCnt filePos();

	char eof();

	BytesCnt getFileSize();

	char truncate();

private:
	friend class FS;
	friend class KernelFS;

	File(Disk *disk, Cache *c, ClusterNo firstLevel, ClusterNo dir, int entry, char mode, BytesCnt sz);  // instance of File can only be created with FS::open()
	KernelFile *myImpl;
};
