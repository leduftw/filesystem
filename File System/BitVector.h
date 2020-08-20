#ifndef BITVECTOR_H
#define BITVECTOR_H

#include "part.h"
#include "Cluster.h"

class BitVector {

	Partition *partition;

	Cluster **clusters;  // ** because Cluster does not have default constructor
	ClusterNo sz;  // length of clusters

	/*
		Gets value of bit bitNo in cluster clusterNo.
	*/
	bool getBit(ClusterNo clusterNo, unsigned long bitNo) const;

	/*
		Sets bit bitNo in cluster clusterNo.
	*/
	bool setBit(ClusterNo clusterNo, unsigned long bitNo);

	/*
		Resets bit bitNo in cluster clusterNo.
		Implementation is identical to setBit.
	*/
	bool resetBit(ClusterNo clusterNo, unsigned long bitNo);

public:

	/*
		Loads data from clusters 0,1,...,end-1 on partition to memory.
	*/
	BitVector(Partition *p, ClusterNo end);

	/*
		Deallocates clusters. That implicitly means that all data from clusters is saved onto disk.
	*/
	~BitVector();

	/*
		Number of clusters which bit vector occupies.
	*/
	ClusterNo size() const {
		return sz;
	}

	/*
		Saves cluster with given number to disk.
	*/
	bool save(ClusterNo cluster) {
		if (cluster >= sz) return false;
		return clusters[cluster]->save();
	}

	/*
		Saves all clusters that belong to bit vector to disk.
	*/
	bool save() {
		bool status = true;
		for (ClusterNo cluster = 0; cluster < sz; cluster++) {
			if (!clusters[cluster]->save()) {
				status = false;
			}
		}

		return status;
	}

	/*
		Sets all bits in bit vector to 0.
	*/
	bool clear() {
		bool status = true;
		for (ClusterNo cluster = 0; cluster < sz; cluster++) {
			if (!clusters[cluster]->clear()) {
				status = false;
			}
		}

		return status;
	}

	/*
		Returns cluster with given ClusterNo.
	*/
	Cluster* getCluster(ClusterNo cluster) const {
		if (cluster >= sz) {
			return nullptr;
		}

		return clusters[cluster];
	}

	/*
		Finds free cluster with lowest number.
		If there are none, this method returns 0.
	*/
	ClusterNo findFreeCluster() const;

	/*
		Returns true if given cluster on partition is free and false otherwise.
	*/
	bool isFree(ClusterNo) const;

	/*
		Sets corresponding bit to 1 (making cluster with given number on partition free).
		Returns true if operation went successful and false otherwise.
	*/
	bool makeFree(ClusterNo);

	/*
		Sets corresponding bit to 0 (making cluster with given number on partition occupied).
		Returns true if operation went successful and false otherwise.
	*/
	bool occupy(ClusterNo);

};

#endif
