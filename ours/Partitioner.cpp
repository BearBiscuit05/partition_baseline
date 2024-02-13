#include "Partitioner.h"


Partitioner::Partitioner() {}

Partitioner::Partitioner(StreamCluster& streamCluster, GlobalConfig config)
    : streamCluster(&streamCluster), config(config) {
    this->gameRoundCnt_hybrid = std::vector<int>(THREADNUM, 0);
    this->gameRoundCnt_inner = std::vector<int>(THREADNUM, 0);
    partitionLoad.resize(config.partitionNum);
    std::cout << config.vCount << std::endl;
    v2p.resize(config.vCount, std::vector<char>(config.partitionNum));
    clusterPartition.resize(2*config.vCount, 0);
   }

void Partitioner::performStep() {
    double maxLoad = config.eCount / config.partitionNum * config.alpha;
    std::string inputGraphPath = config.inputGraphPath;
    std::pair<int,int> edge(-1,-1);
    TGEngine tgEngine(inputGraphPath,config.vCount,config.eCount);
    int partPtr = 0;
    std::cout << maxLoad << std::endl;
    while (-1 != tgEngine.readline(edge)) {
        int src = edge.first;
        int dest = edge.second;
        if (this->streamCluster->isInB[tgEngine.readPtr/2]) {
            int srcPartition = clusterPartition[streamCluster->cluster_B[src]];
            int destPartition = clusterPartition[streamCluster->cluster_B[dest]];
            int edgePartition = -1;
            if (partitionLoad[srcPartition] > maxLoad && partitionLoad[destPartition] > maxLoad) {
                for (int i = 0; i < config.partitionNum; i++) {
                    if (partitionLoad[i] <= maxLoad) {
                        edgePartition = i;
                        srcPartition = i;
                        destPartition = i;
                        break;
                    }
                }
            } else if (partitionLoad[srcPartition] > partitionLoad[destPartition]) {
                edgePartition = destPartition;
                srcPartition = destPartition;
            } else {
                edgePartition = srcPartition;
                destPartition = srcPartition;
            }
            partitionLoad[edgePartition]++;
            v2p[src][srcPartition] = 1;
            v2p[dest][destPartition] = 1;
        } else {
            int srcPartition = clusterPartition[streamCluster->cluster_S[src]+config.vCount];
            int destPartition = clusterPartition[streamCluster->cluster_S[dest]+config.vCount];
            int edgePartition = -1;
            if (partitionLoad[srcPartition] > maxLoad && partitionLoad[destPartition] > maxLoad) {
                for (int i = config.partitionNum - 1; i >= 0; i--) {
                    if (partitionLoad[i] <= maxLoad) {
                        edgePartition = i;
                        srcPartition = i;
                        destPartition = i;
                        break;
                    }
                }
            } else if (partitionLoad[srcPartition] > partitionLoad[destPartition]) {
                edgePartition = destPartition;
                srcPartition = destPartition;
            } else {
                edgePartition = srcPartition;
                destPartition = srcPartition;
            }

            partitionLoad[edgePartition]++;
            v2p[src][srcPartition] = 1;
            v2p[dest][destPartition] = 1;            
        }
    }
}

double Partitioner::getReplicateFactor() {
    int replicateTotal = 0;
    for (int i = 0; i < config.vCount; i++) {
        for (int j = 0; j < config.partitionNum; j++) {
            replicateTotal += v2p[i][j];
        }
    }
    return static_cast<double>(replicateTotal) / config.vCount;
}

double Partitioner::getLoadBalance() {
    int maxLoad = 0;
    for (int i = 0; i < config.partitionNum; i++) {
        if (maxLoad < partitionLoad[i]) {
            maxLoad = partitionLoad[i];
        }
    }
    return static_cast<double>(config.partitionNum) / config.eCount * maxLoad;
}

void Partitioner::startStackelbergGame() {
    int batchSize = config.batchSize;
    std::vector<int> clusterList_B = streamCluster->getClusterList_B();
    std::vector<int> clusterList_S = streamCluster->getClusterList_S();
    int taskNum_B = (clusterList_B.size() + batchSize - 1) / batchSize;
    int taskNum_S = (clusterList_S.size() + batchSize - 1) / batchSize;
    int minTaskNUM = std::min(taskNum_B, taskNum_S);
    int leftTaskNUM = std::abs(taskNum_B - taskNum_S);
    std::vector<ClusterGameTask*> cgt_list(THREADNUM);
    for (int i = 0 ; i < THREADNUM ; i++) {
        ClusterGameTask* cgt = new ClusterGameTask((*streamCluster),this->clusterPartition);
        cgt_list[i] = cgt;
    }


#pragma omp parallel for
    for (int i = 0; i < minTaskNUM; i++) {
        int ompid = omp_get_thread_num();
        cgt_list[ompid]->resize_hyper("hybrid", i);
        cgt_list[ompid]->call();
        this->gameRoundCnt_hybrid[ompid] += cgt_list[ompid]->roundCnt;
    }

    if (taskNum_B > taskNum_S) {
#pragma omp parallel for  
        for (int i = 0 ; i < leftTaskNUM; ++i) {
            int ompid = omp_get_thread_num();
            cgt_list[ompid]->resize("B",i+minTaskNUM);
            cgt_list[ompid]->call();
            this->gameRoundCnt_inner[ompid] += cgt_list[ompid]->roundCnt;
        }
    } else {
#pragma omp parallel for  
        for (int i = 0 ; i < leftTaskNUM; ++i) {
            int ompid = omp_get_thread_num();
            cgt_list[ompid]->resize("S",i+minTaskNUM);
            cgt_list[ompid]->call();
            this->gameRoundCnt_inner[ompid] += cgt_list[ompid]->roundCnt;
        }
    }


}

