//Charles Wallis | CTW170000 | CS4398.001
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>

using namespace std;

//declaring constants
const int LDA_ADDR = 0x1C6;
const int PADDING = 0x400;
const int SECTOR_SIZE = 0x200;
const int SUPERBLOCK_MAGIC_OFFSET = 0x38;
const int BLOCK_SIZE_OFFSET = 0x18;
const int BLOCKS_PER_GROUP_OFFSET = 0x20;
const int BLOCK_GROUP_NUMBER_OFFSET = 0x5A;

//delcaring global var
char* usb;
int partitionAddress;
int BLOCKSIZE;
int BLOCKSPERGROUP;

int getPartAddr() {
    int fd = open(usb, O_RDONLY); //open file

    //read LDA address at 0x1C6
    int ldaAddrChar[4];
    lseek(fd, LDA_ADDR, SEEK_SET);
    read(fd, ldaAddrChar, 4);

    close(fd);
    return *ldaAddrChar * SECTOR_SIZE;
}   //returns partition address


int getSuperBlockAddr(int block_group_number) {
    int fd = open(usb, O_RDONLY); //open file
    int blockGroupAddr;

    if(block_group_number == 0)
        blockGroupAddr = partitionAddress + PADDING;
    else 
        blockGroupAddr = partitionAddress + (block_group_number * BLOCKSIZE * BLOCKSPERGROUP);
    
    close(fd);
    return blockGroupAddr;
}   //return block group address


void getSuperBlock(int superBlockAddr) {
    int fd = open(usb, O_RDONLY); //open file

    //get magic number at offset 0x38
    int addr = superBlockAddr + SUPERBLOCK_MAGIC_OFFSET;
    lseek(fd, addr, SEEK_SET);
    int magic[2];
    read(fd, magic, 2);
    int magicNumber = magic[0] & 0xFFFF;

    //get block size at offset 0x18
    addr = superBlockAddr + BLOCK_SIZE_OFFSET;
    lseek(fd, addr, SEEK_SET);
    int size[2];
    read(fd, size, 2);
    int blocksize = size[0] & 0xFFFF;
    int blockSize = pow(2, 10 + blocksize);

    //get blocks per group at offset 0x20
    addr = superBlockAddr + BLOCKS_PER_GROUP_OFFSET;
    lseek(fd, addr, SEEK_SET);
    int bpg[2];
    read(fd, bpg, 2);
    int blocks_per_group = bpg[0] & 0xFFFF;

    //get block group number at offset 0x5A
    addr = superBlockAddr + BLOCK_GROUP_NUMBER_OFFSET;
    lseek(fd, addr, SEEK_SET);
    int bgn[2];
    read(fd, bgn, 2);
    int block_group_number = bgn[0] & 0xFFFF;
    
    //if I'm at block 0, add to global
    if(superBlockAddr == partitionAddress + PADDING) {
        BLOCKSIZE = blockSize; 
        BLOCKSPERGROUP = blocks_per_group;
    }  

    //displays info
    cout << "Magic Number: 0x" << hex << uppercase << magicNumber << nouppercase << endl;
    cout << "Block Size: " << dec << blockSize << " bytes"<< endl;
    cout << "Blocks per Group: " << dec << blocks_per_group << " blocks "<< endl;
    cout << "Block Group Number: " << dec << block_group_number << endl;

    close(fd);
}   //get superblock


void print_info() {
    partitionAddress = getPartAddr();
    cout << "Partition address: 0x" << hex << partitionAddress << endl;

    int superBlockAddr;
    superBlockAddr = getSuperBlockAddr(0);
    cout << "\nSuperblock Group 0 address: 0x" << hex << superBlockAddr << endl;
    getSuperBlock(superBlockAddr);

    superBlockAddr = getSuperBlockAddr(3);
    cout << "\nSuperblock Group 3 address: 0x" << hex << superBlockAddr << endl;
    getSuperBlock(superBlockAddr);
}   //print info


int main(int argc, char* argv[]) {
    //parameter check
    if (argc != 2) {
        cout << "Usage: ./finder <device path>" << endl;    
        return 1;    
    }

    //set path
    usb = argv[1];

    //run diagnostic
    print_info();
    return 0;
}   //main