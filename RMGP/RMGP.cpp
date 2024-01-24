#include <string>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <limits>
#include <iostream>

int countCVP(
        int nodeId,
        int pid,
        std::unordered_map<int, std::vector<int>> &csr,
        std::unordered_map<int, std::vector<int>> &csrEid
) {
    int res = 0;
    auto nodeList = csr[nodeId];
    for (auto &neig : nodeList) {
        auto neigEid = csrEid[neig];
        // find in pid edge num
        for (auto &eid : neigEid) {
            if (eid == pid)
                res++;
        }
    }
    return res;
}

void RMGP(
        int NUM_NODES, int NUM_EDGES, int NUM_PARTITIONS,
        std::vector<int> &edge2pid, std::vector<std::pair<int, int>> &edges,
        std::unordered_map<int, std::vector<int>> &csr,
        std::unordered_map<int, std::vector<int>> &csrEid
) {
    ///////self define
    float aplhe = 0.3;
    ///////self define

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, NUM_PARTITIONS - 1);

    std::vector<float> edgeSC(NUM_EDGES, -1);

    for (int i = 0; i < NUM_EDGES; i++) { //line 1
        int p_id = dist(gen);
        auto edge = edges[i];
        edge2pid[i] = p_id;                                          // line 2
        edgeSC[i] = (1 - aplhe) * ((csr[edge.first].size() + csr[edge.second].size()) / 2.0f) * 1.0f; // line3
    }

    bool flag = true;
    int loop = 0;
    std::vector<float> costEdge(NUM_PARTITIONS, -1);
    while (flag) //line 4
    {
        flag = false;
        for (int i = 0; i < NUM_EDGES; i++) { // line 5
            auto &edge = edges[i];
            float minCost = std::numeric_limits<float>::max(); // line 6
            for (int pid = 0; pid < NUM_PARTITIONS; pid++) {    //line 7
                int c_VP = countCVP(edge.first, pid, csr, csrEid);
                costEdge[pid] = aplhe * c_VP * 1.0f + edgeSC[i]; //line 8
            }

            for (int eindex = 0; eindex < int(csr[edge.first].size()); eindex++) { // line 9
                int eid = csrEid[edge.first][eindex];
                int sf = edge2pid[eid];
                costEdge[sf] -= (1 - aplhe) * 0.5f; // line 10
            }

            for (int eindex = 0; eindex < int(csr[edge.second].size()); eindex++) { // line 9
                int eid = csrEid[edge.second][eindex];
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
            if (edge2pid[i] != newpid)
            {
                edge2pid[i] = newpid;
                flag = true; // line 14
            }
        }

        if (loop > 100)
        {
            std::cout << "count for more than 100 loop , exit..." << std::endl;
            return;
        }
        loop++;
    }
}

int main()
{
    ///////self define
    int NUM_EDGES = 34681189;
    int NUM_NODES = 3997962;
    int NUM_PARTITIONS = 32;
    std::string binfilePath = "./test.bin";
    ///////self define

    std::vector<std::pair<int, int>> edges;

    std::unordered_map<int, std::vector<int>> csr;
    std::unordered_map<int, std::vector<int>> csrEid;
    std::ifstream fin(binfilePath, std::ios::binary);

    std::vector<int> tmp;
    for (int i = 0; i < NUM_NODES; i++)
    {
        csr[i] = tmp;
        csrEid[i] = tmp;
    }

    edges.resize(NUM_EDGES);
    fin.read((char *)&edges[0], sizeof(std::pair<int, int>) * NUM_EDGES);
    int count = 0;
    for (auto edge : edges)
    {
        csr[edge.first].emplace_back(edge.second);
        csrEid[edge.first].emplace_back(count);
        count++;
    }
    std::vector<int> edge2pid(NUM_EDGES, -1);
    std::cout << "begin RMGP algorithm..." << std::endl;
    RMGP(NUM_NODES, NUM_EDGES, NUM_PARTITIONS, edge2pid, edges, csr, csrEid);
    std::cout << "end RMGP algorithm..." << std::endl;
    return 0;
}
