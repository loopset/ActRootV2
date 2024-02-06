#include "ActDetectorManager.h"
#include "ActEventPainter.h"
#include "ActHistogramPainter.h"
#include "ActInputIterator.h"
#include "ActOptions.h"
#include "ActTypes.h"

#include "TGClient.h"
#include "TRint.h"

#include <exception>

int main(int argc, char* argv[])
{
    auto opts {ActRoot::Options::GetInstance(argc, argv)};
    opts->SetMode(ActRoot::ModeType::EVisual);
    opts->SetDetFile("e796.detector");
    opts->SetInputFile("filter.runs");
    ActRoot::Options::GetInstance()->Print();

    TRint app {"actplot", &argc, argv, nullptr, 0, true};

    try
    {
        // Init input wrapper
        ActRoot::InputWrapper in {opts->GetInputFile(), false};

        // Detector manager
        ActRoot::DetectorManager detman {ActRoot::Options::GetInstance()->GetMode()};
        detman.ReadDetectorFile(opts->GetDetFile());
        detman.SendWrapperData(&in);

        ActRoot::HistogramPainter server {};
        server.SendInputWrapper(&in);
        server.SendParameters(&detman);

        ActRoot::EventPainter painter {gClient->GetRoot(), 800, 600};
        painter.SendDetectorManager(&detman);
        painter.SendInputWrapper(&in);
        painter.SendHistogramServer(&server);

        server.SendCanvas(painter.GetCanvasPtr());


        app.Run(true);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << '\n';
        return 1;
    }
    return 0;
}
