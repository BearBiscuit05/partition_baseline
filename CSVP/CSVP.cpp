#include <string>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <limits>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include <stack>

void RFcount(int nodeNUM, int partNUM,std::vector<int> &edge2pid,std::vector<std::pair<int, int>> &edges) {
    std::vector<std::vector<int>> nodeCount(nodeNUM,std::vector<int>(partNUM,0));

    for(int i = 0; i< edge2pid.size() ; i++) {
        int pid = edge2pid[i];
        nodeCount[edges[i].first][pid] = 1;
        nodeCount[edges[i].second][pid] = 1;
    }

    float sum_result = std::accumulate(nodeCount.begin(), nodeCount.end(), 0,
                                     [](int acc, const std::vector<int>& row) {
                                         return acc + std::accumulate(row.begin(), row.end(), 0);
                                     });
    std::cout << "SUM :" << sum_result << std::endl;
    std::cout << "nodeNUM :" << nodeNUM << std::endl;
    float RF = sum_result * 1.0f / nodeNUM ;
    std::cout << "RF :" << RF << std::endl;
}

int DFS(
        int node, std::unordered_map<int, std::vector<int>> &csr,
        std::vector<int>& visited,int res) {
    std::stack<int> stack;
    stack.push(node);
    visited[node] = node;
    res = 1;
    while (!stack.empty()) {
        int current = stack.top();
        stack.pop();
        for (int neighbor : csr[current]) {
            res += 1;
            if (visited[neighbor] > node) {
                stack.push(neighbor);
                visited[neighbor] = node;
            }
        }
    }
    return res;
}

void DFS_ex(
        int node,int partid,std::vector<int> &edge2pid,
        std::vector<std::pair<int, int>> &edges,
        std::unordered_map<int, std::vector<int>> &csr,
        std::unordered_map<int, std::vector<long long>> &csrEid,
        std::vector<int>& visited,std::vector<std::vector<int>> &nodeStatus,
        long long res) {
    std::stack<int> stack;
    stack.push(node);
    while (!stack.empty()) {
        int current = stack.top();
        stack.pop();
        for (int i = 0 ; i < csr[current].size();i++) {
            int neighbor = csr[current][i];
            if (visited[neighbor] == node) {
                stack.push(neighbor);
                long long eid = csrEid[current][i];
                edge2pid[eid] = partid;
                res--;
                if(res == 0)
                    return;
            }
        }
    }
}

void RMGP(
        int NUM_NODES, long long NUM_EDGES,long long beginIdx ,int NUM_PARTITIONS,
        std::vector<int> &edge2pid, std::vector<std::pair<int, int>> &edges,
        std::unordered_map<int, std::vector<int>> &csr,
        std::unordered_map<int, std::vector<long long>> &csrEid,
        std::vector<std::vector<int>> &nodeStatus
) {
    ///////self define
    float aplhe = 0.5;
    ///////self define

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, NUM_PARTITIONS - 1);

    std::vector<float> edgeSC(NUM_EDGES, -1);

    for (long long i = beginIdx; i < NUM_EDGES; i++) { //line 1
        auto edge = edges[i];
        if (edge.first == -1)
            continue;

        int p_id = dist(gen);

        edge2pid[i] = p_id;                                          // line 2
        edgeSC[i] = (1 - aplhe) * ((csr[edge.first].size() + csr[edge.second].size()) / 2.0f) * 1.0f; // line3
        nodeStatus[edge.first][p_id] += 1;
        nodeStatus[edge.second][p_id] += 1;
    }

    bool flag = true;
    int loop = 0;
    int coreNUM = omp_get_num_procs();
    while (flag) //line 4
    {
        auto start = std::chrono::high_resolution_clock::now();
        flag = false;
        omp_set_num_threads(coreNUM*2-1);
    #pragma omp parallel for
        for (long long i = beginIdx; i < NUM_EDGES; i++) { // line 5
            auto &edge = edges[i];
            if (edge2pid[i] >= NUM_PARTITIONS)
                continue;
            std::vector<float> costEdge(NUM_PARTITIONS, -1);
            float minCost = std::numeric_limits<float>::max(); // line 6
            for (int pid = 0; pid < NUM_PARTITIONS; pid++) {    //line 7
                int c_VP = nodeStatus[edge.first][pid];
                c_VP += nodeStatus[edge.second][pid];
                costEdge[pid] = aplhe * c_VP * 1.0f + edgeSC[i]; //line 8
            }

            for (long long eindex = 0; eindex < (long long)(csr[edge.first].size()); eindex++) { // line 9
                long long eid = csrEid[edge.first][eindex];
                int sf = edge2pid[eid];
                costEdge[sf] -= (1 - aplhe) * 0.5f; // line 10
            }

            for (long long eindex = 0; eindex < (long long)(csr[edge.second].size()); eindex++) { // line 9
                long long eid = csrEid[edge.second][eindex];
                int sf = edge2pid[eid];
                costEdge[sf] -= (1 - aplhe) * 0.5f; // line 10
            }

            int newpid = -1;
            for (int pid = 0; pid < NUM_PARTITIONS; pid++) { // line 11
                if (costEdge[pid] < minCost) {               // line12
                    minCost = costEdge[pid];
                    newpid = pid; // line13
                }
            }
            #pragma omp critical
            if (edge2pid[i] != newpid)
            {
                edge2pid[i] = newpid;
                flag = true; // line 14
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "Time taken by function: " << duration.count() << " seconds" << std::endl;

        return ;
    }
}

void findConnectedComponents(
        int NUM_NODES, long long NUM_EDGES,int NUM_PARTITIONS,std::vector<int> &edge2pid,
        std::vector<std::pair<int, int>> &edges,std::unordered_map<int, std::vector<int>> &csr,
        std::unordered_map<int, std::vector<long long>> &csrEid,std::vector<std::vector<int>> &nodeStatus
) {
    std::vector<int> visited(NUM_NODES, 0);
    for (int i = 0 ; i < NUM_NODES; i++)
        visited[i] = i;
    int res = 0;
    int fatherId = 0,bound = 0;
    for (int i = 0; i < NUM_NODES; ++i) {
        if (visited[i] == i) {
            bound = DFS(i, csr, visited,res) ;
        }
        if (bound > res) {
            bound = res;
            fatherId = i;
        }
    }

    // father id使用
    DFS_ex(fatherId,NUM_PARTITIONS-2,edge2pid,edges,csr,csrEid,visited,nodeStatus,NUM_EDGES/NUM_PARTITIONS);

}



void CVSP_pre(
        int NUM_NODES, long long NUM_EDGES, int NUM_PARTITIONS,
        std::vector<int> &edge2pid, std::vector<std::pair<int, int>> &edges,
        std::unordered_map<int, std::vector<int>> &csr,
        std::unordered_map<int, std::vector<long long>> &csrEid,
        std::vector<std::vector<int>> &nodeStatus
) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, NUM_PARTITIONS - 1);
    // std::vector<int> indices(NUM_EDGES,0);
    // for (int i = 0 ; i < NUM_EDGES; i++)
    //     indices[i] = i;
    std::cout << "begin shuffle algorithm..." << std::endl;
    std::shuffle(edges.begin(), edges.end(), gen);
    long long beginIdx = NUM_EDGES / NUM_PARTITIONS;

    for (long long i = 0 ; i < beginIdx ; i++) {
        edge2pid[i] = NUM_PARTITIONS-1;
    }

    long long count = beginIdx;
    for (long long i = beginIdx ; i < NUM_EDGES ; i++)
    {
        auto edge = edges[i];
        csr[edge.first].emplace_back(edge.second);
        csrEid[edge.first].emplace_back(count);
        count++;
    }
    std::cout << "begin WCC algorithm..." << std::endl;
    findConnectedComponents(NUM_NODES,NUM_EDGES,NUM_PARTITIONS,edge2pid,edges,csr,csrEid,nodeStatus);

    csr.clear();
    csrEid.clear();
    count = beginIdx;
    for (long long i = beginIdx ; i < NUM_EDGES ; i++)
    {
        auto edge = edges[i];
        if (edge.first != -1) {
            csr[edge.first].emplace_back(edge.second);
            csrEid[edge.first].emplace_back(count);
        }
        count++;
    }
    std::cout << "begin RMGP algorithm..." << std::endl;
    RMGP(NUM_NODES,NUM_EDGES,beginIdx,NUM_PARTITIONS-2,edge2pid,edges,csr,csrEid,nodeStatus);
}

int main(int argc, char *argv[]) {
   if (argc != 5) {
       std::cerr << "Usage: " << argv[0] << " <NUM_EDGES> <NUM_NODES> <NUM_PARTITIONS> <binfilePath>\n";
       return 1;
   }

    // int NUM_NODES = 10;
    // long long NUM_EDGES = 12;
    // int NUM_PARTITIONS = 4;
    // std::string binfilePath = "./test.bin";
    int NUM_NODES = std::stoi(argv[1]);
    // int NUM_EDGES = std::stoi(argv[2]);
    long long NUM_EDGES = std::stoll(argv[2]);
    int NUM_PARTITIONS = std::stoi(argv[3]);
    std::string binfilePath = argv[4];

    std::vector<std::pair<int, int>> edges;

    std::unordered_map<int, std::vector<int>> csr;
    std::unordered_map<int, std::vector<long long>> csrEid;
    std::ifstream fin(binfilePath, std::ios::binary);
    std::vector<int> tmp;
    std::vector<long long> lltmp;
    for (int i = 0; i < NUM_NODES; i++)
    {
        csr[i] = tmp;
        csrEid[i] = lltmp;
    }

    edges.resize(NUM_EDGES);
    fin.read((char *)&edges[0], sizeof(std::pair<int, int>) * NUM_EDGES);
    long long count = 0;

    std::vector<std::vector<int>> nodeStatus(NUM_NODES,std::vector<int>(NUM_PARTITIONS,0));

    std::vector<int> edge2pid(NUM_EDGES, -1);
    //std::cout << "begin CVSP algorithm..." << std::endl;
    CVSP_pre(NUM_NODES, NUM_EDGES, NUM_PARTITIONS, edge2pid, edges, csr, csrEid,nodeStatus);
    RFcount(NUM_NODES,NUM_PARTITIONS,edge2pid,edges);
    //std::cout << "end CVSP algorithm..." << std::endl;
    return 0;
}
