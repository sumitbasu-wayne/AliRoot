/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  EMCAL tender, apply corrections to EMCAl clusters                        //
//  and do track matching                                                    //                                                                           
//  Author : Deepa Thomas (Utrecht University)                               //                      
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TInterpreter.h"
#include "TObjArray.h"
#include "TClonesArray.h"

#include <AliLog.h>
#include <AliESDEvent.h>
#include <AliAnalysisManager.h>
#include <AliTender.h>
#include "AliOADBContainer.h"
#include "AliCDBManager.h"
#include "AliCDBStorage.h"
#include "AliCDBEntry.h"
#include "AliMagF.h"
#include "TGeoGlobalMagField.h"

#include "AliESDCaloCluster.h"
#include "AliEMCALTenderSupply.h"

//EMCAL
#include "AliEMCALGeometry.h"
#include "AliEMCALRecoUtils.h"
#include "AliEMCALClusterizer.h"
#include "AliEMCALRecParam.h"
#include "AliEMCALRecPoint.h"
#include "AliEMCALAfterBurnerUF.h"
#include "AliEMCALClusterizerNxN.h"
#include "AliEMCALClusterizerv1.h"
#include "AliEMCALClusterizerv2.h"
#include "AliEMCALDigit.h"

ClassImp(AliEMCALTenderSupply)

// mfasel:
//  Remove all calls to TGrid::Connect - grid connection is global and better steer by the run macro

AliEMCALTenderSupply::AliEMCALTenderSupply() :
	AliTenderSupply()
	,fEMCALGeo(0x0)
	,fEMCALGeoName("EMCAL_FIRSTYEARV1")
//	,fEMCALRecoUtils(new AliEMCALRecoUtils)
	,fEMCALRecoUtils(0)
	,fConfigName("")
	,fDebugLevel(0)
	,fNonLinearFunc(AliEMCALRecoUtils::kNoCorrection) 
	,fNonLinearThreshold(30)      	
	,fReCalibCluster(kFALSE)	
	,fReCalibCell(kFALSE)	
	,fRecalClusPos(kFALSE)
	,fFiducial(kFALSE) 
	,fNCellsFromEMCALBorder(1)	
	,fRecalDistToBadChannels(kFALSE)	
	,fInputTree(0)	
	,fInputFile(0)
	,fFilepass(0) 
	,fMass(0.139)
	,fStep(1)
	,fRcut(0.05)	
  	,fBasePath(".")
	,fReClusterize(kFALSE)
	,fClusterizer(0)
	,fGeomMatrixSet(kFALSE)
	,fLoadGeomMatrices(kFALSE)
	,fRecParam(0)
	,fOCDBpath(" ")
	,fUnfolder(0)
	,fDigitsArr(0)
  	,fClusterArr(0)

{
	//
	// default ctor
	//
	for(Int_t i = 0; i < 10; i++) fEMCALMatrix[i] = 0 ;
	for(Int_t i = 0; i < 10; i++) fGeomMatrix[i] = 0;
	fRecParam        = new AliEMCALRecParam;
	fEMCALRecoUtils = new AliEMCALRecoUtils();
	fDigitsArr       = new TClonesArray("AliEMCALDigit",1000);

}

//_____________________________________________________
AliEMCALTenderSupply::AliEMCALTenderSupply(const char *name, const AliTender *tender) :
	AliTenderSupply(name,tender)
	,fEMCALGeo(0x0)
	,fEMCALGeoName("EMCAL_FIRSTYEARV1")
	,fEMCALRecoUtils(0)
	,fConfigName("") 
	,fDebugLevel(0)
	,fNonLinearFunc(AliEMCALRecoUtils::kNoCorrection)      	
	,fNonLinearThreshold(30)      	
	,fReCalibCluster(kFALSE)	
	,fReCalibCell(kFALSE)	
	,fRecalClusPos(kFALSE)
	,fFiducial(kFALSE) 
	,fNCellsFromEMCALBorder(1)	
	,fRecalDistToBadChannels(kFALSE)	
	,fInputTree(0)	
	,fInputFile(0)
	,fFilepass(0)
	,fMass(0.139)
	,fStep(1)
	,fRcut(0.05)
  	,fBasePath(".")
	,fReClusterize(kFALSE)
	,fClusterizer(0)
	,fGeomMatrixSet(kFALSE)
	,fLoadGeomMatrices(kFALSE)
	,fRecParam(0)
	,fOCDBpath(" ")
	,fUnfolder(0)
	,fDigitsArr(0)
  	,fClusterArr(0)

{
	//
	// named ctor
	//
	for(Int_t i = 0; i < 10; i++) fEMCALMatrix[i] = 0 ;
	for(Int_t i = 0; i < 10; i++) fGeomMatrix[i] = 0;
	fRecParam        = new AliEMCALRecParam;
	fEMCALRecoUtils = new AliEMCALRecoUtils();
	fDigitsArr       = new TClonesArray("AliEMCALDigit",200);
}

//_____________________________________________________
AliEMCALTenderSupply::~AliEMCALTenderSupply()
{
	//Destructor
	delete fEMCALGeo;
	delete fEMCALRecoUtils;
	delete fInputTree;
	delete fInputFile;
	delete fClusterizer;
	delete fUnfolder;

	if (fDigitsArr){
		fDigitsArr->Clear("C");
		delete fDigitsArr; 
	}

}

//_____________________________________________________
void AliEMCALTenderSupply::Init()
{
	//
	// Initialise EMCAL tender
	//

	if (fDebugLevel>0) AliInfo("Init EMCAL Tender supply\n");	

	AliAnalysisManager *mgr=AliAnalysisManager::GetAnalysisManager();

	fInputTree = mgr->GetTree();

	if(gROOT->LoadMacro(fConfigName) >=0){
    		AliDebug(1, Form("Loading settings from macro %s", fConfigName.Data()));
		AliEMCALTenderSupply *tender = (AliEMCALTenderSupply*)gInterpreter->ProcessLine("ConfigEMCALTenderSupply()");
		fDebugLevel         = tender->fDebugLevel;
		fEMCALGeoName       = tender->fEMCALGeoName; 
		fEMCALRecoUtils     = tender->fEMCALRecoUtils; 
		fConfigName         = tender->fConfigName;
		fNonLinearFunc      = tender->fNonLinearFunc;
		fNonLinearThreshold = tender->fNonLinearThreshold;
		fReCalibCluster     = tender->fReCalibCluster;
		fReCalibCell        = tender->fReCalibCell;
		fRecalClusPos       = tender->fRecalClusPos;
		fFiducial	    = tender->fFiducial;
		fNCellsFromEMCALBorder = tender->fNCellsFromEMCALBorder;
		fRecalDistToBadChannels = tender->fRecalDistToBadChannels;    
		fMass               = tender->fMass;
		fStep               = tender->fStep;
		fRcut               = tender->fRcut;
		fReClusterize       = tender->fReClusterize;
	        fLoadGeomMatrices   = tender->fLoadGeomMatrices;
		fRecParam           = tender->fRecParam;
		fOCDBpath           = tender->fOCDBpath;
		for(Int_t i = 0; i < 10; i++) fGeomMatrix[i] = tender->fGeomMatrix[i] ;
	}

	// Init goemetry	
	fEMCALGeo =  AliEMCALGeometry::GetInstance(fEMCALGeoName) ;


	//Initialising Non linearity parameters
	fEMCALRecoUtils->SetNonLinearityThreshold(fNonLinearThreshold);
	fEMCALRecoUtils->SetNonLinearityFunction(fNonLinearFunc);

	//Setting mass, step size and residual cut 
	fEMCALRecoUtils->SwitchOnCutEtaPhiSum(); 
	fEMCALRecoUtils->SetCutR(fRcut);
	fEMCALRecoUtils->SetMass(fMass);
	fEMCALRecoUtils->SetStep(fStep);

	//AliLog::SetGlobalDebugLevel(1);
	
	if(fDebugLevel>1) fEMCALRecoUtils->Print("");
		

}

//_____________________________________________________
void AliEMCALTenderSupply::ProcessEvent()
{
	//Event loop	
	AliESDEvent *event=fTender->GetEvent();
	if (!event) return;

	if(fTender->RunChanged()){ 
		//Initialising parameters once per run number
		if (!InitBadChannels()) return;
		if (fRecalClusPos){ if (!InitMisalignMatrix()) return;}
		if (fReCalibCluster || fReCalibCell){ if (!InitRecalib()) return;}
		if (fReClusterize) InitClusterization();
	}

	AliESDCaloCells *cells= event->GetEMCALCells();

	//------------ EMCAL cells loop ------------
	
	//Recalibrate cells
	if(fReCalibCell) RecalibrateCells();

	//------------ Reclusterize ------------
	if(fReClusterize){
		FillDigitsArray();
		Clusterize();
		UpdateClusters();
	}

	//------------Good clusters------------
	TClonesArray *clusArr;

	clusArr = dynamic_cast<TClonesArray*>(event->FindListObject("caloClusters"));
	if(!clusArr) clusArr = dynamic_cast<TClonesArray*>(event->FindListObject("CaloClusters"));
	if(!clusArr) return;

	Int_t nclusters = clusArr->GetEntriesFast(); //bug fix by Rongrong
	for (Int_t icluster=0; icluster < nclusters; icluster++ ){ 
		AliVCluster *clust = static_cast<AliVCluster*>(clusArr->At(icluster));
		if(!clust) continue;
		if (!clust->IsEMCAL()) continue;
		if(fEMCALRecoUtils->ClusterContainsBadChannel(fEMCALGeo, clust->GetCellsAbsId(), clust->GetNCells() )){
			delete clusArr->RemoveAt(icluster);
			continue;
		}
		if(fFiducial){
			if(!fEMCALRecoUtils->CheckCellFiducialRegion(fEMCALGeo, clust, cells)){
				delete clusArr->RemoveAt(icluster);
				continue;
			}
		}
		fEMCALRecoUtils->CorrectClusterEnergyLinearity(clust);
		if(fRecalDistToBadChannels) fEMCALRecoUtils->RecalculateClusterDistanceToBadChannel(fEMCALGeo, cells, clust);  
		if(fReCalibCluster) fEMCALRecoUtils->RecalibrateClusterEnergy(fEMCALGeo, clust, cells);
		if(fRecalClusPos) fEMCALRecoUtils->RecalculateClusterPosition(fEMCALGeo, cells, clust);
		//fEMCALRecoUtils->SetTimeDependentCorrections(event->GetRunNumber());
	}
	clusArr->Compress();

	
	//-------- Track Matching ------------------

	//set magnetic field
	Double_t  magF = event->GetMagneticField();
	Double_t magSign = 1.0;
	if(magF<0)magSign = -1.0;

	if (!TGeoGlobalMagField::Instance()->GetField()) 
	{
		AliMagF* field = new AliMagF("Maps","Maps", magSign, magSign, AliMagF::k5kG);
		TGeoGlobalMagField::Instance()->SetField(field);
	}


	fEMCALRecoUtils->FindMatches(event,0x0,fEMCALGeo);

	Int_t nTracks = event->GetNumberOfTracks();

	if(nTracks>0){
		SetClusterMatchedToTrack(event);
		SetTracksMatchedToCluster(event);
	}
	
}

//_____________________________________________________
void AliEMCALTenderSupply::SetClusterMatchedToTrack(AliESDEvent *event)
{
	//checks if tracks are matched to EMC clusters and set the matched EMCAL cluster index to ESD track. 

	Int_t matchClusIndex = -1;
	Int_t nTracks = event->GetNumberOfTracks();

	// Track loop 
	for (Int_t iTrack = 0; iTrack < nTracks; iTrack++) 
	{
		AliESDtrack* track = event->GetTrack(iTrack);
		if (!track) {
			printf("ERROR: Could not receive track %d\n", iTrack);
			continue;
		}

		matchClusIndex = fEMCALRecoUtils->GetMatchedClusterIndex(iTrack);		   
		track->SetEMCALcluster(matchClusIndex); //sets -1 if track not matched within residual
		if(matchClusIndex != -1) track->SetStatus(AliESDtrack::kEMCALmatch);
	}

	if (fDebugLevel>2) AliInfo("Track matched to closest cluster\n");	
}

//_____________________________________________________
void AliEMCALTenderSupply::SetTracksMatchedToCluster(AliESDEvent *event)
{
	//checks if EMC clusters are matched to ESD track.
	//Adds track indexes of all the tracks matched to a cluster withing residuals in ESDCalocluster

	Int_t matchTrackIndex = -1;
	Int_t nTracks = event->GetNumberOfTracks();

	// Cluster loop
	for (Int_t iClus=0; iClus < event->GetNumberOfCaloClusters(); iClus++)
	{
		AliESDCaloCluster * cluster = event->GetCaloCluster(iClus);
		if (!cluster->IsEMCAL()) continue;

		Int_t nMatched =0;
		TArrayI arrayTrackMatched(nTracks);

		//get the closest track matched to the cluster
		matchTrackIndex = fEMCALRecoUtils->GetMatchedTrackIndex(iClus); 
		arrayTrackMatched[nMatched] = matchTrackIndex;
		nMatched++;

		//get all other tracks matched to the cluster
		//track loop
		for(Int_t iTrk=0; iTrk<nTracks; iTrk++){
			AliESDtrack* track = event->GetTrack(iTrk);
			if(iTrk == matchTrackIndex) continue;

			if(track->GetEMCALcluster() == iClus){
				arrayTrackMatched[nMatched] = iTrk;
				nMatched++;
			}
		}

		arrayTrackMatched.Set(nMatched);
		cluster->AddTracksMatched(arrayTrackMatched);

		Float_t eta= -999, phi = -999;
		if(matchTrackIndex != -1) fEMCALRecoUtils->GetMatchedResiduals(iClus, eta, phi);
		cluster->SetTrackDistance(phi, eta);
	}
	if (fDebugLevel>2) AliInfo("Cluster matched to tracks \n");	
}

//_____________________________________________________
Bool_t AliEMCALTenderSupply::InitMisalignMatrix()
{
	//Initialising Misalignment matrix

	AliESDEvent *event=fTender->GetEvent();
	if (!event) return kFALSE;
	
	if (fDebugLevel>0) AliInfo("Initialising Misalignment matrix \n");	

	if(fInputTree){ 
		fInputFile = fInputTree->GetCurrentFile();
		if(fInputFile){
			const char *fileName = fInputFile->GetName();
			TString FileName = TString(fileName);
			if     (FileName.Contains("pass1")) fFilepass = TString("pass1");
			else if(FileName.Contains("pass2")) fFilepass = TString("pass2");
			else if(FileName.Contains("pass3")) fFilepass = TString("pass3");
			else AliError("pass number not found");
		}
		else AliError("File not found");
	}
	else AliError("Tree not found");

	Int_t runGM = 0;
	runGM = event->GetRunNumber();

	fEMCALRecoUtils->SetPositionAlgorithm(AliEMCALRecoUtils::kPosTowerGlobal);

	if(runGM <=140000){ //2010 data

		Double_t rotationMatrix[4][9] = {{-0.014587, -0.999892, -0.002031, 0.999892, -0.014591,  0.001979, -0.002009, -0.002002,  0.999996},
			{-0.014587,  0.999892,  0.002031, 0.999892,  0.014591, -0.001979, -0.002009,  0.002002, -0.999996},
			{-0.345864, -0.938278, -0.003412, 0.938276, -0.345874,  0.003010, -0.004004, -0.002161,  0.999990},
			{-0.345861,  0.938280,  0.003412, 0.938276,  0.345874, -0.003010, -0.004004,  0.002161, -0.999990}};

		Double_t translationMatrix[4][3] = {{0.351659,    447.576446,  176.269742},
			{1.062577,    446.893974, -173.728870},
			{-154.213287, 419.306156,  176.753692},
			{-153.018950, 418.623681, -173.243605}};

		for(Int_t mod=0; mod < (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules(); mod++)
		{
			//if(DebugLevel() > 1)  fEMCALMatrix[mod]->Print();
      			fEMCALMatrix[mod] = new TGeoHMatrix();           // mfasel: prevent tender from crashing
			fEMCALMatrix[mod]->SetRotation(rotationMatrix[mod]);
			fEMCALMatrix[mod]->SetTranslation(translationMatrix[mod]);		
			fEMCALGeo->SetMisalMatrix(fEMCALMatrix[mod],mod); 
		}
	}


	else if(runGM>140000 && runGM <148531 && (fFilepass = "pass1"))
	{ // 2011 LHC11a pass1 data
		AliOADBContainer emcalgeoCont(Form("emcal2011"));
		emcalgeoCont.InitFromFile(Form("%s/BetaGood.root",fBasePath.Data()),Form("AliEMCALgeo"));

		TObjArray *mobj=(TObjArray*)emcalgeoCont.GetObject(100,"survey11byS");

		for(Int_t mod=0; mod < (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules(); mod++){
			fEMCALMatrix[mod] = (TGeoHMatrix*) mobj->At(mod);
			fEMCALGeo->SetMisalMatrix(fEMCALMatrix[mod],mod); 

			fEMCALMatrix[mod]->Print();
		}
	}

	else AliInfo("MISALLIGNMENT NOT APPLIED");

	return kTRUE;
}

//_____________________________________________________
Bool_t AliEMCALTenderSupply::InitBadChannels()
{
	//Initialising Bad channel maps

	AliESDEvent *event=fTender->GetEvent();
	if (!event) return kFALSE;

	Int_t fRunBC= 0;
	fRunBC = event->GetRunNumber();

	if (fDebugLevel>0) AliInfo("Initialising Bad channel map \n");	

	if(fInputTree){ 
		fInputFile = fInputTree->GetCurrentFile();
		if(fInputFile){
			const char *fileName = fInputFile->GetName();
			TString FileName = TString(fileName);
			if     (FileName.Contains("pass1")) fFilepass = TString("pass1");
			else if(FileName.Contains("pass2")) fFilepass = TString("pass2");
			else if(FileName.Contains("pass3")) fFilepass = TString("pass3");
			else AliError("pass number not found");
		}
		else AliError("File not found");
	}
	else AliError("Tree not found");

	if(fFiducial){
		fEMCALRecoUtils->SetNumberOfCellsFromEMCALBorder(fNCellsFromEMCALBorder);
		fEMCALRecoUtils->SwitchOnNoFiducialBorderInEMCALEta0();
	}

	fEMCALRecoUtils->SwitchOnBadChannelsRemoval();
	if(fRecalDistToBadChannels) fEMCALRecoUtils->SwitchOnDistToBadChannelRecalculation();

	TFile *fbad;
	//2010 
	if(fRunBC <=140000){
		fbad = new TFile(Form("%s/BadChannels.root",fBasePath.Data()),"read");
		if (fbad->IsZombie()){
			TString fPath = TString(fBasePath.Data());
			if(fPath.Contains("alien")) {
				if (fDebugLevel>1) AliInfo("Connecting to alien to get BadChannels.root \n");	
				fbad = TFile::Open(Form("%s/BadChannelsDB/BadChannels.root",fBasePath.Data()));
			}
		}

		TH2I * hb0 = ( TH2I *)fbad->Get("EMCALBadChannelMap_Mod0");
		TH2I * hb1 = ( TH2I *)fbad->Get("EMCALBadChannelMap_Mod1");
		TH2I * hb2 = ( TH2I *)fbad->Get("EMCALBadChannelMap_Mod2");
		TH2I * hb3 = ( TH2I *)fbad->Get("EMCALBadChannelMap_Mod3"); 
		fEMCALRecoUtils->SetEMCALChannelStatusMap(0,hb0);
		fEMCALRecoUtils->SetEMCALChannelStatusMap(1,hb1);
		fEMCALRecoUtils->SetEMCALChannelStatusMap(2,hb2);
		fEMCALRecoUtils->SetEMCALChannelStatusMap(3,hb3); 
		
	}

	//2011
	Int_t nSupMod=-1, nModule=-1, nIphi=-1, nIeta=-1, iphi=-1, ieta=-1;

	if(fRunBC>=144871 && fRunBC<=146860){ //LHC11a 2.76 TeV pp

		if(fFilepass = "pass1"){ // pass1

			const Int_t nTowers=89;
			Int_t hotChannels[nTowers]={74, 103, 152, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 368, 369, 370, 371, 372, 373, 374,375, 376, 377, 378, 379, 380, 381, 382, 383, 917, 1275, 1288, 1519, 1595, 1860, 1967, 2022, 2026, 2047, 2117, 2298, 2540, 2776, 3135, 3764, 6095, 6111, 6481, 6592, 6800, 6801, 6802, 6803, 6804, 6805, 6806, 6807, 6808, 6809, 6810, 6811, 6812, 6813, 6814, 6815, 7371, 7425, 7430, 7457, 7491, 7709, 8352, 8353, 8356, 8357, 8808, 8810, 8812, 8814, 9056, 9769, 9815, 9837};
			for(Int_t i=0; i<nTowers; i++)
			{
				fEMCALGeo->GetCellIndex(hotChannels[i],nSupMod,nModule,nIphi,nIeta);

				fEMCALGeo->GetCellPhiEtaIndexInSModule(nSupMod,nModule,nIphi,nIeta,iphi,ieta);
				fEMCALRecoUtils->SetEMCALChannelStatus(nSupMod, ieta, iphi);
			}
		}

		if(fFilepass = "pass2"){ // pass2
			const Int_t nTowers=24;
			Int_t hotChannels[nTowers]= {74, 103, 152, 917, 1059, 1175, 1276, 1288, 1376, 1382, 1595, 2022, 2026, 2210, 2540, 2778, 2793, 3135, 3764, 5767, 6481, 7371, 7878, 9769};
			for(Int_t i=0; i<nTowers; i++)
			{
				fEMCALGeo->GetCellIndex(hotChannels[i],nSupMod,nModule,nIphi,nIeta);

				fEMCALGeo->GetCellPhiEtaIndexInSModule(nSupMod,nModule,nIphi,nIeta,iphi,ieta);
				fEMCALRecoUtils->SetEMCALChannelStatus(nSupMod, ieta, iphi);
			}
		}

	}

	if(fRunBC>=151636 && fRunBC<=155384) //LHC11c : 7TeV by Rongrong
	{
		const Int_t nTowers=8;
		Int_t hotChannels[nTowers]={917, 2115, 2123, 2540, 6481, 9815, 10113, 10115};

		for(Int_t i=0; i<nTowers; i++)
		{
			fEMCALGeo->GetCellIndex(hotChannels[i],nSupMod,nModule,nIphi,nIeta);
			fEMCALGeo->GetCellPhiEtaIndexInSModule(nSupMod,nModule,nIphi,nIeta,iphi,ieta);
			fEMCALRecoUtils->SetEMCALChannelStatus(nSupMod, ieta, iphi);
		}		
	}


	return kTRUE;
}

//_____________________________________________________
Bool_t AliEMCALTenderSupply::InitRecalib()
{
	//Initialising Recalibration Factors

	AliESDEvent *event=fTender->GetEvent();
	if (!event) return kFALSE;

	if (fDebugLevel>0) AliInfo("Initialising Recalibration factors \n");

	if(fInputTree){ 
		fInputFile = fInputTree->GetCurrentFile();
		if(fInputFile){
			const char *fileName = fInputFile->GetName();
			TString FileName = TString(fileName);
			if     (FileName.Contains("pass1")) fFilepass = TString("pass1");
			else if(FileName.Contains("pass2")) fFilepass = TString("pass2");
			else if(FileName.Contains("pass3")) fFilepass = TString("pass3");
			else AliError("pass number not found");
		}
		else AliError("File not found");
	}
	else AliError("Tree not found");
	//        else {cout << "Tree not found " <<endl; return kFALSE;}

	Int_t runRC = event->GetRunNumber();

	//if (event->GetRunNumber()==runRC)
	//     return kFALSE;

	fEMCALRecoUtils->SwitchOnRecalibration();

	TFile* fRecalib;

	if(runRC <=140000){
		fRecalib = new TFile(Form("%s/RecalibrationFactors.root",fBasePath.Data()),"read");
		if (fRecalib->IsZombie()){
			TString fPath = TString(fBasePath.Data());
			if(fPath.Contains("alien")) {

				if (fDebugLevel>1) AliInfo("Connecting to alien to get RecalibrationFactors.root \n");	

				if(     (runRC >= 114737 && runRC <= 117223 && (fFilepass = "pass2")) || //LHC10b pass2 
						(runRC >= 118503 && runRC <= 121040 && ((fFilepass = "pass2")||(fFilepass = "pass3"))) || //LHC10c pass2, LHC10c pass3
						(runRC >= 122195 && runRC <= 126437 && (fFilepass = "pass1")) || //LHC10d pass1
						(runRC >= 127712 && runRC <= 130850 && (fFilepass = "pass1")) || //LHC10e pass1
						(runRC >= 133004 && runRC < 134657  && (fFilepass = "pass1")))// LHC10f pass1 <134657
				{
					fRecalib = TFile::Open(Form("%s/RecalDB/summer_december_2010/RecalibrationFactors.root",fBasePath.Data()));
				}

				else if((runRC >= 122195 && runRC <= 126437 && (fFilepass = "pass2")) || //LHC10d pass2
						(runRC >= 134657 && runRC <= 135029 && (fFilepass = "pass1")) || //LHC10f pass1 >= 134657
						(runRC >= 135654 && runRC <= 136377 && (fFilepass = "pass1")) || //LHC10g pass1
						(runRC >= 136851 && runRC < 137231  && (fFilepass = "pass1"))) //LHC10h pass1 untill christmas
				{
					fRecalib = TFile::Open(Form("%s/RecalDB/december2010/RecalibrationFactors.root",fBasePath.Data()));
				}

				else if(runRC >= 137231 && runRC <= 139517 && (fFilepass = "pass1")) //LHC10h pass1 from christmas
				{
					fRecalib = TFile::Open(Form("%s/RecalDB/summer2010/RecalibrationFactors.root",fBasePath.Data()));
				}
				else {
					AliError("Run number or pass number not found; RECALIBRATION NOT APPLIED");
				}

			}
		}

		TH2F * r0 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM0");
		TH2F * r1 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM1");
		TH2F * r2 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM2");
		TH2F * r3 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM3");
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(0,r0);
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(1,r1);
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(2,r2);
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(3,r3);
	}

	if(runRC > 140000){
		fRecalib = new TFile(Form("%s/RecalibrationFactors.root",fBasePath.Data()),"read");
		if (fRecalib->IsZombie()){
			TString fPath = TString(fBasePath.Data());
			if(fPath.Contains("alien")) {
				if (fDebugLevel>1) AliInfo("Connecting to alien to get RecalibrationFactors.root \n");

				fRecalib = TFile::Open(Form("%s/RecalDB/2011_v0/RecalibrationFactors.root",fBasePath.Data()));
				if(!fRecalib) AliError("Recalibration file not found");
			}
		}

		TH2F * r0 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM0");
		TH2F * r1 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM1");
		TH2F * r2 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM2");
		TH2F * r3 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM3");
		TH2F * r4 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM4");
		TH2F * r5 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM5");
		TH2F * r6 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM6");
		TH2F * r7 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM7");
		TH2F * r8 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM8");
		TH2F * r9 = ( TH2F *)fRecalib->Get("EMCALRecalFactors_SM9");

		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(0,r0);
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(1,r1);
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(2,r2);
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(3,r3); 
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(4,r4); 
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(5,r5); 
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(6,r6); 
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(7,r7); 
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(8,r8); 
		fEMCALRecoUtils->SetEMCALChannelRecalibrationFactors(9,r9); 

	}

	return kTRUE;
}

//_____________________________________________________
void AliEMCALTenderSupply::RecalibrateCells()
{
	AliESDCaloCells *cells = fTender->GetEvent()->GetEMCALCells();

	Int_t nEMCcell = cells->GetNumberOfCells();
	Double_t calibFactor = 1.;

	for(Int_t icell=0; icell<nEMCcell; icell++){
		Int_t imod = -1, iphi =-1, ieta=-1,iTower = -1, iIphi = -1, iIeta = -1; 
		fEMCALGeo->GetCellIndex(cells->GetCellNumber(icell),imod,iTower,iIphi,iIeta);
		fEMCALGeo->GetCellPhiEtaIndexInSModule(imod,iTower,iIphi, iIeta,iphi,ieta);	
		calibFactor = fEMCALRecoUtils->GetEMCALChannelRecalibrationFactor(imod,ieta,iphi);
		cells->SetCell(icell,cells->GetCellNumber(icell),cells->GetAmplitude(icell)*calibFactor,cells->GetTime(icell));	
	}	

}

//_____________________________________________________
void AliEMCALTenderSupply::InitClusterization()
{
	//Initialing clusterization/unfolding algorithm and set all the needed parameters
	AliESDEvent *event=fTender->GetEvent();
	if (!event){ 
		AliWarning("Event not available!!!");
		return;
	}

	if (fDebugLevel>0) AliInfo(Form("Initialising Reclustering parameters: Clusterizer-%d \n",fRecParam->GetClusterizerFlag()));

	//---set the geometry matrix

	if (!fGeomMatrixSet) {
		if (fLoadGeomMatrices) {
			for(Int_t mod=0; mod < fEMCALGeo->GetNumberOfSuperModules(); ++mod) {
				if(fGeomMatrix[mod]){
					if(fDebugLevel > 2) fGeomMatrix[mod]->Print();
					fEMCALGeo->SetMisalMatrix(fGeomMatrix[mod],mod);  
				}
			}
		} else { // get matrix from file (work around bug in aliroot)
			for(Int_t mod=0; mod < (fEMCALGeo->GetEMCGeometry())->GetNumberOfSuperModules(); mod++){
				if(fDebugLevel > 2) event->GetEMCALMatrix(mod)->Print();
				if(event->GetEMCALMatrix(mod)) fEMCALGeo->SetMisalMatrix(event->GetEMCALMatrix(mod),mod) ;
			} 
		}
		fGeomMatrixSet=kTRUE;
	}

	//---setup clusterizer

	delete fClusterizer;
	if     (fRecParam->GetClusterizerFlag() == AliEMCALRecParam::kClusterizerv1)
		fClusterizer = new AliEMCALClusterizerv1 (fEMCALGeo);
	else if(fRecParam->GetClusterizerFlag() == AliEMCALRecParam::kClusterizerv2) 
		fClusterizer = new AliEMCALClusterizerv2(fEMCALGeo);
	else if(fRecParam->GetClusterizerFlag() == AliEMCALRecParam::kClusterizerNxN) 
		fClusterizer = new AliEMCALClusterizerNxN(fEMCALGeo);
	else if(fRecParam->GetClusterizerFlag() > AliEMCALRecParam::kClusterizerv2) { //FIX this other way.
		AliEMCALClusterizerNxN *clusterizer = new AliEMCALClusterizerNxN(fEMCALGeo);
		clusterizer->SetNRowDiff(2);
		clusterizer->SetNColDiff(2);
		fClusterizer = clusterizer;
	} else{
		AliFatal(Form("Clusterizer < %d > not available", fRecParam->GetClusterizerFlag()));
	}

	//--- set the parameters

	fClusterizer->SetECAClusteringThreshold( fRecParam->GetClusteringThreshold() );
	fClusterizer->SetECALogWeight          ( fRecParam->GetW0()                  );
	fClusterizer->SetMinECut               ( fRecParam->GetMinECut()             );    
	fClusterizer->SetUnfolding             ( fRecParam->GetUnfold()              );
	fClusterizer->SetECALocalMaxCut        ( fRecParam->GetLocMaxCut()           );
	fClusterizer->SetTimeCut               ( fRecParam->GetTimeCut()             );
	fClusterizer->SetTimeMin               ( fRecParam->GetTimeMin()             );
	fClusterizer->SetTimeMax               ( fRecParam->GetTimeMax()             );
	fClusterizer->SetInputCalibrated       ( kTRUE                               );
	fClusterizer->SetJustClusters          ( kTRUE                               );  

	//In case of unfolding after clusterization is requested, set the corresponding parameters
	if(fRecParam->GetUnfold()){
		Int_t i=0;
		for (i = 0; i < 8; i++) {
			fClusterizer->SetSSPars(i, fRecParam->GetSSPars(i));
		}//end of loop over parameters
		for (i = 0; i < 3; i++) {
			fClusterizer->SetPar5  (i, fRecParam->GetPar5(i));
			fClusterizer->SetPar6  (i, fRecParam->GetPar6(i));
		}//end of loop over parameters

		fClusterizer->InitClusterUnfolding();
	}// to unfold

	fClusterizer->SetDigitsArr(fDigitsArr);
	fClusterizer->SetOutput(0);
	fClusterArr = const_cast<TObjArray *>(fClusterizer->GetRecPoints());
}
//_____________________________________________________
void AliEMCALTenderSupply::FillDigitsArray()
{
	//Fill digits from cells to a TClonesArray
	fDigitsArr->Clear("C");
	
	AliESDEvent *event=fTender->GetEvent();
	if (!event){ 
		AliWarning("Event not available!!!");
		return;
	}
	AliESDCaloCells *cells = event->GetEMCALCells();
	Int_t ncells = cells->GetNumberOfCells();
	for (Int_t icell = 0, idigit = 0; icell < ncells; ++icell) {
		Double_t cellAmplitude=0, cellTime=0;
		Short_t cellNumber=0;
		if (cells->GetCell(icell, cellNumber, cellAmplitude, cellTime) != kTRUE)
			break;
		AliEMCALDigit *digit = static_cast<AliEMCALDigit*>(fDigitsArr->New(idigit));
		digit->SetId(cellNumber);
		digit->SetTime(cellTime);
		digit->SetTimeR(cellTime);
		digit->SetIndexInList(idigit);
		digit->SetType(AliEMCALDigit::kHG);
		digit->SetAmplitude(cellAmplitude);
		idigit++;
	}
}

//_____________________________________________________
void AliEMCALTenderSupply::Clusterize()
{
	//Clusterize
	fClusterizer->Digits2Clusters("");
}

//_____________________________________________________
void AliEMCALTenderSupply::UpdateClusters()
{
	//Update ESD cluster list
	AliESDEvent *event=fTender->GetEvent();
	if (!event){ 
		AliWarning("Event not available!!!");
		return;
	}

	TClonesArray *clus;
	clus = dynamic_cast<TClonesArray*>(event->FindListObject("caloClusters"));
	if(!clus) clus = dynamic_cast<TClonesArray*>(event->FindListObject("CaloClusters"));
	if(!clus) return;

	Int_t nents = clus->GetEntriesFast();
	for (Int_t i=0;i<nents;++i) {
		AliESDCaloCluster *c = static_cast<AliESDCaloCluster*>(clus->At(i));
		if (!c)
			continue;
		if (c->IsEMCAL())
			clus->RemoveAt(i);
	}
	clus->Compress();
	RecPoints2Clusters(clus);
}

//_____________________________________________________
void AliEMCALTenderSupply::RecPoints2Clusters(TClonesArray *clus)
{
	// Convert AliEMCALRecoPoints to AliESDCaloClusters
	// Cluster energy, global position, cells and their amplitude fractions are restored.
	Bool_t esdobjects = 0;
	if (strcmp(clus->GetClass()->GetName(),"AliESDCaloCluster")==0)
		esdobjects = 1;

	AliESDEvent *event=fTender->GetEvent();
	if (!event){ 
		AliWarning("Event not available!!!");
		return;
	}
	
	Int_t ncls = fClusterArr->GetEntriesFast();
	for(Int_t i=0, nout=clus->GetEntriesFast(); i < ncls; ++i) {
		AliEMCALRecPoint *recpoint = static_cast<AliEMCALRecPoint*>(fClusterArr->At(i));

		Int_t ncells_true = 0;
		const Int_t ncells = recpoint->GetMultiplicity();
		UShort_t   absIds[ncells];  
		Double32_t ratios[ncells];
		Int_t *dlist = recpoint->GetDigitsList();
		Float_t *elist = recpoint->GetEnergiesList();
		for (Int_t c = 0; c < ncells; ++c) {
			AliEMCALDigit *digit = static_cast<AliEMCALDigit*>(fDigitsArr->At(dlist[c]));
			absIds[ncells_true] = digit->GetId();
			ratios[ncells_true] = elist[c]/digit->GetAmplitude();
			if (ratios[ncells_true] < 0.001) 
				continue;
			++ncells_true;
		}

		if (ncells_true < 1) {
			AliWarning("Skipping cluster with no cells");
			continue;
		}

		// calculate new cluster position
		TVector3 gpos;
		recpoint->GetGlobalPosition(gpos);
		Float_t g[3];
		gpos.GetXYZ(g);

		AliESDCaloCluster *c = static_cast<AliESDCaloCluster*>(clus->New(nout++));
		c->SetType(AliVCluster::kEMCALClusterv1);
		c->SetE(recpoint->GetEnergy());
		c->SetPosition(g);
		c->SetNCells(ncells_true);
		c->SetDispersion(recpoint->GetDispersion());
		c->SetEmcCpvDistance(-1);            //not yet implemented
		c->SetChi2(-1);                      //not yet implemented
		c->SetTOF(recpoint->GetTime()) ;     //time-of-flight
		c->SetNExMax(recpoint->GetNExMax()); //number of local maxima
		Float_t elipAxis[2];
		recpoint->GetElipsAxis(elipAxis);
		c->SetM02(elipAxis[0]*elipAxis[0]) ;
		c->SetM20(elipAxis[1]*elipAxis[1]) ;

		AliESDCaloCluster *cesd = static_cast<AliESDCaloCluster*>(c);
		cesd->SetCellsAbsId(absIds);
		cesd->SetCellsAmplitudeFraction(ratios);
	}

}

