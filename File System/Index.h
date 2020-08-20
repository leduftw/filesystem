#ifndef INDEX_H
#define INDEX_H

#include "part.h"
#include "Cluster.h"

typedef ClusterNo IndexEntry;  // entries for first-level and second-level index have size of 32 bits

/*
	Index class represents one first-level or second-level cluster.
*/
class Index : public Cluster {

	IndexEntry *entries;

	void copy(const Index &i);
	void move(Index &i);

public:

	/*
		Loads data from given cluster on partition to memory.
	*/
	Index(Partition *, ClusterNo);

	Index(const Index &i) {
		copy(i);
	}

	Index(Index &&i) {
		move(i);
	}

	/*
		Number of entries in first-level and second-level index cluster.
		In this case it is 2048 / 4 = 512.
	*/
	int size() const override {
		return ClusterSize / sizeof(IndexEntry);
	}

	/*
		Index cluster can act as an array. Parameter i is in [0..size()-1].
	*/
	ClusterNo& operator[](int i) {
		return entries[i];
	}

	/*
		Index cluster can act as an array. Parameter i is in [0..size()-1].
	*/
	const ClusterNo& operator[](int i) const {
		return entries[i];
	}

};

#endif
