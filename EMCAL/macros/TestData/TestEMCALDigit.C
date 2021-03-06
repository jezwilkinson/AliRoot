///
/// \file TestEMCALDigit.C
/// \ingroup EMCAL_TestData
/// \brief Digits reading example
///
/// Test Macro, shows how to load EMCal Digits and Geometry, and how can we get 
/// some of the parameters and variables.
///
/// Pass some parameters to be more or less verbose, geometrical location, 
/// MC primaries and minimum amplitude cut
///
/// \author : Gustavo Conesa Balbastre <Gustavo.Conesa.Balbastre@cern.ch>, (LPSC-CNRS)
///

#if !defined(__CINT__) || defined(__MAKECINT__)

//Root include files 
#include <Riostream.h>
#include <TClonesArray.h>
#include <TGeoManager.h>

//AliRoot include files 
#include "AliRun.h"
#include "AliRunLoader.h"
#include "AliEMCALLoader.h"
#include "AliEMCAL.h"
#include "AliEMCALDigit.h"
#include "AliEMCALGeometry.h"

#endif

///
/// Main execution method
///
/// \param ampMin: print digits above this value
/// \param printMC: print digit MC information
/// \param printGeo: print digit geometrical location
///
void TestEMCALDigit(Float_t ampMin = 6, Bool_t printMC = kFALSE, Bool_t printGeo = kFALSE)
{
  //
  // Get EMCAL Detector and Geometry.
  //
  AliRunLoader *rl = AliRunLoader::Open("galice.root",AliConfig::GetDefaultEventFolderName(),"read");
  
  if (rl == 0x0)
    cout<<"Can not instatiate the Run Loader"<<endl;
  
  rl->LoadgAlice();//Needed to get geometry
  
  AliEMCALLoader *emcalLoader = dynamic_cast<AliEMCALLoader*>
  (rl->GetDetectorLoader("EMCAL"));
  
  AliEMCALGeometry * geom ;
  if(printGeo)
  {
    TGeoManager::Import("geometry.root");
    
    AliRun * alirun   = rl->GetAliRun(); // Needed to get Geometry
    
    if(alirun)
    {
      AliEMCAL * emcal  = (AliEMCAL*) alirun->GetDetector("EMCAL");
      geom = emcal->GetGeometry();
    }
    
    if (geom == 0) cout<<"Did not get geometry from EMCALLoader"<<endl;
    //  else           geom->PrintGeometry();
  }
  
  // Load Digits
  rl->LoadDigits("EMCAL");
  
  // Get maximum number of events
  Int_t maxevent =  rl->GetNumberOfEvents();
  cout<<"Number of events "<<maxevent<<endl;
  //maxevent = 10 ;
  
  Int_t iEvent  = -1 ;
  Float_t amp   = -1 ;
  Float_t time  = -1 ;
  Int_t id      = -1 ;
  Int_t iSupMod =  0 ;
  Int_t iTower  =  0 ;
  Int_t iIphi   =  0 ;
  Int_t iIeta   =  0 ;
  Int_t iphi    =  0 ;
  Int_t ieta    =  0 ;
  Int_t nprimaries = 0 ;
  Int_t nparents   = 0 ;
  Int_t type    = -1;
  
  AliEMCALDigit * dig;
  
  for ( iEvent=0; iEvent<maxevent; iEvent++)
  {
    cout <<  " ======> Event " << iEvent << endl ;
    // Load Event
    rl->GetEvent(iEvent);
    
    // Fill array of digits
    TClonesArray *digits = emcalLoader->Digits();	
    
    // Get digits from the list      
    for(Int_t idig = 0; idig< digits->GetEntries();idig++)
    {
      
      //cout<<">> idig "<<idig<<endl;
      
      dig = static_cast<AliEMCALDigit *>(digits->At(idig)) ;
      
      if(!dig)
      {
        printf("Digit null pointer\n");
        continue;
      }
      
      //
      // Basic digit parameters
      ///
      id   = dig->GetId() ;       // cell (digit) absolute Id. number.
      amp  = dig->GetAmplitude(); // amplitude in cell (digit).
      time = dig->GetTime()*1.e9; // time of creation of digit after collision.
      type = dig->GetType();
      
      if(amp < ampMin) continue ;
      
      printf("*** Cell ID %d, Amplitude %4.2f, Time %4.2f, Type %d\n",id,amp,time,type);
      
      //
      // MC
      //
      if(printMC)
      {
        nprimaries = dig->GetNprimary() ;
        nparents   = dig->GetNiparent() ;
        
        
        if ( nprimaries > 0 || nparents > 0) printf("N primary %d; N parent %d\n", nprimaries, nparents);
        
        if(nprimaries > 0)
        {
          for(Int_t iprim = 0; iprim < nprimaries; iprim++)
            printf(" \t primary %d, label %d, edep %2.3f\n", iprim, dig->GetPrimary(iprim+1), dig->GetDEPrimary(iprim+1));
        }
        
        if(nparents > 0)
        {
          for(Int_t ipar = 0; ipar < nparents; ipar++)
            printf("\t parent  %d, label %d, edep %2.3f\n", ipar , dig->GetIparent(ipar +1), dig->GetDEParent (ipar +1));
        }
      }
      
      //
      // Geometry methods
      //
      if( printGeo && geom)
      {
        // Get SM number and in module (4x4 cells) indexes
        geom->GetCellIndex(id,iSupMod,iTower,iIphi,iIeta); 
        // Get tower cell indexes from SM number and module
        geom->GetCellPhiEtaIndexInSModule(iSupMod,iTower,
                                          iIphi, iIeta,iphi,ieta);
        
        printf("Super module number %d; Module: number %d; eta %d, phi %d; Cell/Tower: eta %d, phi %d\n",
               iSupMod,iTower,iIeta,iIphi,ieta,iphi);
        
      }
    }
  }
  
}

