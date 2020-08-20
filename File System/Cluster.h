#ifndef CLUSTER_H
#define CLUSTER_H

#include <cstring>
#include "part.h"

/*
	Cluster class represents arbitrary cluster from partition with no particular structure.
*/
class Cluster {
protected:

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
	virtual ~Cluster();

	/*
		Returns the size of this cluster.
	*/
	virtual int size() const {
		return ClusterSize;
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
