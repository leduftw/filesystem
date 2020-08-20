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

void initTest() {
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

	if (!FS::unmount()) {
		cout << "Unmount unsuccessful.\n";
		exit(1);
	}

	cout << "Unmount successful." << "\n";
}

void oneThreadTwoCopyPaste() {
	// ucitavamo ulazni fajl u bafer
	FILE *file1 = fopen("ulaz1.dat", "rb"), *file2 = fopen("ulaz2.dat", "rb");
	if (file1 == 0 || file2 == 0) {
		cout << "GRESKA: Nisu nadjeni ulazni fajlovi 'ulaz1.dat' i 'ulaz2.dat' u os domacinu!" << endl;
		system("PAUSE");  // pauzira main nit dok user ne pritisne bilo sta
		return;  // exit program
	}

	char *ulazBuffer1 = new char[32 * 1024 * 1024];	// 32MB
	long ulazSize1 = fread(ulazBuffer1, 1, 32 * 1024 * 1024, file1);

	char *ulazBuffer2 = new char[32 * 1024 * 1024];	// 32MB
	long ulazSize2 = fread(ulazBuffer2, 1, 32 * 1024 * 1024, file2);

	fclose(file1);
	fclose(file2);

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

	char filepath1[] = "/fajl1.dat";
	File *f1 = FS::open(filepath1, 'w');

	char filepath2[] = "/fajl2.dat";

	wait(mutex);
	cout << "Kreiran fajl '" << filepath1 << "'" << endl;
	signal(mutex);

	f1->write(ulazSize1, ulazBuffer1);

	wait(mutex);
	cout << "Prepisan sadrzaj 'ulaz1.dat' u '" << filepath1 << "'" << endl;
	signal(mutex);

	wait(mutex);
	cout << "Velicina fajla '" << filepath1 << "': " << f1->getFileSize() << endl;
	signal(mutex);

	delete f1;
	delete[] ulazBuffer1;

	wait(mutex);
	cout << "Zatvoren fajl '" << filepath1 << "'" << endl;
	signal(mutex);
	///////////////////////////////////////////////////////////////////////////

	wait(mutex);
	cout << "Broj fajlova u root direktorijumu: " << FS::readRootDir() << "\n";

	cout << filepath1 << " postoji: " << (FS::doesExist(filepath1) ? "da" : "ne") << "\n";
	cout << filepath2 << " postoji: " << (FS::doesExist(filepath2) ? "da" : "ne") << "\n";
	signal(mutex);

	File *f2 = FS::open(filepath1, 'r');

	wait(mutex);
	cout << "Otvoren fajl " << filepath1 << "" << endl;
	signal(mutex);

	wait(mutex);
	cout << "Velicina fajla '" << filepath1 << "': " << f2->getFileSize() << endl;
	signal(mutex);

	ofstream fout1("izlaz1.dat", ios::out | ios::binary);
	char *buff1 = new char[f2->getFileSize()];
	f2->read(f2->getFileSize(), buff1);
	fout1.write(buff1, f2->getFileSize());

	wait(mutex);
	cout <<"Upisan '" << filepath1 << "' u fajl os domacina 'izlaz1.dat'" << endl;
	signal(mutex);

	delete[] buff1;
	fout1.close();
	delete f2;

	wait(mutex);
	cout << "Zatvoren fajl " << filepath1 << "" << endl;
	signal(mutex);
	///////////////////////////////////////////////////////////////////////////

	
	File *f3 = FS::open(filepath2, 'w');

	wait(mutex);
	cout << "Kreiran fajl '" << filepath2 << "'" << endl;
	signal(mutex);

	f3->write(ulazSize2, ulazBuffer2);

	wait(mutex);
	cout << "Prepisan sadrzaj 'ulaz2.dat' u '" << filepath2 << "'" << endl;
	signal(mutex);

	wait(mutex);
	cout << "Velicina fajla '" << filepath2 << "': " << f3->getFileSize() << endl;
	signal(mutex);

	delete f3;
	delete[] ulazBuffer2;

	wait(mutex);
	cout << "Zatvoren fajl '" << filepath2 << "'" << endl;
	signal(mutex);

	wait(mutex);
	cout << filepath1 << " postoji: " << (FS::doesExist(filepath1) ? "da" : "ne") << "\n";
	cout << filepath2 << " postoji: " << (FS::doesExist(filepath2) ? "da" : "ne") << "\n";
	signal(mutex);
	///////////////////////////////////////////////////////////////////////////

	wait(mutex);
	cout << "Broj fajlova u root direktorijumu: " << FS::readRootDir() << "\n";  // 2
	signal(mutex);

	File *f4 = FS::open(filepath2, 'r');

	wait(mutex);
	cout << "Otvoren fajl " << filepath2 << "" << endl;
	signal(mutex);

	wait(mutex);
	cout << "Velicina fajla '" << filepath2 << "': " << f4->getFileSize() << endl;
	signal(mutex);

	ofstream fout2("izlaz2.dat", ios::out | ios::binary);
	char *buff2 = new char[f4->getFileSize()];
	f4->read(f4->getFileSize(), buff2);
	fout2.write(buff2, f4->getFileSize());

	wait(mutex);
	cout << "Upisan '" << filepath2 << "' u fajl os domacina 'izlaz2.dat'" << endl;
	signal(mutex);

	delete[] buff2;
	fout2.close();
	delete f4;

	wait(mutex);
	cout << "Zatvoren fajl " << filepath2 << "" << endl;
	signal(mutex);

	if (!FS::deleteFile(filepath1)) {
		cout << "Neuspesno brisanje " << filepath1 << "\n";
		exit(1);
	}

	wait(mutex);
	cout << "Fajl " << filepath1 << " uspesno obrisan.\n";

	cout << filepath1 << " postoji: " << (FS::doesExist(filepath1) ? "da" : "ne") << "\n";
	cout << filepath2 << " postoji: " << (FS::doesExist(filepath2) ? "da" : "ne") << "\n";
	cout << "Broj fajlova u direktorijumu: " << FS::readRootDir() << "\n";
	signal(mutex);
	///////////////////////////////////////////////////////////////////////////


	FS::unmount();

	wait(mutex);
	cout << "Demontirana particija p1" << endl;
	signal(mutex);
}

int main() {
	//initTest();
	oneThreadTwoCopyPaste();

	// return publicMain();
}
