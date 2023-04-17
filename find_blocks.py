import os
import sys

def main():
    #PNG filetype signature (first 8 bytes) using HEX
    png_signature = bytes.fromhex('89 50 4E 47 0D 0A 1A 0A')
    
    block_size = 4096        
    
    #path of my partition
    partition_path = "/dev/sda1"
    print(f"partition_path set to {partition_path}")
    
    with open(partition_path, "rb") as partition:
        block_number = 0 
        while True:
            block = partition.read(block_size)
            
            if not block:
                print(f"Reached end of partition after reading {block_number} blocks")
                break
                
            #print(f"Block {block_number}: {block[:8]}")
            if block[:8] == png_signature:
                print(f"Found PNG signature at block {block_number}")
            
            block_number += 1
        
if __name__ == "__main__":
    main()
