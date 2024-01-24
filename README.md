# 流式图划分基线测试合集

本项目基于2ps的整合代码，原仓库地址：https://github.com/mayerrn/two_phase_streaming

原项目中包含2ps,HDRF,DBH的实现，但是由于混杂在一起，进行单独基线测试时需要进行对应修改，并不方便，因此将代码进行重构，每个文件夹代表一个算法实现。

框架新实现算法:
+ greedy: 从PowerGraph框架中进行剥离
+ random: 实现随机划分，作为运行质量下限和运行时间上限
+ RMGP: 基于点划分进行的划分算法迁移
+ our: xxxx

ps:暂时未解封，xxxx无详细说明，2024.3月解封，并调整仓库结构

项目包含的流式代码如下:

\* Two-Phase Streaming(2ps): - Based on the following [publication](https://arxiv.org/pdf/2203.12721.pdf) : Mayer R, Orujzade K, Jacobsen H A. Out-of-core edge partitioning at linear run-time. ICDE 2022.

\* High Degrees Replicated First (HDRF) - Based on the following [publication](http://midlab.diag.uniroma1.it/articoli/PQDKI15CIKM.pdf): F. Petroni, L. Querzoni, G. Iacoboni, K. Daudjee and S. Kamali: "Hdrf: Efficient stream-based partitioning for power-law graphs". CIKM, 2015.

\* Degree-Based Hashing (DBH) - Based on the following [publication](http://papers.nips.cc/paper/5396-distributed-power-law-graph-computing-theoretical-and-empirical-analysis.pdf): C. Xie, L. Yan, W. Li and Z. Zhang: "Distributed Power-law Graph 

\* greedy  - Based on the following [publication](https://www.usenix.org/system/files/conference/osdi12/osdi12-final-167.pdf) :Gonzalez J E, Low Y, Gu H, et al. PowerGraph: Distributed Graph-Parallel computation on natural graphs. OSDI 12.

\* random , other , ours

---
## 下游测试任务选择

下游测试任务为PowerGraph，可以使用我们的重构框架来进行简单测试及分布式测试 : https://github.com/BearBiscuit05/PowerGraph_update

---
## 分区算法运行环境需求

The programs require the below packages: `g++`, `cmake`, `glog`, `gflags`, `boost`:

```bash
sudo apt-get install libgoogle-glog-dev libgflags-dev libboost-all-dev cmake g++
```

