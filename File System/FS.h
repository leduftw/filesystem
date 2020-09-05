#pragma once

typedef long FileCnt;
typedef unsigned long BytesCnt;

const unsigned int FNAMELEN = 8;
const unsigned int FEXTLEN = 3;

class KernelFS;
class Partition;
class File;
class KernelFile;

/*
	Wrapper for KernelFS class.
*/
class FS {
public:

	~FS() { }

	static char mount(Partition *);

	static char unmount();

	static char format();

	static FileCnt readRootDir();

	static char doesExist(char *fname);

	static File* open(char *fname, char mode);

	static char deleteFile(char *fname);

protected:

	friend class KernelFile;

	FS() { }
	static KernelFS *myImpl;

};
