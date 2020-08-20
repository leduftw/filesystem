#include "KernelFS.h"
#include "File.h"
#include "part.h"
#include "FS.h"
#include "Cluster.h"
#include "Utils.h"

using namespace std;

KernelFS::~KernelFS() {
	/*
	delete disk;

	CloseHandle(canMount);
	CloseHandle(canUnmount);
	CloseHandle(canFormat);
	CloseHandle(mutex);
	*/
}

char KernelFS::mount(Partition *partition) {
	if (!partition) {
		return 0;  // return failure
	}

	wait(canMount);  // if partition is already mounted, this thread waits

	disk = new Disk(partition);
	cache = new LRUCache(disk);

	return 1;  // return success
}

char KernelFS::unmount() {
	if (!allFilesClosed()) {  // wait until all files are closed
		wait(canUnmount);
	}

	delete cache;
	cache = nullptr;

	delete disk;
	disk = nullptr;

	signal(canMount);  // tell others they can now mount
	return 1;  // return success
}

char KernelFS::format() {
	if (!disk || !cache) {
		return 0;  // there is no partition to format
	}

	formatCalled = true;  // prohibit file opening until formatting is done

	if (!allFilesClosed()) {  // wait until all files are closed
		wait(canFormat);
	}

	// Formatting is atomic operation
	wait(mutex);
	if (!disk->initializeBitVector()) return 0;  // failure on initializing bit vector
	if (!disk->initializeRootDir()) return 0;  // failure on initializing first-level index of root directory

	signal(mutex);

	formatCalled = false;
	return 1;  // return success
}

FileCnt KernelFS::readRootDir() {
	if (!disk || !cache) {
		return -1;  // partition hasn't been mounted yet
	}

	FileCnt cnt = 0;  // answer

	IndexEntry *firstLevel = (IndexEntry *)disk->getFirstLevelDirectory()->getData();  // first-level index of root directory

	// Go through all entries of first-level index
	for (int i = 0; i < ClusterSize / sizeof(IndexEntry); i++) {
		if (firstLevel[i] == 0) continue;  // doesn't point to second-level index

		// Go through all entries of second-level index
		for (int j = 0; j < ClusterSize / sizeof(IndexEntry); j++) {
			IndexEntry *secondLevel = (IndexEntry *)cache->get(firstLevel[i])->getData();  // second-level index of root directory

			if (secondLevel[j] == 0) continue;  // doesn't point to data cluster

			// Go through all entries of data cluster
			for (int k = 0; k < ClusterSize / sizeof(DirectoryEntry); k++) {
				DirectoryEntry *directory = (DirectoryEntry *)cache->get(secondLevel[j])->getData();  // data cluster of root directory

				// Check first char in each entry. If it's not 0, then that entry contains info about some file.
				if (directory[k].fname[0]) {
					cnt++;
				}
			}
		}
	}

	return cnt;  // return number of files in root directory
}

char KernelFS::doesExist(char *fname) {
	// Invalid arguments
	if (!disk || fname == nullptr) {
		return 0;
	}

	if (strlen(fname) < 4) return 0;  // shortest name "/a.a" has strlen == 4

	// Argument fname has form: /filename.ext
	string whole(fname + 1);  // "filename.ext"

	vector<string> tokens = Utils::split(whole, ".");
	if (tokens.size() != 2) return 0;  // invalid fname

	string givenName = tokens[0];  // "filename"
	string givenExtension = tokens[1];  // "ext"

	if (givenName.length() > 8 || givenExtension.length() > 3) return 0;

	IndexEntry *firstLevel = (IndexEntry *)disk->getFirstLevelDirectory()->getData();
	for (int i = 0; i < ClusterSize / sizeof(IndexEntry); i++) {
		if (firstLevel[i] == 0) continue;

		for (int j = 0; j < ClusterSize / sizeof(IndexEntry); j++) {
			IndexEntry *secondLevel = (IndexEntry *)cache->get(firstLevel[i])->getData();

			if (secondLevel[j] == 0) continue;

			for (int k = 0; k < ClusterSize / sizeof(DirectoryEntry); k++) {
				DirectoryEntry *directory = (DirectoryEntry *)cache->get(secondLevel[j])->getData();  // data cluster of root directory

				if (!directory[k].fname[0]) continue;  // entry does not contain file

				int endName = 0;
				while (endName < FNAMELEN && directory[k].fname[endName] != ' ') {
					endName++;
				}

				// Range construction
				string curName(directory[k].fname, directory[k].fname + endName);

				int endExt = 0;
				while (endExt < FEXTLEN && directory[k].ext[endExt] != ' ') {
					endExt++;
				}
				string curExtension(directory[k].ext, directory[k].ext + endExt);

				if (givenName == curName && givenExtension == curExtension) {  // file is found
					return 1;
				}
			}
		}
	}

	return 0;  // searched file doesn't exists
}

File* KernelFS::open(char *fname, char mode) {
	if (!disk || !cache || !fname || !Utils::isCorrect(mode)) {
		return nullptr;
	}

	if (formatCalled) {
		return nullptr;
	}

	if (strlen(fname) < 4) return nullptr;

	string whole(fname + 1);

	vector<string> tokens = Utils::split(whole, ".");
	if (tokens.size() != 2) return nullptr;

	string givenName = tokens[0];
	string givenExtension = tokens[1];

	if (givenName.length() > 8 || givenExtension.length() > 3) return nullptr;

	bool exist = doesExist(fname);

	if (!exist && (mode == 'r' || mode == 'a')) {
		return nullptr;
	}

	if (exist && mode == 'w') {
		FS::deleteFile(fname);
	}

	IndexEntry *firstLevel = (IndexEntry *)disk->getFirstLevelDirectory()->getData();
	for (int i = 0; i < ClusterSize / sizeof(IndexEntry); i++) {
		if (firstLevel[i] == 0) {
			if (exist) continue;

			ClusterNo free = disk->getBitVector()->findFreeCluster();
			if (free == 0) return nullptr;

			disk->getBitVector()->occupy(free);
			Cluster *secondLevel = cache->get(free);
			secondLevel->clear();

			firstLevel[i] = free;
		}

		for (int j = 0; j < ClusterSize / sizeof(IndexEntry); j++) {
			IndexEntry *secondLevel = (IndexEntry *)cache->get(firstLevel[i])->getData();
			if (secondLevel[j] == 0) {
				if (exist) continue;

				ClusterNo free = disk->getBitVector()->findFreeCluster();
				if (free == 0) return nullptr;

				disk->getBitVector()->occupy(free);
				Cluster *data = cache->get(free);
				data->clear();

				secondLevel[j] = free;
			}

			for (int k = 0; k < ClusterSize / sizeof(DirectoryEntry); k++) {
				Cluster *dir = cache->get(secondLevel[j]);
				DirectoryEntry *directory = (DirectoryEntry *)dir->getData();
				if (exist && directory[k].fname[0]) {
					int endName = 0;
					while (endName < FNAMELEN && directory[k].fname[endName] != ' ') {
						endName++;
					}

					string curName(directory[k].fname, directory[k].fname + endName);

					int endExt = 0;
					while (endExt < FEXTLEN && directory[k].ext[endExt] != ' ') {
						endExt++;
					}
					string curExtension(directory[k].ext, directory[k].ext + endExt);

					if (givenName == curName && givenExtension == curExtension) {  // file is found

						File *ret = new File(disk, cache, directory[k].firstLevelCluster, dir->getClusterNo(), k, mode, directory[k].fileSize);
						ret->myImpl->path = string(fname, strlen(fname));

						return ret;
					}
				} else if (!exist && !directory[k].fname[0]) {
					// mode must be 'w'

					for (int i = 0; i < (int)givenName.length(); i++) {
						directory[k].fname[i] = givenName[i];
					}
					for (int i = givenName.length(); i < (int)FNAMELEN; i++) {
						directory[k].fname[i] = ' ';  // pad with spaces
					}


					for (int i = 0; i < (int)givenExtension.length(); i++) {
						directory[k].ext[i] = givenExtension[i];
					}
					for (int i = givenExtension.length(); i < (int)FEXTLEN; i++) {
						directory[k].ext[i] = ' ';  // pad with spaces
					}

					directory[k].notUsed = 0;

					ClusterNo free = disk->getBitVector()->findFreeCluster();
					if (free == 0) return nullptr;
					directory[k].firstLevelCluster = free;
					disk->getBitVector()->occupy(free);
					Cluster *firstLevel = cache->get(free);
					firstLevel->clear();

					directory[k].fileSize = 0;
					directory[k].free[0]++;  // number of threads reading file

					File *ret = new File(disk, cache, free, dir->getClusterNo(), k, mode, 0);
					ret->myImpl->path = string(fname, strlen(fname));

					return ret;
				}
			}
		}
	}

	openFilesCnt++;  // not quite accurate
	return nullptr;
}

char KernelFS::deleteFile(char *fname) {
	if (!disk || fname == nullptr) {
		return 0;
	}

	if (strlen(fname) < 4) return 0;

	string whole(fname + 1);

	vector<string> tokens = Utils::split(whole, ".");
	if (tokens.size() != 2) return 0;

	string givenName = tokens[0];
	string givenExtension = tokens[1];

	if (givenName.length() > 8 || givenExtension.length() > 3) return 0;

	IndexEntry *firstLevel = (IndexEntry *)disk->getFirstLevelDirectory()->getData();
	for (int i = 0; i < ClusterSize / sizeof(IndexEntry); i++) {
		if (firstLevel[i] == 0) continue;

		for (int j = 0; j < ClusterSize / sizeof(IndexEntry); j++) {
			IndexEntry *secondLevel = (IndexEntry *)cache->get(firstLevel[i])->getData();
			if (secondLevel[j] == 0) continue;

			for (int k = 0; k < ClusterSize / sizeof(DirectoryEntry); k++) {
				Cluster *dir = cache->get(secondLevel[j]);
				DirectoryEntry *directory = (DirectoryEntry *)dir->getData();
				if (!directory[k].fname[0]) continue;
				int endName = 0;
				while (endName < FNAMELEN && directory[k].fname[endName] != ' ') {
					endName++;
				}

				string curName(directory[k].fname, directory[k].fname + endName);

				int endExt = 0;
				while (endExt < FEXTLEN && directory[k].ext[endExt] != ' ') {
					endExt++;
				}
				string curExtension(directory[k].ext, directory[k].ext + endExt);

				if (givenName == curName && givenExtension == curExtension) {  // file descriptor is found

					File *ret = new File(disk, cache, directory[k].firstLevelCluster, dir->getClusterNo(), k, 'w', directory[k].fileSize);
					ret->myImpl->path = string(fname, strlen(fname));
					
					ret->seek(0);
					ret->truncate();

					delete ret;

					// Clear entry
					memset(directory[k].fname, 0, FNAMELEN);
					memset(directory[k].ext, 0, FEXTLEN);
					directory[k].notUsed = 0;

					cache->get(directory[k].firstLevelCluster)->clear();
					disk->getBitVector()->makeFree(directory[k].firstLevelCluster);
					directory[k].firstLevelCluster = 0;

					directory[k].fileSize = 0;
					memset(directory[k].free, 0, 12);

					return 1;
				}
			}
		}
	}

	return 0;
}
