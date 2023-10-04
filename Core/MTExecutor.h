#ifndef ActMTExecutor_h
#define ActMTExecutor_h

#include "BS_thread_pool.h"

#include "ActDetectorManager.h"
#include "ActInputData.h"
#include "OutputData.h"

#include "TROOT.h"
#include "TStopwatch.h"

#include <iomanip>
#include <iostream>
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
        //Cout
        BS::synced_stream ftpcout;
        //Thread pool
        BS::thread_pool ftp;
        //Input data
        InputData* fInput;
        //Output data
        OutputData* fOutput;
        //Vector of DetMan for workers
        std::vector<DetectorManager> fDetMans;
        //List of runs per worker
        std::vector<std::vector<int>> fRunsPerThread;
        //For print progress, miscellanea
        std::vector<std::pair<double, double>> fProgress;
        double fPercentPrint {10};

    public:
        MTExecutor(int nthreads = std::thread::hardware_concurrency());
        void SetInputAndOutput(InputData* in, OutputData* out);
        void SetDetectorConfig(const std::string& detfile,
                               const std::string& calfile);
        void BuildEventData();
    private:
        void ComputeRunsPerThread();
        bool IsThreadEmpty(int t);
        void StepProgress(int thread, double total);
        void PrintProgress(int thread, int run, double current, double total);
    };
}

#endif
