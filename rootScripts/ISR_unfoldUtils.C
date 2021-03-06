#include "ISR_unfoldUtils.h"

void ISRUnfold::setBias(double bias)
{
   nominal_bias = bias;
}

TMatrixD ISRUnfold::makeMatrixFromHist(TH2F*hist)
{
    int nBinsX = hist->GetNbinsX();
    int nBinsY = hist->GetNbinsY();
    TMatrixD matrix(nBinsY,nBinsX);
    for(int i=1;i<=nBinsX;i++){
        for(int j=1;j<=nBinsY;j++){
            matrix(j-1,i-1) = hist->GetBinContent(i,j);
        }
    }
    return matrix;
}//end makeMatrixFromHist

void ISRUnfold::checkMatrixCond()
{
    TH2F* hProb = NULL;
    hProb = (TH2F*) nominalTUnfold->GetProbabilityMatrix("hProb_"+var);

    int nBinsX = hProb->GetNbinsX();
    int nBinsY = hProb->GetNbinsY();
    TMatrixD matrix = makeMatrixFromHist(hProb);

    TDecompSVD decomp(matrix);
    double condition = decomp.Condition();
    cout << "The condition number for " << ": " << condition << endl;

    double determinant;
    TMatrixD mInverse = matrix.Invert(&determinant);
    cout << "The determinant of " << " is " << determinant << endl;
    //return condition;

/*
    // TODO check hProb->GetNbinsY()+2,hProb->GetNbinsX()+2 without "F"
    TMatrixD* mProb=new TMatrixD(hProb->GetNbinsX()+2,hProb->GetNbinsY()+2,hProb->GetArray(),"F"); // +2 for under/overflow bins
    TMatrixD* mtProb=new TMatrixD(hProb->GetNbinsY()+2,hProb->GetNbinsX()+2);
    mtProb->Transpose(*mProb);
    TDecompSVD* svdProb=new TDecompSVD(*mtProb);
    //mtProb->Print();
    cout << "Decompose(), successed? " << svdProb->Decompose() << endl;
    const Int_t colLwb = svdProb->GetColLwb();
    const Int_t nCols  = svdProb->GetNcols();
    const TVectorD& singularValues = svdProb->GetSig();

    // Find minimum (>0)
    int i = colLwb+nCols-1;
    while(i >= colLwb)
    {
        if(singularValues[i] > 0)
        {
            double min = singularValues[i];
            cout << var << ", Cond(): " << singularValues[colLwb]/min << endl;
            break;
        }
        else
        {
           i--;
        }
    }
    return svdProb->GetSig();
*/
}

double ISRUnfold::getSmearedChi2(TString filePath, TString dirName, TString steering, bool useAxis)
{
    double chi2 = 0.;
    double ndf  = 0.;

    TH1* hData; // Data - Bkg
    TH1* hDY; // DY MC

    TString DYHistName_ = "histo_DYJetsToMuMu";
    if(channel_name == "electron") DYHistName_ = "histo_DYJetsToEE";
    hData = nominalTUnfold->GetInput("hData_"+var, 0, 0, steering, useAxis);
    hDY = getRawHist(filePath, dirName, DYHistName_, "Signal_"+var, steering, useAxis); ;

    for(int i=1;i<=hDY->GetNbinsX();i++)
    {
        ndf += 1.;
        if(hData->GetBinError(i) == 0)
        {
          std::cout << "unfolded " << i << " bin: " << hData->GetBinContent(i) << std::endl;
          std::cout << "error is zero in the " << i << " bin..." << std::endl;
          std::cout << "so skip this bin" << std::endl;
          continue;
        }
        // TODO add option to use input covariance matrix
        double pull=(hData->GetBinContent(i)-hDY->GetBinContent(i))/hData->GetBinError(i);
        //cout << "data: " << hData->GetBinContent(i) << " mc: " << hDY->GetBinContent(i) << " data error: " << hData->GetBinError(i) << endl;
        chi2+= pull*pull;
    }
    //cout << "chi^{2}, " << chi2 << endl;
    return chi2;
}

double ISRUnfold::getUnfoldedChi2(TString steering, bool useAxis)
{
    double chi2 = 0.;
    double ndf  = 0.;

    TH1* hData; // Data - Bkg
    TH1* hDY; // DY MC
    TH2* hRho;

    //TH1 *g_fcnHist=0;
    TMatrixD *g_fcnMatrix=0;

    hData = nominalTUnfold->GetOutput("hData_"+var, 0, 0, steering, useAxis);
    hDY = nominalTUnfold->GetBias("hData_"+var, 0, 0, steering, useAxis);
    hRho = nominalTUnfold->GetRhoIJtotal("histRho_chi_"+var, 0,0, steering, useAxis);

    // FIXME check if "n" or "n+1"
    int n = hData->GetNbinsX();
    TMatrixDSym v(n);
    for(int i=0;i<n;i++)
    {
       for(int j=0;j<n;j++)
        {
            v(i,j)=hRho->GetBinContent(i+1,j+1)*(hData->GetBinError(i+1)*hData->GetBinError(j+1));
        }
    }

    TMatrixDSymEigen ev(v);
    TMatrixD d(n,n);
    TVectorD di(ev.GetEigenValues());
    for(int i=0;i<n;i++) {
       if(di(i)>0.0) {
          d(i,i)=1./di(i);
       } else {
          cout<<"bad eigenvalue i="<<i<<" di="<<di(i)<<"\n";
          exit(0);
       }
    }

    TMatrixD O(ev.GetEigenVectors());
    TMatrixD DOT(d,TMatrixD::kMultTranspose,O);
    g_fcnMatrix=new TMatrixD(O,TMatrixD::kMult,DOT);
    TMatrixD test(*g_fcnMatrix,TMatrixD::kMult,v);
    int error=0;

    for(int i=0;i<n;i++)
    {
        if(TMath::Abs(test(i,i)-1.0)>1.E-7)
        {
            error++;
        }
        for(int j=0;j<n;j++)
        {
            if(i==j) continue;
            if(TMath::Abs(test(i,j)>1.E-7)) error++;
        }
    }


    // Calculate chi2
    //for(int i=0;i<hData->GetNbinsX();i++)
    //{
    //
    //    double di_=hData->GetBinContent(i+1)-hDY->GetBinContent(i+1);
    //    if(g_fcnMatrix)
    //    {
    //        for(int j=0;j<hData->GetNbinsX();j++)
    //        {
    //            double dj=hData->GetBinContent(j+1)-hDY->GetBinContent(j+1);
    //            chi2+=di_*dj*(*g_fcnMatrix)(i,j);
    //        }
    //    }
    //    else
    //    {
    //        double pull=di_/hData->GetBinError(i+1);
    //        chi2+=pull*pull;
    //    }
    //    ndf+=1.0;
    //}

    for(int i=1;i<=hDY->GetNbinsX();i++)
    {
        ndf += 1.;
        if(hData->GetBinError(i) == 0)
        {
          std::cout << "unfolded " << i << " bin: " << hData->GetBinContent(i) << std::endl;
          std::cout << "error is zero in the " << i << " bin..." << std::endl;
          std::cout << "so skip this bin" << std::endl;
          continue;
        }
        // TODO add option to use input covariance matrix
        double pull=(hData->GetBinContent(i)-hDY->GetBinContent(i))/hData->GetBinError(i);
        //cout << "data: " << hData->GetBinContent(i) << " mc: " << hDY->GetBinContent(i) << " data error: " << hData->GetBinError(i) << endl;
        chi2+= pull*pull;
    }
    //cout << "chi^{2}, " << chi2 << endl;
    return chi2;

}

void ISRUnfold::checkIterEMUnfold(void)
{
    double yMin=1.;
    double yLine=10.;
    double yMax=graph_SURE_IterativeSURE->GetMaximum();

    gStyle->SetPadBottomMargin(0.2);
    TCanvas *canvas1=new TCanvas("compare","",3600,1200);
    canvas1->Divide(2,1);

    canvas1->cd(1);
    gPad->SetLogy();
    graph_SURE_IterativeSURE->GetYaxis()->SetRangeUser(yMin,yMax);
    graph_SURE_IterativeSURE->GetXaxis()->SetRangeUser(-1.5,100.5);
    graph_SURE_IterativeSURE->GetXaxis()->SetTitle("iteration");
    graph_SURE_IterativeSURE->GetXaxis()->SetTitleOffset(1.2);
    graph_SURE_IterativeSURE->GetXaxis()->SetTitleFont(43);
    graph_SURE_IterativeSURE->GetXaxis()->SetTitleSize(100);
    graph_SURE_IterativeSURE->SetMarkerColor(kBlue);
    graph_SURE_IterativeSURE->SetMarkerStyle(20);
    graph_SURE_IterativeSURE->SetMarkerSize(2);
    graph_SURE_IterativeSURE->DrawClone("APW");
    int n_scanSURE_iterative=graph_SURE_IterativeSURE->GetN();
    double const *nIter_scanSURE_iterative=graph_SURE_IterativeSURE->GetX();
    double const *DF_scanSURE_iterative=graph_DFdeviance_IterativeSURE->GetX();
    double const *deviance_scanSURE=graph_DFdeviance_IterativeSURE->GetY();
    TGraph *DF_iterative=new TGraph
       (n_scanSURE_iterative,nIter_scanSURE_iterative,DF_scanSURE_iterative);
    TGraph *deviance_iterative=new TGraph
       (n_scanSURE_iterative,nIter_scanSURE_iterative,deviance_scanSURE);
    DF_iterative->SetMarkerColor(kRed);
    DF_iterative->SetMarkerStyle(24);
    DF_iterative->SetMarkerSize(2);
    DF_iterative->DrawClone("P");
    deviance_iterative->SetMarkerColor(kMagenta);
    deviance_iterative->SetMarkerStyle(22);
    deviance_iterative->SetMarkerSize(2);
    deviance_iterative->DrawClone("P");
    TLine *line2=new TLine(iBest,yLine,iBest,1e4);
    line2->SetLineStyle(1);
    line2->Draw();
    TLegend *legend3=new TLegend(0.25,0.2,0.9,0.45,"Iterative EM, minimize SURE");
    legend3->SetBorderSize(0);
    legend3->SetFillStyle(0);
    legend3->SetTextSize(0.045);
    legend3->AddEntry(graph_SURE_IterativeSURE,"SURE","p");
    legend3->AddEntry(DF_iterative,"D.F.","p");
    legend3->AddEntry(deviance_iterative,"deviance","p");

    legend3->AddEntry(line2,TString::Format
                      ("min(SURE) at iteration=%d",iBest),"l");
    legend3->AddEntry((TObject *)0,TString::Format
                      ("D.F.=%3g",DF_scanSURE_iterative[iBest]),"");
    legend3->Draw();
    canvas1->SaveAs("ISR_scan.pdf");
}

// Set the nominal response matrix
void ISRUnfold::setNominalRM(TString filepath, TString dirName, TString binDef)
{
    //cout << "ISRUnfold::setNominalRM set response matrix..." << endl;
    TH1::AddDirectory(kFALSE);
    TFile* filein = new TFile(filepath, "READ");

    TString fullDirPath = dirName + "/" + var + "_ResMatrix_" + binDef + "/";
    //cout << "ISRUnfold::setNominalRM fullDirPath : " << fullDirPath << endl;

    TString Rec_binName = "Rec_"+var;
    TString Gen_binName = "Gen_"+var;
    Rec_binName = fullDirPath + Rec_binName;
    Gen_binName = fullDirPath + Gen_binName;

    // Set bin definition
    binning_Rec = (TUnfoldBinning*)filein->Get(Rec_binName);
    binning_Gen = (TUnfoldBinning*)filein->Get(Gen_binName);

    // Set response matrix
    // First, get the response matrix
    TH2* hmcGenRec = (TH2*)filein->Get(fullDirPath + "hmc" + var + "GenRec");
    cout << fullDirPath + "hmc" + var + "GenRec" << endl;

    nominalTUnfold = new TUnfoldDensity(hmcGenRec, TUnfold::kHistMapOutputHoriz, regMode, TUnfold::kEConstraintArea, TUnfoldDensity::kDensityModeBinWidth, binning_Gen, binning_Rec);
    cout << "Used TUnfold version " << nominalTUnfold->GetTUnfoldVersion() << endl;
    hResponseM = (TH2*) hmcGenRec->Clone("hResponseM");

    // For statistical uncertainty
    if(doInputStatUnc)
    {
        // cout << "Create response matrix for statistical uncertainty..." << endl;
        for(int i = 0; i < statSize; i++)
        {
            UnfoldingInputStatTUnfold.push_back(new TUnfoldDensity(hmcGenRec, TUnfold::kHistMapOutputHoriz, regMode, TUnfold::kEConstraintArea, TUnfoldDensity::kDensityModeBinWidth, binning_Gen, binning_Rec));
        }
    }
    if(doRMStatUnc)
    {
        for(int i = 0; i < statSize; i++)
        {
            TString nth_;
            nth_.Form("%d", i);
            TH2* tempRM = (TH2*) hmcGenRec->Clone("hRM_stat_" + nth_);

            for(int xbin=1; xbin <= tempRM->GetXaxis()->GetNbins(); xbin++)
            {
                for(int ybin=0; ybin <= tempRM->GetYaxis()->GetNbins(); ybin++)
                {
                    double err = tempRM->GetBinError(xbin, ybin);
                    if(err >= 0.0)
                        tempRM->SetBinContent(xbin, ybin, tempRM->GetBinContent(xbin, ybin) + gRandom->Gaus(0, err));
                }
            }

            UnfoldingMatrixStatTUnfold.push_back(new TUnfoldDensity(tempRM, TUnfold::kHistMapOutputHoriz, regMode, TUnfold::kEConstraintArea, TUnfoldDensity::kDensityModeBinWidth, binning_Gen, binning_Rec));
        }
    }

    // Save migration and response matrix
    TDirectory* topDir;
    TDirectory* varDir;

    topDir=fUnfoldOut->GetDirectory("matrix");
    varDir=fUnfoldOut->GetDirectory("matrix/"+var);
    varDir->cd();
    binning_Gen->Write();
    binning_Rec->Write();

    TH2F* hResponseM = (TH2F*) nominalTUnfold->GetProbabilityMatrix("hResponseM"); 
    hResponseM->Write();
    hmcGenRec->SetName("hMigrationM");
    hmcGenRec->Write();

    // Save projection of the migration matrix

    topDir=fUnfoldOut->GetDirectory("unfolded");
    varDir=fUnfoldOut->GetDirectory("unfolded/"+var);
    varDir->cd();

    TH1D* hProjectedTruth = (TH1D*) hmcGenRec->ProjectionX("histo_ProjectedTruth", 0, -1, "e");  //
    TH1D* hProjectedReco = (TH1D*) hmcGenRec->ProjectionY("histo_ProjectedReco", 1, -1, "e");  //

    hProjectedTruth->Write();
    hProjectedReco->Write();

    filein->Close();
    delete filein;
}

void ISRUnfold::setFromPrevUnfResult(ISRUnfold* unfold, bool useAccept)
{
    //cout << "setFromPrevUnfResult(), useAccept? " << useAccept << endl;
    // Loop over sytematics considered in the previous unfold class
    // So first get sysVector map object
    std::vector<TString> sysVector_previous = unfold->getSystematicVector();
    std::vector<TString>::iterator it = sysVector_previous.begin();
    while(it != sysVector_previous.end())
    {
        //cout << "Systematic name: " << it->first << endl;
        std::vector<TString>::iterator found = find(this->sysVector.begin(), this->sysVector.end(), *it);
        if(found == this->sysVector.end())
        //if(this->sysVector.find(it->first) == this->sysVector.end())
        {
            // Not found in this ISRUnfold class, but exits in the previous one
            // Create TUnfoldDensity using the DEFAULT response matrix
            //cout << "Systematic variation, " << sysVector_previous[it->first][ith] << endl;
            if((*it).Contains("IterEM"))
            {
                this->iterEMTUnfold   = new TUnfoldIterativeEM(hResponseM,TUnfoldDensity::kHistMapOutputHoriz,binning_Gen,binning_Rec);

                if(!useAccept)
                {
                    this->iterEMTUnfold->SetInput(unfold->iterEMTUnfold->GetOutput("hUnfolded" + var + "_"+ *it + "_" + *it,0,0,"*[*]",false), nominal_bias);
                }
                else
                {
                    //cout << "use acceptance corrected output!" << endl;
                    this->iterEMTUnfold->SetInput(unfold->hSysFullPhaseData[*it], nominal_bias);
                }
            }
            else
            {
                this->systematicTUnfold[*it] = new TUnfoldDensity(hResponseM,TUnfold::kHistMapOutputHoriz,regMode, TUnfold::kEConstraintArea, TUnfoldDensity::kDensityModeBinWidth, binning_Gen,binning_Rec);

                if(!useAccept)
                {
                     this->systematicTUnfold[*it]->SetInput(unfold->systematicTUnfold[*it]->GetOutput("hUnfolded" + var + "_"+ *it + "_" + *it,0,0,"*[*]",false), nominal_bias);
                }
                else
                {
                    //cout << "use acceptance corrected output!" << endl;
                    this->systematicTUnfold[*it]->SetInput(unfold->hSysFullPhaseData[*it], nominal_bias);
                }
            }
            this->sysVector.push_back(*it);
        }
        it++;
    }
    // Loop over variations of event selection efficiency correction
}

// Option for unfold options
void ISRUnfold::setSystematicRM(TString filepath, TString dirName, TString binDef, TString sysName, TString histPostfix)
{
    TFile* filein = new TFile(filepath, "READ");
    TH2* hmcGenRec = NULL;

    TString histNameWithSystematic = "hmc" + var + "GenRec" + histPostfix;
    hmcGenRec = (TH2*)filein->Get(dirName + "/" + var + "_ResMatrix_" + binDef + "/" + histNameWithSystematic);
    cout << "ISRUnfold::setSystematicRM " << filepath << " " << dirName + "/" + var + "_ResMatrix_" + binDef + "/" + histNameWithSystematic << endl;

    if(sysName.Contains("IterEM"))
    {
        iterEMTUnfold = new TUnfoldIterativeEM(hmcGenRec,TUnfoldDensity::kHistMapOutputHoriz,binning_Gen,binning_Rec);
    }
    else
    {
        systematicTUnfold[sysName] = new TUnfoldDensity(hmcGenRec, TUnfold::kHistMapOutputHoriz, regMode, TUnfold::kEConstraintArea, TUnfoldDensity::kDensityModeBinWidth, binning_Gen, binning_Rec);
    }

    filein->Close();
    delete filein;
}

// Set input histogram using the nominal output of the previous unfolding 
void ISRUnfold::setUnfInput(ISRUnfold* unfold, TString thisSysType, TString sysName, bool useAccept)
{
    TH1::AddDirectory(kFALSE);

    if(!useAccept)
    {
        if(thisSysType=="Type_0")
        {
            nominalTUnfold->SetInput(unfold->getUnfoldedHists(var, "UnfoldOut_"+var, "*[*]"), 1.);
        }
        else
        {
            // FIXME
            systematicTUnfold[sysName]->SetInput(unfold->getUnfoldedHists(var, "UnfoldOut_"+var+thisSysType+sysName, "*[*]"), 1.);
        }
    }
    else
    {
        //cout << "set from previous unfold class, isSys " << isSys << endl;
        if(thisSysType=="Type_0")
        {
            nominalTUnfold->SetInput(unfold->hFullPhaseData, 1.);
        }
        else
        {
            //
            if(sysName.Contains("IterEM"))
            {
                iterEMTUnfold->SetInput(unfold->hFullPhaseData, 1.);
            }
            else
            {
                TH1* htemp = unfold->hSysFullPhaseData[sysName];
                systematicTUnfold[sysName]->SetInput(htemp, 1.);
            }
        }
    }
}

// Set input histogram from root file
void ISRUnfold::setUnfInput(TString filepath, TString dirName, TString binDef, TString histName, TString sysType, TString sysName, TString histPostfix, bool isFSR)
{
    TH1::AddDirectory(kFALSE);

    TFile* filein = new TFile(filepath);
    TH1* hRec = NULL;
    //cout << dirName+"/"+var+ "_" + binDef+"/"+histName << endl;
    hRec = (TH1*)filein->Get(dirName+"/"+var+ "_" + binDef+"/"+histName + histPostfix);

    // Use DY MC as unfolding input, i.e. simple closure test
    if(histName.Contains("DYJetsTo"))
    {
        if(!isFSR)
        {
            histName.ReplaceAll("DYJetsTo", "DYJets10to50To");
            hRec->Add((TH1*)filein->Get(dirName+"/"+var+ "_" + binDef+"/"+histName));
        }
        else
        {
            histName.ReplaceAll("DYJets", "DYJets10to50");
            hRec->Add((TH1*)filein->Get(dirName+"/"+var+ "_" + binDef+"/"+histName));
        }
    }

    // Very preliminary test for input covariance using ID SF
    //TFile* fcov = new TFile("/home/jhkim/ISR_Run2/unfolding/TUnfoldISR2016/rootScripts/covariance.root");
    //TFile* fcov_pt = new TFile("/home/jhkim/ISR_Run2/unfolding/TUnfoldISR2016/rootScripts/covariance_pt.root");
    //TH2* hCov = (TH2*) fcov->Get("cov");
    //TH2* hCov_pt = (TH2*) fcov_pt->Get("cov");

    // Nominal
    if(sysType == "Type_0")
    {
        nominalTUnfold->SetInput(hRec, nominal_bias, 0);
    }
    else
    // Systematic histograms
    {
        if(sysName.Contains("IterEM")) // FIXME
        {
            iterEMTUnfold->SetInput(hRec, nominal_bias);
        }
        else
        {
            systematicTUnfold[sysName]->SetInput(hRec, nominal_bias);
        }
    }

    filein->Close();
    delete filein;
}

void ISRUnfold::subBkgs(TString filepath, TString dirName, TString binDef, TString bkgName, TString sysType, TString sysName, TString histPostfix)
{
    TFile* filein = new TFile(filepath);
    TH1* hRec = NULL;

    hRec = (TH1*)filein->Get(dirName + "/" + var + "_" + binDef+"/histo_" + bkgName + histPostfix);

    // Nominal histograms
    if(sysType=="Type_0")
    {
        nominalTUnfold->  SubtractBackground(hRec, bkgName);
    }
    else
    // Systematic
    {
        //cout << "file path: " << filepath << endl;
        //cout << dirName + "/Pt"+binDef+"/histo_" + histNameWithSystematic << endl;

        if(sysName.Contains("IterEM"))
        {
            iterEMTUnfold->SubtractBackground(hRec, bkgName);
        }
        else
        {
            // FIXME temporary method for background systematic
            if(sysName.Contains("Background"))
            {
                if(sysName.Contains("Up"))
                {
                    systematicTUnfold[sysName]->SubtractBackground(hRec, bkgName, 1.05);
                }
                if(sysName.Contains("Down"))
                {
                    systematicTUnfold[sysName]->SubtractBackground(hRec, bkgName, 0.95);
                }
            }
            else
            {
                //cout << "ISRUnfold::subBkgs bkgName: " << bkgName << endl;
                //cout << dirName + "/" + var + "_" + binDef+"/histo_" + bkgName + histPostfix << endl;
                //cout << "histPostfix: " << histPostfix << endl;
                systematicTUnfold[sysName]->SubtractBackground(hRec, bkgName);
            }
        }
    }

    filein->Close();
    delete filein;
}

void ISRUnfold::setSystematics(TString sysHistName)
{
    sysVector.push_back(sysHistName);
}

void ISRUnfold::doISRUnfold()
{

    TDirectory* topDir;
    TDirectory* varDir;

    topDir=fUnfoldOut->GetDirectory("unfolded");
    varDir=fUnfoldOut->GetDirectory("unfolded/"+var);
    varDir->cd();
    binning_Gen->Write();

    topDir->cd();

    // No regularisation
    if(regMode == TUnfold::kRegModeNone)
    {
        // Nominal unfolding
        nominalTUnfold->DoUnfold(0);
    }
    /*
    else
    {
        nomMassUnfold->DoUnfold(0);

        if(var=="Pt")
        {
        int istart = binning_Gen->GetGlobalBinNumber(0., 200.);
        int iend = binning_Gen->GetGlobalBinNumber(99., 200.);
        nominalTUnfold->RegularizeBins(istart, 1, iend-istart+1, regMode);

        double tauMin=1.e-4;
        double tauMax=1.e-1;
        nominalTUnfold->ScanLcurve(100, tauMin, tauMax, 0);

        TH2 *histL= nominalTUnfold->GetL("L");
        if(histL)
        {
            for(Int_t j=1;j<=histL->GetNbinsY();j++)
            {
                cout<<"L["<<nominalTUnfold->GetLBinning()->GetBinName(j)<<"]";
                for(Int_t i=1;i<=histL->GetNbinsX();i++) {
                    Double_t c=histL->GetBinContent(i,j);
                    if(c!=0.0) cout<<" ["<<i<<"]="<<c;
                }
                cout<<"\n";
            }
        }
        }
    }
    */
    varDir->cd();

    bool useAxisBinning = true; 
    if(var == "Pt")
    {
        useAxisBinning = false;   
    }  

    nominalTUnfold->GetInput("histo_UnfoldInput", 0, 0, 0, false)->Write();
    nominalTUnfold->GetRhoIJtotal("hCorrelation", 0, 0, 0, useAxisBinning)->Write();
    nominalTUnfold->GetOutput("histo_Data",0,0, "*[*]", false)->Write();
    nominalTUnfold->GetBias("histo_DY", 0, 0, "*[*]", false)->Write();

    if(doInputStatUnc)
    {
        for(int istat = 0; istat < statSize; istat++)
        {
            //cout << istat << " th stat.." << endl;
            TH1* tempInput;

            TString nth_;
            nth_.Form("%d", istat);
            tempInput = nominalTUnfold->GetInput("temp" + var + "Hist_" + nth_, 0, 0, 0, false);

            // randomize histogram bin content
            for(int ibin = 1; ibin<tempInput->GetNbinsX()+1;ibin++)
            {
                double err = tempInput->GetBinError(ibin);
                if(err > 0.0)
                {
                    tempInput->SetBinContent(ibin, tempInput->GetBinContent(ibin) + gRandom->Gaus(0,err));
                }
            }

            UnfoldingInputStatTUnfold.at(istat)->SetInput(tempInput, nominal_bias);
            UnfoldingInputStatTUnfold.at(istat)->DoUnfold(0);

            varDir->cd();
            UnfoldingInputStatTUnfold.at(istat)->GetOutput("histo_Data_UnfoldingInputStat_" + nth_, 0, 0, "*[*]", false)->Write();

            delete tempInput;
            delete UnfoldingInputStatTUnfold.at(istat);
        }
        UnfoldingInputStatTUnfold.clear();
    }

    if(doRMStatUnc)
    {
        TH1* tempInput = nominalTUnfold->GetInput("BkgSubtractedInput", 0, 0, 0, false);

        for(int istat = 0; istat < statSize; istat++)
        {
            TString nth_;
            nth_.Form("%d", istat);

            UnfoldingMatrixStatTUnfold.at(istat)->SetInput(tempInput, nominal_bias);
            UnfoldingMatrixStatTUnfold.at(istat)->DoUnfold(0);

            varDir->cd();
            UnfoldingMatrixStatTUnfold.at(istat)->GetOutput("histo_Data_UnfoldingMatrixStat_" + nth_, 0, 0, "*[*]", false)->Write();

            delete UnfoldingMatrixStatTUnfold.at(istat);
        }
        delete tempInput;
        UnfoldingMatrixStatTUnfold.clear();
    }

    // For systematic
    std::vector<TString>::iterator it = sysVector.begin();
    while(it != sysVector.end())
    {
        if( (*it).Contains("IterEM"))
        {
            iBest=iterEMTUnfold->ScanSURE(NITER_Iterative, &graph_SURE_IterativeSURE, &graph_DFdeviance_IterativeSURE);
            cout << "iBest: " << iBest << endl;

            varDir->cd();
            iterEMTUnfold->GetOutput("histo_Data_"+(*it),0,0, "*[*]", false)->Write();
            nominalTUnfold->GetBias("histo_DY_"+(*it), 0, 0, "*[*]", false)->Write();
        }
        else
        {

            if(regMode == TUnfold::kRegModeNone)
            {
                systematicTUnfold[*it]->DoUnfold(0);
            }
            else
            {
                double tauMin=1.e-4;
                double tauMax=1.e-1;
                systematicTUnfold[*it]->ScanLcurve(100, tauMin, tauMax, 0);
            }

            varDir->cd();
            systematicTUnfold[*it]->GetOutput("histo_Data_"+(*it),0,0, "*[*]", false)->Write();
            systematicTUnfold[*it]->GetBias("histo_DY_"+(*it), 0, 0, "*[*]", false)->Write();
        }
        it++;
    }

    topDir->Write();
}

void ISRUnfold::doAcceptCorr(TString filePath, TString binDef, bool isAccept)
{
    TDirectory* topDir;
    TDirectory* varDir;

    //if(!gSystem->AccessPathName(fullPath, kFileExists))
    topDir=fUnfoldOut->GetDirectory("acceptance");
    varDir=fUnfoldOut->GetDirectory("acceptance/"+var);
    varDir->cd();
    binning_Gen->Write();
    topDir->cd();

    TFile* filein = new TFile(filePath);

    TString accepCorrOrEffCorr;
    if(isAccept)
        accepCorrOrEffCorr = "Acceptance";
    else
        accepCorrOrEffCorr = "Efficiency";

    TH1* hFiducialPhaseMC = NULL;

    hFullPhaseMC = (TH1*) filein->Get("Acceptance/"+var+ "_" + binDef + "/histo_DYJets");
    if(year==2016)
        hFullPhaseMC->Add((TH1*) filein->Get("Acceptance/"+var+ "_" + binDef + "/histo_DYJets10to50"));
    else
        hFullPhaseMC->Add((TH1*) filein->Get("Acceptance/"+var+ "_" + binDef + "/histo_DYJets10to50_MG"));

    hFiducialPhaseMC = nominalTUnfold->GetBias("hFiducial"+var, 0, 0, "*[*]", false);
    hAcceptance = (TH1*) hFullPhaseMC->Clone("hAcceptance"+var);
    hAcceptance->Divide(hFiducialPhaseMC);

    hAcceptanceFraction = (TH1*) hFiducialPhaseMC->Clone("hAcceptanceFraction"+var);
    hAcceptanceFraction->Divide(hFullPhaseMC);

    hFullPhaseData = nominalTUnfold->GetOutput("histo_Data",0,0, "*[*]", false);
    hFullPhaseData->Multiply(hAcceptance);

    varDir->cd();
    hFullPhaseData->Write();
    hFullPhaseMC->SetName("histo_DY");
    hFullPhaseMC->Write();
    hAcceptance->Write();

    if(doInputStatUnc)
    {
        TH1::AddDirectory(kFALSE);
        TH1* hFullPhaseDataTemp = NULL;
        for(int istat = 0; istat < statSize; istat++) 
        {
            TString nth_;
            nth_.Form("%d", istat);


            hFullPhaseDataTemp=(TH1*)fUnfoldOut->Get("unfolded/"+var+"/"+"histo_Data_UnfoldingInputStat_" + nth_);
            hFullPhaseDataTemp->Multiply(hAcceptance);

            varDir->cd();
            hFullPhaseDataTemp->Write(); 
           
            delete hFullPhaseDataTemp; 
        }
    }

    if(doRMStatUnc)
    {
        TH1::AddDirectory(kFALSE);
        TH1* hFullPhaseDataTemp = NULL;
        for(int istat = 0; istat < statSize; istat++) 
        {
            TString nth_;
            nth_.Form("%d", istat);

            //hFullPhaseDataTemp=UnfoldingMatrixStatTUnfold.at(istat)->GetOutput("histo_Data_UnfoldingMatrixStat_" + nth_, 0, 0, "*[*]", false);
            hFullPhaseDataTemp=(TH1*)fUnfoldOut->Get("unfolded/"+var+"/"+"histo_Data_UnfoldingMatrixStat_" + nth_);
            hFullPhaseDataTemp->Multiply(hAcceptance);

            varDir->cd();
            hFullPhaseDataTemp->Write(); 
           
            delete hFullPhaseDataTemp; 
        }
    }

    std::vector<TString>::iterator it = sysVector.begin();
    while(it != sysVector.end())
    {
        TH1* hFullPhaseMC_raw_sys = NULL;
        TH1* hFiducialPhaseMC_sys = NULL;

        if((*it).Contains("IterEM"))
        {
            hSysFullPhaseData[*it]   = iterEMTUnfold->GetOutput("histo_Data_"+(*it),0,0, "*[*]", false);
            hFiducialPhaseMC_sys=hFiducialPhaseMC;
            hFiducialPhaseMC_sys->SetName("histo_DY_"+(*it));
        }
        else
        {
            hSysFullPhaseData[*it]   = systematicTUnfold[*it]->GetOutput("histo_Data_"+(*it),0,0, "*[*]", false);
            hFiducialPhaseMC_sys = systematicTUnfold[*it]->GetBias("hFiducial"+var+"_sys"+(*it), 0, 0, "*[*]", false);
        }

        // For PDF, AlphaS, Scale etc, nominator (of acceptance) also changes
        if( ( ((*it).Contains("Scale") && !(*it).Contains("Lep")) || (*it).Contains("PDF") || (*it).Contains("AlphaS")) && !(*it).Contains("_") )
        {
            hFullPhaseMC_raw_sys = (TH1*) filein->Get("Acceptance/"+var+ "_" + binDef + "/histo_DYJets_"+(*it));
            if(year==2016)
                hFullPhaseMC_raw_sys->Add((TH1*) filein->Get("Acceptance/"+var+ "_" + binDef + "/histo_DYJets10to50_"+(*it)));
            else
                hFullPhaseMC_raw_sys->Add((TH1*) filein->Get("Acceptance/"+var+ "_" + binDef + "/histo_DYJets10to50_MG_"+(*it)));
        }
        else
        {
            hFullPhaseMC_raw_sys=hFullPhaseMC;
        }

        TH1* hAcceptance_sys = (TH1*) hFullPhaseMC_raw_sys->Clone("hAcceptance_sys");
        hAcceptance_sys->Divide(hFiducialPhaseMC_sys);

        TH1* hAcceptanceFraction_sys = (TH1*) hFiducialPhaseMC_sys->Clone("hAcceptanceFraction_sys");
        hAcceptanceFraction_sys->Divide(hFullPhaseMC_raw_sys);

        hSysFullPhaseData[*it]->Multiply(hAcceptance_sys);
        hSysFullPhaseMC[*it] = hFullPhaseMC_raw_sys;

        delete hAcceptance_sys;
        delete hAcceptanceFraction_sys;

        varDir->cd();

        hSysFullPhaseData[*it]->Write();
        hFullPhaseMC_raw_sys->SetName("histo_DY_"+(*it));
        hFullPhaseMC_raw_sys->Write();
        it++;
    }

    topDir->Write();;

    delete hFiducialPhaseMC;
}

void ISRUnfold::varyHistWithStatError(TH1* hist, int sys)
{
    for(int ibin = 1; ibin < hist->GetNbinsX()+1; ibin++)
    {
        hist->SetBinContent(ibin, hist->GetBinContent(ibin) + double(sys) * hist->GetBinError(ibin));
    }
}

TH1* ISRUnfold::getUnfoldedHists(TString outHistName, TString steering, bool useAxis)
{
    TH1* outHist = NULL;
    outHist = nominalTUnfold->GetOutput(outHistName,0,0,steering,useAxis);
    return outHist;
}

TH1* ISRUnfold::getRawHist(TString filePath, TString dirName, TString histName, TString outHistName, TString steering, bool useAxis)
{
    TH1::AddDirectory(kFALSE);
    TFile* filein = new TFile(filePath);
    TH1* hist = NULL;

    if(steering != "")
    {
        TH1* raw_hist = (TH1*)filein->Get(dirName+"/"+var+"/"+histName);
        if(histName.Contains("DYJetsTo") && !histName.Contains("Tau"))
        {
            histName.ReplaceAll("DYJetsTo", "DYJets10to50To");
            raw_hist->Add((TH1*)filein->Get(dirName+"/"+var+"/"+histName));
        }

        hist = binning_Rec->ExtractHistogram(outHistName, raw_hist, 0, useAxis, steering);

        delete raw_hist;
    }
    else
    {
        //cout << "Steering not provided, get raw histogram." << endl;
        //cout << dirName+"/"+var+"/"+histName << endl;
        hist = (TH1*)filein->Get(dirName+"/"+var+"/"+histName);
        if(histName.Contains("DYJetsTo") && !histName.Contains("Tau"))
        {
            histName.ReplaceAll("DYJetsTo", "DYJets10to50To");
            hist->Add((TH1*)filein->Get(dirName+"/"+var+"/"+histName));
        }
    }

    delete filein;

    return hist;
}

