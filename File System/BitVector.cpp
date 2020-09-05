#include "BitVector.h"

BitVector::BitVector(Partition *p, ClusterNo end) : partition(p), sz(end) {
	clusters = new Cluster*[sz];
	for (ClusterNo cluster = 0; cluster < end; cluster++) {
		clusters[cluster] = new Cluster(partition, cluster);
	}

	mutex = CreateSemaphore(NULL, 1, 1, NULL);
}

BitVector::~BitVector() {
	// save(); // no need for this, because destructor of Cluster class saves data to disk
	for (ClusterNo cluster = 0; cluster < sz; cluster++) {
		delete clusters[cluster];
		clusters[cluster] = nullptr;
	}

	delete[] clusters;
	clusters = nullptr;

	sz = 0;

	CloseHandle(mutex);
}

bool BitVector::getBit(ClusterNo clusterNo, unsigned long bitNo) const {
	if (bitNo >= (ClusterSize << 3)) {  // bitNo is out of range
		return false;
	}

	wait(mutex);

	// Find index in cluster and position for bitNo on that index
	int ind = bitNo >> 3, pos = bitNo % 8;
	char *data = clusters[clusterNo]->getData();

	signal(mutex);

	return data[ind] & (1 << pos);  // bit on position pos

}

bool BitVector::setBit(ClusterNo clusterNo, unsigned long bitNo) {
	if (bitNo >= (ClusterSize << 3)) {
		return false;
	}

	wait(mutex);

	int ind = bitNo >> 3, pos = bitNo % 8;
	char *data = clusters[clusterNo]->getData();

	data[ind] |= (1 << pos);  // sets the bit

	signal(mutex);

	return true;
}

bool BitVector::resetBit(ClusterNo clusterNo, unsigned long bitNo) {
	if (bitNo >= (ClusterSize << 3)) {
		return false;
	}

	wait(mutex);

	int ind = bitNo >> 3, pos = bitNo % 8;
	char *data = clusters[clusterNo]->getData();

	data[ind] &= ~(1 << pos);  // resets the bit

	signal(mutex);

	return true;
}

ClusterNo BitVector::findFreeCluster() const {
	ClusterNo n = partition->getNumOfClusters();
	for (ClusterNo free = 0; free < n; free++) {
		ClusterNo cur = free / (ClusterSize << 3);  // we should search in this cluster
		if (getBit(cur, free % (ClusterSize << 3))) {
			return free;
		}
	}
	return 0;
}

bool BitVector::isFree(ClusterNo cluster) const {
	if (cluster >= partition->getNumOfClusters()) return false;

	ClusterNo cur = cluster / (ClusterSize << 3);
	return getBit(cur, cluster % (ClusterSize << 3));
}

bool BitVector::makeFree(ClusterNo cluster) {
	if (cluster >= partition->getNumOfClusters()) return false;

	ClusterNo cur = cluster / (ClusterSize << 3);
	return setBit(cur, cluster % (ClusterSize << 3));
}

bool BitVector::occupy(ClusterNo cluster) {
	if (cluster >= partition->getNumOfClusters()) return false;

	ClusterNo cur = cluster / (ClusterSize << 3);
	return resetBit(cur, cluster % (ClusterSize << 3));
}
