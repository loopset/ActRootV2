#include "ActDataManager.h"
#include "ActDetectorManager.h"
#include "ActInputData.h"
#include "ActMTExecutor.h"
#include "ActOptions.h"
#include "ActOutputData.h"
#include "ActTypes.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        auto opts {ActRoot::Options::GetInstance(argc, argv)};
        opts->Print();
        if(opts->GetMode() == ActRoot::ModeType::ENone)
            return 0;
        if(opts->GetMode() == ActRoot::ModeType::EGui)
        {
            std::cout << "actroot cannot run in Gui mode! Use actplot executable for that" << '\n';
            return 0;
        }

        // Manage data
        ActRoot::DataManager datman {opts->GetMode()};
        datman.ReadDataFile(opts->GetDataFile());

        if(opts->GetIsMT())
        {
            ActRoot::MTExecutor mt;
            mt.SetDataManager(&datman);
            mt.SetDetectorConfig(opts->GetDetFile(), opts->GetCalFile());
            mt.BuildEvent();
        }
        else // ST mode
        {
            // Init input and output
            auto input {datman.GetInput()};
            auto output {datman.GetOuput()};

            ActRoot::DetectorManager detman {opts->GetMode()};
            detman.ReadDetectorFile(opts->GetDetFile());
            detman.ReadCalibrationsFile(opts->GetCalFile());

            TStopwatch timer {};
            timer.Start();
            for(const auto& run : input.GetRunList())
            {
                std::cout << "Building event data for run " << run << '\n';
                detman.InitInput(input.GetTree(run));
                detman.InitOutput(output.GetTree(run));
                int nentries {input.GetNEntries(run)};
                for(int entry = 0; entry < nentries; entry++)
                {
                    std::cout << "\r" << entry << std::flush;
                    input.GetEntry(run, entry);
                    detman.BuildEvent(run, entry);
                    output.Fill(run);
                }
                output.Close(run);
                input.Close(run);
                std::cout << '\n' << "->Processed events = " << nentries << '\n';
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
