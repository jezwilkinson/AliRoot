/**************************************************************************
 * To Create PMD Default Pedestal Object in CDb format
 * sjena@cern.ch
 * Mon Nov 22 19:54:27 CET 2010
 * Deafult values are randomly generated by a gausian function
                 
 **************************************************************************/

void MakePMDPedCDB(){

    AliCDBManager* man = AliCDBManager::Instance();
	
    man->SetDefaultStorage("local://CDB_PED");
    
    AliPMDPedestal *pedestal = new AliPMDPedestal();
    
    TRandom random;
    AliCDBId id("PMD/Calib/Ped",0,0);

    const Int_t kDet = 2;
    const Int_t kMod = 24;
    const Int_t kRow = 48;
    const Int_t kCol = 96;
    
    Float_t mean = 100.0;
    
    for(int idet = 0; idet < kDet; idet++)
    {
	for(int imod = 0; imod < kMod; imod++) 
	{
	    for(int irow = 0; irow < kRow; irow++)
	    {
		for(int icol = 0; icol < kCol; icol++)
		{
		    Float_t rms = random.Gaus(15,2);
		    pedestal->SetPedMeanRms(idet, imod, irow, icol,
					    mean, rms);
		    id.SetRunRange(0,50);
		}
	    }
	}
    }
		
    AliCDBMetaData md;
    md.SetResponsible("Satyajit Jena");
    md.SetComment("Default Ped Object");
    man->Put(pedestal, id, &md);
}
