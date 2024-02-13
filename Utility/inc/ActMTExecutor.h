#ifndef ActMTExecutor_h
#define ActMTExecutor_h

#include "ActDataManager.h"
#include "ActDetectorManager.h"

#include "BS_thread_pool.h"
#include "BS_thread_pool_utils.h"

#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ActRoot
{
//! A class to perform multiple tasks in MT mode (experimental!)
class MTExecutor
{
private:
    // Cout
    BS::synced_stream ftpcout;
    // Thread pool
    BS::thread_pool ftp;
    // Pointer to DataManager
    DataManager* fDatMan {};
    // Vector of DetMan for workers
    std::vector<DetectorManager> fDetMans;
    // List of runs per worker
    std::vector<std::set<int>> fRunsPerThread;
    // For print progress, miscellanea
    std::vector<std::pair<double, double>> fProgress;
    double fPercentPrint {10};

public:
    MTExecutor(int nthreads = 1.5 * std::thread::hardware_concurrency());
    void SetDataManager(DataManager* datman);
    void SetDetectorConfig(const std::string& detfile, const std::string& calfile);
    void BuildEvent();

private:
    void ComputeRunsPerThread();
    bool IsThreadEmpty(int t);
    void StepProgress(int thread, double total);
    void PrintProgress(int thread, int run, double current, double total);
};
} // namespace ActRoot

#endif
