#include "Partitioner.h"


// init global config set
GlobalConfig config("./project.properties");

int main() {
    omp_set_num_threads(THREADNUM);
    //GlobalConfig configInfo("./project.properties");
    using highclock = std::chrono::high_resolution_clock;

    // -------------------cluster-------------------------
    std::cout << "[===start S5V cluster===]" << std::endl;
    auto startTime = highclock::now();
    auto ClusterStartTime = highclock::now();
    StreamCluster streamCluster;
    auto InitialClusteringTime = highclock::now();
    
    streamCluster.startStreamCluster();
    std::cout << "Big clustersize:" << streamCluster.getClusterList_B().size() << std::endl;
    std::cout << "Small clustersize:" << streamCluster.getClusterList_S().size()<< std::endl;
    auto ClusteringTime = highclock::now();
    std::cout << "End Clustering" << std::endl;
    streamCluster.computeHybridInfo();
    std::cout << "partitioner config:" << config.batchSize << std::endl;
    auto ClusterEndTime = highclock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(InitialClusteringTime - ClusterStartTime);
    std::cout << "Initial Clustering time: " << duration.count() << " ms" << std::endl;
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(ClusteringTime - InitialClusteringTime);
    std::cout << "Clustering Core time: " << duration.count() << " ms" << std::endl;
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(ClusterEndTime - ClusteringTime);
    std::cout << "ComputeHybridInfo time: " << duration.count() << " ms" << std::endl;
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(ClusterEndTime - ClusterStartTime);
    std::cout << "-----> ALL Cluster Time: " << duration.count() << " ms" << std::endl;
    
    // -------------------partition-----------------------
    auto gameStartTime = highclock::now();
    std::cout << "[===start S5V Game===]" << std::endl;
    Partitioner partitioner(streamCluster);
    auto PartitionerInitEndTime = highclock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(PartitionerInitEndTime - gameStartTime);
    std::cout << "Partitioner init time: " << duration.count() << " ms" << std::endl;
    
    partitioner.startStackelbergGame();
    auto gameEndTime = highclock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(gameEndTime - gameStartTime);
    std::cout << "-----> S5V game time: " << duration.count() << " ms" << std::endl;
    
    int gameRoundCnt_hybrid = 0;
    for(auto & t : partitioner.gameRoundCnt_hybrid) {
        gameRoundCnt_hybrid  += t;
    }
    int gameRoundCnt_inner = 0;
    for(auto & t : partitioner.gameRoundCnt_inner) {
        gameRoundCnt_inner  += t;
    }

    int roundCnt = gameRoundCnt_hybrid + gameRoundCnt_inner;
    std::cout << "Total game round:" << roundCnt << std::endl;
    std::cout << "Total hybrid game round:" << gameRoundCnt_hybrid << std::endl;
    std::cout << "Total inner game round:" << gameRoundCnt_inner << std::endl;
    // -------------------perform-----------------------
    std::cout << "[===start S5V perform===]" << std::endl;
    auto performStepStartTime = highclock::now();
    partitioner.performStep();
    auto performStepEndTime = highclock::now();

    duration = std::chrono::duration_cast<std::chrono::milliseconds>(performStepEndTime - performStepStartTime);
    std::cout << "-----> S5V perform time:: " << duration.count() << " ms" << std::endl;

    auto endTime = highclock::now();
    
    // -------------------output-----------------------
    double rf = partitioner.getReplicateFactor();
    double lb = partitioner.getLoadBalance();
    

    
    std::cout << "[===S5V-data===]" << std::endl;
    std::cout << "Partition num:" << config.partitionNum << std::endl;
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Partition time: " << duration.count() << " ms" << std::endl;
    std::cout << "Relative balance load:" << lb << std::endl;
    std::cout << "Replicate factor: " << rf << std::endl;
    std::cout << "Total game round:" << roundCnt << std::endl;
    std::cout << "Total hybrid game round:" << gameRoundCnt_hybrid << std::endl;
    std::cout << "Total inner game round:" << gameRoundCnt_inner << std::endl;
    std::cout << "[===S5V-end===]" << std::endl;

    return 0;
}


