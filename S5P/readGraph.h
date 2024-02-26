#include "common.h"



class TGEngine{
public:
    std::string graphPath;

    int Fd;
    off_t edgeLength;
    int* edgeAddr;

    int64_t edgeNUM=0;
    int nodeNUM=0;
    int64_t readPtr=0;
    std::vector<int> degrees;
    size_t readSize = 4096 * 16;
    int batch = readSize / sizeof(int);
    off_t chunkSize = 0;
    off_t offset = 0;

    TGEngine();
    TGEngine(int nodeNUM,int64_t edgeNUM);
    TGEngine(std::string graphPath,int nodeNUM,int64_t edgeNUM);
    void loadingMmapBlock();
    void unmapBlock(int* addr, off_t size);
    int readline(std::pair<int, int> &edge);
    void convert2bin(std::string raw_graphPath,std::string new_graphPath,char delimiter,bool saveDegree,std::string degreePath);
    void readDegree(std::string degreePath,std::vector<int>& degreeList);
    void writeVec(std::string savePath,std::vector<int>& vec);
    void convert_edgelist(std::string inputfile,std::string outputfile);
};
