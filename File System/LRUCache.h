#pragma once

#include <list>
#include <unordered_map>
#include <utility>

#include <Windows.h>

#include "part.h"
#include "Cache.h"
#include "Cluster.h"
#include "Disk.h"

using namespace std;

#define wait(semaphore) WaitForSingleObject(semaphore, INFINITE)
#define signal(semaphore) ReleaseSemaphore(semaphore, 1, NULL)

class LRUCache : public Cache {

	// FACTOR == 10
	static const int FACTOR;  // cache has capacity == capacity(disk) / FACTOR

	Disk *disk;

	ClusterNo capacity;

	list<pair<ClusterNo, Cluster *>> lruList;  // (key, value) pair
	unordered_map<ClusterNo, list<pair<ClusterNo, Cluster *>>::iterator > hash;  // key to (key, val) iterator map

	HANDLE mutex;

	/*
		When we access to some cluster, it becomes most recently used, so it needs to be at the front of the list.

		Time complexity: O(1).
	*/
	void moveToFront(ClusterNo key, Cluster *value);

public:

	/*
		Calculates capacity of cache and initializes mutex.
	*/
	LRUCache(Disk *);

	/*
		Saves all cached clusters to disk and releases handles.
	*/
	~LRUCache();

	/*
		Gets cluster with given ClusterNo.
		If cluster is not already in cache, then it is loaded from disk,
		possibly removing (saving to disk) least recently used cluster.

		Time complexity: O(1).
	*/
	Cluster* getCluster(ClusterNo key) override;

	/*
		Loads given cluster from disk to cache.
		This possibly removes least recently used cluster if cache is full.

		Time complexity: O(1).
	*/
	void putCluster(Cluster *value) override;

};
