// $Id$

//**************************************************************************
//* This file is property of and copyright by the ALICE HLT Project        * 
//* ALICE Experiment at CERN, All rights reserved.                         *
//*                                                                        *
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *
//*                  for The ALICE HLT Project.                            *
//*                                                                        *
//* Permission to use, copy, modify and distribute this software and its   *
//* documentation strictly for non-commercial purposes is hereby granted   *
//* without fee, provided that the above copyright notice appears in all   *
//* copies and that both the copyright notice and this permission notice   *
//* appear in the supporting documentation. The authors make no claims     *
//* about the suitability of this software for any purpose. It is          *
//* provided "as is" without express or implied warranty.                  *
//**************************************************************************

//  @file   AliHLTGlobalEsdConverterComponent.cxx
//  @author Matthias Richter
//  @date   
//  @brief  Global ESD converter component.
// 

// see header file for class documentation
// or
// refer to README to build package
// or
// visit http://web.ift.uib.no/~kjeks/doc/alice-hlt

#include <cassert>
#include "AliHLTGlobalEsdConverterComponent.h"
#include "AliHLTGlobalBarrelTrack.h"
#include "AliHLTExternalTrackParam.h"
#include "AliHLTTrackMCLabel.h"
#include "AliHLTCTPData.h"
#include "AliHLTTPCDefinitions.h"
#include "AliHLTTPCSpacePointData.h"
#include "AliHLTTPCClusterDataFormat.h"
#include "AliTPCclusterMI.h"
#include "AliTPCseed.h"
#include "AliITStrackV2.h"
#include "AliESDfriend.h"
#include "AliESDfriendTrack.h"
#include "AliHLTTPCGeometry.h"
#include "AliHLTErrorGuard.h"
#include "AliESDEvent.h"
#include "AliESDtrack.h"
#include "AliESDMuonTrack.h"
#include "AliESDMuonCluster.h"
#include "AliCDBEntry.h"
#include "AliCDBManager.h"
#include "AliPID.h"
#include "TTree.h"
#include "TList.h"
#include "TClonesArray.h"
#include "TTimeStamp.h"
#include "AliHLTESDCaloClusterMaker.h"
#include "AliHLTCaloClusterDataStruct.h"
#include "AliHLTCaloClusterReader.h"
#include "AliHLTCaloTriggerDataStruct.h"
#include "AliHLTCaloTriggerHeaderStruct.h"
#include "AliESDCaloCluster.h"
#include "AliESDCaloTrigger.h"
#include "AliESDVZERO.h"
#include "AliHLTGlobalVertexerComponent.h"
#include "AliHLTVertexFinderBase.h"
#include "AliSysInfo.h"
#include "AliHLTSAPTrackerData.h"
#include "AliFlatESDVertex.h"

/** ROOT macro for the implementation of ROOT specific class methods */
ClassImp(AliHLTGlobalEsdConverterComponent)

AliHLTGlobalEsdConverterComponent::AliHLTGlobalEsdConverterComponent()
  : AliHLTProcessor()
  , fWriteTree(0)
  , fVerbosity(0)
  , fESD(NULL)
  , fESDfriend(NULL)
  , fScaleDownTracks(0)
  , fSolenoidBz(-5.00668)
  , fMakeFriends(0)
  , fBenchmark("EsdConverter")
{
  // see header file for class documentation
  // or
  // refer to README to build package
  // or
  // visit http://web.ift.uib.no/~kjeks/doc/alice-hlt
  for( int i=0; i<fkNPartition; i++ ){
       fPartitionClusters[i]  = 0;    
       fNPartitionClusters[i] = 0;    
  }  
}

AliHLTGlobalEsdConverterComponent::~AliHLTGlobalEsdConverterComponent()
{
  // see header file for class documentation
  if (fESD) delete fESD;
  if (fESDfriend) delete fESDfriend;
  for( int i=0; i<fkNPartition; i++ ){
    delete[] fPartitionClusters[i];
  }
}

int AliHLTGlobalEsdConverterComponent::Reconfigure(const char* cdbEntry, const char* chainId)
{
  // see header file for class documentation
  return 0;
}

void AliHLTGlobalEsdConverterComponent::GetInputDataTypes(AliHLTComponentDataTypeList& list)
{
  // see header file for class documentation
  list.push_back(kAliHLTDataTypeTrack);
  list.push_back(kAliHLTDataTypeTrackMC);
  list.push_back(kAliHLTDataTypeCaloCluster);
  list.push_back(kAliHLTDataTypeCaloTrigger);
  list.push_back(kAliHLTDataTypedEdx );
  list.push_back(kAliHLTDataTypeESDVertex );
  list.push_back(kAliHLTDataTypeESDObject);
  list.push_back(kAliHLTDataTypeTObject);
  list.push_back(kAliHLTDataTypeGlobalVertexer);
  list.push_back(kAliHLTDataTypeV0Finder); // array of track ids for V0s
  list.push_back(kAliHLTDataTypeKFVertex); // KFVertex object from vertexer
  list.push_back(kAliHLTDataTypePrimaryFinder); // array of track ids for prim vertex
  list.push_back(kAliHLTDataTypeESDContent);
  list.push_back(kAliHLTDataTypeESDFriendContent);
  list.push_back( AliHLTTPCDefinitions::fgkClustersDataType   );
  list.push_back(kAliHLTDataTypeFlatESDVertex); // VertexTracks resonctructed using SAP ITS tracks
  list.push_back(kAliHLTDataTypeITSSAPData);    // SAP ITS tracks
}

AliHLTComponentDataType AliHLTGlobalEsdConverterComponent::GetOutputDataType()
{
  // see header file for class documentation
  return kAliHLTMultipleDataType;
}

int AliHLTGlobalEsdConverterComponent::GetOutputDataTypes(AliHLTComponentDataTypeList& tgtList){ 
// see header file for class documentation

  tgtList.clear();
  tgtList.push_back( kAliHLTDataTypeESDObject|kAliHLTDataOriginOut );
  tgtList.push_back( kAliHLTDataTypeESDfriendObject|kAliHLTDataOriginOut );
  tgtList.push_back( kAliHLTDataTypeStreamerInfo|kAliHLTDataOriginHLT );
  return tgtList.size();
}

void AliHLTGlobalEsdConverterComponent::GetOutputDataSize(unsigned long& constBase, double& inputMultiplier)
{
  // see header file for class documentation
  constBase=2000000;
  inputMultiplier=3.0;
}

int AliHLTGlobalEsdConverterComponent::DoInit(int argc, const char** argv)
{
  // see header file for class documentation
  int iResult=0;
  TString argument="";
  int bMissingParam=0;

  // default list of skiped ESD objects
  TString skipObjects=
    // "AliESDRun,"
    // "AliESDHeader,"
    // "AliESDZDC,"
    "AliESDFMD,"
    // "AliESDVZERO,"
    // "AliESDTZERO,"
    // "TPCVertex,"
    // "SPDVertex,"
    // "PrimaryVertex,"
    // "AliMultiplicity,"
    // "PHOSTrigger,"
    // "EMCALTrigger,"
    // "SPDPileupVertices,"
    // "TrkPileupVertices,"
    "Cascades,"
    "Kinks,"
    "AliRawDataErrorLogs,"
    "AliESDACORDE";

  iResult=Reconfigure(NULL, NULL);
  TString allArgs = "";
  for ( int i = 0; i < argc; i++ ) {
    if ( !allArgs.IsNull() ) allArgs += " ";
    allArgs += argv[i];
  }

  TObjArray* pTokens=allArgs.Tokenize(" ");
  if (pTokens) {
    for (int i=0; i<pTokens->GetEntries() && iResult>=0; i++) {
      argument=((TObjString*)pTokens->At(i))->String();	
      if (argument.IsNull()) continue;

      // -notree
      if (argument.CompareTo("-notree")==0) {
	fWriteTree=0;
	
	// -tree
      } else if (argument.CompareTo("-tree")==0) {
	fWriteTree=1;
      } else if (argument.CompareTo("-solenoidBz")==0) {
	if ((bMissingParam=(++i>=pTokens->GetEntries()))) break;
	HLTInfo("Magnetic Field set to: %s", ((TObjString*)pTokens->At(i))->String().Data());
	HLTWarning("argument '-solenoidBz' is deprecated, solenoid field initiaized from CDB settings");
	continue;
      } else if (argument.CompareTo("-make-friends")==0) {
	fMakeFriends = 1;      
      } else if ( argument.CompareTo( "-ScaleDownTracks" ) == 0 ) {
	if ((bMissingParam=(++i>=pTokens->GetEntries()))) break;
        fScaleDownTracks = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      } else if (argument.Contains("-skipobject=")) {
	argument.ReplaceAll("-skipobject=", "");
	skipObjects=argument;
      } else if (argument.CompareTo("-schema")==0) {
        SetUseSchema();
      }else {
	HLTError("unknown argument %s", argument.Data());
	iResult=-EINVAL;
	break;
      }
    }
  }
  if (bMissingParam) {
    HLTError("missing parameter for argument %s", argument.Data());
    iResult=-EINVAL;
  }

  fSolenoidBz=GetBz();

  if(fESD) delete fESD;
  fESD = NULL;
  if(fESDfriend) delete fESDfriend;
  fESDfriend=NULL;

  if (iResult>=0) {
    fESD = new AliESDEvent;
    if (fESD) {
      fESD->CreateStdContent();

      // remove some of the objects which are not needed
      if (fESD->GetList() && !skipObjects.IsNull()) {
	pTokens=skipObjects.Tokenize(",");
	if (pTokens) {
	  const char* id=NULL;
	  TIter next(pTokens);
	  TObject* pObject=NULL;
	  while ((pObject=next())!=NULL) {
	    id=((TObjString*)pObject)->String().Data();
	    TObject* pObj=fESD->GetList()->FindObject(id);
	    if (pObj) {
	      HLTDebug("removing object %s", id);
	      fESD->GetList()->Remove(pObj);
	      delete pObj;
	    } else {
	      HLTWarning("failed to remove object '%s' from ESD", id);
	    }
	  }
	  fESD->GetStdContent();
	  delete pTokens;
	}
      }
    } else {
      iResult=-ENOMEM;
    }

    SetupCTPData();
  }
  
  if( iResult>=0 && fMakeFriends ){
    fESDfriend = new AliESDfriend();
  }

  fBenchmark.SetTimer(0,"total");

  return iResult;
}

int AliHLTGlobalEsdConverterComponent::DoDeinit()
{
  // see header file for class documentation
  if(fESD) delete fESD;
  fESD=NULL;
  if(fESDfriend) delete fESDfriend;
  fESDfriend = NULL;
  return 0;
}

int AliHLTGlobalEsdConverterComponent::DoEvent(const AliHLTComponentEventData& evtData, 
					       AliHLTComponentTriggerData& trigData)
{
  // see header file for class documentation
  int iResult=0;

  if (!fESD) return -ENODEV;

  if (!IsDataEvent()) return iResult;
  fBenchmark.StartNewEvent();
  fBenchmark.Start(0);

  for(Int_t i=0; i<fkNPartition; i++){
    delete[] fPartitionClusters[i];    
    fPartitionClusters[i]  = 0;
    fNPartitionClusters[i] = 0;    
  }  
  
  AliESDEvent* pESD = fESD;
  
  pESD->Reset(); 
  pESD->SetMagneticField(fSolenoidBz);
  pESD->SetRunNumber(GetRunNo());
  pESD->SetPeriodNumber(GetPeriodNumber());
  pESD->SetOrbitNumber(GetOrbitNumber());
  pESD->SetBunchCrossNumber(GetBunchCrossNumber());
  pESD->SetTimeStamp(GetTimeStamp());
  if (pESD->SetESDDownscaledOnline(fScaleDownTracks > 0))
  {
    HLTError("AliESDRun missing");
  }

  const AliHLTCTPData* pCTPData=CTPData();
  if (pCTPData) {
    AliHLTTriggerMask_t mask=pCTPData->ActiveTriggers(trigData);
    for (int index=0; index<gkNCTPTriggerClasses; index++) {
      if (!mask.test(index)) continue;
      const char* name = pCTPData->Name(index);
      if( name && name[0]!='\0' && strncmp(name,"AliHLTReadoutList",17)!=0){
	pESD->SetTriggerClass(name, index);
      }
    }
    ULong64_t low, high;
    pCTPData->GetTriggerMaskAll(low,high);
    //first 50 triggers
    pESD->SetTriggerMask(low);
    //next 50 triggers
    pESD->SetTriggerMaskNext50(high);
  }

  TTree* pTree = NULL;
  if (fWriteTree)
    pTree = new TTree("esdTree", "Tree with HLT ESD objects");
 
  if (pTree) {
    pTree->SetDirectory(0);
  }

  if( fESDfriend ) fESDfriend->Reset();

  if ((iResult=ProcessBlocks(pTree, pESD, fESDfriend))>=0) {
    // TODO: set the specification correctly
    if (pTree) {
      // the esd structure is written to the user info and is
      // needed in te ReadFromTree method to read all objects correctly
      pTree->GetUserInfo()->Add(pESD);
      pESD->WriteToTree(pTree);
      iResult=PushBack(pTree, kAliHLTDataTypeESDTree|kAliHLTDataOriginOut, 0);

    } else {
      iResult=PushBack(pESD, kAliHLTDataTypeESDObject|kAliHLTDataOriginOut, 0);
    }
    fBenchmark.AddOutput(GetLastObjectSize());
    if( iResult>=0 && fMakeFriends ){
      iResult=PushBack(fESDfriend, kAliHLTDataTypeESDfriendObject|kAliHLTDataOriginOut, 0);
      fBenchmark.AddOutput(GetLastObjectSize());
     }
  }
  if (pTree) {
    // clear user info list to prevent objects from being deleted
    pTree->GetUserInfo()->Clear();
    delete pTree;
  }

  //push back the root streamers if they are available and changed
  PushBackSchema();

  fBenchmark.Stop(0);
  HLTBenchmark( fBenchmark.GetStatistics() );
  
  for(Int_t i=0; i<fkNPartition; i++){
    delete[] fPartitionClusters[i];    
    fPartitionClusters[i]  = 0;
    fNPartitionClusters[i] = 0;    
  }

  if( fESDfriend ) fESDfriend->Reset();
  if( fESD ) fESD->Reset();

  return iResult;
}

int AliHLTGlobalEsdConverterComponent::ProcessBlocks(TTree* pTree, AliESDEvent* pESD, AliESDfriend *pESDfriend )
{
  // see header file for class documentation

  int iResult=0;
  int iAddedDataBlocks=0;

  // check if there is an ESD block in the input and copy its content first to the
  // ESD
  const TObject* pEsdObj = GetFirstInputObject(kAliHLTDataTypeESDObject, "AliESDEvent");
  AliESDEvent* pInputESD=NULL;
  if (pEsdObj) pInputESD=dynamic_cast<AliESDEvent*>(const_cast<TObject*>(pEsdObj));
  if (pInputESD) {
    pInputESD->GetStdContent();
    *pESD=*pInputESD;
  }
  if (GetNextInputObject()!=NULL) {
    HLTWarning("handling of multiple ESD input objects not implemented, skipping input");
  }

  // Barrel tracking
  // tracks are based on the TPC tracks, and only updated from the ITS information
  // Sequence:
  // 1) extract MC information for TPC and ITS from specific data blocks and store in
  //    intermediate vector arrays
  // 2) extract TPC tracks, update with MC labels if available, the track parameters
  //    are estimated at the first cluster position
  // 2.1) propagate to last cluster position and update kTPCout, sets also outer param (fOp)
  // 2.2) update kTPCin, sets also inner param (fIp) and TPC inner param (fTPCInner)
  // 2.3) update kTPCrefit using the same parameters at the first cluster position
  //      HLT has strictly spoking no refit, but we want the flag to be set
  //      can be changed to be done after all the individual barrel detector parameters
  //      have been updated by looping over the tracks again
  // 3) extract ITS tracks, the tracks are actually TPC tracks updated from the ITS
  //    tracking information
  // 3.1) TODO 2010-07-12: handle ITS standalone tracks by updating kITSout before kITSin
  // 3.2) update with kITSin
  //    TODO 2010-07-12 find out if the kITSrefit has to be set as well
  // 4) extract TRD tracks and add to ESD
  //    TODO 2010-07-12 at the moment there is no matching or merging of TPC and TRD tracks
  // 5) Add Trigger Detectors 
  //    VZERO, ZDC

  // read the clusters
  // ---------- Access to clusters --------------------//
  /*
  const AliHLTComponentBlockData* pBl0=GetFirstInputBlock();
  int cnt = 0;
  while (pBl0) {
    char tp[9],org[5];
    strncpy(tp,pBl0->fDataType.fID,8);
    tp[8] = '0';
    strncpy(org,pBl0->fDataType.fOrigin,4);
    org[4] = '0';
    //
    printf(">>> Bl%3d %8s|%4s of size %d\n",cnt++,tp,org,pBl0->fSize);
    pBl0 = GetNextInputBlock();
  };
  */
  for(Int_t i=0; i<fkNPartition; i++){
    delete[] fPartitionClusters[i];    
    fPartitionClusters[i]  = 0;
    fNPartitionClusters[i] = 0;    
  }
  
  bool storeTracks = true;
  if (fScaleDownTracks)
  {
    bool tracksPresent = 0;
    static int nEventsWithTracks = 0;
    for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack|kAliHLTDataOriginTPC);pBlock!=NULL; pBlock=GetNextInputBlock())
    {
      if (((AliHLTTracksData*) pBlock->fPtr)->fCount)
      {
        tracksPresent = 1;
      }
    }
    {
      const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeITSSAPData | kAliHLTDataOriginITS);
      if (pBlock)
      {
        AliHLTITSSAPTrackerDataContainer* inPtr = reinterpret_cast<AliHLTITSSAPTrackerDataContainer*>(pBlock->fPtr);
        if (inPtr->fCount) tracksPresent = 1;
      }
    }
    if (tracksPresent)
    {
      if (++nEventsWithTracks % fScaleDownTracks) storeTracks = false;
    }
  }

  if( pESDfriend && storeTracks){

    int nInputClusters = 0;
    
    for(const AliHLTComponentBlockData *iter = GetFirstInputBlock(AliHLTTPCDefinitions::fgkClustersDataType); iter != NULL; iter = GetNextInputBlock()){
      
      if(iter->fDataType != AliHLTTPCDefinitions::fgkClustersDataType) continue;    
      Int_t slice     = AliHLTTPCDefinitions::GetMinSliceNr(iter->fSpecification);
      Int_t partition = AliHLTTPCDefinitions::GetMinPatchNr(iter->fSpecification);    
      Int_t slicepartition = slice*6+partition;      
      if(slicepartition<0 || slicepartition > fkNPartition){
	HLTWarning("Wrong header of TPC cluster data, slice %d, partition %d", slice, partition );
	continue;
      }
      
      AliHLTTPCClusterData *inPtrSP = ( AliHLTTPCClusterData* )( iter->fPtr );
      nInputClusters += inPtrSP->fSpacePointCnt;
      
      delete[] fPartitionClusters[slicepartition];
      fPartitionClusters[slicepartition]  = new AliTPCclusterMI[inPtrSP->fSpacePointCnt];
      fNPartitionClusters[slicepartition] = inPtrSP->fSpacePointCnt;
    
      // create  offline clusters out of the HLT clusters

      for ( unsigned int i = 0; i < inPtrSP->fSpacePointCnt; i++ ) {
	AliHLTTPCSpacePointData *chlt = &( inPtrSP->fSpacePoints[i] );
	AliTPCclusterMI *c = fPartitionClusters[slicepartition]+i;
	c->SetPad( chlt->GetPad() );
	c->SetTimeBin( chlt->GetTime() );
	c->SetX(chlt->fX);
	c->SetY(chlt->fY);
	c->SetZ(chlt->fZ);
	c->SetSigmaY2(chlt->fSigmaY2);
	c->SetSigmaYZ( 0 );
	c->SetSigmaZ2(chlt->fSigmaZ2);
	c->SetQ( chlt->fCharge );
	c->SetMax( chlt->fQMax );
	Int_t sector, row;
	Float_t padtime[3] = {0,chlt->fY,chlt->fZ};
	AliHLTTPCGeometry::Slice2Sector(slice,chlt->fPadRow, sector, row);
	c->SetDetector( sector );
	c->SetRow( row );
      }
    } // end of loop over blocks of clusters    
    
    
    // fill event info
    {
      for( Int_t iSlice=0; iSlice<36; iSlice++ ){
	int iSector = iSlice;
	int nclu = 0;
	for( Int_t iPartition=0; iPartition<3; iPartition++){	    
	  int slicepartition = iSlice*6+iPartition;
	  nclu+= fNPartitionClusters[slicepartition];
	}
	pESDfriend->SetNclustersTPC( iSector, nclu );
	iSector = 36+iSlice;
	nclu = 0;
	for( Int_t iPartition=3; iPartition<6; iPartition++){	    
	  int slicepartition = iSlice*6+iPartition;
	  nclu+= fNPartitionClusters[slicepartition];
	}
	pESDfriend->SetNclustersTPC( iSector, nclu );
      }
    }
    
    // fill VZERO info
    {
      const TObject *pObject = GetFirstInputObject(kAliHLTDataTypeESDFriendContent|kAliHLTDataOriginVZERO); 
      if( pObject ){
	const AliESDVZEROfriend *esdVZEROfriend = dynamic_cast<const AliESDVZEROfriend*>( pObject );
	if (esdVZEROfriend) {
	  pESDfriend->SetVZEROfriend( esdVZEROfriend );
 	} else {
	  ALIHLTERRORGUARD(1, "input object of data type %s is not of class AliESDVZEROfriend",
			   DataType2Text(kAliHLTDataTypeESDFriendContent|kAliHLTDataOriginVZERO).c_str());
	}
      }
    }
  }    

  // 1) first read MC information (if present)
  std::map<int,int> mcLabelsTPC;
  std::map<int,int> mcLabelsITS;

  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrackMC|kAliHLTDataOriginTPC);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {

    fBenchmark.AddInput(pBlock->fSize);

    AliHLTTrackMCData* dataPtr = reinterpret_cast<AliHLTTrackMCData*>( pBlock->fPtr );
    if (sizeof(AliHLTTrackMCData)+dataPtr->fCount*sizeof(AliHLTTrackMCLabel)==pBlock->fSize) {
      for( unsigned int il=0; il<dataPtr->fCount; il++ ){
	AliHLTTrackMCLabel &lab = dataPtr->fLabels[il];
	mcLabelsTPC[lab.fTrackID] = lab.fMCLabel;
      }
    } else {
      HLTWarning("data mismatch in block %s (0x%08x): count %d, size %d -> ignoring track MC information", 
		 DataType2Text(pBlock->fDataType).c_str(), pBlock->fSpecification, 
		 dataPtr->fCount, pBlock->fSize);
    }
  }
 
  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrackMC|kAliHLTDataOriginITS);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {

    fBenchmark.AddInput(pBlock->fSize);

    AliHLTTrackMCData* dataPtr = reinterpret_cast<AliHLTTrackMCData*>( pBlock->fPtr );
    if (sizeof(AliHLTTrackMCData)+dataPtr->fCount*sizeof(AliHLTTrackMCLabel)==pBlock->fSize) {
      for( unsigned int il=0; il<dataPtr->fCount; il++ ){
	AliHLTTrackMCLabel &lab = dataPtr->fLabels[il];
	mcLabelsITS[lab.fTrackID] = lab.fMCLabel;
      }
    } else {
      HLTWarning("data mismatch in block %s (0x%08x): count %d, size %d -> ignoring track MC information", 
		 DataType2Text(pBlock->fDataType).c_str(), pBlock->fSpecification, 
		 dataPtr->fCount, pBlock->fSize);
    }
  }


  // read dEdx information (if present)
  // TODO 2010-07-12 this needs to be verified

  AliHLTFloat32_t *dEdxTPC = 0; 
  Int_t ndEdxTPC = 0;
  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypedEdx|kAliHLTDataOriginTPC);
       pBlock!=NULL; pBlock=NULL/*GetNextInputBlock() there is only one block*/) {
    fBenchmark.AddInput(pBlock->fSize);
    dEdxTPC = reinterpret_cast<AliHLTFloat32_t*>( pBlock->fPtr );
    ndEdxTPC = pBlock->fSize / (3*sizeof(AliHLTFloat32_t));
  }

  
  std::map<int,int> mapTpcId2esdId;

  // 2) convert the TPC tracks to ESD tracks
  if (storeTracks) for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack|kAliHLTDataOriginTPC);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    if (pInputESD && pInputESD->GetNumberOfTracks()>0) {
      HLTWarning("Tracks array already filled from the input esd block, additional filling from TPC tracks block might cause inconsistent content");
    }
    fBenchmark.AddInput(pBlock->fSize);
    vector<AliHLTGlobalBarrelTrack> tracks;
    if ((iResult=AliHLTGlobalBarrelTrack::ConvertTrackDataArray(reinterpret_cast<const AliHLTTracksData*>(pBlock->fPtr), pBlock->fSize, tracks))>=0) {
      int iTrack = 0;
      for (vector<AliHLTGlobalBarrelTrack>::iterator element=tracks.begin();
	   element!=tracks.end(); element++, iTrack++ ) {
	Float_t points[4] = {
	  static_cast<Float_t>(element->GetX()),
	  static_cast<Float_t>(element->GetY()),
	  static_cast<Float_t>(element->GetLastPointX()),
	  static_cast<Float_t>(element->GetLastPointY())
	};

	Int_t mcLabel = -1;
	if( mcLabelsTPC.find(element->TrackID())!=mcLabelsTPC.end() )
	  mcLabel = mcLabelsTPC[element->TrackID()];
	element->SetLabel( mcLabel );

	AliESDtrack iotrack;

	// for the moment, the number of clusters are not set when processing the
	// kTPCin update, only at kTPCout
	// there ar emainly three parameters updated for kTPCout
	//   number of clusters
	//   chi2
	//   pid signal
	// The first one can be updated already at that stage here, while the two others
	// eventually require to update from the ITS tracks before. The exact scheme
	// needs to be checked
	iotrack.SetID( element->TrackID() );

	// 2.1 set kTPCout
	// TODO 2010-07-12 disabled for the moment because of bug
	// https://savannah.cern.ch/bugs/index.php?69875
	// the propagation does not work, there is an offset in z
	// propagate to last cluster and update parameters with flag kTPCout
	// Note: updating with flag kITSout further down will overwrite the outer
	// parameter again so this can be done only for ITS standalone tracks, meaning
	// tracks in the ITS not associated with any TPC track
	// HLT does not provide such standalone tracking
	AliHLTGlobalBarrelTrack outPar(*element);	  
	{
	  //outPar.AliExternalTrackParam::PropagateTo( element->GetLastPointX(), fSolenoidBz );
	  const Int_t N=10; // number of steps.
	  const Float_t xRange = element->GetLastPointX() - element->GetX();
	  const Float_t xStep = xRange / N ;
	  for(int i = 1; i <= N; ++i) {
	    if(!outPar.AliExternalTrackParam::PropagateTo(element->GetX() + xStep * i, fSolenoidBz)) break;
	  }
	  outPar.SetLabel(element->GetLabel());
	  iotrack.UpdateTrackParams(&outPar,AliESDtrack::kTPCout);
	}
	// 2.2 TPC tracking estimates parameters at first cluster
	iotrack.UpdateTrackParams(&(*element),AliESDtrack::kTPCin);

	// 2.3 use the same parameters also for kTPCrefit, there is no proper refit in HLT
	// maybe this can be done later, however also the inner parameter is changed which
	// is used as a reference point in the display. That point should be at the first
	// TPC cluster
	iotrack.UpdateTrackParams(&(*element),AliESDtrack::kTPCrefit);
	iotrack.SetTPCPoints(points);
	if( iTrack < ndEdxTPC ){
	  AliHLTFloat32_t *val = &(dEdxTPC[3*iTrack]);
	  iotrack.SetTPCsignal( val[0], val[1], (UChar_t) val[2] ); 
	  //AliTPCseed s;
	  //s.Set( element->GetX(), element->GetAlpha(),
	  //element->GetParameter(), element->GetCovariance() );
	  //s.SetdEdx( val[0] );
	  //s.CookPID();
	  //iotrack.SetTPCpid(s.TPCrPIDs() );
	} else {
	  if( dEdxTPC ) HLTWarning("Wrong number of dEdx TPC labels");
	}
	iotrack.SetLabel(mcLabel);

	mapTpcId2esdId[element->TrackID()] = pESD->GetNumberOfTracks();

	pESD->AddTrack(&iotrack);

	if (fVerbosity>0) element->Print();

	if( pESDfriend ){ // create friend track

	  AliHLTGlobalBarrelTrack gb(*element);
	  AliTPCseed tTPC;
	  tTPC.Set( gb.GetX(), gb.GetAlpha(), gb.GetParameter(), gb.GetCovariance() );	  
	  tTPC.SetLabel(mcLabel);
	  
	  // set the clusters 
	  UInt_t nClusters = element->GetNumberOfPoints();
	  const UInt_t*clusterIDs = element->GetPoints();

	  int nClustersSet=0;
	  for(UInt_t ic=0; ic<nClusters; ic++){	 

	    UInt_t id      = clusterIDs[ic];	     
	    int iSlice = AliHLTTPCSpacePointData::GetSlice(id);
	    int iPartition = AliHLTTPCSpacePointData::GetPatch(id);
	    int iCluster = AliHLTTPCSpacePointData::GetNumber(id);
	    
	    if(iSlice<0 || iSlice>36 || iPartition<0 || iPartition>5){
	      HLTError("Corrupted TPC cluster Id: slice %d, partition %d, cluster %d", iSlice, iPartition, iCluster);
	      continue;
	    }
	    
	    AliTPCclusterMI *patchClusters = fPartitionClusters[iSlice*6 + iPartition];
	    if(!patchClusters){
	      HLTError("Clusters are missed for slice %d, partition %d", iSlice, iPartition );
	      continue;
	    }
	    
	    if(iCluster >= fNPartitionClusters[iSlice*6 + iPartition]){
	      HLTError("TPC slice %d, partition %d: ClusterID==%d >= N Cluaters==%d ", iSlice, iPartition,iCluster, fNPartitionClusters[iSlice*6 + iPartition] );
	      continue;
	    }
	
	    AliTPCclusterMI *c = &(patchClusters[iCluster]);	  	
	    int sec = c->GetDetector();
	    int row = c->GetRow();
	    if(sec >= 36) row = row + AliHLTTPCGeometry::GetNRowLow();
	    
	    if( tTPC.GetClusterPointer(row) ) continue;
	    tTPC.SetClusterPointer(row, c);	
	    nClustersSet++;

	    AliTPCTrackerPoints::Point &point = *( (AliTPCTrackerPoints::Point*)tTPC.GetTrackPoint( row ) );
	    tTPC.Propagate( TMath::DegToRad()*(sec%18*20.+10.), c->GetX(), fSolenoidBz );
	    Double_t angle2 = tTPC.GetSnp()*tTPC.GetSnp();
	    angle2 = (angle2<1) ?TMath::Sqrt(angle2/(1-angle2)) :10.; 
	    point.SetAngleY( angle2 );
	    point.SetAngleZ( tTPC.GetTgl() );
	  } // end of associated cluster loop

	  tTPC.SetNumberOfClusters(nClustersSet);
  
	  // Cook dEdx
	  
	  //AliTPCseed *seed = &(tTPC);      
	  //fSeedArray->AddAt( seed, TMath::Abs(seed->GetLabel()) );
	  //fdEdx->Fill( seed->P()*seed->Charge(), seed->CookdEdx(0.02, 0.6) );

	  AliESDfriendTrack friendTrack;
	  friendTrack.AddCalibObject(&tTPC);
	  friendTrack.SetTPCOut( outPar);
	  pESDfriend->AddTrack(&friendTrack);
	}
      }

      HLTInfo("converted %d track(s) to AliESDtrack and added to ESD", tracks.size());
      iAddedDataBlocks++;
    } else if (iResult<0) {
      HLTError("can not extract tracks from data block of type %s (specification %08x) of size %d: error %d", 
	       DataType2Text(pBlock->fDataType).c_str(), pBlock->fSpecification, pBlock->fSize, iResult);
    }
  }


  // Get ITS SPD vertex
  for( const AliHLTComponentBlockData *i= GetFirstInputBlock(kAliHLTDataTypeESDVertex|kAliHLTDataOriginITS); i!=NULL; i=GetNextInputBlock() ){
    fBenchmark.AddInput(i->fSize);
  }

  for ( const TObject *iter = GetFirstInputObject(kAliHLTDataTypeESDVertex|kAliHLTDataOriginITS); iter != NULL; iter = GetNextInputObject() ) {
    AliESDVertex *vtx = dynamic_cast<AliESDVertex*>(const_cast<TObject*>( iter ) );
    pESD->SetPrimaryVertexSPD( vtx );
  }

  // Get ITS Standalone primaries (SAP) vertexTracks
  {
    const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeFlatESDVertex|kAliHLTDataOriginITS);
    if (pBlock) {
      fBenchmark.AddInput(pBlock->fSize);
      AliFlatESDVertex *vtxFlat =  reinterpret_cast<AliFlatESDVertex*>( pBlock->fPtr );
      if (vtxFlat->GetNContributors()>0) {
	AliESDVertex vtx;
	vtxFlat->GetESDVertex(vtx);
	vtx.SetTitle("vertexITSSAP");
	pESD->SetPrimaryVertexTracks( &vtx );
      }
    }
  }
  

  // Get ITS Standalone primary (SAP) Tracks

  if (storeTracks) {
    const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeITSSAPData|kAliHLTDataOriginITS);
    if (pBlock) {
      fBenchmark.AddInput(pBlock->fSize);
      const AliHLTITSSAPTrackerDataContainer *dataSAP = reinterpret_cast<const AliHLTITSSAPTrackerDataContainer*>(pBlock->fPtr);
      AliITStrackV2 trcV2;
      int ntrITSSAP = dataSAP->fCount;
      for (int itr=0;itr<ntrITSSAP;itr++) {
	const AliHLTITSSAPTrackerData& trcFlatSAP = dataSAP->fTracks[itr];
	AliESDtrack inpESDtrc;
	inpESDtrc.SetID(pESD->GetNumberOfTracks());
	trcFlatSAP.paramInw.GetExternalTrackParam(trcV2); // track at the vertex
	trcV2.SetLabel(trcFlatSAP.label);
	trcV2.SetNumberOfClusters(trcFlatSAP.ncl);
	trcV2.SetChi2(trcFlatSAP.chi2);
	inpESDtrc.UpdateTrackParams(&trcV2,AliESDtrack::kITSrefit);
	inpESDtrc.SetStatus( (AliESDtrack::kITSin|AliESDtrack::kITSout|AliESDtrack::kITSpureSA) );
	pESD->AddTrack(&inpESDtrc);
	//
	if( pESDfriend ) {
	  AliESDfriendTrack friendTrack;
	  trcFlatSAP.paramOut.GetExternalTrackParam(trcV2); // track at the vertex
	  friendTrack.SetITSOut(trcV2);
	  pESDfriend->AddTrack(&friendTrack);
	}
      }
    }
  }


  // 3.1. now update ESD tracks with the ITSOut info
  // updating track parameters with flag kITSout will overwrite parameter set above with flag kTPCout
  // TODO 2010-07-12 there are some issues with this updating sequence, for the moment update with
  // flag kITSout is disabled, would require
  // a) to update kTPCout after kITSout
  // b) update only for ITS standalone tracks. The HLT ITS tracker does not provide standalone
  //    tracking, every track is related to a TPC track
  // Furthermore there seems to be a bug as the data blocks describe track parameters around the
  // vertex, not at the outer ITS
  // bug https://savannah.cern.ch/bugs/index.php?69872
  if (storeTracks) for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack|kAliHLTDataOriginITSOut);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    fBenchmark.AddInput(pBlock->fSize);
    vector<AliHLTGlobalBarrelTrack> tracks;
    if ((iResult=AliHLTGlobalBarrelTrack::ConvertTrackDataArray(reinterpret_cast<const AliHLTTracksData*>(pBlock->fPtr), pBlock->fSize, tracks))>0) {
      for (vector<AliHLTGlobalBarrelTrack>::iterator element=tracks.begin();
	   element!=tracks.end(); element++) {
	int tpcID=element->TrackID();
	// the ITS tracker assigns the TPC track used as seed for a certain track to
	// the trackID
	Int_t esdID = -1;
	if( mapTpcId2esdId.find(tpcID) != mapTpcId2esdId.end() ) esdID = mapTpcId2esdId[tpcID];	
	if( esdID<0 || esdID>=pESD->GetNumberOfTracks()) continue;
	//AliESDtrack *tESD = pESD->GetTrack( esdID );
	//element->SetLabel(tESD->GetLabel());
	// 2010-07-12 disabled, see above, bugfix https://savannah.cern.ch/bugs/index.php?69872
	//if( tESD ) tESD->UpdateTrackParams( &(*element), AliESDtrack::kITSout );
	if( pESDfriend ) {
	  AliESDfriendTrack *friendTrack = pESDfriend->GetTrack(esdID);
	  if( friendTrack ) friendTrack->SetITSOut( (*element) );	  
	}
      }
    }
  }

  // 3.2. now update ESD tracks with the ITS info
  if (storeTracks) for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack|kAliHLTDataOriginITS);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    fBenchmark.AddInput(pBlock->fSize);
    vector<AliHLTGlobalBarrelTrack> tracks;
    if ((iResult=AliHLTGlobalBarrelTrack::ConvertTrackDataArray(reinterpret_cast<const AliHLTTracksData*>(pBlock->fPtr), pBlock->fSize, tracks))>0) {
      for (vector<AliHLTGlobalBarrelTrack>::iterator element=tracks.begin();
	   element!=tracks.end(); element++) {
	int tpcID=element->TrackID();

	Int_t esdID = -1;
	if( mapTpcId2esdId.find(tpcID) != mapTpcId2esdId.end() ) esdID = mapTpcId2esdId[tpcID];
	
	// the ITS tracker assigns the TPC track used as seed for a certain track to
	// the trackID
	if( esdID<0 || esdID>=pESD->GetNumberOfTracks()) continue;
	Int_t mcLabel = -1;
	if( mcLabelsITS.find(tpcID)!=mcLabelsITS.end() )
	  mcLabel = mcLabelsITS[tpcID];

	AliESDtrack *tESD = pESD->GetTrack( esdID );
	
	if (!tESD) continue;
	// the labels for the TPC and ITS tracking params can be different, e.g.
	// there can be a decay. The ITS label should then be the better one, the
	// TPC label is saved in a member of AliESDtrack
	if (mcLabel>=0) {
	  // upadte only if the ITS label is available, otherwise keep TPC label
	  element->SetLabel( mcLabel );
	} else {
	  // bugfix https://savannah.cern.ch/bugs/?69713
	  element->SetLabel( tESD->GetLabel() );	  
	}
	tESD->UpdateTrackParams( &(*element), AliESDtrack::kITSin );

	// TODO: add a proper refit
	//tESD->UpdateTrackParams( &(*element), AliESDtrack::kTPCrefit );
      }
    }
  }

  // update with  vertices and vertex-fitted tracks
  // output of the GlobalVertexerComponent
  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeGlobalVertexer);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    fBenchmark.AddInput(pBlock->fSize);   
    AliHLTGlobalVertexerComponent::FillESD( pESD, reinterpret_cast<AliHLTGlobalVertexerComponent::AliHLTGlobalVertexerData* >(pBlock->fPtr) );
  }

  // update with  vertices and vertex-fitted tracks
  // output of PrimaryVertexer and V0Finder components
  
  TObject* pBase = (TObject*)GetFirstInputObject(kAliHLTDataTypeKFVertex | kAliHLTDataOriginOut);
  if (pBase) {
    AliKFVertex* kfVertex = dynamic_cast<AliKFVertex *>(pBase);
    if (kfVertex) {
      const AliHLTComponentBlockData* pP = GetFirstInputBlock(kAliHLTDataTypePrimaryFinder | kAliHLTDataOriginOut);
      if (pP && pP->fSize && pP->fPtr) {
	const AliHLTComponentBlockData* pV0 = GetFirstInputBlock(kAliHLTDataTypeV0Finder | kAliHLTDataOriginOut);
	if (pV0 && pV0->fPtr && pInputESD && pInputESD->GetNumberOfV0s()>0) {
	  const int* v0s = static_cast<const int*>(pV0->fPtr);
	  HLTWarning("V0 array already filled from the input esd block, additional filling from V0 block of %d entries might cause inconsistent content", v0s[0]);
	}
	AliHLTVertexFinderBase::FillESD(pESD, kfVertex, pP->fPtr, pV0?pV0->fPtr:NULL);
      } else
	HLTWarning("Problem with primary finder's data block");
    } else {
      HLTWarning("primary vertex block of wrong type, expecting AliKFVertex instead of %s", pBase->GetName());
    }
  } else {
    // throw an error if there is a V0 data block which can not be handled without
    // the AliKFVertex object
    if (GetFirstInputBlock(kAliHLTDataTypeV0Finder | kAliHLTDataOriginOut)!=NULL) {
      ALIHLTERRORGUARD(1, "missing AliKFVertex object ignoring V0 data block of type %s",
		       DataType2Text(kAliHLTDataTypeV0Finder|kAliHLTDataOriginOut).c_str());
    }
  }

  // Fill DCA parameters for TPC tracks
  // TODO 2010-07-12 this propagates also the TPC inner param to beamline
  // sounds not very reasonable
  // https://savannah.cern.ch/bugs/index.php?69873
  if( storeTracks && pESD->GetPrimaryVertex() && pESD->GetPrimaryVertex()->GetStatus() ){
    for (int i=0; i<pESD->GetNumberOfTracks(); i++) {
      if (!pESD->GetTrack(i) || 
	  !pESD->GetTrack(i)->GetTPCInnerParam() ) continue;
      pESD->GetTrack(i)->RelateToVertexTPC(pESD->GetPrimaryVertex(), fSolenoidBz, 1000 );
      pESD->GetTrack(i)->RelateToVertex(pESD->GetPrimaryVertex(), fSolenoidBz, 1000 );
   }
  }


  // loop over all tracks and set the TPC refit flag by updating with the
  // original TPC inner parameter if not yet set
  // TODO: replace this by a proper refit
  // code is comented for the moment as it does not fully solve the problems with
  // the display
  // - would set the main parameters to the TPC inner wall again, or
  // - changes the inner param if the parameters are propagated, so we loose the track
  //   reference point for the display
  // with the current sequence we have the latter case as the DCA operations above
  // change the TPC inner parameters
  /*
  if (storeTracks) for (int i=0; i<pESD->GetNumberOfTracks(); i++) {
    if (!pESD->GetTrack(i) || 
	!pESD->GetTrack(i)->GetTPCInnerParam() ||
	pESD->GetTrack(i)->IsOn(AliESDtrack::kTPCrefit)) continue;
    AliESDtrack* tESD=pESD->GetTrack(i);
    AliHLTGlobalBarrelTrack inner(*tESD->GetTPCInnerParam());
    inner.SetLabel(tESD->GetLabel());
    tESD->UpdateTrackParams(&inner, AliESDtrack::kTPCrefit);
  }
  */

  // 4. convert the HLT TRD tracks to ESD tracks                        
  if (storeTracks) for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack | kAliHLTDataOriginTRD);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    fBenchmark.AddInput(pBlock->fSize);
    vector<AliHLTGlobalBarrelTrack> tracks;
    if ((iResult=AliHLTGlobalBarrelTrack::ConvertTrackDataArray(reinterpret_cast<const AliHLTTracksData*>(pBlock->fPtr), pBlock->fSize, tracks))>0) {
      for (vector<AliHLTGlobalBarrelTrack>::iterator element=tracks.begin();
	   element!=tracks.end(); element++) {
	
	Double_t TRDpid[AliPID::kSPECIES], eProb(0.2), restProb((1-eProb)/(AliPID::kSPECIES-1)); //eprob(element->GetTRDpid...);
	for(Int_t i=0; i<AliPID::kSPECIES; i++){
	  switch(i){
	  case AliPID::kElectron: TRDpid[AliPID::kElectron]=eProb; break;
	  default: TRDpid[i]=restProb; break;
	  }
	}
	
	AliESDtrack iotrack;
	iotrack.UpdateTrackParams(&(*element),AliESDtrack::kTRDout);
	iotrack.SetStatus(AliESDtrack::kTRDin);
	iotrack.SetTRDpid(TRDpid);
	
	pESD->AddTrack(&iotrack);
	if (fVerbosity>0) element->Print();
      }
      HLTInfo("converted %d track(s) to AliESDtrack and added to ESD", tracks.size());
      iAddedDataBlocks++;
    } else if (iResult<0) {
      HLTError("can not extract tracks from data block of type %s (specification %08x) of size %d: error %d", 
	       DataType2Text(pBlock->fDataType).c_str(), pBlock->fSpecification, pBlock->fSize, iResult);
    }
  }
  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeCaloCluster | kAliHLTDataOriginAny); pBlock!=NULL; pBlock=GetNextInputBlock()) 
    {
      fBenchmark.AddInput(pBlock->fSize);
      AliHLTCaloClusterHeaderStruct *caloClusterHeaderPtr = reinterpret_cast<AliHLTCaloClusterHeaderStruct*>(pBlock->fPtr);

      HLTDebug("%d HLT clusters from spec: 0x%X", caloClusterHeaderPtr->fNClusters, pBlock->fSpecification);

      //AliHLTCaloClusterReader reader;
      //reader.SetMemory(caloClusterHeaderPtr);

      AliHLTESDCaloClusterMaker clusterMaker;

      int nClusters = clusterMaker.FillESD(pESD, caloClusterHeaderPtr);
     
      HLTInfo("converted %d cluster(s) to AliESDCaloCluster and added to ESD", nClusters);
      iAddedDataBlocks++;
    }
  for(const AliHLTComponentBlockData *pBlock = GetFirstInputBlock(kAliHLTDataTypeCaloTrigger | kAliHLTDataOriginEMCAL); pBlock != NULL; pBlock = GetNextInputBlock()){
    fBenchmark.AddInput(pBlock->fSize);

    AliHLTCaloTriggerHeaderStruct *caloTriggerHeaderPtr = reinterpret_cast<AliHLTCaloTriggerHeaderStruct *>(pBlock->fPtr);
    AliHLTCaloTriggerDataStruct *triggerdata = reinterpret_cast<AliHLTCaloTriggerDataStruct *>(reinterpret_cast<AliHLTUInt8_t *>(pBlock->fPtr) + sizeof(AliHLTCaloTriggerHeaderStruct));
    AliESDCaloTrigger *emcaltrigger = pESD->GetCaloTrigger("EMCAL");
    emcaltrigger->Allocate(caloTriggerHeaderPtr->fNfastor);
    emcaltrigger->SetL1V0(caloTriggerHeaderPtr->fL1V0);
    emcaltrigger->SetL1FrameMask(caloTriggerHeaderPtr->fL1FrameMask);
    for (int i = 0; i < 4; i++) emcaltrigger->SetL1Threshold(i, caloTriggerHeaderPtr->fL1Threshold[i]);
    for(int idata = 0; idata < caloTriggerHeaderPtr->fNfastor; idata++){
      Int_t l0times[10];
      for(int itime = 0; itime < 10; itime++) l0times[itime] = triggerdata->fL0Times[itime];
      emcaltrigger->Add((int)triggerdata->fCol, (int)triggerdata->fRow, triggerdata->fAmplitude, triggerdata->fTime, l0times, (int)triggerdata->fNL0Times, (int)triggerdata->fL1TimeSum, triggerdata->fTriggerBits);
    }
    HLTInfo("converted EMCAL trigger data with %d trigger channels and added to ESD", caloTriggerHeaderPtr->fNfastor);
  }
  
  // 5) Add Trigger Detectors 
  //    VZERO, ZDC

  // FIXME: the size of all input blocks can be added in one loop
  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeESDContent|kAliHLTDataOriginVZERO);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    fBenchmark.AddInput(pBlock->fSize);
  }

  for ( const TObject *pObject = GetFirstInputObject(kAliHLTDataTypeESDContent|kAliHLTDataOriginVZERO); 
	pObject != NULL; pObject = GetNextInputObject() ) {
    AliESDVZERO *esdVZERO = dynamic_cast<AliESDVZERO*>(const_cast<TObject*>( pObject ) );  
    if (esdVZERO) {
      pESD->SetVZEROData( esdVZERO );
     break;
    } else {
      ALIHLTERRORGUARD(1, "input object of data type %s is not of class AliESDVZERO",
		       DataType2Text(kAliHLTDataTypeESDContent|kAliHLTDataOriginVZERO).c_str());
    }
  }

  // FIXME: the size of all input blocks can be added in one loop
  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeESDContent|kAliHLTDataOriginZDC);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    fBenchmark.AddInput(pBlock->fSize);
  }
  for ( const TObject *pObject = GetFirstInputObject(kAliHLTDataTypeESDContent|kAliHLTDataOriginZDC); 
	pObject != NULL; pObject = GetNextInputObject() ) {
    AliESDZDC *esdZDC = dynamic_cast<AliESDZDC*>(const_cast<TObject*>( pObject ) );
    if (esdZDC) {
#ifndef HAVE_NOT_ALIZDCRECONSTRUCTOR_r43770
      pESD->SetZDCData( esdZDC );
#else
      ALIHLTERRORGUARD(1, "Processing of ZDC data requires AliRoot r43770m skipping data block of type %s",
		       DataType2Text(kAliHLTDataTypeESDContent|kAliHLTDataOriginZDC).c_str());
#endif
      break;
    } else {
      ALIHLTERRORGUARD(1, "input object of data type %s is not of class AliESDZDC",
		       DataType2Text(kAliHLTDataTypeESDContent|kAliHLTDataOriginZDC).c_str());
    }
  }

  // Add tracks and clusters from MUON.
  for( const AliHLTComponentBlockData *i= GetFirstInputBlock(kAliHLTAnyDataType | kAliHLTDataOriginMUON); i!=NULL; i=GetNextInputBlock() ){
    fBenchmark.AddInput(i->fSize);
  }

  for (const TObject* obj = GetFirstInputObject(kAliHLTAnyDataType | kAliHLTDataOriginMUON);
       obj != NULL;
       obj = GetNextInputObject()
      )
  {
    const TClonesArray* tracklist = NULL;
    const TClonesArray* clusterlist = NULL;
    if (obj->IsA() == AliESDEvent::Class())
    {
      const AliESDEvent* event = static_cast<const AliESDEvent*>(obj);
      HLTDebug("Received a MUON ESD with specification: 0x%X", GetSpecification(obj));
      if (event->GetList() == NULL) continue;
      tracklist = dynamic_cast<const TClonesArray*>(event->GetList()->FindObject("MuonTracks"));
      if (tracklist == NULL) continue;
      clusterlist = dynamic_cast<const TClonesArray*>(event->GetList()->FindObject("MuonClusters"));
      if (clusterlist == NULL) continue;
    }
    else if (obj->IsA() == TClonesArray::Class())
    {
      if (!strcmp(obj->GetName(), "MuonTracks")) {
	tracklist = static_cast<const TClonesArray*>(obj);
	HLTDebug("Received a MUON TClonesArray of tracks with specification: 0x%X", GetSpecification(obj));
      } else {
	clusterlist = static_cast<const TClonesArray*>(obj);
	HLTDebug("Received a MUON TClonesArray of clusters with specification: 0x%X", GetSpecification(obj));
      }
    }
    else
    {
      // Cannot handle this object type.
      continue;
    }
    // copy tracks
    if (tracklist) {
      HLTDebug("Received %d MUON tracks.", tracklist->GetEntriesFast());
      if (tracklist->GetEntriesFast() > 0)
      {
	const AliESDMuonTrack* track = dynamic_cast<const AliESDMuonTrack*>(tracklist->UncheckedAt(0));
	if (track == NULL)
	{
	  HLTError(Form("%s from MUON does not contain AliESDMuonTrack objects.", obj->ClassName()));
	  continue;
	}
      }
      for (Int_t i = 0; i < tracklist->GetEntriesFast(); ++i)
      {
	AliESDMuonTrack* track = pESD->NewMuonTrack();
	*track = *(static_cast<const AliESDMuonTrack*>(tracklist->UncheckedAt(i)));
      }
    }
    // copy clusters
    if (clusterlist) {
      HLTDebug("Received %d MUON clusters.", clusterlist->GetEntriesFast());
      if (clusterlist->GetEntriesFast() > 0)
      {
	const AliESDMuonCluster* cluster = dynamic_cast<const AliESDMuonCluster*>(clusterlist->UncheckedAt(0));
	if (cluster == NULL)
	{
	  HLTError(Form("%s from MUON does not contain AliESDMuonCluster objects.", obj->ClassName()));
	  continue;
	}
      }
      for (Int_t i = 0; i < clusterlist->GetEntriesFast(); ++i)
      {
	AliESDMuonCluster* cluster = pESD->NewMuonCluster();
	*cluster = *(static_cast<const AliESDMuonCluster*>(clusterlist->UncheckedAt(i)));
      }
    }
  }

  if( storeTracks && fMakeFriends && pESDfriend ){ // create friend track
    pESD->SetESDfriend( pESDfriend );
 }
  
 
  if (iAddedDataBlocks>0 && pTree) {
    pTree->Fill();
  }  
  
  if (iResult>=0) iResult=iAddedDataBlocks;
  return iResult;
}
