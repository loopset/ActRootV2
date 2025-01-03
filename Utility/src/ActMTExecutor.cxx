#include "ActMTExecutor.h"

#include "ActColors.h"
#include "ActDetectorManager.h"
#include "ActInputData.h"
#include "ActOptions.h"
#include "ActOutputData.h"
#include "ActProgressBar.h"

#include "TFile.h"
#include "TROOT.h"
#include "TStopwatch.h"

#include "BS_thread_pool.h"

#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

ActRoot::MTExecutor::MTExecutor(int nthreads) : ftp(BS::thread_pool(nthreads)), fRunsPerThread(nthreads), fProgBar()
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
    // Init Progress bar (referred as monitor later)
    fProgBar.SetNThreads(fDetMans.size());
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
        int count {};
        int w {5};
        int s {1};
        int ncols {4};
        for(auto& run : fRunsPerThread[t])
        {
            std::cout << std::setw(w) << run << std::string(s, ' ');
            count++;
            // Display as columns
            if(count % ncols == 0)
                std::cout << '\n';
        }
        // Close last column
        if(count % ncols != 0)
            std::cout << '\n';
        std::cout << "------------------------" << '\n';
    }
    std::cout << RESET;
}

bool ActRoot::MTExecutor::IsThreadEmpty(int t)
{
    return fRunsPerThread.at(t).size() == 0;
}

void ActRoot::MTExecutor::BuildEvent()
{
    // Build lambda for each worker
    auto build = [this](unsigned int thread)
    {
        unsigned int count {1};
        for(const auto& run : fRunsPerThread[thread])
        {
            // Init in/out data
            auto input {fDatMan->GetInputForThread({run})};
            auto output {fDatMan->GetOutputForThread({run})};
            // Send them to detectors
            fDetMans[thread].InitInput(input.GetTree(run));
            fDetMans[thread].InitOutput(output.GetTree(run));
            auto nentries {input.GetNEntries(run)};
            fProgBar.SetThreadInfo(thread, nentries, fRunsPerThread[thread].size());
            // Run for each entry!
            for(int entry = 0; entry < nentries; entry++)
            {
                input.GetEntry(run, entry);
                fDetMans[thread].BuildEvent(run, entry);
                output.Fill(run);
                fProgBar.SetThreadStatus(thread, entry, nentries, run, count);
            }
            output.Close(run);
            input.Close(run);
            count++;
        }
        fProgBar.IncrementCompleted(); // increase inner atomic telling monitor that task ended
    };
    // Add a global timer
    TStopwatch timer {};
    timer.Start();
    // Start monitor
    fProgBar.Init();
    // Parallelize loop
    ftp.detach_sequence(0, (int)fDetMans.size(), build);
    // Wait for tasks to finish
    ftp.wait();
    // End monitor thread once tasks have finished
    fProgBar.Join();
    // Finish execution by couting elapased time
    timer.Stop();
    std::cout << std::endl;
    timer.Print();
}
