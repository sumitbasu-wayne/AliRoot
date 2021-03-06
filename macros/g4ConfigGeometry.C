// $Id: g4Config.C 18590 2007-05-15 13:24:50Z hristov $
//
// Configuration macro for building AliRoot geometry 
// using g4ConfigCommon.C.
// TGeant3TGeo has to be used to build geometry as
// TGeant4 does not support mixing calls to gMC and TGeo.
//
// By I. Hrivnacova, IPN Orsay
 	

void Config()
{
  cout << "Running g4ConfigGometry.C ... " << endl;

  // Load Geant3 + Geant3 VMC libraries
  //
  //  Libraries required by geant321
  gSystem->Load("liblhapdf");      // Parton density functions
  gSystem->Load("libpythia6");     // Pythia
  gSystem->Load("libgeant321");
  gSystem->Load("libEG");
  gSystem->Load("libEGPythia6");
  gSystem->Load("libAliPythia6");  // ALICE specific implementations

  new TGeant3TGeo("C++ Interface to Geant3");
  
  
  // AliRoot setup
  //
  gROOT->LoadMacro("$ALICE_ROOT/macros/g4ConfigCommon1.C");
  ConfigCommon1(kFALSE);

  cout << "Running g4ConfigGometry.C finished ... " << endl;

}
