#ifndef FS_H
#define FS_H

typedef long FileCnt;
typedef unsigned long BytesCnt;

const unsigned int FNAMELEN = 8;
const unsigned int FEXTLEN = 3;

class KernelFS;
class Partition;
class File;

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
	FS() { }
	static KernelFS *myImpl;
};

#endif
