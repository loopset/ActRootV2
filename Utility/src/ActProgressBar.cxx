#include "ActProgressBar.h"

#include "ActColors.h"

#include <iomanip>
#include <iostream>
#include <thread>

void ActRoot::ProgressBar::SetNThreads(unsigned int nthreads)
{
    fNThreads = nthreads;
    fInfo.resize(nthreads);
    fStatus.resize(nthreads);
}

void ActRoot::ProgressBar::Init()
{
    std::cout.flush();
    for(int i = 0; i < fNThreads; i++)
        std::cout << i << std::endl;
    std::cout << std::flush; // flush always to ensure output is written to terminal
    // Init separate thread to monitor progress
    fCoutThread = std::thread(&ActRoot::ProgressBar::Display, this);
}

void ActRoot::ProgressBar::Join()
{
    fCoutThread.join();
}

void ActRoot::ProgressBar::SetThreadInfo(unsigned int thread, unsigned int nentries, int nruns)
{
    fInfo[thread].fNEntries = nentries;
    // Compute step to update run info
    fInfo[thread].fStep = nentries / (100. / fPercentUpdate);
    fInfo[thread].fNRuns = nruns;
    fStatus[thread] = {}; // reset current status
}

void ActRoot::ProgressBar::SetThreadStatus(unsigned int thread, int entry, int nentries, int run, unsigned int count)
{
    if(entry > fStatus[thread].fCurrentEntry)
    {
        fStatus[thread].fCurrentEntry += fInfo[thread].fStep;
        fStatus[thread].fPercent = (double)fStatus[thread].fCurrentEntry / fInfo[thread].fNEntries;
        fStatus[thread].fCurrentRun = run;
        fStatus[thread].fRunCount = count;
    }
}

void ActRoot::ProgressBar::Display()
{
    while(true)
    {
        // Print progress bars for each thread
        // 1-> Go up NThread lines
        std::cout << "\033[" << fNThreads << "A";
        for(int i = 0; i < fNThreads; i++)
        {
            // 2-> Clear line
            std::cout << "\033[K";
            int bar_width = 50;
            int pos = bar_width * fStatus[i].fPercent;
            std::cout << BOLDGREEN << std::setw(3) << i << " : Run " << fStatus[i].fCurrentRun << " [";
            for(int j = 0; j < bar_width; j++)
            {
                if(j < pos)
                    std::cout << "=";
                else if(j == pos)
                    std::cout << ">";
                else
                    std::cout << " ";
            }
            std::cout << "] " << fStatus[i].fRunCount << " of " << fInfo[i].fNRuns << RESET << std::endl;
            std::cout << std::flush;
        }

        // Check if all threads are done
        if(fCompletedThreads == fNThreads)
            break;

        // Printing interval
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Update interval
    }
}
