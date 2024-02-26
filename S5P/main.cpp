#include "Partitioner.h"


// init global config set
GlobalConfig config("./project.properties");

template<typename Time>
inline void printCostTime(Time begin , Time end, std::string des) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(begin - end);
    std::cout << des << duration.count() << " ms" << std::endl;
}

int main() {
    omp_set_num_threads(THREADNUM);
    using highclock = std::chrono::high_resolution_clock;

    // -------------------cluster-------------------------
    std::cout << "[===start S5V cluster===]" << std::endl;
    auto startTime = highclock::now();
    auto ClusterStartTime = highclock::now();
    StreamCluster streamCluster;
    streamCluster.startStreamCluster();
    std::cout << "Big clustersize:" << streamCluster.getClusterPtrB()->size() << std::endl;
    std::cout << "Small clustersize:" << streamCluster.getClusterPtrS()->size()<< std::endl;
    streamCluster.computeHybridInfo();
    auto ClusterEndTime = highclock::now();
    
    printCostTime(ClusterEndTime,ClusterStartTime,"S5P stream Cluster Time: ");
    
    // -------------------partition-----------------------
    auto gameStartTime = highclock::now();
    std::cout << "[===start S5V Game===]" << std::endl;
    Partitioner partitioner(streamCluster);
    partitioner.startStackelbergGame();
    auto gameEndTime = highclock::now();
    printCostTime(gameEndTime,gameStartTime,"S5V Stackelber game time: ");
    
    
    // -------------------perform-----------------------
    std::cout << "[===start S5V perform===]" << std::endl;
    auto performStepStartTime = highclock::now();
    partitioner.performStep();
    auto performStepEndTime = highclock::now();
    printCostTime(performStepEndTime,performStepStartTime,"S5V perform time: ");
    auto endTime = highclock::now();
    
    // -------------------output-----------------------
    double rf = partitioner.getReplicateFactor();
    double lb = partitioner.getLoadBalance();
    int gameRoundCnt_hybrid = std::accumulate(
        partitioner.gameRoundCnt_hybrid.begin(),
        partitioner.gameRoundCnt_hybrid.end(),0);
    int gameRoundCnt_inner = std::accumulate(
        partitioner.gameRoundCnt_inner.begin(),
        partitioner.gameRoundCnt_inner.end(),0);
    int roundCnt = gameRoundCnt_hybrid + gameRoundCnt_inner;

    std::cout << "[===S5V-data===]" << std::endl;
    std::cout << "Partition num:" << config.partitionNum << std::endl;
    printCostTime(endTime,startTime,"Partition time: ");
    std::cout << "Relative balance load:" << lb << std::endl;
    std::cout << "Replicate factor: " << rf << std::endl;
    std::cout << "Total game round:" << roundCnt << std::endl;
    std::cout << "Total hybrid game round:" << gameRoundCnt_hybrid << std::endl;
    std::cout << "Total inner game round:" << gameRoundCnt_inner << std::endl;
    std::cout << "[===S5V-end===]" << std::endl;

    return 0;
}


