# 流式图划分基线测试合集

本项目基于2ps的整合代码，原仓库地址：https://github.com/mayerrn/two_phase_streaming

本项目包含的流式代码如下:

\* Two-Phase Streaming(2ps): - Based on the following [publication](https://arxiv.org/pdf/2203.12721.pdf) : Mayer R, Orujzade K, Jacobsen H A. Out-of-core edge partitioning at linear run-time. ICDE 2022.

\* High Degrees Replicated First (HDRF) - Based on the following [publication](http://midlab.diag.uniroma1.it/articoli/PQDKI15CIKM.pdf): F. Petroni, L. Querzoni, G. Iacoboni, K. Daudjee and S. Kamali: "Hdrf: Efficient stream-based partitioning for power-law graphs". CIKM, 2015.

\* Degree-Based Hashing (DBH) - Based on the following [publication](http://papers.nips.cc/paper/5396-distributed-power-law-graph-computing-theoretical-and-empirical-analysis.pdf): C. Xie, L. Yan, W. Li and Z. Zhang: "Distributed Power-law Graph 

\* greedy  - Based on the following [publication](https://www.usenix.org/system/files/conference/osdi12/osdi12-final-167.pdf) :Gonzalez J E, Low Y, Gu H, et al. PowerGraph: Distributed Graph-Parallel computation on natural graphs. OSDI 12.

\* random

---

The programs require the below packages: `g++`, `cmake`, `glog`, `gflags`, `boost`:

```bash
sudo apt-get install libgoogle-glog-dev libgflags-dev libboost-all-dev cmake g++
```

