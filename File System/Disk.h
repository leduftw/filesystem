#pragma once

#include "part.h"
#include "BitVector.h"
#include "Cluster.h"

class Disk {

	Partition *partition;

	// Bit vector and first-level index of root directory are always cached in memory
	BitVector *bitVector;
	Cluster *firstLevelDirectory;

public:

	/*
		Loads clusters that represent bit vector and first-level index of root directory to memory.
	*/
	Disk(Partition *);

	/*
		Saves bit vector data and first-level index of root directory data to disk.
	*/
	~Disk();

	/*
		Returns partition.
	*/
	Partition* getPartition() const {
		return partition;
	}

	/*
		Returns bit vector.
	*/
	BitVector* getBitVector() const {
		return bitVector;
	}

	/*
		Returns first-level index of root directory.
	*/
	Cluster* getFirstLevelDirectory() const {
		return firstLevelDirectory;
	}

	/*
		Initialization of bit vector during formating.
		If there are m clusters for bit vector and 1 for first-level index of root directory,
		and if n is the total number of clusters, then this operation resets first m + 1 bits,
		and sets next n - m - 1 bits in clusters dedicated for bit vector.
	*/
	bool initializeBitVector();

	/*
		Initialization of first-level index of root directory during formating.
		First-level index of root directory in the beginning doesn't contain any files,
		so each entry has value 0.
	*/
	bool initializeRootDir();

	/*
		Do not use this!
		Returns pointer to Cluster object with given ClusterNo.
		Use LRUCache::getCluster(ClusterNo) instead of this!
	*/
	Cluster* getCluster(ClusterNo) const;

};
