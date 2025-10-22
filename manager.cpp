#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include "defns.h"

struct User {
    std::string name, ip;
    int m_port, c_port;
};

struct Disk {
    std::string name, ip;
    int m_port, c_port;
    bool inDSS, failed;
};

struct DSS {
    std::string name;
    int n, stripingUnit;
    std::vector<std::string> disks;
};

struct File {
    std::string name, owner;
    int size;
    std::vector<std::string> disks;
};

std::unordered_map<std::string, User> users;
std::unordered_map<std::string, Disk> disks;
std::unordered_map<std::string, DSS> dssArrays;
std::unordered_map<std::string, std::vector<File>> dssFiles;

void DieWithError(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}

bool isPowerOfTwo(int x) {
    return x && !(x & (x - 1));
}

std::string handleCommand(const DSSMessage &msg, DSSMessage &resp) {
    std::string op(msg.command);

    if(op=="register-user") {
        std::string name(msg.owner);
        if(users.find(name)!=users.end()) return "FAILURE user exists";
        users[name] = {name,"127.0.0.1",20001,20002};
        return "SUCCESS";
    }

    if(op=="register-disk") {
        std::string name(msg.fileName);
        if(disks.find(name)!=disks.end()) return "FAILURE disk exists";
        disks[name] = {name,"127.0.0.1",20001,20002,false,false};
        return "SUCCESS";
    }

    if(op=="configure-dss") {
        std::string dssName(msg.dssName);
        int n = msg.fileSize;
        int stripUnit = msg.blockIndex;
        if(dssArrays.find(dssName)!=dssArrays.end()) return "FAILURE DSS exists";
        if(n<3 || !isPowerOfTwo(stripUnit)) return "FAILURE invalid DSS";

        std::vector<std::string> freeDisks;
        for(auto &[k,d]:disks) if(!d.inDSS && !d.failed) freeDisks.push_back(k);
        if((int)freeDisks.size()<n) return "FAILURE not enough free disks";

        std::srand(std::time(nullptr));
        std::random_shuffle(freeDisks.begin(), freeDisks.end());
        std::vector<std::string> selected(freeDisks.begin(), freeDisks.begin()+n);
        for(auto &dname:selected) disks[dname].inDSS=true;

        dssArrays[dssName] = {dssName,n,stripUnit,selected};
        return "SUCCESS";
    }

    if(op=="copy") {
        std::string dssName(msg.dssName);
        if(dssArrays.find(dssName)==dssArrays.end()) return "FAILURE no DSS";

        File f = {msg.fileName, msg.owner, msg.fileSize, dssArrays[dssName].disks};
        dssFiles[dssName].push_back(f);
        return "SUCCESS";
    }

    if(op=="read") {
        std::string dssName(msg.dssName);
        if(dssFiles.find(dssName)==dssFiles.end()) return "FAILURE no files";
        for(auto &f:dssFiles[dssName]) {
            if(f.name==msg.fileName) {
                if(f.owner!=msg.owner) return "FAILURE permission denied";
                return "SUCCESS";
            }
        }
        return "FAILURE no such file";
    }

    if(op=="ls") {
        std::string dssName(msg.dssName);
        std::string files;
        for(auto &f:dssFiles[dssName]) files += f.name + " ";
        return files.empty()?"No files":files;
    }

    if(op=="disk-failure") {
        std::string diskName(msg.fileName);
        if(disks.find(diskName)==disks.end()) return "FAILURE no disk";
        disks[diskName].failed=true;
        return "SUCCESS";
    }

    if(op=="decommission-dss") {
        std::string dssName(msg.dssName);
        if(dssArrays.find(dssName)==dssArrays.end()) return "FAILURE no DSS";
        for(auto &dname:dssArrays[dssName].disks) disks[dname].inDSS=false;
        dssArrays.erase(dssName);
        dssFiles.erase(dssName);
        return "SUCCESS";
    }

    return "FAILURE unknown command";
}

int main(int argc,char *argv[]){
    if(argc!=2){std::cerr<<"Usage: "<<argv[0]<<" <manager-port>\n"; return 1;}
    int port = std::stoi(argv[1]);
    int sock = socket(AF_INET,SOCK_DGRAM,0); if(sock<0) DieWithError("socket failed");

    sockaddr_in servAddr; memset(&servAddr,0,sizeof(servAddr));
    servAddr.sin_family=AF_INET;
    servAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servAddr.sin_port=htons(port);

    if(bind(sock,(struct sockaddr*)&servAddr,sizeof(servAddr))<0) DieWithError("bind failed");

    std::cout<<"Manager running on port "<<port<<"\n";

    DSSMessage msg, resp;
    sockaddr_in cliAddr;
    socklen_t cliLen = sizeof(cliAddr);

    while(true){
        int recvLen = recvfrom(sock,&msg,sizeof(msg),0,(struct sockaddr*)&cliAddr,&cliLen);
        if(recvLen<0) DieWithError("recvfrom failed");
        std::string result = handleCommand(msg,resp);
        strncpy(resp.command,result.c_str(),31);
        sendto(sock,&resp,sizeof(resp),0,(struct sockaddr*)&cliAddr,cliLen);
    }
    close(sock);
    return 0;
}
