#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "defns.h"

void DieWithError(const std::string &msg){perror(msg.c_str());exit(1);}

void sendCommand(int sock, sockaddr_in &addr, DSSMessage &msg, DSSMessage &resp){
    sendto(sock,&msg,sizeof(msg),0,(struct sockaddr*)&addr,sizeof(addr));
    socklen_t addrLen = sizeof(addr);
    recvfrom(sock,&resp,sizeof(resp),0,(struct sockaddr*)&addr,&addrLen);
    std::cout<<"User received: "<<resp.command<<"\n";
}

int main(int argc,char* argv[]){
    if(argc!=3){std::cerr<<"Usage: "<<argv[0]<<" <manager-ip> <manager-port>\n"; return 1;}
    int sock = socket(AF_INET,SOCK_DGRAM,0); if(sock<0) DieWithError("socket failed");

    sockaddr_in mgrAddr; memset(&mgrAddr,0,sizeof(mgrAddr));
    mgrAddr.sin_family=AF_INET;
    mgrAddr.sin_addr.s_addr=inet_addr(argv[1]);
    mgrAddr.sin_port=htons(std::stoi(argv[2]));

    DSSMessage msg, resp;

    // Example: register-user
    strncpy(msg.command,"register-user",31);
    strncpy(msg.owner,"User1",31);
    sendCommand(sock,mgrAddr,msg,resp);

    // Example: configure-dss
    strncpy(msg.command,"configure-dss",31);
    strncpy(msg.dssName,"DSS1",31);
    msg.fileSize=3;  // n disks
    msg.blockIndex=128; // striping unit
    sendCommand(sock,mgrAddr,msg,resp);

    // Example: copy a file
    strncpy(msg.command,"copy",31);
    strncpy(msg.fileName,"day-of-affirmation.txt",63);
    strncpy(msg.data,"Hello World",MAX_DATA);
    strncpy(msg.owner,"User1",31);
    msg.fileSize=11;
    sendCommand(sock,mgrAddr,msg,resp);

    // Example: read file
    strncpy(msg.command,"read",31);
    strncpy(msg.fileName,"day-of-affirmation.txt",63);
    strncpy(msg.owner,"User1",31);
    sendCommand(sock,mgrAddr,msg,resp);

    close(sock);
    return 0;
}
