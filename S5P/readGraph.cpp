#include "readGraph.h"

void FIXLINE(char *s)
{
    int len = (int)strlen(s) - 1;
    if (s[len] == '\n')
        s[len] = 0;
}

TGEngine::TGEngine() {}

TGEngine::TGEngine(int nodeNUM,int64_t edgeNUM) {
    this->edgeNUM = edgeNUM*2;
    this->nodeNUM = nodeNUM;
}

TGEngine::TGEngine(std::string graphPath,int nodeNUM,int64_t edgeNUM) {
    this->graphPath = graphPath;

    Fd = open(this->graphPath.c_str(), O_RDONLY);
    if (Fd == -1) {
        perror("open");
    }

    struct stat sb;
    if (fstat(Fd, &sb) == -1) {
        perror("fstat");close(Fd);
    }
    edgeLength = sb.st_size;
    this->edgeNUM = edgeNUM*2;
    this->nodeNUM = nodeNUM;
}

void TGEngine::loadingMmapBlock() {
    chunkSize = std::min((long)readSize, edgeLength - offset);
    edgeAddr = static_cast<int*>(mmap(nullptr, chunkSize, PROT_READ, MAP_SHARED, Fd, offset));
    if(edgeAddr == MAP_FAILED)
    {
        perror("mmap");
        close(Fd);
    }
    offset += chunkSize;
}

void TGEngine::unmapBlock(int* addr, off_t size) {
    munmap(addr, size);
}

int TGEngine::readline(std::pair<int, int> &edge) {
    if (readPtr == edgeNUM){
        unmapBlock(edgeAddr,chunkSize);
        return -1;
    }
        
    if (readPtr % batch == 0){
        if (chunkSize != 0) {
            unmapBlock(edgeAddr,chunkSize);
        }
        loadingMmapBlock();
    }
    edge.first = edgeAddr[readPtr%batch];
    edge.second = edgeAddr[readPtr%batch+1];
    readPtr += 2;
    return 0;
}

void TGEngine::convert2bin(std::string raw_graphPath,std::string new_graphPath,char delimiter,bool saveDegree,std::string degreePath="") {
    if (saveDegree) {
        degrees.resize(this->nodeNUM,0);
    }
    std::ifstream inputFile(raw_graphPath);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open input file: " << raw_graphPath << std::endl;
        return;
    }

    std::ofstream outputFile(new_graphPath, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << new_graphPath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        int src, dst;
        char block;
        if (delimiter == ' ')
            iss >> src >> dst;
        else
            iss >> src >> block >> dst;
        if (saveDegree) {
            degrees[src]++;
            degrees[dst]++;
        }
        outputFile.write((char *)&src, sizeof(int));
        outputFile.write((char *)&dst, sizeof(int));  
    }
    inputFile.close();
    outputFile.close();
    if (saveDegree) {
        outputFile.open(degreePath, std::ios::binary);
        outputFile.write((char *)&degrees[0], degrees.size() * sizeof(int));
        outputFile.close();
    }
}

void TGEngine::convert_edgelist(std::string inputfile,std::string outputfile)
{
    FILE *inf = fopen(inputfile.c_str(), "r");
    size_t bytesread = 0;
    size_t linenum = 0;
    if (inf == NULL) {
        std::cout << "Could not load:" << inputfile
                   << ", error: " << strerror(errno) << std::endl;
    }

    std::ofstream outputFile(outputfile, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputfile << std::endl;
        return;
    }

    std::cout << "Reading in edge list format!" << std::endl;
    char s[1024];
    while (fgets(s, 1024, inf) != NULL) {
        linenum++;
        if (linenum % 10000000 == 0) {
            std::cout << "Read " << linenum << " lines, "
                      << bytesread / 1024 / 1024. << " MB" << std::endl;
        }
        FIXLINE(s);
        bytesread += strlen(s);
        if (s[0] == '#')
            continue; // Comment
        if (s[0] == '%')
            continue; // Comment

        char delims[] = "\t, ";
        char *t;
        t = strtok(s, delims);
        if (t == NULL) {
            std::cout << "Input file is not in right format. "
                       << "Expecting \"<from>\t<to>\". "
                       << "Current line: \"" << s << "\"\n";
        }
        int from = atoi(t);
        t = strtok(NULL, delims);
        if (t == NULL) {
            std::cout << "Input file is not in right format. "
                       << "Expecting \"<from>\t<to>\". "
                       << "Current line: \"" << s << "\"\n";
        }
        int to = atoi(t);
        outputFile.write((char *)&from, sizeof(int));
        outputFile.write((char *)&to, sizeof(int));  
    }
    fclose(inf);
    outputFile.close();
}

void TGEngine::readDegree(std::string degreePath,std::vector<int>& degreeList) {
    std::ifstream file(degreePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << degreePath << std::endl;
        return;
    }
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    size_t numIntegers = fileSize / sizeof(int);
    degreeList.resize(numIntegers);
    file.read(reinterpret_cast<char*>(degreeList.data()), fileSize);
    file.close();
    return;
}

void TGEngine::writeVec(std::string savePath,std::vector<int>& vec) {
    std::ofstream file(savePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << savePath << std::endl;
        return;
    }
    file.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(int));
    file.close();
    std::cout << "Data has been written to " << savePath << std::endl;
    return;
}









