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

// --- ROOT system ---
#include <TClonesArray.h>
#include <TFile.h> 
#include <TH1F.h> 
#include <TH1I.h> 
#include <TH2F.h> 
#include <TTree.h>

// --- Standard library ---

// --- AliRoot header files ---
#include "AliESDCaloCluster.h"
#include "AliLog.h"
#include "AliEMCALDigit.h"
#include "AliEMCALHit.h"
#include "AliEMCALQADataMakerSim.h"
#include "AliQAChecker.h"
#include "AliEMCALSDigitizer.h"

/// \cond CLASSIMP
ClassImp(AliEMCALQADataMakerSim) ;
/// \endcond
 
///
/// Constructor.
//____________________________________________________________________________ 
AliEMCALQADataMakerSim::AliEMCALQADataMakerSim() : 
  AliQADataMakerSim(AliQAv1::GetDetName(AliQAv1::kEMCAL), "EMCAL Quality Assurance Data Maker")
{}

///
/// Copy Constructor.
//____________________________________________________________________________ 
AliEMCALQADataMakerSim::AliEMCALQADataMakerSim(const AliEMCALQADataMakerSim& qadm) :
  AliQADataMakerSim()
{
  SetName((const char*)qadm.GetName()) ; 
  SetTitle((const char*)qadm.GetTitle()); 
}

///
/// Assignment operator.
//__________________________________________________________________
AliEMCALQADataMakerSim& AliEMCALQADataMakerSim::operator = (const AliEMCALQADataMakerSim& qadm )
{
  this->~AliEMCALQADataMakerSim();
  new(this) AliEMCALQADataMakerSim(qadm);
  return *this;
}

///
/// Detector specific actions at end of cycle
/// do the QA checking
//____________________________________________________________________________ 
void AliEMCALQADataMakerSim::EndOfDetectorCycle(AliQAv1::TASKINDEX_t task, TObjArray ** list)
{
  ResetEventTrigClasses(); // reset triggers list to select all histos
  AliQAChecker::Instance()->Run(AliQAv1::kEMCAL, task, list) ;  
}

///
/// Create Hits histograms in Hits subdir
//____________________________________________________________________________ 
void AliEMCALQADataMakerSim::InitHits()
{
  const Bool_t expert   = kTRUE ; 
  const Bool_t image    = kTRUE ; 
  
  TH1F * h0 = new TH1F("hEmcalHits",    "Hits energy distribution in EMCAL;Energy [MeV];Counts",       200, 0., 2.) ; //GeV
  h0->Sumw2() ;
  Add2HitsList(h0, 0, !expert, image) ;
  
  TH1I * h1  = new TH1I("hEmcalHitsMul", "Hits multiplicity distribution in EMCAL;# of Hits;Entries", 1000, 0, 10000) ; 
  h1->Sumw2() ;
  Add2HitsList(h1, 1, !expert, image) ;
  //
  ClonePerTrigClass(AliQAv1::kHITS); // this should be the last line
}

///
/// Create Digits histograms in Digits subdir
//____________________________________________________________________________ 
void AliEMCALQADataMakerSim::InitDigits()
{
  const Bool_t expert   = kTRUE ; 
  const Bool_t image    = kTRUE ; 
  
  TH1I * h0 = new TH1I("hEmcalDigits",    "Digits amplitude distribution in EMCAL;Amplitude [ADC counts];Counts",    500, 0, 500) ; 
  h0->Sumw2() ;
  Add2DigitsList(h0, 0, !expert, image) ;
  
  TH1I * h1 = new TH1I("hEmcalDigitsMul", "Digits multiplicity distribution in EMCAL;# of Digits;Entries", 200, 0, 2000) ; 
  h1->Sumw2() ;
  Add2DigitsList(h1, 1, !expert, image) ;
  //
  ClonePerTrigClass(AliQAv1::kDIGITS); // this should be the last line
}

///
/// Create SDigits histograms in SDigits subdir
//____________________________________________________________________________ 
void AliEMCALQADataMakerSim::InitSDigits()
{
  const Bool_t expert   = kTRUE ; 
  const Bool_t image    = kTRUE ; 
  
  TH1F * h0 = new TH1F("hEmcalSDigits",    "SDigits energy distribution in EMCAL;Energy [MeV];Counts",       200, 0., 20.) ; 
  h0->Sumw2() ;
  Add2SDigitsList(h0, 0, !expert, image) ;
  
  TH1I * h1 = new TH1I("hEmcalSDigitsMul", "SDigits multiplicity distribution in EMCAL;# of SDigits;Entries", 500, 0,  5000) ; 
  h1->Sumw2() ;
  Add2SDigitsList(h1, 1, !expert, image) ;
  //
  ClonePerTrigClass(AliQAv1::kSDIGITS); // this should be the last line
}

///
/// Make QA data from Hits
//____________________________________________________________________________
void AliEMCALQADataMakerSim::MakeHits()
{ 
  FillHitsData(1,fHitsArray->GetEntriesFast()) ; 
  
  TIter next(fHitsArray) ; 
  AliEMCALHit * hit ; 
  while ( (hit = dynamic_cast<AliEMCALHit *>(next())) )
  {
    FillHitsData(0, hit->GetEnergy()) ;
  }
}

///
/// Make QA data from Hit Tree
//____________________________________________________________________________
void AliEMCALQADataMakerSim::MakeHits(TTree * hitTree)
{
  if (fHitsArray) 
    fHitsArray->Clear() ; 
  else
    fHitsArray = new TClonesArray("AliEMCALHit", 1000);
  
  TBranch * branch = hitTree->GetBranch("EMCAL") ;
  if ( ! branch ) { AliWarning("EMCAL branch in Hit Tree not found") ; return;}
  //
  branch->SetAddress(&fHitsArray) ;
  for (Int_t ientry = 0 ; ientry < branch->GetEntries() ; ientry++) 
  {
    branch->GetEntry(ientry) ; 
    MakeHits() ; 
    fHitsArray->Clear() ; 
  }
  
  IncEvCountCycleHits();
  IncEvCountTotalHits();
  //
}

///
/// Makes data from Digits
//____________________________________________________________________________
void AliEMCALQADataMakerSim::MakeDigits()
{
  FillDigitsData(1,fDigitsArray->GetEntriesFast()) ; 
  
  TIter next(fDigitsArray) ; 
  AliEMCALDigit * digit ; 
  while ( (digit = dynamic_cast<AliEMCALDigit *>(next())) ) 
  {
    FillDigitsData(0, digit->GetAmp()) ;
  }  
}

///
/// Makes data from Digit Tree
//____________________________________________________________________________
void AliEMCALQADataMakerSim::MakeDigits(TTree * digitTree)
{
  if (fDigitsArray) 
    fDigitsArray->Clear("C") ; 
  else
    fDigitsArray = new TClonesArray("AliEMCALDigit", 1000) ; 
  
  TBranch * branch = digitTree->GetBranch("EMCAL") ;
  if ( ! branch ) { AliWarning("EMCAL branch in Digit Tree not found") ; return; }
  //
  branch->SetAddress(&fDigitsArray) ;
  branch->GetEntry(0) ; 
  MakeDigits() ; 
  //
  IncEvCountCycleDigits();
  IncEvCountTotalDigits();
}

///
/// Makes data from SDigits
//____________________________________________________________________________
void AliEMCALQADataMakerSim::MakeSDigits()
{
  // Need a copy of the SDigitizer to calibrate 
  // the sdigit amplitude to energy in GeV
  AliEMCALSDigitizer* sDigitizer = new AliEMCALSDigitizer();

  FillSDigitsData(1,fSDigitsArray->GetEntriesFast()) ; 
  
  TIter next(fSDigitsArray) ; 
  AliEMCALDigit * sdigit ; 
  while ( (sdigit = dynamic_cast<AliEMCALDigit *>(next())) ) 
  {
    FillSDigitsData(0, sDigitizer->Calibrate(sdigit->GetAmp())) ;
  } 
  
  delete sDigitizer;
}

///
/// Makes data from SDigit Tree
//____________________________________________________________________________
void AliEMCALQADataMakerSim::MakeSDigits(TTree * sdigitTree)
{
  if (fSDigitsArray) 
    fSDigitsArray->Clear("C") ; 
  else 
    fSDigitsArray = new TClonesArray("AliEMCALDigit", 1000) ; 
  
  TBranch * branch = sdigitTree->GetBranch("EMCAL") ;
  if ( ! branch ) { AliWarning("EMCAL branch in SDigit Tree not found"); return;}
  //
  branch->SetAddress(&fSDigitsArray);
  branch->GetEntry(0);
  MakeSDigits(); 
  //
  IncEvCountCycleSDigits();
  IncEvCountTotalSDigits();
  //
}

///
/// Detector specific actions at start of cycle, none so far.
//____________________________________________________________________________ 
void AliEMCALQADataMakerSim::StartOfDetectorCycle()
{  }
