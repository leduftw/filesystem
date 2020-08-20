#ifndef PARTITION_H
#define PARTITION_H

typedef unsigned long ClusterNo;
const unsigned long ClusterSize = 2048;

class PartitionImpl;

class Partition {
public:
	Partition(char *);  // parameter is path to .ini file

	virtual ClusterNo getNumOfClusters() const;

	virtual int readCluster(ClusterNo, char *buffer);  // returns 1 on success, 0 otherwise
	virtual int writeCluster(ClusterNo, const char *buffer);  // returns 1 on success, 0 otherwise

	virtual ~Partition();
private:
	PartitionImpl *myImpl;
};

#endif