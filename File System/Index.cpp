#include "Index.h"

Index::Index(Partition *p, ClusterNo c) : Cluster(p, c) {
	entries = (IndexEntry *)data;
}
