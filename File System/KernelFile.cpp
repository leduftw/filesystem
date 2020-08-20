#include <algorithm>

#include "KernelFile.h"
#include "KernelFS.h"

KernelFile::KernelFile(Disk *d, LRUCache *c, ClusterNo start, ClusterNo directory, int entry, char m, BytesCnt sz) {
	disk = d;
	cache = c;

	first = start;
	dir = directory;
	entryNo = entry;

	mode = m;
	fileSize = sz;

	isClosed = false;
	cursor = 0;  // ?
}

KernelFile::~KernelFile() {
	isClosed = true;

	if (mode == 'w') {
		// Update fileSize field in file descriptor in root directory
		DirectoryEntry *directory = (DirectoryEntry *)cache->get(dir)->getData();
		directory[entryNo].fileSize = fileSize;
	}
}

char KernelFile::write(BytesCnt howMany, char *buffer) {
	if (isClosed || mode == 'r' || buffer == nullptr || howMany == 0) {
		return 0;
	}

	ClusterNo target = cursor / ClusterSize;  // data cluster number where writing begins
	BytesCnt offset = cursor % ClusterSize;  // offset in target where writing begins

	BytesCnt buffRel = 0;  // position in buffer where reading begins
	bool done = false;

	IndexEntry firstLevelEntry = target / (ClusterSize / sizeof(IndexEntry));
	if ((int)firstLevelEntry >= (ClusterSize / sizeof(IndexEntry))) return 0;

	for (int i = firstLevelEntry; i < ClusterSize / sizeof(IndexEntry); i++) {
		IndexEntry *firstLevel = (IndexEntry *)cache->get(first)->getData();  // not before for loop because it can removed from cache!!!
		if (firstLevel[i] == 0) {
			// Extend file with one second-level cluster

			ClusterNo free = disk->getBitVector()->findFreeCluster();
			if (free == 0) {  // there is no space on disk
				return 0;
			}

			disk->getBitVector()->occupy(free);
			Cluster *cluster = cache->get(free);
			cluster->clear();

			firstLevel[i] = free;
		}

		IndexEntry secondLevelEntry = (i == firstLevelEntry ? target % (ClusterSize / sizeof(IndexEntry)) : 0);

		for (int j = secondLevelEntry; j < ClusterSize / sizeof(IndexEntry); j++) {
			IndexEntry *secondLevel = (IndexEntry *)cache->get(firstLevel[i])->getData();
			if (secondLevel[j] == 0) {
				// Extend file with data cluster

				ClusterNo free = disk->getBitVector()->findFreeCluster();
				if (free == 0) return 0;

				disk->getBitVector()->occupy(free);
				Cluster *cluster = cache->get(free);
				cluster->clear();

				secondLevel[j] = free;
			}

			BytesCnt toWrite = min(howMany, ClusterSize - offset);

			Cluster *dataCluster = cache->get(secondLevel[j]);
			char *data = dataCluster->getData();
			memcpy(data + offset, buffer + buffRel, toWrite);

			if (eof()) {  // only if we are appending to file, fileSize grows
				fileSize += toWrite;
			}

			buffRel += toWrite;
			howMany -= toWrite;
			offset = 0;

			cursor += toWrite;

			if (howMany == 0) {
				done = true;
				break;
			}

		}

		if (done) break;
	}

	return done;
}

BytesCnt KernelFile::read(BytesCnt howMany, char *buffer) {
	if (isClosed || buffer == nullptr || eof() || getFileSize() == 0) {
		return 0;
	}

	BytesCnt bytesRead = 0;

	ClusterNo target = cursor / ClusterSize;  // data cluster number where reading begins
	BytesCnt offset = cursor % ClusterSize;  // offset in target where reading begins

	BytesCnt buffRel = 0;  // position in buffer where writing begins
	bool done = false;

	IndexEntry firstLevelEntry = target / (ClusterSize / sizeof(IndexEntry));
	if ((int)firstLevelEntry >= ClusterSize / sizeof(IndexEntry)) return 0;

	for (int i = firstLevelEntry; i < ClusterSize / sizeof(IndexEntry); i++) {
		IndexEntry *firstLevel = (IndexEntry *)cache->get(first)->getData();

		if (firstLevel[i] == 0) {  // should not happen
			done = true;
			break;
		}

		IndexEntry secondLevelEntry = (i == firstLevelEntry ? target % (ClusterSize / sizeof(IndexEntry)) : 0);

		for (int j = secondLevelEntry; j < ClusterSize / sizeof(IndexEntry); j++) {
			IndexEntry *secondLevel = (IndexEntry *)cache->get(firstLevel[i])->getData();
			if (secondLevel[j] == 0) {  // should not happen
				done = true;
				break;
			}

			BytesCnt toRead = min(min(howMany, ClusterSize - offset), fileSize - cursor);

			Cluster *dataCluster = cache->get(secondLevel[j]);
			char *data = dataCluster->getData();
			memcpy(buffer + buffRel, data + offset, toRead);

			buffRel += toRead;
			howMany -= toRead;
			offset = 0;

			cursor += toRead;
			bytesRead += toRead;

			if (howMany == 0 || eof()) {
				done = true;
				break;
			}

		}

		if (done) break;
	}

	return bytesRead;
}

char KernelFile::seek(BytesCnt position) {
	if (isClosed || position > getFileSize()) {
		return 0;
	}

	// By setting cursor to getFileSize()
	// we ensure that eof() returns true
	cursor = position;

	return 1;
}

BytesCnt KernelFile::filePos() {
	if (isClosed) {
		return 0;  // cursor can be 0 too
	}
	return cursor;
}

char KernelFile::eof() {
	if (isClosed) {
		return 1;
	}

	if (getFileSize() == 0) return 1;  // file is empty
	return (filePos() == getFileSize() ? 2 : 0);
}

BytesCnt KernelFile::getFileSize() {
	if (isClosed) {
		return 0;  // fileSize can also be 0
	}

	return fileSize;
}

char KernelFile::truncate() {
	if (isClosed || eof() || mode == 'r') {
		return 0;
	}

	bool done = false;
	BytesCnt end = fileSize % ClusterSize ? fileSize : fileSize - 1;
	ClusterNo lastCluster = end / ClusterSize;  // last data cluster that should be freed
	ClusterNo firstCluster = cursor % ClusterSize ? (cursor / ClusterSize + 1) : (cursor / ClusterSize);  // first data cluster that should be freed

	IndexEntry firstLevelEntry = firstCluster / (ClusterSize / sizeof(IndexEntry));

	for (int i = firstLevelEntry; i < ClusterSize / sizeof(IndexEntry); i++) {
		IndexEntry *firstLevel = (IndexEntry *)cache->get(first)->getData();
		if (firstLevel[i] == 0) return 0;

		IndexEntry secondLevelEntry = (i == firstLevelEntry ? firstCluster % (ClusterSize / sizeof(IndexEntry)) : 0);

		for (int j = secondLevelEntry; j < ClusterSize / sizeof(IndexEntry); j++) {
			IndexEntry *secondLevel = (IndexEntry *)cache->get(firstLevel[i])->getData();
			if (secondLevel[j] == 0) return 0;

			disk->getBitVector()->makeFree(secondLevel[j]);
			secondLevel[j] = 0;

			if (firstCluster++ >= lastCluster) {
				done = true;
				break;
			}
		}

		if (secondLevelEntry == 0) {  // then also second-level index should be freed
			disk->getBitVector()->makeFree(firstLevel[i]);
			firstLevel[i] = 0;
		}

		if (done) {
			break;
		}
	}

	// Even if whole file is truncated, we save first-level index

	// Set new fileSize
	fileSize = cursor;

	return done;
}
