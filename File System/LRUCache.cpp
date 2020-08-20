#include "LRUCache.h"

const int LRUCache::FACTOR = 10;

LRUCache::LRUCache(Disk *d) : disk(d) {
	ClusterNo n = disk->getPartition()->getNumOfClusters();
	capacity = n / FACTOR;
}

LRUCache::~LRUCache() {
	for (auto data : lruList) {
		delete data.second;  // save cached cluster data to disk
	}
}

void LRUCache::moveToFront(ClusterNo key, Cluster *value) {
	// Erase and add a new entry to front
	lruList.erase(hash[key]);  // this is O(1) since we are using iterator
	lruList.push_front({ key, value });
	hash[key] = lruList.begin();
}

Cluster* LRUCache::get(ClusterNo key) {
	if (key >= disk->getPartition()->getNumOfClusters()) {
		return nullptr;
	}

	if (hash.find(key) == hash.end()) {  // if not in cache, get it from disk
		Cluster *cluster = new Cluster(disk->getPartition(), key);
		put(key, cluster);
		return cluster;
	}

	// Move the (key, value) pair to front
	Cluster *value = hash[key]->second;
	moveToFront(key, value);
	return value;
}

void LRUCache::put(ClusterNo key, Cluster *value) {
	if (hash.find(key) != hash.end()) {  // when key is already in hash
		moveToFront(key, value);
	} else {  // add to the cache
		lruList.push_front({ key, value });
		hash[key] = lruList.begin();
		if (hash.size() > capacity) {  // erase
			hash.erase(lruList.back().first);

			// Save to the disk
			Cluster *toSave = lruList.back().second;
			delete toSave;  // saves data

			lruList.pop_back();
		}
	}
}
