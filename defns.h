#ifndef DEFNS_H
#define DEFNS_H

#include <string>
#include <vector>

#define MAX_DATA 1024

struct DSSMessage {
    char command[32];     // e.g., register-user, copy, read, ls
    char dssName[32];     // DSS name
    char fileName[64];    // File name
    char owner[32];       // User who owns the file
    int fileSize;         // Size of file or block
    int blockIndex;       // Block index in DSS
    char data[MAX_DATA];  // Block data or message
};

#endif