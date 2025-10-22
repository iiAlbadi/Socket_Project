#include <iostream>
#include <thread>
#include <unordered_map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "defns.h"

std::unordered_map<std::string, std::string> storedBlocks;

void DieWithError(const std::string &msg) { perror(msg.c_str()); exit(1); }

void diskThread(int sock) {
    DSSMessage msg, resp;
    sockaddr_in fromAddr; socklen_t fromLen = sizeof(fromAddr);
    while(true){
        int recvLen = recvfrom(sock,&msg,sizeof(msg),0,(struct sockaddr*)&fromAddr,&fromLen);
        if(recvLen<0) DieWithError("disk recvfrom failed");

        std::string op(msg.command);
        if(op=="copy") storedBlocks[msg.fileName] = msg.data;
        if(op=="read") strncpy(resp.data, storedBlocks[msg.fileName].c_str(),MAX_DATA);
        strncpy(resp.command,"SUCCESS",31);
        sendto(sock,&resp,sizeof(resp),0,(struct sockaddr*)&fromAddr,fromLen);
    }
}

int main(int argc,char* argv[]){
    if(argc!=2){std::cerr<<"Usage: "<<argv[0]<<" <disk-port>\n"; return 1;}
    int sock = socket(AF_INET,SOCK_DGRAM,0); if(sock<0) DieWithError("socket failed");

    sockaddr_in servAddr; memset(&servAddr,0,sizeof(servAddr));
    servAddr.sin_family=AF_INET; servAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servAddr.sin_port=htons(std::stoi(argv[1]));

    if(bind(sock,(struct sockaddr*)&servAddr,sizeof(servAddr))<0) DieWithError("bind failed");

    std::thread t(diskThread,sock);
    t.join();
    close(sock);
    return 0;
}
