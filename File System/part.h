#ifndef PARTITION_H
#define PARTITION_H

typedef unsigned long ClusterNo;
const unsigned long ClusterSize = 2048;

class PartitionImpl;

class Partition {
public:

	/*
		Parameter is path to configuration file.
		Example: "p1.ini".
	*/
	Partition(char *pathToConfFile);

	/*
		Returns number of clusters.
	*/
	virtual ClusterNo getNumOfClusters() const;

	/*
		Returns 1 on success and 0 otherwise.
	*/
	virtual int readCluster(ClusterNo, char *buffer);

	/*
		Returns 1 on success and 0 otherwise.
	*/
	virtual int writeCluster(ClusterNo, const char *buffer);

	/*
		Call at the end!!!
	*/
	virtual ~Partition();

private:

	PartitionImpl *myImpl;

};

#endif
