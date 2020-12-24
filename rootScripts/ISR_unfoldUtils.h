#ifndef UNFOLDUTILS_H
#define UNFOLDUTILS_H

#include <iostream>
#include <fstream>
#include <iomanip> 
#include <map>
#include <cmath>
#include <TMath.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TProfile.h>
#include <TExec.h>
#include <TDOMParser.h>
#include <TXMLDocument.h>
#include <TSystem.h>

#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TFrame.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TLine.h>
#include <TSpline.h>
#include <TGaxis.h>
#include <TMathText.h>
#include <TLatex.h>
#include <THStack.h>
#include <TMatrixDSymEigen.h>
#include "TUnfoldBinningXML.h"
#include "TUnfoldBinning.h"
#include "TUnfoldDensity.h"
#include "TRandom.h"
#include "TF1Convolution.h"
#include "TDecompSVD.h"

#include "TVectorD.h"
#include "TUnfoldBinningXML.h"
#include "TUnfoldBinning.h"
#include "TUnfoldDensity.h"
#include "TUnfoldIterativeEM.h"

using namespace std; 

const int statSize = 1000;

class ISRUnfold{

private:

    // Bin definitions
    TUnfoldBinning* pt_binning_Rec = NULL;
    TUnfoldBinning* pt_binning_Gen = NULL;
    TUnfoldBinning* mass_binning_Rec = NULL;
    TUnfoldBinning* mass_binning_Gen = NULL;

    std::map<TString, vector<TString>> sysMap;

    // Unfolding
    // For nominal results
    TUnfoldDensity* nomPtUnfold;
    TUnfoldDensity* nomMassUnfold;

    TH2* hPtResponseM;
    TH2* hMassResponseM;

    // For statistical uncertainty
    std::vector<TUnfoldDensity*> statPtUnfold;
    std::vector<TUnfoldDensity*> statMassUnfold;

    // For systematic uncertainty
    std::map<TString, std::map<TString, TUnfoldDensity*>> sysPtUnfold;
    std::map<TString, std::map<TString, TUnfoldDensity*>> sysMassUnfold;

    TUnfoldIterativeEM* iterEMPtUnfold;
    TUnfoldIterativeEM* iterEMMassUnfold;

    int iBest_pt;
    int iBest_mass;
    const int NITER_Iterative = 500;

    TGraph *graph_SURE_IterativeSURE_pt,*graph_DFdeviance_IterativeSURE_pt;
    TGraph *graph_SURE_IterativeSURE_mass,*graph_DFdeviance_IterativeSURE_mass;
    
    // Acceptance correction
    TH1* hFullPhaseMassData; // data after acceptance correction 
    TH1* hFullPhasePtData;  
    TH1* hFullPhaseMassMC;  
    TH1* hFullPhasePtMC;  

    std::map<TString, std::map<TString, TH1*>> hSysFullPhaseMassData;
    std::map<TString, std::map<TString, TH1*>> hSysFullPhasePtData;
    std::map<TString, std::map<TString, TH1*>> hSysFullPhaseMassMC;
    std::map<TString, std::map<TString, TH1*>> hSysFullPhasePtMC;

    TH1* hAcceptanceMass;  
    TH1* hAcceptancePt;  

    TH1* hAcceptanceFractionMass;  
    TH1* hAcceptanceFractionPt;  

    std::map<TString, std::map<TString, TH1*>> hSysAcceptanceMass;
    std::map<TString, std::map<TString, TH1*>> hSysAcceptancePt;

    std::map<TString, std::map<TString, TH1*>> hSysAcceptanceFractionMass;
    std::map<TString, std::map<TString, TH1*>> hSysAcceptanceFractionPt;

    bool verbose;

    // Conditions for unfolding
    TUnfold::ERegMode regMode;
    double nominal_bias;
    
    TString output_baseDir;
    TString unfold_name;
    TString channel_name;
    TString var;
    int year;

    bool makeStatUnfold; 
        
public:
    
    // Constructor
    ISRUnfold(TString unfold_name_, TString channel, int year_ = 2016, int regMode_ = 0, bool makeStatUnfold_ = true, bool verbose_ = false, TString var_ = "Mass")
    {
        cout << "ISRUnfold set! " << var_ << endl;

        unfold_name = unfold_name_;
        channel_name = channel;
        year = year_;
        var = var_;

        nominal_bias = 1.;  

        if(regMode_ == 0)
            regMode = TUnfold::kRegModeNone;
        if(regMode_ == 1)
            regMode = TUnfold::kRegModeSize;
        if(regMode_ == 2)
            regMode = TUnfold::kRegModeDerivative; 
        if(regMode_ == 3)
            regMode = TUnfold::kRegModeCurvature; 

        makeStatUnfold = makeStatUnfold_;

        verbose = verbose_;
    }
    // Destructor
    ~ISRUnfold(){}

    void drawResponseM(TString sysName = "", TString sysPostfix = "", bool isDetector = true);
    void checkIterEMUnfold(void);

    const TVectorD& checkMatrixCond();
    double getSmearedChi2(TString filePath, TString dirName, TString steering, bool useAxis, bool divBinWidth = false);
    double getUnfoldedChi2(TString steering, bool useAxis, bool divBinWidth = false);

    void setOutputBaseDir(TString outPath);
    void setBias(double bias);

    // Set nominal TUnfoldDensity 
    void setNominalRM(TString filepath, TString dirName, TString histName, TString binDef = "");
    void setFromPrevUnfResult(ISRUnfold* unfold, bool useAccept = false);

    // Set input histogram
    void setUnfInput(TString varPostfix = "", TString filepath = "", TString dirName ="", TString histName = "", bool isSys = false, TString sysName = "", TString sysPostfix = "", bool isFSR = false);
    void setUnfInput(ISRUnfold* unfold, bool isSys = false, TString sysName = "", TString sysPostfix = "", bool useAccept = false);

    // Set background histograms
    void subBkgs(TString filepath, std::pair<TString, TString>& bkgInfo, 
                 bool isSys = false, TString binDef = "", TString dirName = "", TString sysName = "", TString sysPostfix = "", TString histPostfix = "");

    // Set systematic TUnfoldDensity
    void setSystematicRM(TString filepath, TString dirName, TString histName, TString sysName, TString sysPostfix, TString histPostfix, TString binDef);

    void setSystematics(TString sysName, TString sysHistName);
    void inline printSystematics()
    {
        std::map<TString, std::vector<TString>>::iterator it = sysMap.begin();
        while(it != sysMap.end())
        {
            cout << "Systematic name: " << it->first << endl;
            it++;
        }
    }
    inline std::map<TString, std::vector<TString>>& getSystematicMap()
    {
        return sysMap;
    }

    void divideByBinWidth(TH1* hist, bool norm = false);
    void varyHistWithStatError(TH1* hist, int sys);

    // Do unfold 
    void doISRUnfold( bool doSys = false);
    void doStatUnfold(); 

    void doAcceptCorr(TString filePath, TString binDef, bool doSys = false, TString outName = "", bool isAccept = false);
    void drawCorrelation(TString steering, bool useAis, TString outName = "");

    // Get histograms
    TH1* getUnfoldedHists(TString outHistName = "", TString steering = "", bool useAxis = true, bool divBinWidth = false);
    TH1* getRawHist(TString filePath, TString dirName, TString histName, TString outHistName, TString steering, bool useAxis, bool divBinWidth = false);

    TH1* getUnfInput(TString steering, bool useAxis, int massBin, bool binWidth);

    // Helper functions
    void doNorm(TH1* hist, bool norm = true); 
};

#endif
