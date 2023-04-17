//Charles Wallis | 4398.001 | Digital Forensics | 03/25/2023
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

//define global var
char* devicePath;
int indirect_block_count = 0;
int block_size = 4096;
int previousByte = 0;

int getIndirectBlockNum(){
    int fd = open(devicePath, O_RDONLY);    //open device

    int end = lseek(fd, 0, SEEK_END);  //go to end of the partition
    int num_blocks = end / block_size; //calculate number of blocks
    lseek(fd, 0, SEEK_SET); //go back to the beginning of the partition

    for (int block_num = 0; block_num < num_blocks; block_num++) {
        char buffer[16];
        int bytes_read = read(fd, buffer, sizeof(buffer)); //read first 16 bytes (buffer)
        int byteVal = *(reinterpret_cast<int*>(buffer));
    
        if (block_num == 0 || byteVal > previousByte) {
            indirect_block_count++;
        }   //if previous value is smaller, then mark it as indirect
        previousByte = byteVal;
    }   //for each blocks in partition

    return indirect_block_count;
    close(fd);
}   //get indirect block number


int main(int argc, char* argv[]) {
    if (argc != 2) {    // check for correct number of arguments
        cout << "Make sure you enter correct number of arguments" << endl;
        return 1;
    }

    devicePath = argv[1]; //assign argument to devicePath

    //run function and print number of indirect blocks
    int iBlockNum = getIndirectBlockNum();
    cout << "Indirect blocks: " << iBlockNum << endl;

    return 0;
}   //main