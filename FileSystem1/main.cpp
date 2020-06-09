#include <fstream>
#include <iostream>
using namespace std;


const int FILE_NAME_LEN = 8;
const int FILE_EXT_LEN = 8;

const int BlockSize = 512;
const int InodeBlockSum = 30;

const int DataBlockSum = 512;
const int DiskBlockSum = 1 + 1+ InodeBlockSum + DataBlockSum;
const int InodeSize = 32;
const int DataBlockStart = InodeBlockSum;

const int InodeSumInOneBlock = BlockSize / InodeSize;
const int InodeSum = InodeSumInOneBlock * 32;

const int FreeBlockSum = DataBlockSum / 100 + 1;
const int MaxFreeBlockCount = 100;

struct block
{
	bool bit[512 * 8] = { 0 };
};



const int NameLen = 22;



struct inode {  //32byte
	int inodeId = 0;
	char Name[NameLen] = { 0 };
	int firstDataBlockIndex = { 0 };
};


struct datablock {
	int nextBlock;
	char data[512 - sizeof(int)] = { 0 };
};


struct Disk {

	block data[DiskBlockSum];
};


struct FreeBlock
{

	int EmptyBlockList[101];
};

struct superblock {
	int totalINodeSum;
	int freeINodeSum;
	int totalBlockSum;
	int freeBlockSum;
	char name[20] = { 0 };
	int firstInode = 0;
	FreeBlock SuperEmptyBlockList;

};


superblock SuperBlock;
inode Inode[InodeSum];

bool InodeBitmap[InodeSum] = { 0 }; //0表示inode节点没被用 1表示inode节点被用 最大支持512*8个inode节点索引


Disk disk;


int GetAInode()
{
	static int lastPos = 0;

	for (int i = lastPos; i < InodeSum; i++)
	{
		if (InodeBitmap[i] == 0)
		{
			InodeBitmap[i] = 1;
			lastPos = i;
			return i;

		}
	}
	for (int i = 0; i < lastPos; i++)
	{
		if (InodeBitmap[i] == 0)
		{
			InodeBitmap[i] = 1;
			lastPos = i;
			return i;

		}
	}

	return -1;
}

bool FreeAInode(int index)
{
	if (index < InodeSum)
	{
		InodeBitmap[index] = 0;
		return true;
	}
	return false;
}



FreeBlock makeFreeBlock(Disk &disk, int index)
{
	FreeBlock freeBlock;
	freeBlock.EmptyBlockList[0] = 0;
	if (index + 100 < DiskBlockSum)
	{
		freeBlock.EmptyBlockList[1] = index + 100;
	}
	else
	{
		freeBlock.EmptyBlockList[1] = 0;
	}
	for (int i = 1; i < 100; i++)
	{
		
		if (index + i >= DiskBlockSum)
		{
			break;
		}
		freeBlock.EmptyBlockList[0]++;
		freeBlock.EmptyBlockList[i+1] = index + i;
		cout << "Link " << index + i << endl;
	}
	return freeBlock;
}



void initGroupLink(Disk& disk)
{

	int FreeBlockSum = DataBlockSum / 100+1;
	SuperBlock.SuperEmptyBlockList.EmptyBlockList[0] = 1;
	SuperBlock.SuperEmptyBlockList.EmptyBlockList[1] = DataBlockStart;
	for (int i = 0; i < FreeBlockSum; i++)
	{
		FreeBlock freeBlock = makeFreeBlock(disk, i * 100+DataBlockStart);

		memset(&disk.data[i * 100 + DataBlockStart], 0, sizeof(block));
		memcpy(&disk.data[i * 100 + DataBlockStart], &freeBlock, sizeof(FreeBlock));
		cout << "Make linklist " << i * 100 + DataBlockStart << endl;
	}
}

FreeBlock* LoadBlockAsFreeBlock(Disk& disk, int index)
{
	if (index > DataBlockSum + DataBlockStart)
	{
		return NULL;
	}
	FreeBlock *freeBlock=new FreeBlock;
	
	memcpy(freeBlock, &disk.data[index], sizeof(FreeBlock));
	return freeBlock;
}

bool SaveFreeBlockToDisk(Disk& disk, int index, FreeBlock& freeBlock)
{
if (index >= DiskBlockSum)
{
	return false;
}

memset(&disk.data[index], 0, sizeof(block));
memcpy(&disk.data[index], &freeBlock, sizeof(FreeBlock));
return true;
}

void ShowFreeBlock(FreeBlock* freeBlock, Disk& disk, int deep)
{
	if (freeBlock->EmptyBlockList[1] != 0)
	{
		FreeBlock* freeBlockNext = LoadBlockAsFreeBlock(disk, freeBlock->EmptyBlockList[1]);
		ShowFreeBlock(freeBlockNext, disk, deep + 1);
	}
	int ShowSum = freeBlock->EmptyBlockList[0];
	if (freeBlock->EmptyBlockList[1] != 0)
	{
		ShowSum--;
	}
	for (int i = 0; i < ShowSum; i++)
	{
		for (int j = 0; j <= deep; j++)
		{
			cout << "\t";
		}
		cout << freeBlock->EmptyBlockList[i + 2] << endl;
	}

}

void showAll()
{
	for (int i = 0; i < SuperBlock.SuperEmptyBlockList.EmptyBlockList[0]; i++)
	{
		FreeBlock* freeBlock = LoadBlockAsFreeBlock(disk, SuperBlock.SuperEmptyBlockList.EmptyBlockList[i + 1]);
		ShowFreeBlock(freeBlock, disk, 0);
	}
	for (int i = 0; i <= SuperBlock.SuperEmptyBlockList.EmptyBlockList[0]; i++)
	{
		cout << SuperBlock.SuperEmptyBlockList.EmptyBlockList[i] << endl;
	}
}




int GetOneBlock(Disk& disk)
{

	if (SuperBlock.SuperEmptyBlockList.EmptyBlockList[0] == 1)
	{
		if (SuperBlock.SuperEmptyBlockList.EmptyBlockList[1] == 0)
		{
			return -1;
		}
		else
		{
			int index = SuperBlock.SuperEmptyBlockList.EmptyBlockList[1];
			SuperBlock.SuperEmptyBlockList = *LoadBlockAsFreeBlock(disk, index);
			return index;
		}
	}
	else {
		int index = SuperBlock.SuperEmptyBlockList.EmptyBlockList[SuperBlock.SuperEmptyBlockList.EmptyBlockList[0]];
		SuperBlock.SuperEmptyBlockList.EmptyBlockList[0]--;
		return index;
	}
}

void FreeABlock(Disk& disk, int index)
{
	if (SuperBlock.SuperEmptyBlockList.EmptyBlockList[0] == MaxFreeBlockCount)
	{
		SaveFreeBlockToDisk(disk, index, SuperBlock.SuperEmptyBlockList);
		SuperBlock.SuperEmptyBlockList.EmptyBlockList[0] = 1;
		SuperBlock.SuperEmptyBlockList.EmptyBlockList[1] = index;
	}
	else
	{
		SuperBlock.SuperEmptyBlockList.EmptyBlockList[0]++;
		SuperBlock.SuperEmptyBlockList.EmptyBlockList[SuperBlock.SuperEmptyBlockList.EmptyBlockList[0]] = index;
	}
}

void ShowAllBlock(Disk& disk)
{
	cout << "Total Block:" << DiskBlockSum << endl;

}


void SaveSuperBlockToDisk(superblock& SuperBlock, Disk& disk)
{
	memcpy(&disk.data[0], &SuperBlock, sizeof(block));
}
void SaveInodeToDisk(bool& bitmap, inode& inodeList, Disk& disk)
{
	memcpy(&disk.data[1], &bitmap, sizeof(block));
	memcpy(&disk.data[2], &inodeList, sizeof(inode) * InodeSum);

}

void LoadSuperBlockToDisk(superblock& SuperBlock, Disk& disk)
{
	memcpy(&SuperBlock, &disk.data[0], sizeof(superblock));

}

void LoadInodeFromDisk(bool& bitmap, inode& inodeList, Disk& disk)
{
	memcpy(&bitmap , &disk.data[1], sizeof(block));
	memcpy(&inodeList , &disk.data[2], sizeof(inode) * InodeSum);
}

void SaveDisk()
{
	SaveSuperBlockToDisk(SuperBlock, disk);
	SaveInodeToDisk(*InodeBitmap, *Inode, disk);
	ofstream out;
	out.open("data.dat", ios::out|ios::binary);

	char* buffer = (char*)malloc(sizeof(Disk));
	memcpy(buffer, &disk, sizeof(Disk));
	out.write(buffer, sizeof(disk));
	out.close();
	free(buffer);
}


void LoadDisk()
{
	ifstream in;
	in.open("data.dat", ios::in | ios::binary);
	char* buffer = (char*)malloc(sizeof(Disk));
	in.read(buffer, sizeof(disk));
	in.close();
	memcpy(&disk, buffer, sizeof(Disk));
	free(buffer);

	LoadSuperBlockToDisk(SuperBlock, disk);
	LoadInodeFromDisk(*InodeBitmap, *Inode, disk);
}


int main()
{
	initGroupLink(disk);

	showAll();
	while (1)
	{
		char a;
		cin >> a;
		
		if (a == 'r')
		{
			cout << GetOneBlock(disk) << endl;
		}
		if (a == 's')
		{
			showAll();
		}
		if (a == 'f')
		{
			int index;
			cin >> index;
			FreeABlock(disk, index);
		}

		if (a == 'z')
		{
			cout << GetAInode() << endl;
		}
		if (a == 'x')
		{
			int index;
			cin >> index;
			FreeAInode(index);
		}
		if (a == 'c')
		{
			SaveDisk();
		}
		if (a == 'v')
		{
			LoadDisk();
		}
		if (a == 'l')
		{
			char name[20];
			cin >> name;
			strcpy_s(SuperBlock.name, name);
		}
		if (a == 'k')
		{
			cout << SuperBlock.name << endl;
		}

	}
	


	return 0;
}