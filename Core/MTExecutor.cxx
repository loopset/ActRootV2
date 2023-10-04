#include "MTExecutor.h"

#include "BS_thread_pool.h"

#include "CalibrationManager.h"
#include "DetectorManager.h"
#include "InputData.h"
#include "OutputData.h"
#include "Colors.h"

#include "TROOT.h"
#include "TStopwatch.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

ActRoot::MTExecutor::MTExecutor(int nthreads)
    : ftp(BS::thread_pool(nthreads)),
      fDetMans(nthreads),
      fRunsPerThread(nthreads),
      fProgress(nthreads)
{
    //Mandatory to be very cautious with concurrency
    ROOT::EnableThreadSafety();
    ROOT::EnableImplicitMT(nthreads);
}

void ActRoot::MTExecutor::SetInputAndOutput(InputData *in, OutputData *out)
{
    //Assign
    fInput = in;
    fOutput = out;
    //Compute runs per worker
    ComputeRunsPerThread();
}

void ActRoot::MTExecutor::SetDetectorConfig(const std::string &detfile,
                                            const std::string& calfile)
{
    for(int d = 0; d < fDetMans.size(); d++)
    {
        if(IsThreadEmpty(d))//skip empty threads
            continue;
        fDetMans[d] = DetectorManager(detfile);
        fDetMans[d].ReadCalibrations(calfile);
    }
}

void ActRoot::MTExecutor::ComputeRunsPerThread()
{
    auto runs {fInput->GetTreeList()};
    auto nthreads {static_cast<int>(ftp.get_thread_count())};
    int idx {};
    for(const auto& run : runs)
    {
        if(idx == nthreads)
            idx = 0;
        fRunsPerThread[idx].push_back(run);
        idx++;
    }

    //Print
    std::cout<<"----- MTExecutor -----"<<'\n';
    for(int t = 0; t < fRunsPerThread.size(); t++)
    {
        std::cout<<"Thread "<<t<<'\n';
        for(auto& run : fRunsPerThread[t])
        {
            std::cout<<"-> Run "<<run<<'\n';
        }
        std::cout<<"------------------------"<<'\n';
    }
}

bool ActRoot::MTExecutor::IsThreadEmpty(int t)
{
    return fRunsPerThread.at(t).size() == 0;
}

void ActRoot::MTExecutor::StepProgress(int thread, double total)
{
    double step {total / (100. / fPercentPrint)};
    fProgress[thread] = {step, step};
}

void ActRoot::MTExecutor::PrintProgress(int thread, int run, double current, double total)
{
    static bool style {false};
    if(current >= fProgress[thread].second)
    {
        auto percent {static_cast<int>(100 * current / total)};
        ftpcout.print("\r", "                                             ", BS::synced_stream::flush);//to avoid garbage in cout, sinc \r does not clean previous line
        ftpcout.print("\r", BOLDGREEN, "Run ", run, " -> ", std::string(percent / fPercentPrint, (style) ? '|' : '#'), percent, "%", RESET, BS::synced_stream::flush);
        fProgress[thread].second += fProgress[thread].first;
        style = !style;
    }
}

void ActRoot::MTExecutor::BuildEventData()
{
    //Build lambda per worker
    auto build = [this](const int thread)
    {
        //ftpcout.println("Running thread ",thread);
        for(const auto& run : fRunsPerThread[thread])
        {
            //ftpcout.println("->Run ", run);
            fDetMans[thread].InitializeDataInputRaw(fInput->GetTree(run), run);
            fDetMans[thread].InitializeDataOutput(fOutput->GetTree(run));
            auto nentries {fInput->GetNEntries(run)};
            StepProgress(thread, nentries);
            //Run for each entry!
            for(int entry = 0; entry < nentries; entry++)
            {
                fInput->GetEntry(run, entry);
                fDetMans[thread].BuildEventData();
                fOutput->Fill(run);
                PrintProgress(thread, run, entry, nentries);
            }
        }
    };
    //And push to ThreadPool!
    TStopwatch timer {}; timer.Start();
    for(int thread = 0; thread < fDetMans.size(); thread++)
    {
        if(!IsThreadEmpty(thread))
            ftp.push_task(build, thread);
    }
    ftp.wait_for_tasks();
    timer.Stop();
    std::cout<<std::endl;
    timer.Print();
}
