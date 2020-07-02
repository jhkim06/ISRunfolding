import os
import sys
from array import array
import ROOT as rt

import pyScripts.unfoldUtil as unfoldutil

class ISRAnalysis:
    
    def __init__(self, year_ = "2016", channel_= "electron", sys_ = False, matrix_filekey_ = "matrix",
                 matrix_dirPath_ = "Detector_Dressed_DRp1_Fiducial", matrix_histName_ = "Detector_Dressed_DRp1", binDef_ = ""):
        
        # 디텍터 언폴딩 디렉토리 경로 & 매트릭스 이름 
        self.matrix_filekey = matrix_filekey_
        self.matrix_dirPath  = matrix_dirPath_
        self.matrix_histName = matrix_histName_
        self.binDef = binDef_
        
        self.channel = channel_
        self.year = year_
        self.nMassBins = None
       
        # Set data and Drell-Yan input histogram names
        dataHistPostfix = "Double"
        if self.channel == "electron":
            dataHistPostfix = dataHistPostfix + "EG"
            if self.year == "2018":
                dataHistPostfix = "EGamma"
        if self.channel == "muon":
            dataHistPostfix = dataHistPostfix + "Muon"
        self.dataHistName = "histo_"+dataHistPostfix
            
        self.dy10to50HistName = "DYJets10to50"
        MG_postfix = "_MG"
        if self.year != "2016":
            self.dy10to50HistName += MG_postfix
        
        # 아웃풋 디렉토리
        self.outDirPath = "output/"+self.year+"/new_"+self.channel+"/"
        # 인풋 히스토그램 텍스트파일
        self.inHistPathTxt = "inFiles/"+self.year+"/"+self.channel+"/fhist.txt"
    
        # Make output directory
        if not os.path.exists(self.outDirPath):
            os.makedirs(self.outDirPath)
    
        # Read text file including root file paths for unfolding
        self.filePaths = open(self.inHistPathTxt, 'r')
        self.inHistDic = {}
        
        for path in self.filePaths:
            modifiedPath = path.lstrip(' ').rstrip(' ').rstrip('\n')
            self.inHistDic[modifiedPath.split()[1]] = modifiedPath.split()[2]
        
        # 언폴딩 컨피규레이션
        self.bias = 1.0
        self.mode = 0 # 레귤라이제이션 모드
        
        # Create ISRUnfold object
        self.unfold = rt.ISRUnfold(self.channel, int(self.year), int(self.mode), sys_)
        self.unfold.setOutputBaseDir(self.outDirPath)
        self.unfold.setBias(self.bias)
        
        # Set response matrix
        self.unfold.setNomResMatrix("Pt", self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, self.binDef)
        self.unfold.setNomResMatrix("Mass", self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, self.binDef)
       
    def checkMatrixCond(self, var = "Mass"):
        return self.unfold.checkMatrixCond(var)

    def setFromPrevUnfold(self, preUnfold):

        self.unfold.setFromPrevUnfResult(preUnfold)
 
    def setInputHist(self, useMCInput = False, useUnfoldOut = False, unfoldObj = None, dirName = "Detector", isSys = False, sysName = "nominal", sysPostfix = ""):
        
        inputHistName = self.dataHistName
        if useMCInput == True:
            if self.channel == "electron":
                inputHistName = "histo_DYJetsToEE"
            else :
                inputHistName = "histo_DYJetsToMuMu"
        
        if useUnfoldOut == False:
            if "LepMom" in sysName :
                if sysPostfix == "Nominal" :
                    self.unfold.setUnfInput("Pt",   self.binDef, self.inHistDic['hist'], dirName, inputHistName, isSys, sysName, sysPostfix)
                    self.unfold.setUnfInput("Mass", self.binDef, self.inHistDic['hist'], dirName, inputHistName, isSys, sysName, sysPostfix)
                else :
                    self.unfold.setUnfInput("Pt",   self.binDef, self.inHistDic['hist_lepScale'], dirName+"_"+sysPostfix, inputHistName, isSys, sysName, sysPostfix)
                    self.unfold.setUnfInput("Mass", self.binDef, self.inHistDic['hist_lepScale'], dirName+"_"+sysPostfix, inputHistName, isSys, sysName, sysPostfix)
            else :
                self.unfold.setUnfInput("Pt",   self.binDef, self.inHistDic['hist'], dirName, inputHistName, isSys, sysName, sysPostfix)
                self.unfold.setUnfInput("Mass", self.binDef, self.inHistDic['hist'], dirName, inputHistName, isSys, sysName, sysPostfix)
        else:
            # Let's set systematic input histograms also!
            self.unfold.setUnfInput(unfoldObj, "Pt", isSys, sysName, sysPostfix)
            self.unfold.setUnfInput(unfoldObj, "Mass", isSys, sysName, sysPostfix)

    def setFromPreviousUnfold(self, unfoldObj) :
        self.unfold.setFromPrevUnfResult(unfoldObj)
            
    def subFake(self, isSys = False, systName = "nominal", sysPostfix = ""):
            
        fakeList = {"DYJets": "DY", self.dy10to50HistName:"DY"}
        
        for fake in fakeList.items():
            # TODO If sysPostfix is "Nominal", then use self.inHistDic[self.matrix_filekey], self.matrix_dirPath
            if "LepMom" in systName :
                if sysPostfix == "Nominal" : 
                    self.unfold.subBkgs(self.inHistDic['matrix'], fake, isSys, self.binDef, "detector_level_DY_Fake", systName, sysPostfix)
                else :
                    self.unfold.subBkgs(self.inHistDic['matrix_lepScale'], fake, isSys, self.binDef, "detector_level_DY_Fake_"+sysPostfix, systName, sysPostfix)
            else :
                self.unfold.subBkgs(self.inHistDic['matrix'], fake, isSys, self.binDef, "detector_level_DY_Fake", systName, sysPostfix)
        
    def setUnfoldBkgs(self, doSystematic = False , dirName = "Detector",systName = "nominal", sysPostfix = ""):
   
        bkgList = {}
        # 2016 데이터만 single top 샘플을 갖고 있다 
        if self.year == "2016" or self.year == "2017" or self.year == "2018":
            bkgList = {"QCD": "Fake", "WJet": "Fake",\
            #bkgList = {"WJets_MG": "WJets",\
                       "WW_pythia": "EWK", "WZ_pythia": "EWK", "ZZ_pythia": "EWK", \
                       "DYJets10to50ToTauTau":"EWK", "DYJetsToTauTau":"EWK", \
                       "TTLL_powheg": "Top"} # "SingleTop_tW_top_Incl": "Top", "SingleTop_tW_antitop_Incl": "Top"}
        else :
            bkgList = {"WJets_MG": "WJets", \
                       "WW_pythia": "EWK", "WZ_pythia": "EWK", "ZZ_pythia": "EWK", \
                       "DYJets10to50ToTauTau":"EWK", "DYJetsToTauTau":"EWK", \
                       "TTLL_powheg": "Top"}
        
        for bkg in bkgList.items():
            # TODO If sysPostfix is "Nominal", then use self.inHistDic[self.matrix_filekey], self.matrix_dirPath
            if "LepMom" in systName :
                if sysPostfix == "Nominal" :
                    self.unfold.subBkgs(self.inHistDic['hist'], bkg, doSystematic, self.binDef, dirName, systName, sysPostfix)
                else : 
                    self.unfold.subBkgs(self.inHistDic['hist_lepScale'], bkg, doSystematic, self.binDef, dirName+"_"+sysPostfix, systName, sysPostfix)
            elif "Unfold_det" == systName :
                self.unfold.subBkgs(self.inHistDic['hist'], bkg, doSystematic, self.binDef, dirName, systName, sysPostfix)
            else :
                self.unfold.subBkgs(self.inHistDic['hist'], bkg, doSystematic, self.binDef, dirName, systName, sysPostfix)
            
    def setSystematics(self, sysName, sysHistName):
        self.unfold.setSystematics(sysName, sysHistName)

        if "LepMom" in sysName :
            if sysHistName == "Nominal" :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
            else :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic["matrix_lepScale"], self.matrix_dirPath+"_"+sysHistName, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic["matrix_lepScale"], self.matrix_dirPath+"_"+sysHistName, self.matrix_histName, sysName, sysHistName, self.binDef)
        elif "Unfold" == sysName :
            if sysHistName == "Nominal" :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
            else :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic["fsr_matrix_powheg_pythia"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic["fsr_matrix_powheg_pythia"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
        elif "Unfold_det" == sysName :
            if sysHistName == "Nominal" :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
            else :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic["matrix_zptcorr"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic["matrix_zptcorr"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)

        elif "FSR" in sysName :
            if sysHistName == "PHOTOS" :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic["fsr_matrix_powheg_photos"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic["fsr_matrix_powheg_photos"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
            else :
                self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic["fsr_matrix_powheg_pythia"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
                self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic["fsr_matrix_powheg_pythia"], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
        else :
            self.unfold.setSysTUnfoldDensity("Pt",   self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)
            self.unfold.setSysTUnfoldDensity("Mass", self.inHistDic[self.matrix_filekey], self.matrix_dirPath, self.matrix_histName, sysName, sysHistName, self.binDef)

    def getSystematics(self):
        self.unfold.printSystematics()

    def printMeanValues(self):
        self.unfold.printMeanValues(True)

    def drawResponseM(self, var = "Mass", sysName = "", sysPostfix = "", isDetector = True):
        self.unfold.drawResponseM(var, sysName, sysPostfix, isDetector)

    def drawDetPlot(self, var = "Mass", dirName = "Detector", steering = None, useAxis = True, sysName = "", outName = "", massBin = 0, binWidth = False):
        if "LepMom" in sysName :
            self.unfold.drawFoldedHists(var, self.inHistDic['hist'], dirName, steering, useAxis, sysName, outName, massBin, binWidth, self.inHistDic['hist_lepScale'])
        else : 
            self.unfold.drawFoldedHists(var, self.inHistDic['hist'], dirName, steering, useAxis, sysName, outName, massBin, binWidth)

    def drawUnfPlot(self, var = "Mass", steering = None, useAxis = True, sysName = "", outName = "", massBin = 0, binWidth = False):
        self.unfold.drawUnfoldedHists(var, steering, useAxis, sysName, outName, massBin, binWidth)

    def drawSystematics(self, var = "Pt") :
        self.unfold.drawSystematics(var) 
    # Do unfold! 
    def doUnfold(self, doSystematic = False, doRegularize = False):
        self.unfold.doISRUnfold(doSystematic, doRegularize)

    def doStatUnfold(self):
        self.unfold.doStatUnfold()

    def setMeanValues(self):
        # Set mean mass and pt values
        self.nMassBins = self.unfold.setMeanMass()
        self.unfold.setMeanPt()

    def setAcceptMeanValues(self):
        self.unfold.setMeanPt_Accept()
        self.unfold.setMeanMass_Accept()

    def setSysMeanValues(self):
        self.unfold.setSysMeanMass()
        self.unfold.setSysMeanPt()

    def setAcceptSysMeanValues(self):
        self.unfold.setSysMeanMass_Accept()
        self.unfold.setSysMeanPt_Accept()

    def setStatError(self):
        self.unfold.setStatError()

    def setSysError(self):
        self.unfold.setSysError()

    def setAcceptSysError(self):
        self.unfold.setSysError_Accept()

    def setTotSysError(self):
        self.unfold.setTotSysError()

    def setAcceptTotSysError(self):
        self.unfold.setTotSysError_Accept()
    
    def getISRUnfold(self):
        
        return self.unfold
    
    def drawStatVar(self, isPt = True):

        for ibin in range(self.nMassBins): 
            self.unfold.drawStatVariation(isPt, ibin)

    def drawPDFVar(self, isPt = True):

        for ibin in range(self.nMassBins): 
            self.unfold.drawPDFVariation(isPt, ibin)

    def drawSysVar(self, sysName, var = "Pt"):

        for ibin in range(self.nMassBins): 
            self.unfold.drawSysVariation(sysName, var, ibin)

    def doAcceptance(self, doSys = False) :
        #self.unfold.doAcceptCorr(self.inHistDic['hist_accept'], self.binDef, doSys)
        self.unfold.doAcceptCorr(self.inHistDic['hist_accept'], "_FineCoarse", doSys)

    def drawAcceptPlot(self, var = "Mass", steering = None, useAxis = True, sysName = "", outName = "", massBin = 0, binWidth = False): 
        self.unfold.drawAcceptCorrHists(var, self.inHistDic['hist_accept'], self.binDef, steering, useAxis, sysName, outName, massBin, binWidth)  

    def drawCorrelation(self, var = "Mass", steering = None, useAxis = True, outName = ""):
        self.unfold.drawCorrelation(var, steering, useAxis, outName)

    def getUnfInHist(self, var = "Mass", steering = None, useAxis = True, sysName = "", outName = "", massBin = 0, binWidth = False):
        return self.unfold.getUnfInput(var, steering, useAxis, massBin, binWidth)

    def getGenMCHist(self, var = "Mass", steering = None, useAxis = True, sysName = "", outName = "", massBin = 0, binWidth = False):
        return self.unfold.getGenMCHist(var, steering, useAxis, massBin, binWidth)

    # Get histograms
    def getPtVsMassTGraph(self, grTitle = "", isUnfolded = True, isAccepted = False, doSys = False):
        meanMass, meanPt = array('d'), array('d')
        meanMassStatErr, meanPtStatErr = array('d'), array('d')
        meanMassSysErr, meanPtSysErr = array('d'), array('d')

        for ibin in range(self.nMassBins):
           
            if isUnfolded:
                meanMass.append(self.unfold.getUnfMeanMass(ibin))
                meanPt.append(self.unfold.getUnfMeanPt(ibin))
                meanMassStatErr.append(self.unfold.getUnfMeanMassError(ibin))
                meanPtStatErr.append(self.unfold.getUnfMeanPtError(ibin))
                if doSys :
                    meanMassSysErr.append(self.unfold.getUnfMeanMassSysError(ibin))
                    meanPtSysErr.append(self.unfold.getUnfMeanPtSysError(ibin))
            elif isAccepted:
                meanMass.append(self.unfold.getAccMeanMass(ibin))
                meanPt.append(self.unfold.getAccMeanPt(ibin))
                meanMassStatErr.append(self.unfold.getAccMeanMassError(ibin))
                meanPtStatErr.append(self.unfold.getAccMeanPtError(ibin))
                if doSys :
                    meanMassSysErr.append(self.unfold.getAccMeanMassSysError(ibin))
                    meanPtSysErr.append(self.unfold.getAccMeanPtSysError(ibin))
            else:
                meanMass.append(self.unfold.getDetMeanMass(ibin))
                meanPt.append(self.unfold.getDetMeanPt(ibin))
                meanMassStatErr.append(self.unfold.getDetMeanMassError(ibin))
                meanPtStatErr.append(self.unfold.getDetMeanPtError(ibin))
            
        if doSys == False:
            gr = rt.TGraphErrors(self.nMassBins, meanMass, meanPt, meanMassStatErr, meanPtStatErr)
        else :
            gr = rt.TGraphErrors(self.nMassBins, meanMass, meanPt, meanMassSysErr, meanPtSysErr)
    
        gr.SetName(grTitle)
        return gr
