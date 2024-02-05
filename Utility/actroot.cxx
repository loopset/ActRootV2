#include "ActDetectorManager.h"
#include "ActInputData.h"
#include "ActMTExecutor.h"
#include "ActOptions.h"
#include "ActOutputData.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        auto opts {ActRoot::Options::GetInstance(argc, argv)};
        opts->Print();

        // Init input data
        ActRoot::InputData input {opts->GetRunFile()};
        ActRoot::OutputData output {input};
        output.ReadConfiguration(opts->GetRunFile());

        if(opts->GetIsMT())
        {
            ActRoot::MTExecutor mt;
            mt.SetInputAndOutput(&input, &output);
            mt.SetDetectorConfig(opts->GetDetFile(), opts->GetCalFile());
            mt.BuildEvent();
        }
        else // ST mode
        {
            ActRoot::DetectorManager detman {opts->GetMode()};
            detman.ReadDetectorFile(opts->GetDetFile());
            detman.ReadCalibrationsFile(opts->GetCalFile());

            TStopwatch timer {};
            timer.Start();
            for(const auto& run : input.GetTreeList())
            {
                std::cout << "Building event data for run " << run << '\n';
                detman.InitInput(input.GetTree(run));
                detman.InitOutput(output.GetTree(run));
                for(int entry = 0; entry < input.GetTree(run)->GetEntries(); entry++)
                {
                    std::cout << "\r" << entry << std::flush;
                    input.GetEntry(run, entry);
                    detman.BuildEvent();
                    output.Fill(run);
                }
                output.Close(run);
                input.Close(run);
                std::cout << '\n' << "->Processed events = " << output.GetTree(run)->GetEntries() << '\n';
            }
            detman.PrintReports();
            timer.Stop();
            timer.Print();
        }
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << '\n';
        return 1;
    }
    return 0;
}
