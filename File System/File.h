#ifndef FILE_H
#define FILE_H

typedef unsigned long BytesCnt;
typedef unsigned long ClusterNo;

#include "KernelFile.h"

// class KernelFile;
class Partition;

class Disk;

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

	File(Disk *disk, ClusterNo firstLevel, ClusterNo dir, char mode, BytesCnt sz);  // instance of File can only be created with FS::open()
	KernelFile *myImpl;
};

#endif
