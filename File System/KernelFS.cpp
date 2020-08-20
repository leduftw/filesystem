#include "KernelFS.h"
#include "File.h"
#include "part.h"
#include "FS.h"
#include "Index.h"
#include "Directory.h"
#include "Utils.h"

using namespace std;

KernelFS::~KernelFS() {
	delete disk;

	CloseHandle(canMount);
	CloseHandle(canUnmount);
	CloseHandle(canFormat);
	CloseHandle(mutex);
}

char KernelFS::mount(Partition *partition) {
	if (partition == nullptr) {
		return 0;  // return failure
	}

	wait(canMount);  // if partition is already mounted, this thread waits

	disk = new Disk(partition);  // mount the partition

	return 1;  // return success
}

char KernelFS::unmount() {
	if (!allFilesClosed()) {  // wait until all files are closed
		wait(canUnmount);
	}

	delete disk;  // then unmount
	disk = nullptr;

	signal(canMount);  // tell others they can now mount
	return 1;  // return success
}

char KernelFS::format() {
	if (!disk) {
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

	disk->getBitVector()->save();
	disk->getFirstLevelDirectory()->save();

	signal(mutex);

	formatCalled = false;
	return 1;  // return success
}

FileCnt KernelFS::readRootDir() {
	if (!disk) {
		return -1;  // partition hasn't been mounted yet
	}

	FileCnt cnt = 0;  // answer

	Index firstLevel(disk->getPartition(), disk->getFirstLevelDirectory()->getClusterNo());  // first-level index of root directory

	// Go through all entries of first-level index
	for (int i = 0; i < firstLevel.size(); i++) {
		if (firstLevel[i] == 0) continue;  // doesn't point to second-level index

		Index secondLevel(disk->getPartition(), firstLevel[i]);  // second-level index of root directory

		// Go through all entries of second-level index
		for (int j = 0; j < secondLevel.size(); j++) {
			if (secondLevel[j] == 0) continue;  // doesn't point to data cluster

			Directory directory(disk->getPartition(), secondLevel[j]);  // data cluster of root directory

			// Go through all entries of data cluster
			for (int k = 0; k < directory.size(); k++) {
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

	Index firstLevel(disk->getPartition(), disk->getFirstLevelDirectory()->getClusterNo());
	for (int i = 0; i < firstLevel.size(); i++) {
		if (firstLevel[i] == 0) continue;

		Index secondLevel(disk->getPartition(), firstLevel[i]);
		for (int j = 0; j < secondLevel.size(); j++) {
			if (secondLevel[j] == 0) continue;

			Directory directory(disk->getPartition(), secondLevel[j]);
			for (int k = 0; k < directory.size(); k++) {
				if (!directory[k].fname[0]) continue;  // entry does not contain file

				int endName = 0;
				while (endName < FNAMELEN && directory[k].fname[endName] != ' ') {
					endName++;
				}

				// Range construction
				string curName(directory[k].fname, directory[k].fname + endName);

				int endExt = 0;
				while (endExt < FEXTLEN && directory[k].ext[endExt] != ' ') {
					endExt;
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

// Prolazimo kroz sve ulaze u direktorijumu. Ako fajl ne postoji (!doesExist), nadjemo ulaz koji je slobodan
// i tu ce biti deskriptor za taj fajl. U bit vektoru nadjemo slobodan blok i to ce biti indeks prvog nivoa za nas fajl.
// Indeks prvog nivoa popunimo nulama i u bit vektoru kazemo da je ovaj blok sada zauzet.
// Paziti na ogranicenja! Ako je mode == 'w', zovemo prvo FS::deleteFile(fname) a zatim kreiramo novi prazan fajl.
File* KernelFS::open(char *fname, char mode) {
	if (!disk || !fname || !Utils::isCorrect(mode)) {   // invalid arguments
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

	if (givenName.length() > 8 || givenExtension.length() > 3) return 0;

	bool exist = doesExist(fname);

	if (!exist && (mode == 'r' || mode == 'a')) {
		return nullptr;
	}

	if (exist && mode == 'w') {
		FS::deleteFile(fname);
	}

	Index firstLevel(disk->getPartition(), disk->getFirstLevelDirectory()->getClusterNo());
	for (int i = 0; i < firstLevel.size(); i++) {
		if (firstLevel[i] == 0) {
			if (exist) continue;

			ClusterNo free = disk->getBitVector()->findFreeCluster();
			if (free == 0) return nullptr;

			disk->getBitVector()->occupy(free);
			Cluster *secondLevel = disk->getCluster(free);
			secondLevel->clear();
			delete secondLevel;

			firstLevel[i] = free;
		}

		Index secondLevel(disk->getPartition(), firstLevel[i]);
		for (int j = 0; j < secondLevel.size(); j++) {
			if (secondLevel[j] == 0) {
				if (exist) continue;

				ClusterNo free = disk->getBitVector()->findFreeCluster();
				if (free == 0) return nullptr;

				disk->getBitVector()->occupy(free);
				Cluster *data = disk->getCluster(free);
				data->clear();
				delete data;

				secondLevel[j] = free;
			}

			Directory dir(disk->getPartition(), secondLevel[j]);
			for (int k = 0; k < dir.size(); k++) {
				if (exist && dir[k].fname[0]) {
					int endName = 0;
					while (endName < FNAMELEN && dir[k].fname[endName] != ' ') {
						endName++;
					}

					string curName(dir[k].fname, dir[k].fname + endName);

					int endExt = 0;
					while (endExt < FEXTLEN && dir[k].ext[endExt] != ' ') {
						endExt;
					}
					string curExtension(dir[k].ext, dir[k].ext + endExt);

					if (givenName == curName && givenExtension == curExtension) {  // file is found
						
						File *ret = new File(disk, dir[k].firstLevelCluster, dir.getClusterNo(), mode, dir[k].fileSize);
						ret->myImpl->path = string(fname, strlen(fname));

						return ret;
					}
				} else if (!exist && !dir[k].fname[0]) {
					// mode must be 'w'

					for (int i = 0; i < (int)givenName.length(); i++) {
						dir[k].fname[i] = givenName[i];
					}
					
					for (int i = 0; i < (int)givenExtension.length(); i++) {
						dir[k].ext[i] = givenExtension[i];
					}

					dir[k].notUsed = 0;

					ClusterNo free = disk->getBitVector()->findFreeCluster();
					if (free == 0) return nullptr;
					dir[k].firstLevelCluster = free;
					disk->getBitVector()->occupy(free);
					Cluster *firstLevel = disk->getCluster(free);
					firstLevel->clear();
					delete firstLevel;

					dir[k].fileSize = 0;
					dir[k].free[0]++;  // number of threads reading file

					File *ret = new File(disk, free, dir.getClusterNo(), mode, 0);
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

	/*
	DirectoryEntry *entry = disk->findFile(fname);
	if (!entry) return 0;

	// free[0] represents number of threads that are using that file
	if (entry->free[0]) return 0;  // other threads are using this file

	// Delete contents of file
	File *file = FS::open(fname, 'w');
	if (!file) return 0;
	file->seek(0);
	file->truncate();
	delete file;

	// Clear entry
	memset(entry->fname, 0, FNAMELEN);
	memset(entry->ext, 0, FEXTLEN);
	entry->notUsed = 0;
	entry->firstLevelCluster = 0;
	entry->fileSize = 0;
	memset(entry->free, 0, 12);
	*/

	return 0;
}
