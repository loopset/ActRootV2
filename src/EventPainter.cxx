#include "EventPainter.h"

#include "GuiTypes.h"
#include "InputParser.h"
#include "TCanvas.h"
#include "TGFrame.h"
#include "TGLayout.h"
#include "TGStatusBar.h"
#include "TGWindow.h"
#include "TH2.h"
#include "TPCData.h"
#include "TRootEmbeddedCanvas.h"
#include "TApplication.h"
#include "TGText.h"
#include "TGButton.h"

#include <iostream>
#include <memory>
#include <string>

ActRoot::EventPainter::~EventPainter()
{
    Cleanup();
    delete fEmCanv;
}

void ActRoot::EventPainter::SetInputData(ActRoot::InputData *input)
{
    fInput = input;
    fCurrentRun = fInput->GetTreeList().front();
    fCurrentEntry = 0;
    fData = new TPCData;
    fInput->GetTree(fCurrentRun)->SetBranchAddress("data", &fData);
    fInput->GetEntry(fCurrentRun, fCurrentEntry);
    DoReset();
    DoFill();
    DoDraw();
}
void ActRoot::EventPainter::CloseWindow()
{
    DoExit();
}

void ActRoot::EventPainter::DoDraw()
{
    for(auto& [idx, h] : fHist2d)
    {
        fCanv->cd(idx);
        h->Draw("colz");
    }
    fCanv->Update();
}

void ActRoot::EventPainter::DoReset()
{
    ResetCanvas();
    ResetHistograms();
}

void ActRoot::EventPainter::DoFill()
{
    for(auto& voxel : fData->fVoxels)
    {
        //Pad
        fHist2d[1]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Y());
        //Side
        fHist2d[2]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Z());
        //Front
        fHist2d[3]->Fill(voxel.GetPosition().Y(), voxel.GetPosition().Z());
    }
}
void ActRoot::EventPainter::DoNextEvent()
{
    DoReset();
    fCurrentEntry++;
    fInput->GetEntry(fCurrentRun, fCurrentEntry);
    std::cout<<"Entries in run "<<fInput->GetTree(fCurrentRun)->GetEntries()<<'\n';
    DoFill();
    DoDraw();
}

void ActRoot::EventPainter::DoExit()
{
    std::cout<<"Exiting ActRoot::EventPainter..."<<'\n';
    gApplication->Terminate();
}


void ActRoot::EventPainter::ReadConfiguration(const std::string &file)
{
    ActRoot::InputParser parser {file};
    auto config {parser.GetBlock("Actar")};
    ftpc = TPCParameters(config->GetString("Type"));
    if(config->CheckTokenExists("RebinZ", true))
        ftpc.SetREBINZ(config->GetInt("RebinZ"));
}

void ActRoot::EventPainter::InitCanvas()
{
    fCanv = new TCanvas("cPainter", 10, 10, fEmCanv->GetCanvasWindowId());
    fCanv->Divide(3, 2);
}

void ActRoot::EventPainter::Init2DHistograms()
{
    //Pad
    fHist2d[1] = std::make_shared<TH2F>("hPad", "Pad;X [pad];Y [pad]",
                                            ftpc.GetNPADSX(), 0, ftpc.GetNPADSX(),
                                            ftpc.GetNPADSY(), 0, ftpc.GetNPADSY());
    //Side
    fHist2d[2] = std::make_shared<TH2F>("hSide", "Side;X [pad];Z [tb]",
                                             ftpc.GetNPADSX(), 0, ftpc.GetNPADSX(),
                                             ftpc.GetNPADSZ() / ftpc.GetREBINZ(), 0, ftpc.GetNPADSZ());
    //Front
    fHist2d[3] = std::make_shared<TH2F>("hFront", "Front;Y [pad];Z [tb]",
                                              ftpc.GetNPADSY(), 0, ftpc.GetNPADSY(),
                                              ftpc.GetNPADSZ() / ftpc.GetREBINZ(), 0 , ftpc.GetNPADSZ());
}

void ActRoot::EventPainter::ResetHistograms()
{
    //2D
    for(auto& [key, h] : fHist2d)
        h->Reset();
}

void ActRoot::EventPainter::ResetCanvas()
{
    fCanv->Clear("D");
    fCanv->Update();
}

ActRoot::EventPainter::EventPainter(const std::string& file, const TGWindow* window, unsigned int width, unsigned int height)
    : TGMainFrame(window, width, height)
{
    ReadConfiguration(file);
    //Embebbed canvas
    fEmCanv = new TRootEmbeddedCanvas(0, this, 500, 400);
    InitCanvas();
    fEmCanv->AdoptCanvas(fCanv);
    AddFrame(fEmCanv, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 0, 0, 1, 1));
    //Status bar
    int pars [] = {45, 15, 10, 30};
    fStatus = new TGStatusBar(this, 50, 10, kVerticalFrame);
    fStatus->SetParts(pars, 4);
    AddFrame(fStatus, new TGLayoutHints(kLHintsExpandX, 0, 0, 10, 0));
    //Buttons bar
    fButtonsFrame = new TGHorizontalFrame(this, 200, 40);
    //1->Exit button
    TGTextButton* exit = new TGTextButton(fButtonsFrame, "&Exit ");
    exit->Connect("Pressed()", "ActRoot::EventPainter", this, "DoExit()");
    fButtonsFrame->AddFrame(exit, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    //2->Draw
    TGTextButton* draw = new TGTextButton(fButtonsFrame, "&Draw ");
    draw->Connect("Pressed()", "ActRoot::EventPainter", this, "DoDraw()");
    fButtonsFrame->AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    //3->Reset
    TGTextButton* reset = new TGTextButton(fButtonsFrame, "&Reset ");
    reset->Connect("Pressed()", "ActRoot::EventPainter", this, "DoReset()");
    fButtonsFrame->AddFrame(reset, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    TGTextButton* next = new TGTextButton(fButtonsFrame, "&Next ");
    next->Connect("Pressed()", "ActRoot::EventPainter", this, "DoNextEvent()");
    fButtonsFrame->AddFrame(next, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    
    AddFrame(fButtonsFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    //Other configs
    SetWindowName("ActRoot EventPainter");
    MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();

    //Init histograms
    Init2DHistograms();
}
