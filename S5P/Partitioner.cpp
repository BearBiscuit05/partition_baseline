#include "Partitioner.h"


Partitioner::Partitioner() {}

Partitioner::Partitioner(StreamCluster& streamCluster)
    : streamCluster(&streamCluster){
    this->gameRoundCnt_hybrid.resize(THREADNUM, 0);
    this->gameRoundCnt_inner.resize(THREADNUM, 0);
    this->partitionLoad.resize(config.partitionNum);
    this->v2p.resize(config.vCount, std::vector<bool>(config.partitionNum));
    this->clusterPartition.resize(2*config.vCount, 0);
   }

void Partitioner::performStep() {
    /*
        Assign each edge to the corresponding partition
    */
    double maxLoad = config.eCount / config.partitionNum * config.alpha;
    std::pair<int,int> edge(-1,-1);
    TGEngine tgEngine(config.inputGraphPath,config.vCount,config.eCount);
    while (-1 != tgEngine.readline(edge)) {
        int src = edge.first;
        int dest = edge.second;
        int srcPart,destPart,edgePart=-1;
        if (this->streamCluster->isInB[tgEngine.readPtr/2]) {
            srcPart = clusterPartition[streamCluster->clusterB[src]];
            destPart = clusterPartition[streamCluster->clusterB[dest]];
            // both exceed then random find one part
            if (partitionLoad[srcPart] > maxLoad && partitionLoad[destPart] > maxLoad) {
                for (int i = 0; i < config.partitionNum; i++) {
                    if (partitionLoad[i] <= maxLoad) {
                        edgePart = i;
                        srcPart = i;
                        destPart = i;
                        break;
                    }
                }
            } else if (partitionLoad[srcPart] > partitionLoad[destPart]) { // set dest
                edgePart = destPart;
                srcPart = destPart;
            } else {
                edgePart = srcPart;
                destPart = srcPart;
            }
        } else {
            srcPart = clusterPartition[streamCluster->clusterS[src]+config.vCount];
            destPart = clusterPartition[streamCluster->clusterS[dest]+config.vCount];
            if (partitionLoad[srcPart] > maxLoad && partitionLoad[destPart] > maxLoad) {
                for (int i = config.partitionNum - 1; i >= 0; i--) {
                    if (partitionLoad[i] <= maxLoad) {
                        edgePart = i;
                        srcPart = i;
                        destPart = i;
                        break;
                    }
                }
            } else if (partitionLoad[srcPart] > partitionLoad[destPart]) {
                edgePart = destPart;
                srcPart = destPart;
            } else {
                edgePart = srcPart;
                destPart = srcPart;
            }      
        }
        partitionLoad[edgePart]++;
        v2p[src][srcPart] = 1;
        v2p[dest][destPart] = 1;
    }
}

double Partitioner::getReplicateFactor() {
    double replicateTotal = 0;
    for (int i = 0; i < config.vCount; i++) {
        for (int j = 0; j < config.partitionNum; j++) {
            replicateTotal += v2p[i][j];
        }
    }
    return replicateTotal / config.vCount;
}

double Partitioner::getLoadBalance() {
    auto maxIter = std::max_element(partitionLoad.begin(), partitionLoad.end());
    int maxLoad = *maxIter; 
    return static_cast<double>(config.partitionNum) / config.eCount * maxLoad;
}

void Partitioner::startStackelbergGame() {
    int batchSize = config.batchSize;
    auto clusterList_B = streamCluster->getClusterPtrB();
    auto clusterList_S = streamCluster->getClusterPtrS();
    int taskNumB = (clusterList_B->size() + batchSize - 1) / batchSize;
    int taskNumS = (clusterList_S->size() + batchSize - 1) / batchSize;
    int minTaskNUM = std::min(taskNumB, taskNumS);
    int leftTaskNUM = std::abs(taskNumB - taskNumS);
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

    if (taskNumB > taskNumS) {
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

