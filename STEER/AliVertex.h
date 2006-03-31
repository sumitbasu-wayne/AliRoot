#ifndef ALIVERTEX_H
#define ALIVERTEX_H
/* Copyright(c) 1998-2003, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */


//-------------------------------------------------------
//                    Base Vertex Class
//   Used for secondary vertices and as a base class for primary vertices
//   Origin: F. Prino, Torino, prino@to.infn.it
//-------------------------------------------------------


#include <TNamed.h>

class AliVertex : public TNamed {
 
 public:
 
  AliVertex();
  AliVertex(Double_t position[3],Double_t dispersion,
		Int_t nContributors);
  virtual ~AliVertex();


  virtual void   SetXYZ(Double_t pos[3]) 
                   {for(Int_t j=0; j<3; j++) fPosition[j]=pos[j];}
  virtual void   SetXv(Double_t xVert) {fPosition[0]=xVert; }
  virtual void   SetYv(Double_t yVert) {fPosition[1]=yVert; }
  virtual void   SetZv(Double_t zVert) {fPosition[2]=zVert; }
  virtual void   SetDispersion(Double_t disp) { fSigma=disp; }
  virtual void   SetNContributors(Int_t nContr) {fNContributors=nContr; }

  virtual void     GetXYZ(Double_t position[3]) const;
  virtual Double_t GetXv() const { return fPosition[0]; }
  virtual Double_t GetYv() const { return fPosition[1]; }
  virtual Double_t GetZv() const { return fPosition[2]; }
  virtual Double_t GetDispersion() const { return fSigma; }
  virtual Int_t    GetNContributors() const { return fNContributors; }

  virtual void     Print(Option_t* option = "") const;

 protected:

  Double_t fPosition[3];  // vertex position
  Double_t fSigma;  // track dispersion around found vertex
  Int_t    fNContributors;  // # of tracklets/tracks used for the estimate 


  ClassDef(AliVertex,1)  // Class for Primary Vertex
};

#endif
