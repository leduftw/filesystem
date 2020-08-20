#include "testprimer.h"

using namespace std;

HANDLE nit1, nit2;
DWORD ThreadID;

HANDLE semMain = CreateSemaphore(NULL, 0, 32, NULL);
HANDLE sem12 = CreateSemaphore(NULL, 0, 32, NULL);
HANDLE sem21 = CreateSemaphore(NULL, 0, 32, NULL);
HANDLE mutex = CreateSemaphore(NULL, 1, 32, NULL);

Partition *partition;

char *ulazBuffer;
int ulazSize;

// Public test
int publicMain() {
	clock_t startTime, endTime;
	cout << "Pocetak testa!" << endl;
	startTime = clock();  // pocni merenje vremena

	{ // ucitavamo ulazni fajl u bafer, da bi nit 1 i 2 mogle paralelno da citaju
		FILE *f = fopen("ulaz.dat", "rb");
		if (f == 0) {
			cout << "GRESKA: Nije nadjen ulazni fajl 'ulaz.dat' u os domacinu!" << endl;
			system("PAUSE");  // pauzira main nit dok user ne pritisne bilo sta
			return 0;  // exit program
		}

		ulazBuffer = new char[32 * 1024 * 1024];	// 32MB
		ulazSize = fread(ulazBuffer, 1, 32 * 1024 * 1024, f);

		fclose(f);
	}

	// kreira i startuje niti
	nit1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)nit1run, NULL, 0, &ThreadID);
	nit2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)nit2run, NULL, 0, &ThreadID);

	for (int i = 0; i < 2; i++) {
		wait(semMain);  // cekamo da se niti zavrse
	}

	delete[] ulazBuffer;
	endTime = clock();

	cout << "Kraj test primera!" << endl;
	cout << "Vreme izvrsavanja: " << ((double)(endTime - startTime) / ((double)CLOCKS_PER_SEC / 1000.0)) << "ms!" << endl;

	CloseHandle(mutex);
	CloseHandle(semMain);
	CloseHandle(sem12);
	CloseHandle(sem21);

	CloseHandle(nit1);
	CloseHandle(nit2);

	return 0;
}

void mountAndFormatTest() {
	// Test za disk sa 1000 klastera. Napomene za proveru ispravnosti:

	// Jedan klaster zauzima 2048 adresa (2048 bajtova) u hexdump-u odnosno prvi klaster
	// zauzima adrese [00-7FF], sledeci zauzima adrese [800, FFF], itd.

	// 1000 klastera -> 1000 bita za bit vektor -> 125 bajtova za bit vektor.
	// Dakle, za bit vektor je dovoljan samo nulti klaster, a indeks prvog nivoa
	// korenog direktorijuma zauzima klaster broj 1. Hexdump daje u prvom bajtu FC (1111 1100),
	// a naredna 124 bajta imaju vrednost FF. Hex vrednosti adresa idu od 00 do 7C (124 dekadno).

	Partition *partition = new Partition((char *)"p1.ini");
	cout << "Number of clusters: " << partition->getNumOfClusters() << "\n\n";

	if (!FS::mount(partition)) {
		cout << "Mount unsuccessful.\n";
		exit(1);
	}

	cout << "Mount successful." << "\n";

	if (!FS::format()) {
		cout << "Format unsuccessful.\n";
		exit(1);
	}

	cout << "Format successful." << "\n";
}

void oneThreadCopyPaste() {
	// ucitavamo ulazni fajl u bafer
	FILE *file = fopen("ulaz.dat", "rb");
	if (file == 0) {
		cout << "GRESKA: Nije nadjen ulazni fajl 'ulaz.dat' u os domacinu!" << endl;
		system("PAUSE");  // pauzira main nit dok user ne pritisne bilo sta
		return;  // exit program
	}

	ulazBuffer = new char[32 * 1024 * 1024];	// 32MB
	ulazSize = fread(ulazBuffer, 1, 32 * 1024 * 1024, file);

	fclose(file);

	///////////////////////////////////////////////////////////////////////////
	wait(mutex);
	partition = new Partition((char *)"p1.ini");
	signal(mutex);

	wait(mutex);
	cout << "Kreirana particija" << endl;
	signal(mutex);

	FS::mount(partition);  // montiranje

	wait(mutex);
	cout << "Montirana particija" << endl;
	signal(mutex);

	FS::format();  // formatiranje

	wait(mutex);
	cout << "Formatirana particija" << endl;
	signal(mutex);
	///////////////////////////////////////////////////////////////////////////

	char filepath[] = "/fajl1.dat";
	File *f = FS::open(filepath, 'w');

	wait(mutex);
	cout << "Kreiran fajl '" << filepath << "'" << endl;
	signal(mutex);

	f->write(ulazSize, ulazBuffer);

	wait(mutex);
	cout << "Prepisan sadrzaj 'ulaz.dat' u '" << filepath << "'" << endl;
	signal(mutex);

	delete f;
	delete[] ulazBuffer;

	wait(mutex);
	cout << "Zatvoren fajl '" << filepath << "'" << endl;
	signal(mutex);
	///////////////////////////////////////////////////////////////////////////

	/*
	f = FS::open(filepath, 'r');

	wait(mutex);
	cout << "Otvoren fajl " << filepath << "" << endl;
	signal(mutex);

	ofstream fout("izlaz1.dat", ios::out | ios::binary);
	char *buff = new char[f->getFileSize()];
	f->read(f->getFileSize(), buff);
	fout.write(buff, f->getFileSize());

	wait(mutex);
	cout <<"Upisan '" << filepath << "' u fajl os domacina 'izlaz1.dat'" << endl;
	signal(mutex);

	delete[] buff;
	fout.close();
	delete f;

	wait(mutex);
	cout << "Zatvoren fajl " << filepath << "" << endl;
	signal(mutex);
	///////////////////////////////////////////////////////////////////////////

	
	*/

	// FS::unmount();
	wait(mutex);
	cout << "Demontirana particija p1" << endl;
	signal(mutex);

	delete partition;
}

int main() {
	// mountAndFormatTest();
	oneThreadCopyPaste();

	// return publicMain();
}
