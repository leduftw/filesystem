#ifndef KERNELFS_H
#define KERNELFS_H

#include <Windows.h>
#include "Disk.h"

#define wait(semaphore) WaitForSingleObject(semaphore, INFINITE)
#define signal(semaphore) ReleaseSemaphore(semaphore, 1, NULL)

typedef long FileCnt;
typedef unsigned long ClusterNo;

class Partition;
class File;

class KernelFS {
public:

	/*
		Releases all allocated resources.
	*/
	~KernelFS();

	/*
		Mounts the partition. Returns 1 on success and 0 on failure.
	*/
	char mount(Partition *partition);

	/*
		Unmounts the partition. Returns 1 on success and 0 on failure.
	*/
	char unmount();

	/*
		Formats the partition. Returns 1 on success and 0 on failure.
	*/
	char format();

	/*
		Returns number of files in root directory or -1 in case of failure.
	*/
	FileCnt readRootDir();

	/*
		Checks if file given with absolute path exists.
		Absolute path has same structure as paths in Linux.
	*/
	char doesExist(char *fname);

	File* open(char *fname, char mode);
	char deleteFile(char *fname);

private:
	// Second argument is initial count and third is maximum count
	HANDLE canMount = CreateSemaphore(NULL, 1, 1, NULL);
	HANDLE canUnmount = CreateSemaphore(NULL, 0, 1, NULL);
	HANDLE canFormat = CreateSemaphore(NULL, 0, 1, NULL);
	HANDLE mutex = CreateSemaphore(NULL, 1, 1, NULL);

	Disk *disk = nullptr;

	// Number of currently opened files
	int openFilesCnt = 0;

	// If format is called, new tries of opening files fail until formatting is done
	bool formatCalled = false;

	/*
		Checks if all files are closed.
	*/
	bool allFilesClosed() const {
		return openFilesCnt == 0;
	}

};

#endif
