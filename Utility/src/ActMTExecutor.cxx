#include "ActMTExecutor.h"

#include "ActColors.h"
#include "ActDetectorManager.h"
#include "ActInputData.h"
#include "ActOptions.h"
#include "ActOutputData.h"

#include "TFile.h"
#include "TROOT.h"
#include "TStopwatch.h"

#include "BS_thread_pool.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

ActRoot::MTExecutor::MTExecutor(int nthreads)
    : ftp(BS::thread_pool(nthreads)),
      fRunsPerThread(nthreads),
      fProgress(nthreads)
{
    // Mandatory to be very cautious with concurrency
    ROOT::EnableThreadSafety();
    ROOT::EnableImplicitMT(nthreads);
}

void ActRoot::MTExecutor::SetDataManager(DataManager* datman)
{
    // Assign
    fDatMan = datman;
    // Compute runs per worker
    ComputeRunsPerThread();
}

void ActRoot::MTExecutor::SetDetectorConfig(const std::string& detfile, const std::string& calfile)
{
    for(int thread = 0; thread < fRunsPerThread.size(); thread++)
    {
        if(IsThreadEmpty(thread)) // skip empty threads
            continue;             // so number of workers can be less than thread pool size
        fDetMans.push_back(DetectorManager {ActRoot::Options::GetInstance()->GetMode()});
        fDetMans.back().ReadDetectorFile(detfile, (thread == 0) ? true : false);
        fDetMans.back().ReadCalibrationsFile(calfile);
    }
    // Print
    std::cout << BOLDYELLOW << "Pool size : " << fRunsPerThread.size() << " but DetMan size : " << fDetMans.size()
              << RESET << '\n';
}

void ActRoot::MTExecutor::ComputeRunsPerThread()
{
    auto runs {fDatMan->GetRunList()};
    auto nthreads {static_cast<int>(ftp.get_thread_count())};
    int idx {};
    for(const auto& run : runs)
    {
        if(idx == nthreads)
            idx = 0;
        fRunsPerThread[idx].insert(run);
        idx++;
    }

    // Print
    std::cout << BOLDYELLOW << "----- MTExecutor -----" << '\n';
    for(int t = 0; t < fRunsPerThread.size(); t++)
    {
        if(IsThreadEmpty(t))
            continue;
        std::cout << "Thread " << t << '\n';
        for(auto& run : fRunsPerThread[t])
        {
            std::cout << "-> Run " << run << '\n';
        }
        std::cout << "------------------------" << '\n';
    }
    std::cout << RESET;
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
        ftpcout.print("\r", "                                             ",
                      BS::synced_stream::flush); // to avoid garbage in cout, since \r does not clean previous line
        ftpcout.print("\r", BOLDGREEN, "Run ", run, " -> ", std::string(percent / fPercentPrint, (style) ? '|' : '#'),
                      percent, "%", RESET, BS::synced_stream::flush);
        fProgress[thread].second += fProgress[thread].first;
        style = !style;
    }
}

void ActRoot::MTExecutor::BuildEvent()
{
    // Build lambda for each worker
    auto build = [this](unsigned int thread)
    {
        for(const auto& run : fRunsPerThread[thread])
        {
            // Init in/out data
            auto input {fDatMan->GetInputForThread({run})};
            auto output {fDatMan->GetOutputForThread({run})};
            // Send them to detectors
            fDetMans[thread].InitInput(input.GetTree(run));
            fDetMans[thread].InitOutput(output.GetTree(run));
            auto nentries {input.GetNEntries(run)};
            StepProgress(thread, nentries);
            // Run for each entry!
            for(int entry = 0; entry < nentries; entry++)
            {
                input.GetEntry(run, entry);
                fDetMans[thread].BuildEvent(run, entry);
                output.Fill(run);
                PrintProgress(thread, run, entry + 1, nentries);
            }
            output.Close(run);
            input.Close(run);
        }
    };
    // Add a global timer
    TStopwatch timer {};
    timer.Start();
    // Parallelize loop
    ftp.detach_sequence(0, (int)fDetMans.size(), build);
    // Wait for tasks to finish
    ftp.wait();
    // Finish execution by couting elapased time
    timer.Stop();
    std::cout << std::endl;
    timer.Print();
}
