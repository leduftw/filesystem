#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <list>
#include <unordered_map>
#include <utility>

#include "part.h"
#include "Cluster.h"
#include "Disk.h"

using namespace std;

class LRUCache {

	// FACTOR == 10
	static const int FACTOR;  // cache has capacity == capacity(disk) / FACTOR

	Disk *disk;

	ClusterNo capacity;

	list<pair<ClusterNo, Cluster *>> lruList;  // (key, value) pair
	unordered_map<ClusterNo, list<pair<ClusterNo, Cluster *>>::iterator > hash;  // key to (key, val) iterator map

	void moveToFront(ClusterNo key, Cluster *value);

public:

	/*
		
	*/
	LRUCache(Disk *);

	/*
		O(1).
	*/
	Cluster* get(ClusterNo key);

	/*
		O(1).
	*/
	void put(ClusterNo key, Cluster *value);

};

#endif
