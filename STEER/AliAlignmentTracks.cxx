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

//-----------------------------------------------------------------
//   Implementation of the alignment steering class
//   It provides an access to the track space points
//   written along the esd tracks. The class enables
//   the user to plug any track fitter (deriving from
//   AliTrackFitter class) and minimization fo the
//   track residual sums (deriving from the AliTrackResiduals).
//-----------------------------------------------------------------

#include <TChain.h>
#include <TFile.h>

#include "AliAlignmentTracks.h"
#include "AliTrackPointArray.h"
#include "AliAlignObjAngles.h"
#include "AliTrackFitterRieman.h"
#include "AliTrackResidualsChi2.h"
#include "AliESD.h"
#include "AliLog.h"

ClassImp(AliAlignmentTracks)

//______________________________________________________________________________
AliAlignmentTracks::AliAlignmentTracks():
  fESDChain(0),
  fPointsFilename("AliTrackPoints.root"),
  fPointsFile(0),
  fPointsTree(0),
  fLastIndex(0),
  fArrayIndex(0),
  fIsIndexBuilt(kFALSE),
  fMisalignObjs(0),
  fTrackFitter(0),
  fMinimizer(0)
{
  // Default constructor
  InitIndex();
  InitAlignObjs();
}

//______________________________________________________________________________
AliAlignmentTracks::AliAlignmentTracks(TChain *esdchain):
  fESDChain(esdchain),
  fPointsFilename("AliTrackPoints.root"),
  fPointsFile(0),
  fPointsTree(0),
  fLastIndex(0),
  fArrayIndex(0),
  fIsIndexBuilt(kFALSE),
  fMisalignObjs(0),
  fTrackFitter(0),
  fMinimizer(0)
{
  // Constructor in the case
  // the user provides an already
  // built TChain with ESD trees
  InitIndex();
  InitAlignObjs();
}


//______________________________________________________________________________
AliAlignmentTracks::AliAlignmentTracks(const char *esdfilename, const char *esdtreename):
  fPointsFilename("AliTrackPoints.root"),
  fPointsFile(0),
  fPointsTree(0),
  fLastIndex(0),
  fArrayIndex(0),
  fIsIndexBuilt(kFALSE),
  fMisalignObjs(0),
  fTrackFitter(0),
  fMinimizer(0)
{
  // Constructor in the case
  // the user provides a single ESD file
  // or a directory containing ESD files
  fESDChain = new TChain(esdtreename);
  fESDChain->Add(esdfilename);

  InitIndex();
  InitAlignObjs();
}

//______________________________________________________________________________
AliAlignmentTracks::AliAlignmentTracks(const AliAlignmentTracks &alignment):
  TObject(alignment)
{
  // Copy constructor
  // not implemented
  AliWarning("Copy constructor not implemented!");
}

//______________________________________________________________________________
AliAlignmentTracks& AliAlignmentTracks::operator= (const AliAlignmentTracks& alignment)
{
  // Asignment operator
  // not implemented
  if(this==&alignment) return *this;

  AliWarning("Asignment operator not implemented!");

  ((TObject *)this)->operator=(alignment);

  return *this;
}

//______________________________________________________________________________
AliAlignmentTracks::~AliAlignmentTracks()
{
  // Destructor
  if (fESDChain) delete fESDChain;

  DeleteIndex();
  DeleteAlignObjs();

  delete fTrackFitter;
  delete fMinimizer;

  if (fPointsFile) fPointsFile->Close();
}

//______________________________________________________________________________
void AliAlignmentTracks::AddESD(TChain *esdchain)
{
  // Add a chain with ESD files
  if (fESDChain)
    fESDChain->Add(esdchain);
  else
    fESDChain = esdchain;
}

//______________________________________________________________________________
void AliAlignmentTracks::AddESD(const char *esdfilename, const char *esdtreename)
{
  // Add a single file or
  // a directory to the chain
  // with the ESD files
  if (fESDChain)
    fESDChain->AddFile(esdfilename,TChain::kBigNumber,esdtreename);
  else {
    fESDChain = new TChain(esdtreename);
    fESDChain->Add(esdfilename);
  }
}

//______________________________________________________________________________
void AliAlignmentTracks::ProcessESD()
{
  // Analyzes and filters ESD tracks
  // Stores the selected track space points
  // into the output file

  if (!fESDChain) return;

  AliESD *esd = 0;
  fESDChain->SetBranchAddress("ESD",&esd);

  // Open the output file
  if (fPointsFilename.Data() == "") {
    AliWarning("Incorrect output filename!");
    return;
  }

  TFile *pointsFile = TFile::Open(fPointsFilename,"RECREATE");
  if (!pointsFile || !pointsFile->IsOpen()) {
    AliWarning(Form("Can't open %s !",fPointsFilename.Data()));
    return;
  }

  TTree *pointsTree = new TTree("spTree", "Tree with track space point arrays");
  AliTrackPointArray *array = 0;
  pointsTree->Branch("SP","AliTrackPointArray", &array);

  Int_t ievent = 0;
  while (fESDChain->GetEntry(ievent++)) {
    if (!esd) break;
    Int_t ntracks = esd->GetNumberOfTracks();
    for (Int_t itrack=0; itrack < ntracks; itrack++) {
      AliESDtrack * track = esd->GetTrack(itrack);
      if (!track) continue;
 
      UInt_t status = AliESDtrack::kITSpid; 
      status|=AliESDtrack::kTPCpid; 
      status|=AliESDtrack::kTRDpid; 
      if ((track->GetStatus() & status) != status) continue;

      if (track->GetP() < 0.5) continue;

      array = track->GetTrackPointArray();
      pointsTree->Fill();
    }
  }

  if (!pointsTree->Write()) {
    AliWarning("Can't write the tree with track point arrays!");
    return;
  }

  pointsFile->Close();
}

//______________________________________________________________________________
void AliAlignmentTracks::ProcessESD(TSelector *selector)
{
  AliWarning(Form("ESD processing based on selector is not yet implemented (%p) !",selector));
}

//______________________________________________________________________________
void AliAlignmentTracks::BuildIndex()
{
  // Build index of points tree entries
  // Used for access based on the volume IDs
  if (fIsIndexBuilt) return;

  fIsIndexBuilt = kTRUE;

  TFile *fPointsFile = TFile::Open(fPointsFilename);
  if (!fPointsFile || !fPointsFile->IsOpen()) {
    AliWarning(Form("Can't open %s !",fPointsFilename.Data()));
    return;
  }
  
  //  AliTrackPointArray* array = new AliTrackPointArray;
  AliTrackPointArray* array = 0;
  fPointsTree = (TTree*) fPointsFile->Get("spTree");
  if (!fPointsTree) {
    AliWarning("No pointsTree found!");
    return;
  }
  fPointsTree->SetBranchAddress("SP", &array);

  Int_t nArrays = fPointsTree->GetEntries();
  for (Int_t iArray = 0; iArray < nArrays; iArray++)
    {
      fPointsTree->GetEvent(iArray);
      if (!array) continue;
      for (Int_t ipoint = 0; ipoint < array->GetNPoints(); ipoint++) {
	UShort_t volId = array->GetVolumeID()[ipoint];
	Int_t modId;
	Int_t layerId = AliAlignObj::VolUIDToLayer(volId,modId)
	              - AliAlignObj::kFirstLayer;
	if (!fArrayIndex[layerId][modId]) {
	  //first entry for this volume
	  fArrayIndex[layerId][modId] = new TArrayI(1000);
	}
	else {
	  Int_t size = fArrayIndex[layerId][modId]->GetSize();
	  // If needed allocate new size
	  if (fLastIndex[layerId][modId] >= size)
	    fArrayIndex[layerId][modId]->Set(size + 1000);
	}

	// Check if the index is already filled
	Bool_t fillIndex = kTRUE;
	if (fLastIndex[layerId][modId] != 0) {
	  if ((*fArrayIndex[layerId][modId])[fLastIndex[layerId][modId]-1] == iArray)
	    fillIndex = kFALSE;
	}
	// Fill the index array and store last filled index
	if (fillIndex) {
	  (*fArrayIndex[layerId][modId])[fLastIndex[layerId][modId]] = iArray;
	  fLastIndex[layerId][modId]++;
	}
      }
    }

}

// //______________________________________________________________________________
// void AliAlignmentTracks::BuildIndexLayer(AliAlignObj::ELayerID layer)
// {
// }

// //______________________________________________________________________________
// void AliAlignmentTracks::BuildIndexVolume(UShort_t volid)
// {
// }

//______________________________________________________________________________
void AliAlignmentTracks::InitIndex()
{
  // Initialize the index arrays
  Int_t nLayers = AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer;
  fLastIndex = new Int_t*[nLayers];
  fArrayIndex = new TArrayI**[nLayers];
  for (Int_t iLayer = 0; iLayer < (AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer); iLayer++) {
    fLastIndex[iLayer] = new Int_t[AliAlignObj::LayerSize(iLayer)];
    fArrayIndex[iLayer] = new TArrayI*[AliAlignObj::LayerSize(iLayer)];
    for (Int_t iModule = 0; iModule < AliAlignObj::LayerSize(iLayer); iModule++) {
      fLastIndex[iLayer][iModule] = 0;
      fArrayIndex[iLayer][iModule] = 0;
    }
  }
}

//______________________________________________________________________________
void AliAlignmentTracks::ResetIndex()
{
  // Reset the value of the last filled index
  // Do not realocate memory

  fIsIndexBuilt = kFALSE;
  
  for (Int_t iLayer = 0; iLayer < AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer; iLayer++) {
    for (Int_t iModule = 0; iModule < AliAlignObj::LayerSize(iLayer); iModule++) {
      fLastIndex[iLayer][iModule] = 0;
    }
  }
}

//______________________________________________________________________________
void AliAlignmentTracks::DeleteIndex()
{
  // Delete the index arrays
  // Called by the destructor
  for (Int_t iLayer = 0; iLayer < (AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer); iLayer++) {
    for (Int_t iModule = 0; iModule < AliAlignObj::LayerSize(iLayer); iModule++) {
      if (fArrayIndex[iLayer][iModule]) {
	delete fArrayIndex[iLayer][iModule];
	fArrayIndex[iLayer][iModule] = 0;
      }
    }
    delete [] fLastIndex[iLayer];
    delete [] fArrayIndex[iLayer];
  }
  delete [] fLastIndex;
  delete [] fArrayIndex;
}

//______________________________________________________________________________
Bool_t AliAlignmentTracks::ReadAlignObjs(const char *alignObjFileName, const char* arrayName)
{
  // Read alignment object from a file
  // To be replaced by a call to CDB
  AliWarning(Form("Method not yet implemented (%s in %s) !",arrayName,alignObjFileName));

  return kFALSE;
}

//______________________________________________________________________________
void AliAlignmentTracks::InitAlignObjs()
{
  // Initialize the alignment objects array
  Int_t nLayers = AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer;
  fAlignObjs = new AliAlignObj**[nLayers];
  for (Int_t iLayer = 0; iLayer < (AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer); iLayer++) {
    fAlignObjs[iLayer] = new AliAlignObj*[AliAlignObj::LayerSize(iLayer)];
    for (Int_t iModule = 0; iModule < AliAlignObj::LayerSize(iLayer); iModule++) {
      UShort_t volid = AliAlignObj::LayerToVolUID(iLayer+ AliAlignObj::kFirstLayer,iModule);
      fAlignObjs[iLayer][iModule] = new AliAlignObjAngles("",volid,0,0,0,0,0,0);
    }
  }
}

//______________________________________________________________________________
void AliAlignmentTracks::ResetAlignObjs()
{
  // Reset the alignment objects array
  for (Int_t iLayer = 0; iLayer < (AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer); iLayer++) {
    for (Int_t iModule = 0; iModule < AliAlignObj::LayerSize(iLayer); iModule++)
      fAlignObjs[iLayer][iModule]->SetPars(0,0,0,0,0,0);
  }
}

//______________________________________________________________________________
void AliAlignmentTracks::DeleteAlignObjs()
{
  // Delete the alignment objects array
  for (Int_t iLayer = 0; iLayer < (AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer); iLayer++) {
    for (Int_t iModule = 0; iModule < AliAlignObj::LayerSize(iLayer); iModule++)
      if (fAlignObjs[iLayer][iModule])
	delete fAlignObjs[iLayer][iModule];
    delete [] fAlignObjs[iLayer];
  }
  delete [] fAlignObjs;
  fAlignObjs = 0;
}

//______________________________________________________________________________
void AliAlignmentTracks::Align(Int_t iterations)
{
  // This method is just an example
  // how one can user AlignLayer and
  // AlignVolume methods to construct
  // a custom alignment procedure
  while (iterations > 0) {
    // First inward pass
    AlignLayer(AliAlignObj::kTPC1);
    AlignLayer(AliAlignObj::kSSD2);
    AlignLayer(AliAlignObj::kSSD1);
    AlignLayer(AliAlignObj::kSDD2);
    AlignLayer(AliAlignObj::kSDD1);
    AlignLayer(AliAlignObj::kSPD2);
    AlignLayer(AliAlignObj::kSPD1);
    // Outward pass
    AlignLayer(AliAlignObj::kSPD2);
    AlignLayer(AliAlignObj::kSDD1);
    AlignLayer(AliAlignObj::kSDD2);
    AlignLayer(AliAlignObj::kSSD1);
    AlignLayer(AliAlignObj::kSSD2);
    AlignLayer(AliAlignObj::kTPC1);
    AlignLayer(AliAlignObj::kTPC2);
    AlignLayer(AliAlignObj::kTRD1);
    AlignLayer(AliAlignObj::kTRD2);
    AlignLayer(AliAlignObj::kTRD3);
    AlignLayer(AliAlignObj::kTRD4);
    AlignLayer(AliAlignObj::kTRD5);
    AlignLayer(AliAlignObj::kTRD6);
    AlignLayer(AliAlignObj::kTOF);
    // Again inward
    AlignLayer(AliAlignObj::kTRD6);
    AlignLayer(AliAlignObj::kTRD5);
    AlignLayer(AliAlignObj::kTRD4);
    AlignLayer(AliAlignObj::kTRD3);
    AlignLayer(AliAlignObj::kTRD2);
    AlignLayer(AliAlignObj::kTRD1);
    AlignLayer(AliAlignObj::kTPC2);
  }
}

//______________________________________________________________________________
void AliAlignmentTracks::AlignLayer(AliAlignObj::ELayerID layer,
				    AliAlignObj::ELayerID layerRangeMin,
				    AliAlignObj::ELayerID layerRangeMax,
				    Int_t iterations)
{
  // Align detector volumes within
  // a given layer.
  // Tracks are fitted only within
  // the range defined by the user.
  Int_t nModules = AliAlignObj::LayerSize(layer - AliAlignObj::kFirstLayer);
  while (iterations > 0) {
    for (Int_t iModule = 0; iModule < nModules; iModule++) {
      UShort_t volId = AliAlignObj::LayerToVolUID(layer,iModule);
      AlignVolume(volId,layerRangeMin,layerRangeMax);
    }
    iterations--;
  }
}

//______________________________________________________________________________
void AliAlignmentTracks::AlignVolume(UShort_t volid, UShort_t volidfit,
				     AliAlignObj::ELayerID layerRangeMin,
				     AliAlignObj::ELayerID layerRangeMax,
				     Int_t iterations)
{
  // Align a single detector volume.
  // Tracks are fitted only within
  // the range defined by the user
  // (by layerRangeMin and layerRangeMax)
  // or within the volid2
  // Repeat the procedure 'iterations' times

  // First take the alignment object to be updated
  Int_t iModule;
  AliAlignObj::ELayerID iLayer = AliAlignObj::VolUIDToLayer(volid,iModule);
  AliAlignObj *alignObj = fAlignObjs[iLayer-AliAlignObj::kFirstLayer][iModule];

  // Then load only the tracks with at least one
  // space point in the volume (volid)
  BuildIndex();
  AliTrackPointArray **points;
  // Start the iterations
  while (iterations > 0) {
    Int_t nArrays = LoadPoints(volid, points);
    if (nArrays == 0) return;

    AliTrackResiduals *minimizer = CreateMinimizer();
    minimizer->SetNTracks(nArrays);
    minimizer->SetAlignObj(alignObj);
    AliTrackFitter *fitter = CreateFitter();
    for (Int_t iArray = 0; iArray < nArrays; iArray++) {
      fitter->SetTrackPointArray(points[iArray], kFALSE);
      fitter->Fit(volid,volidfit,layerRangeMin,layerRangeMax);
      AliTrackPointArray *pVolId,*pTrack;
      fitter->GetTrackResiduals(pVolId,pTrack);
      minimizer->AddTrackPointArrays(pVolId,pTrack);
    }
    minimizer->Minimize();
    *alignObj *= *minimizer->GetAlignObj();

    UnloadPoints(nArrays, points);
    
    iterations--;
  }
}
  
//______________________________________________________________________________
Int_t AliAlignmentTracks::LoadPoints(UShort_t volid, AliTrackPointArray** &points)
{
  // Load track point arrays with at least
  // one space point in a given detector
  // volume (volid).
  // Use the already created tree index for
  // fast access.

  if (!fPointsTree) {
    AliWarning("Tree with the space point arrays not initialized!");
    points = 0;
    return 0;
  }

  Int_t iModule;
  AliAlignObj::ELayerID iLayer = AliAlignObj::VolUIDToLayer(volid,iModule);
  Int_t nArrays = fLastIndex[iLayer-AliAlignObj::kFirstLayer][iModule];

  // In case of empty index
  if (nArrays == 0) {
    points = 0;
    return 0;
  }

  AliTrackPointArray* array = 0;
  fPointsTree->SetBranchAddress("SP", &array);

  points = new AliTrackPointArray*[nArrays];
  TArrayI *index = fArrayIndex[iLayer-AliAlignObj::kFirstLayer][iModule];
  AliTrackPoint p;
  for (Int_t iArray = 0; iArray < nArrays; iArray++) {
    fPointsTree->GetEvent((*index)[iArray]);
    if (!array) {
      AliWarning("Wrong space point array index!");
      continue;
    }
    Int_t nPoints = array->GetNPoints();
    points[iArray] = new AliTrackPointArray(nPoints);
    for (Int_t iPoint = 0; iPoint < nPoints; iPoint++) {
      array->GetPoint(p,iPoint);
      Int_t modnum;
      AliAlignObj::ELayerID layer = AliAlignObj::VolUIDToLayer(p.GetVolumeID(),modnum);

      // Misalignment is introduced here
      // Switch it off in case of real
      // alignment job!
      if (fMisalignObjs) {
	AliAlignObj *misalignObj = fMisalignObjs[layer-AliAlignObj::kFirstLayer][modnum];
	if (misalignObj)
	  misalignObj->Transform(p);
      }
      // End of misalignment

      AliAlignObj *alignObj = fAlignObjs[layer-AliAlignObj::kFirstLayer][modnum];
      alignObj->Transform(p);
      points[iArray]->AddPoint(iPoint,&p);
    }
  }

  return nArrays;
}

//______________________________________________________________________________
void AliAlignmentTracks::UnloadPoints(Int_t n, AliTrackPointArray **points)
{
  // Unload track point arrays for a given
  // detector volume
  for (Int_t iArray = 0; iArray < n; iArray++)
    delete points[iArray];
  delete [] points;
}

//______________________________________________________________________________
AliTrackFitter *AliAlignmentTracks::CreateFitter()
{
  // Check if the user has already supplied
  // a track fitter object.
  // If not, create a default one.
  if (!fTrackFitter)
    fTrackFitter = new AliTrackFitterRieman;

  return fTrackFitter;
}

//______________________________________________________________________________
AliTrackResiduals *AliAlignmentTracks::CreateMinimizer()
{
  // Check if the user has already supplied
  // a track residuals minimizer object.
  // If not, create a default one.
  if (!fMinimizer)
    fMinimizer = new AliTrackResidualsChi2;

  return fMinimizer;
}

//______________________________________________________________________________
Bool_t AliAlignmentTracks::Misalign(const char *misalignObjFileName, const char* arrayName)
{
  // The method reads from a file a set of AliAlignObj which are
  // then used to apply misalignments directly on the track
  // space-points. The method is supposed to be used only for
  // fast development and debugging of the alignment algorithms.
  // Be careful not to use it in the case of 'real' alignment
  // scenario since it will bias the results.

  // Initialize the misalignment objects array
  Int_t nLayers = AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer;
  fMisalignObjs = new AliAlignObj**[nLayers];
  for (Int_t iLayer = 0; iLayer < (AliAlignObj::kLastLayer - AliAlignObj::kFirstLayer); iLayer++) {
    fMisalignObjs[iLayer] = new AliAlignObj*[AliAlignObj::LayerSize(iLayer)];
    for (Int_t iModule = 0; iModule < AliAlignObj::LayerSize(iLayer); iModule++)
      fMisalignObjs[iLayer][iModule] = 0x0;
  }

  // Open the misliagnment file and load the array with
  // misalignment objects
  TFile* inFile = TFile::Open(misalignObjFileName,"READ");
  if (!inFile || !inFile->IsOpen()) {
    AliError(Form("Could not open misalignment file %s !",misalignObjFileName));
    return kFALSE;
  }

  TClonesArray* array = ((TClonesArray*) inFile->Get(arrayName));
  if (!array) {
    AliError(Form("Could not find misalignment array %s in the file %s !",arrayName,misalignObjFileName));
    inFile->Close();
    return kFALSE;
  }
  inFile->Close();

  // Store the misalignment objects for further usage  
  Int_t nObjs = array->GetEntriesFast();
  AliAlignObj::ELayerID layerId; // volume layer
  Int_t modId; // volume ID inside the layer
  for(Int_t i=0; i<nObjs; i++)
    {
      AliAlignObj* alObj = (AliAlignObj*)array->UncheckedAt(i);
      alObj->GetVolUID(layerId,modId);
      fMisalignObjs[layerId-AliAlignObj::kFirstLayer][modId] = alObj;
    }
  return kTRUE;
}
