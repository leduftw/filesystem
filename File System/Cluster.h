#ifndef CLUSTER_H
#define CLUSTER_H

#include <cstring>

#include "part.h"
#include "FS.h"

typedef ClusterNo IndexEntry;  // entries for first-level and second-level index have size of 32 bits

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
	Cluster class represents arbitrary cluster from partition with no particular structure.
*/
class Cluster {
protected:

	static const int sz;

	Partition *partition;

	ClusterNo clusterNo;

	char *data;

public:

	/*
		Loads bytes from cluster with number clusterNo from partition to memory.
	*/
	Cluster(Partition *, ClusterNo);

	/*
		Saves data to disk and deallocates resources.
	*/
	~Cluster();

	/*
		Returns the size of this cluster.
	*/
	int size() const {
		return sz;
	}

	/*
		Save contents of this cluster to disk.
	*/
	bool save() {
		return partition->writeCluster(clusterNo, (const char *)data);
	}

	/*
		Returns cluster number that this cluster represents.
	*/
	ClusterNo getClusterNo() const {
		return clusterNo;
	}

	/*
		Returns raw data from this cluster.
	*/
	char* getData() const {
		return data;
	}

	/*
		Sets all bits in data to 0.
	*/
	bool clear() {
		return memset(data, 0, ClusterSize);
	}

};

#endif
