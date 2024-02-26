#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include <fcntl.h>
#include <stdexcept>
#include <limits>
#include <memory>
#include "omp.h"


# define THREADNUM 16
# define BATCH 204800