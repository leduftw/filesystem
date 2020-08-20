#include "Directory.h"

Directory::Directory(Partition *p, ClusterNo c) : Cluster(p, c) {
	entries = (DirectoryEntry *)data;
}
