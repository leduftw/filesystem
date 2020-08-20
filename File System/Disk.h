#ifndef DISK_H
#define DISK_H

#include "part.h"
#include "BitVector.h"
#include "Index.h"
#include "Directory.h"

class Disk {

	Partition *partition;

	// Bit vector and first-level index of root directory are always cached in memory
	BitVector *bitVector;
	Index *firstLevelDirectory;

public:

	Disk(Partition *);

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
	Index* getFirstLevelDirectory() const {
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
		Returns pointer to Cluster object with given ClusterNo.
	*/
	Cluster* getCluster(ClusterNo) const;

};

#endif
