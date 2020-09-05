#pragma once

typedef unsigned long ClusterNo;

class Cluster;

class Cache {
public:

	virtual ~Cache() { }

	virtual Cluster* getCluster(ClusterNo) = 0;

	virtual void putCluster(Cluster *) = 0;

};
