#include "File.h"
#include "KernelFile.h"

File::File(Disk *disk, ClusterNo firstLevel, ClusterNo dir, char mode, BytesCnt sz) {
	myImpl = new KernelFile(disk, firstLevel, dir, mode, sz);
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
