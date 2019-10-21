import argparse
import os
import sys
import ROOT as rt

# TODO make seperate helper module python file
def setUnfoldBkgs(unfold_class, hfile_path, syst_name, isSys, nthSys, nTotSys):
    
    unfold_class.subBkgs("Pt", hfile_path, "DYJetsToTauTau",       isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Pt", hfile_path, "DYJets10to50ToTauTau", isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Pt", hfile_path, "TTLL_powheg",          isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Pt", hfile_path, "WW_pythia",            isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Pt", hfile_path, "WZ_pythia",            isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Pt", hfile_path, "ZZ_pythia",            isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Pt", hfile_path, "WJets_MG",             isSys, syst_name, nTotSys, nthSys, "detector_level")
    
    unfold_class.subBkgs("Mass", hfile_path, "DYJetsToTauTau",       isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Mass", hfile_path, "DYJets10to50ToTauTau", isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Mass", hfile_path, "TTLL_powheg",          isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Mass", hfile_path, "WW_pythia",            isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Mass", hfile_path, "WZ_pythia",            isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Mass", hfile_path, "ZZ_pythia",            isSys, syst_name, nTotSys, nthSys, "detector_level")
    unfold_class.subBkgs("Mass", hfile_path, "WJets_MG",             isSys, syst_name, nTotSys, nthSys, "detector_level")

parser = argparse.ArgumentParser(description='Unfolding for ISR analysis')

parser.add_argument('--channel' , dest = 'channel', default = 'electron', help = 'select channel electron or muon')
parser.add_argument('--year' , dest = 'year', default = '2016', help = 'select year')
parser.add_argument('--phase_space_detector' , dest = 'phase_space_detector', default = 'fiducial_phase_pre_FSR_dRp1', help = 'select unfolded phase space')
parser.add_argument('--FSR_dR_detector' , dest = 'FSR_dR_detector', default = 'dressed_dRp1', help = 'select size of dR for dressed lepton')
parser.add_argument('--phase_space_fsr' , dest = 'phase_space_fsr', default = 'full_phase_dRp1', help = 'select unfolded phase space')
parser.add_argument('--FSR_dR_fsr' , dest = 'FSR_dR_fsr', default = 'dRp1_pre_fsr', help = 'select size of dR for dressed lepton')
parser.add_argument('--createInputHists'  , action='store_true'  , help = 'create input histograms')
parser.add_argument('--getUnfoldResults'  , action='store_true'  , help = 'Get unfolding resutls')
parser.add_argument('--doSys'  , action='store_true'  , default = False, help = 'Calculate systematics')

# TODO options
parser.add_argument('--getCombinedResults'  , action='store_true'  , help = 'Combine 2016 and 2017')
parser.add_argument('--getEMuCombinedResults'  , action='store_true'  , help = 'Combine electron and muon')

args = parser.parse_args()

# get input root files information using sampleDef.py
import etc.sampleDef as isrSamples
import pyScripts.unfoldInputUtil as histUtil

# set output directory
outputDirectory = 'output/'+args.year+'/' + args.channel + "/"  
inputfhisttxtName = outputDirectory + "fhist.txt"

# make output directory
if not os.path.exists( outputDirectory ):
	os.makedirs( outputDirectory )

print "channel to run: " + args.channel

if args.getUnfoldResults:
        # read text file including root file path for unfolding
        fOutTxtCheck = open( inputfhisttxtName,'r')
        unfoldInputList = {}

        for line in fOutTxtCheck:
                modifiedLine = line.lstrip(' ').rstrip(' ').rstrip('\n')
                if modifiedLine.split()[1] == "matrix":
                        unfoldInputList['matrix'] = modifiedLine.split()[2]

                if modifiedLine.split()[1] == "fsr_matrix":
                        unfoldInputList['fsr_matrix'] = modifiedLine.split()[2]

                if modifiedLine.split()[1] == "fsr_photos_matrix":
                        unfoldInputList['fsr_photos_matrix'] = modifiedLine.split()[2]

                if modifiedLine.split()[1] == "fsr_pythia_matrix":
                        unfoldInputList['fsr_pythia_matrix'] = modifiedLine.split()[2]

                if modifiedLine.split()[1] == "hist":
                        unfoldInputList['hist'] = modifiedLine.split()[2]

        print unfoldInputList

        import pyScripts.unfoldUtil as unfoldutil
        import pyScripts.drawUtil as drawutil

        # create unfold class
        unfoldClass = rt.ISRUnfold(args.channel, unfoldInputList['hist'], False, int(args.year))

        # set response matrix
        unfoldClass.setNomTUnfoldDensity("Pt",  unfoldInputList['matrix'], args.phase_space_detector, args.FSR_dR_detector)
        unfoldClass.setNomTUnfoldDensity("Mass",unfoldInputList['matrix'], args.phase_space_detector, args.FSR_dR_detector)

        # for closure test
        unfoldClass.setSysTUnfoldDensity("Pt",   unfoldInputList['matrix'],  "Closure", -1, -1, args.phase_space_detector, args.FSR_dR_detector)
        unfoldClass.setSysTUnfoldDensity("Mass", unfoldInputList['matrix'],  "Closure", -1, -1, args.phase_space_detector, args.FSR_dR_detector)

        unfoldClass.setInput("Pt",   unfoldInputList['matrix'], True, "Closure", 0, 1., args.phase_space_detector)
        unfoldClass.setInput("Mass", unfoldInputList['matrix'], True, "Closure", 0, 1., args.phase_space_detector)

        # set unfolding input histogram
	unfoldClass.setInput("Pt",   unfoldInputList['hist'], False, "nominal", 0, 1., "detector_level")
	unfoldClass.setInput("Mass", unfoldInputList['hist'], False, "nominal", 0, 1., "detector_level")
        setUnfoldBkgs(unfoldClass, unfoldInputList['hist'], "nominal", False, 0, -1) 

        # set systematic response matrix and input histograms
	if args.channel == "electron" : sysDict = {"PU": 2, "trgSF": 2, "recoSF": 2, "IdSF": 2, "L1Prefire": 2, "AlphaS": 2, "Scale": 6, "PDFerror": 101, "unfoldBias": 1, "unfoldScan": 1}
        if args.channel == "muon" :     sysDict = {"PU": 2, "trgSF": 2, "IsoSF": 2, "IdSF": 2, "L1Prefire": 2, "AlphaS": 2, "Scale": 6, "PDFerror": 101, "unfoldBias": 1, "unfoldScan": 1}
        if args.doSys == True:
	    #sysDict = {"PU": 2, "trgSF": 2, "recoSF": 2, "IdSF": 2, "IsoSF": 2, "unfoldsys": 1, "AlphaS": 2, "Scale": 9, "PDFerror": 100, "Alt": 1, "L1Prefire": 2, "LepScale": 2, "LepRes": 2, "FSRDR": 30, "unfoldBias": 1, "unfoldScan": 1}

	    for sysName, nSys in sysDict.items():
	    	for nthSys in range(0,nSys):
                    
                    print "sysName: " + sysName + " nthSys: " + str(nthSys) + " #####################################################"
                    
                    # set systematic response matrix
                    unfoldClass.setSysTUnfoldDensity("Pt",  unfoldInputList['matrix'],  sysName, nSys, nthSys, args.phase_space_detector, args.FSR_dR_detector)
                    unfoldClass.setSysTUnfoldDensity("Mass",unfoldInputList['matrix'],  sysName, nSys, nthSys, args.phase_space_detector, args.FSR_dR_detector)
                    
                    bias = 1.;
                    if sysName == "unfoldBias": bias = 0.95 
                    
                    # set systematic input histograms
                    unfoldClass.setInput("Pt",   unfoldInputList['hist'], True, sysName, nthSys, bias, "detector_level")
                    unfoldClass.setInput("Mass", unfoldInputList['hist'], True, sysName, nthSys, bias, "detector_level")
                    
                    # set systematic background histograms
                    setUnfoldBkgs(unfoldClass, unfoldInputList['hist'], sysName, True, nthSys, nSys)

	# unfold 
	unfoldClass.doISRUnfold(args.doSys)

        if args.doSys == True:
            unfoldClass.drawLCurve(outputDirectory + "LCurve_" + args.channel + ".pdf", "Pt")
            unfoldClass.drawLCurve(outputDirectory + "LCurveMass_" + args.channel + ".pdf", "Mass")

            unfoldClass.drawRhoLog(outputDirectory + "RhoLog_" + args.channel + ".pdf", "Pt")
            unfoldClass.drawRhoLog(outputDirectory + "RhoLogMass_" + args.channel + ".pdf", "Mass")

        # set QED FSR unfolding response matrix and input
        unfoldClass.setNomFSRTUnfoldDensity("Pt",    unfoldInputList['fsr_matrix'], args.phase_space_fsr, args.FSR_dR_fsr)
        unfoldClass.setNomFSRTUnfoldDensity("Mass",  unfoldInputList['fsr_matrix'], args.phase_space_fsr, args.FSR_dR_fsr)
        unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], False, "", -1, args.phase_space_fsr)
        
        if args.doSys == True:
            # systematic from detector unfolding
            for sysName, nSys in sysDict.items():
                for nthSys in range(0,nSys):
                    unfoldClass.setSysFSRTUnfoldDensity("Pt",   unfoldInputList['fsr_matrix'], sysName, nSys, nthSys, args.phase_space_fsr, args.FSR_dR_fsr) 
                    unfoldClass.setSysFSRTUnfoldDensity("Mass", unfoldInputList['fsr_matrix'], sysName, nSys, nthSys, args.phase_space_fsr, args.FSR_dR_fsr) 

                    unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], True, sysName, nthSys)

        # QED FSR ststematic
        unfoldClass.setSysFSRTUnfoldDensity("Pt",   unfoldInputList['fsr_photos_matrix'], "QED_FSR", 2, 0, args.phase_space_fsr, args.FSR_dR_fsr)
        unfoldClass.setSysFSRTUnfoldDensity("Mass", unfoldInputList['fsr_photos_matrix'], "QED_FSR", 2, 0, args.phase_space_fsr, args.FSR_dR_fsr)

        unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], True, "QED_FSR", 0)
        unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], True, "QED_FSR", 0)

        unfoldClass.setSysFSRTUnfoldDensity("Pt",   unfoldInputList['fsr_pythia_matrix'], "QED_FSR", 2, 1, args.phase_space_fsr, args.FSR_dR_fsr)
        unfoldClass.setSysFSRTUnfoldDensity("Mass", unfoldInputList['fsr_pythia_matrix'], "QED_FSR", 2, 1, args.phase_space_fsr, args.FSR_dR_fsr)

        unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], True, "QED_FSR", 1)
        unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], True, "QED_FSR", 1)
        
        unfoldClass.setSysFSRTUnfoldDensity("Pt",   unfoldInputList['fsr_matrix'], "QED_FSR_dRp1", 1, 0, args.phase_space_fsr, args.FSR_dR_fsr)
        unfoldClass.setSysFSRTUnfoldDensity("Mass", unfoldInputList['fsr_matrix'], "QED_FSR_dRp1", 1, 0, args.phase_space_fsr, args.FSR_dR_fsr)

        unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], True, "QED_FSR_dRp1", 0)
        unfoldClass.setFSRUnfoldInput(unfoldInputList['fsr_matrix'], True, "QED_FSR_dRp1", 0)

        # unfolding for QED FSR
        unfoldClass.doISRQEDFSRUnfold(True)

        # set nominal value and also systematic values
        unfoldClass.setMeanPt(args.doSys, False, args.doSys)
        unfoldClass.setMeanMass(args.doSys, False, args.doSys)
        #unfoldClass.setMCPreFSRMeanValues(unfoldInputList['fsr_matrix'])

        for massBin in range(0,5):
             
            if args.doSys:
                unfoldClass.drawClosurePlots(outputDirectory + "Closure_"+args.channel, "Pt", massBin)
                unfoldClass.drawClosurePlots(outputDirectory + "Closure_"+args.channel, "Mass", massBin)

            unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel, "Pt", massBin)
            unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel, "Mass", massBin)

        for massBin in range(0,5):

           unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel, "Pt",   massBin, "", False, True)
           unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel, "Mass", massBin, "", False, True)

        # draw plots including systematic 
        if args.doSys == True:
            for sysName, nSys in sysDict.items():
            
                if sysName == "Closure" : continue
                
                for massBin in range(0,5):
                    
                    unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel+sysName, "Pt", massBin, sysName, args.doSys, True)
                    unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel+sysName, "Mass", massBin, sysName, args.doSys, True)
                    unfoldClass.drawInputPlots(outputDirectory + args.channel + sysName, "Pt", massBin, sysName)
                    unfoldClass.drawSysPlots(outputDirectory + "Sys_" + args.channel , massBin, sysName, True)
                    unfoldClass.drawSysPlots(outputDirectory + "Sys_" + args.channel , massBin, sysName, False)

                for massBin in range(0,5):

                    unfoldClass.drawSysPlots(outputDirectory + "Sys_" + args.channel , massBin, "QED_FSR", False)
                    unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel+"QED_FSR", "Pt", massBin, "QED_FSR", args.doSys, True)
                    unfoldClass.drawNominalPlots(outputDirectory + "Unfolded_"+args.channel+"QED_FSR", "Mass", massBin, "QED_FSR", args.doSys, True)
             


	unfoldClass.drawISRresult(outputDirectory + "ISRfit_", False, False)
        unfoldClass.drawISRMatrixInfo("Pt", outputDirectory, True)
        unfoldClass.drawISRMatrixInfo("Pt", outputDirectory, False)

        unfoldClass.drawISRMatrixInfo("Mass", outputDirectory, True)
        unfoldClass.drawISRMatrixInfo("Mass", outputDirectory, False)

        unfoldClass.drawISRMatrixInfo("Pt", outputDirectory, False, True)
        unfoldClass.drawISRMatrixInfo("Mass", outputDirectory, False, True)


	del unfoldClass

def makeRecoPlots():
        pass
# test 
if __name__ == "__main__": 
	makeRecoPlots()
