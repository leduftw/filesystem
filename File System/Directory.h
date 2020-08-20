#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "part.h"
#include "FS.h"
#include "Cluster.h"

/*
	Entries in data cluster of root directory have size of 32 bytes.
*/
#pragma pack(push, 1)  // disable padding for following structure
struct DirectoryEntry {
	char fname[FNAMELEN];  // 8 bytes
	char ext[FEXTLEN];  // 3 bytes

	char notUsed = 0;  // 1 byte

	ClusterNo firstLevelCluster;  // 4 bytes
	BytesCnt fileSize;  // 4 bytes

	char free[12];  // 12 bytes
};  // total: 32 bytes
#pragma pack(pop)  // returns to normal settings

/*
	Directory class only holds one data cluster of root directory.
*/
class Directory : public Cluster {

	DirectoryEntry *entries;

public:

	/*
		Loads data from given cluster on partition to memory.
	*/
	Directory(Partition *, ClusterNo);

	/*
		Number of entries in data cluster of root directory.
		In this case it is 2048 / 32 = 64.
	*/
	int size() const override {
		return ClusterSize / sizeof(DirectoryEntry);
	}

	DirectoryEntry* getEntries() const {
		return entries;
	}

	/*
		Data cluster of root directory can act as an array. Parameter i is in [0..size()-1].
	*/
	DirectoryEntry& operator[](int i) {
		return entries[i];
	}

	/*
		Data cluster of root directory can act as an array. Parameter i is in [0..size()-1].
	*/
	const DirectoryEntry& operator[](int i) const {
		return entries[i];
	}

};

#endif
