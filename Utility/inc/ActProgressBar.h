#ifndef ActProgressBar_h
#define ActProgressBar_h

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace ActRoot
{
class ProgressBar
{
public:
    class ThreadInfo
    {
    public:
        unsigned int fNEntries {};
        unsigned int fStep {};
        unsigned int fNRuns {};
    };
    class ThreadStatus
    {
    public:
        double fPercent {};
        unsigned int fCurrentEntry {};
        int fCurrentRun {};
        unsigned int fRunCount {};
    };

private:
    std::vector<ThreadInfo> fInfo {};
    std::vector<ThreadStatus> fStatus {};
    std::thread fCoutThread {};
    unsigned int fNThreads {};
    std::atomic<unsigned int> fCompletedThreads {};
    double fPercentUpdate {10};
    std::mutex fMutex {};
    std::condition_variable fCV {};
    bool fUpdated {false};

public:
    ProgressBar() = default;

    void SetNThreads(unsigned int nthreads);
    void Init();
    void SetThreadInfo(unsigned int thread, unsigned int nentries, int nruns);
    void SetThreadStatus(unsigned int thread, int entry, int nentries, int run, unsigned int count);
    void Display();
    void IncrementCompleted()
    {
        fCompletedThreads++;
        fCV.notify_one();
    }
    void Join();
};
} // namespace ActRoot

#endif
