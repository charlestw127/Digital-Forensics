//Charles Wallis | CS 4398.001 | Final Project Recovery
#include <iostream>
#include <fstream>
#include <list>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
using namespace std;

//global constants
#define BLOCK_SIZE 4096
//global var
char* devicePath;
list<uint32_t> PNGBlockNums;

//functions
void recoverFile();
void startPNG();
uint8_t *copyBlock(uint32_t block_num);
void writeRecoveryFile(uint32_t block_num, int count);
uint32_t getDataBlockPointer(uint32_t block_number);
uint32_t getIndirectBlockPointer(uint32_t block_number);
uint32_t readIndirectBlocks(uint32_t block_num, uint32_t readFrom, ofstream& output);
uint32_t readDoubleIndirectBlocks(uint32_t block_num, uint32_t readFrom, ofstream& output);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: sudo ./recover <device path>" << endl;
        return 1;
    }   //check arg count
    devicePath = argv[1];
    recoverFile();
    return 0;
}   //main

void recoverFile(){
    startPNG();
    
    int count = 1;
    //create recovery file for each png file found
    cout << "\n=========================Start Recovery=========================\n" << endl;
    while(!PNGBlockNums.empty()){
        uint32_t block_num = PNGBlockNums.front();
        PNGBlockNums.pop_front();
        cout << "=====================Recovering PNG File #" << count << "====================="<< endl;
        writeRecoveryFile(block_num, count);
        count++;
    }   //while png file exists
}   //recover file

void startPNG() {
    //Open the device
    int fd = open(devicePath, O_RDONLY);
    int file_num = 1;
    if (fd == -1) {
        cerr << "Failed to open device";
        exit(1);
    }   //check if device opened

    //Calculate number of blocks
    uint64_t end = lseek(fd, 0, SEEK_END);
    uint32_t num_blocks = end / BLOCK_SIZE;

    lseek(fd, 0, SEEK_SET); //Reset offset to partition start

    for (uint32_t block_num = 0; block_num < num_blocks; block_num++) {
        // get the block offset
        uint64_t offset = block_num * BLOCK_SIZE;

        if (offset > end) {
            cerr << "Offset is out of bounds";
            exit(1);
        }   //check if offset is correct

        //seek offset
        lseek(fd, offset, SEEK_SET);

        //read 8 bytes
        unsigned char readblock[8];
        read(fd, readblock, 8);
        
        uint32_t firstPNGSign = (readblock[0] << 24) | (readblock[1] << 16) | (readblock[2] << 8) | readblock[3];
        uint32_t secondPNGSign = (readblock[4] << 24) | (readblock[5] << 16) | (readblock[6] << 8) | readblock[7];

        if ((firstPNGSign == 0x89504E47) && (secondPNGSign == 0x0D0A1A0A)){ //PNG file signature in two parts
            cout << "PNG File #" << file_num << " found at block " << block_num << endl;
            PNGBlockNums.push_back(block_num);
            file_num++;
        }   //if png file found, add to list
    }   //for each block

    // Close the device
    close(fd);
}   //identify PNG blocks

uint8_t* copyBlock(uint32_t block_num) {
    int fd = open(devicePath, O_RDONLY);

    if (fd == -1) {
        cerr << "Failed to open device in copyBlock";
        exit(1);
    }   //check if device opened

    off_t offset = block_num * BLOCK_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek file");
        close(fd);
        return NULL;
    }   //check if seek successful

    uint8_t* buf = (uint8_t*)malloc(BLOCK_SIZE);
    if (buf == NULL) {
        perror("Failed to allocate memory");
        close(fd);
        return NULL;
    }   //check if memory allocated

    ssize_t num_read = 0;
    ssize_t total_read = 0;
    while (total_read < BLOCK_SIZE) {
        num_read = read(fd, buf + total_read, BLOCK_SIZE - total_read);
        if (num_read == -1) {
            perror("Failed to read file");
            delete[] buf;
            close(fd);
            return NULL;
        }
        total_read += num_read;
    }   //while reading..

    close(fd);
    return buf;
}   //copy the data

void writeRecoveryFile(uint32_t block_num, int count){
    ofstream output;
    string fileName = "recovered_" + to_string(count) + ".png";

    output.open(fileName, ios::out | ios::binary);
    if(output.is_open()){
        for(int i = 0; i < 12; i++){
            uint8_t *buffer = copyBlock(block_num+i);
            output.write((char*)buffer, BLOCK_SIZE);
        }   //for direct blocks

        uint32_t lastSingleIndirectBlock = readIndirectBlocks(block_num+12, block_num+12, output);
        if(lastSingleIndirectBlock > 1){
            uint32_t lastDoubleIndirectBlock = readDoubleIndirectBlocks(lastSingleIndirectBlock+1, lastSingleIndirectBlock+1, output);
        }   //if single indirect blocks exist

    }
    output.close();
    cout << "File recovered at " << fileName << endl;
    cout << endl;
}   //create recovery file
       
uint32_t getDataBlockPointer(uint32_t block_number){
    // Open the device or partition in read-only mode
    int fd = open(devicePath, O_RDONLY);
    int file_num = 1;

    // Check if the device was opened successfully
    if (fd == -1) {
        cerr << "Failed to open device";
        exit(1);
    }

    // Calculate the total number of blocks
    uint64_t end = lseek(fd, 0, SEEK_END);
    uint32_t num_blocks = end / BLOCK_SIZE; 

    // Reset the file offset to the beginning of the partition
    lseek(fd, 0, SEEK_SET);

    for (uint32_t block_num = 10000; block_num < num_blocks; block_num++) {
        //get the block offset
        uint64_t offset = block_num * BLOCK_SIZE;

        if (offset > end) {
            cerr << "Offset is out of bounds";
            exit(1);
        }   //check offset

        //go to block offset
        lseek(fd, offset, SEEK_SET);

        //read 8 bytes
        uint32_t readblock[8];
        read(fd, readblock, 8);
        
        if(readblock[0] == block_number){
            cout << "Found data block " << dec << block_number << " from pointer block " << block_num << endl;
            return block_num;
        }   //if block number matches, return block number

    }   //iterate through blocks 

    close(fd);
    return (uint32_t)NULL;
}   //get data block pointer

uint32_t getIndirectBlockPointer(uint32_t block_number){
    int fd = open(devicePath, O_RDONLY);
    int file_num = 1;

    if (fd == -1) {
        cerr << "Failed to open device";
        exit(1);
    }   //check if device opened

    //Calculate the total number of blocks
    uint64_t end = lseek(fd, 0, SEEK_END);
    uint32_t num_blocks = end / BLOCK_SIZE;

    //reset offset to start of partition
    lseek(fd, 0, SEEK_SET);

    for (uint32_t block_num = 10000; block_num < num_blocks; block_num++) {
        //get block offset
        uint64_t offset = block_num * BLOCK_SIZE;

        if (offset > end) {
            cerr << "Offset is out of bounds";
            exit(1);
        }   //check offset

        //seek to block offset
        lseek(fd, offset, SEEK_SET);

        //read 8 bytes
        uint32_t readblock[8];
        read(fd, readblock, 8);
        
        if(readblock[0] == block_number ){
            cout << "Found data block " << dec << block_number << " from pointer block " << block_num << endl;
            uint32_t doublePointer = getDataBlockPointer(block_num);
            cout << "found pointer block " << block_num << " from double pointer block " << doublePointer << endl;
            return doublePointer;
        }  

    }   //iterate through blocks

    close(fd);
    return (uint32_t)NULL;
}   //get indirect block pointer

uint32_t readIndirectBlocks(uint32_t block_num, uint32_t readFrom, ofstream& output) {
    if(block_num == 0){return 0;}   //if no indirect block, return 0

    uint32_t singleIndirectBlock = getDataBlockPointer(readFrom);
        
    //print each buffer in singleIndirectBuffer
    cout << "Single Indirect Block Location: " << singleIndirectBlock << endl;
    uint8_t *singleBuffer = copyBlock(singleIndirectBlock);

    uint32_t lastSingleIndirectBlock = 0;

    for(int i = 0; i < 1024; i++){
        uint32_t block = (uint32_t) singleBuffer[i*4] | (uint32_t) singleBuffer[i*4+1] << 8 | 
                         (uint32_t) singleBuffer[i*4+2] << 16 | (uint32_t) singleBuffer[i*4+3] << 24;
        
        //If block is not 0, copy the block to output
        if(block != 0){
            lastSingleIndirectBlock = block;
            uint8_t *buffer = copyBlock(block);
            output.write((char*)buffer, BLOCK_SIZE);
        } 
        else {
            lastSingleIndirectBlock = block;
            break;
        }
    }
    
    if(singleIndirectBlock != 0){
        cout << "Last data block from single indirect block: " << lastSingleIndirectBlock << endl;
    }   //Return the last block number if the loop reaches 1024 from the above for-loop

    return lastSingleIndirectBlock;
}   //copy single indirect blocks

uint32_t readDoubleIndirectBlocks(uint32_t block_num, uint32_t readFrom, ofstream& output) {
    //if double indirect block doesn't exist, return 0
    if (block_num == 0) {return 0;} 
    
    uint32_t nextBlock = 0;
    uint32_t doubleIndirectBlock = getIndirectBlockPointer(block_num);

    if (doubleIndirectBlock == 0) {
        cout << "Double Indirect Block not found" << endl;
        return 0;
    }   //if double indirect block doesn't exist, return 0

    nextBlock = block_num;  //set nextBlock

    //Print each buffer in singleIndirectBuffer
    cout << "Double Indirect Block Location: " << doubleIndirectBlock << endl;

    uint8_t* doubleBuffer = copyBlock(doubleIndirectBlock);
    uint32_t lastDoubleIndirectBlock = 0;

    for (int i = 0; i < 1024; i++) {
        uint32_t singleIndirectBlock = (uint32_t)doubleBuffer[i * 4] | 
                                        (uint32_t)doubleBuffer[i * 4 + 1] << 8 |
                                        (uint32_t)doubleBuffer[i * 4 + 2] << 16 | 
                                        (uint32_t)doubleBuffer[i * 4 + 3] << 24;

        if (singleIndirectBlock != 0) {
            cout << "Going to Single Indirect Block: " << singleIndirectBlock << endl;
            uint32_t lastSingleIndirectBlock = readIndirectBlocks(singleIndirectBlock, nextBlock, output);
            nextBlock = lastSingleIndirectBlock + 1;
            
            if (lastSingleIndirectBlock == 0) {
                break;
            } else {
                lastDoubleIndirectBlock = singleIndirectBlock;
            }

        } else {break;}
    }

    delete[] doubleBuffer;  //free buffer for memory
    return lastDoubleIndirectBlock;
}   //read double indirect blocks
