#include "File.h"

File::File(Disk *disk, Cache *c, ClusterNo firstLevel, ClusterNo dir, int entry, char mode, BytesCnt sz) {
	myImpl = new KernelFile(disk, c, firstLevel, dir, entry, mode, sz);
}

File::~File() {
	delete myImpl;
}

char File::write(BytesCnt howMany, char *buffer) {
	return myImpl->write(howMany, buffer);
}

BytesCnt File::read(BytesCnt howMany, char *buffer) {
	return myImpl->read(howMany, buffer);
}

char File::seek(BytesCnt position) {
	return myImpl->seek(position);
}

BytesCnt File::filePos() {
	return myImpl->filePos();
}

char File::eof() {
	return myImpl->eof();
}

BytesCnt File::getFileSize() {
	return myImpl->getFileSize();
}

char File::truncate() {
	return myImpl->truncate();
}
