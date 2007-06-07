/**************************************************************************
 * Copyright(c) 1998-2007, ALICE Experiment at CERN, All rights reserved. *
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

/* $Id$ */

/**
 * @file   AliHLTMUONRecHitsSource.cxx
 * @author Artur Szostak <artursz@iafrica.com>
 * @date   
 * @brief  Implementation of the AliHLTMUONRecHitsSource component.
 */

#include "AliHLTMUONRecHitsSource.h"
#include "AliHLTMUONConstants.h"
#include "AliHLTMUONDataBlockWriter.h"
#include "AliMUONSimData.h"
#include "AliMUONRecData.h"
#include "AliMUONHit.h"
#include "AliMUONRawCluster.h"
#include "AliMUONConstants.h"
#include "AliRunLoader.h"
#include "AliLoader.h"
#include "TClonesArray.h"
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cassert>
#include <new>

namespace
{
	// The global object used for automatic component registration.
	// Note DO NOT use this component for calculation!
	AliHLTMUONRecHitsSource gAliHLTMUONRecHitsSource;
}


ClassImp(AliHLTMUONRecHitsSource);


AliHLTMUONRecHitsSource::AliHLTMUONRecHitsSource() :
	AliHLTOfflineDataSource(),
	fSimData(NULL), fRecData(NULL),
	fRunLoader(NULL), fLoader(NULL),
	fSelection(kWholePlane)
{
	for (Int_t i = 0; i < AliMUONConstants::NTrackingCh(); i++)
		fServeChamber[i] = false;
}


AliHLTMUONRecHitsSource::~AliHLTMUONRecHitsSource()
{
	assert( fSimData == NULL );
	assert( fRecData == NULL );
	assert( fRunLoader == NULL );
	assert( fLoader == NULL );
}


int AliHLTMUONRecHitsSource::DoInit(int argc, const char** argv)
{
	// Parse the command line arguments:
	bool simdata = false;
	bool recdata = false;
	bool chamberWasSet = false;
	
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-simdata") == 0)
		{
			simdata = true;
		}
		else if (strcmp(argv[i], "-recdata") == 0)
		{
			recdata = true;
		}
		else if (strcmp(argv[i], "-plane") == 0)
		{
			i++;
			if (i >= argc)
			{
				Logging(kHLTLogError,
					"AliHLTMUONRecHitsSource::DoInit",
					"Missing parameter",
					"Expected one of 'left', 'right' or 'all' after '-plane'."
				);
				return EINVAL;
			}
			if (strcmp(argv[i], "left") == 0)
				fSelection = kLeftPlane;
			else if (strcmp(argv[i], "right") == 0)
				fSelection = kRightPlane;
			else if (strcmp(argv[i], "all") == 0)
				fSelection = kWholePlane;
			else
			{
				Logging(kHLTLogError,
					"AliHLTMUONRecHitsSource::DoInit",
					"Invalid parameter",
					"The parameter '%s' is invalid and must be one of 'left',"
					  " 'right' or 'all'.",
					argv[i]
				);
				return EINVAL;
			}
		}
		else if (strcmp(argv[i], "-chamber") == 0)
		{
			i++;
			if (i >= argc)
			{
				Logging(kHLTLogError,
					"AliHLTMUONRecHitsSource::DoInit",
					"Missing parameter",
					"Expected a chamber number, range eg. '1-10' or list eg."
					  " '1,2,3' after '-chamber'."
				);
				return EINVAL;
			}
			int result = ParseChamberString(argv[i]);
			if (result != 0) return result;
			chamberWasSet = true;
		}
		else
		{
			Logging(kHLTLogError,
				"AliHLTMUONRecHitsSource::DoInit",
				"Unknown argument",
				"The argument '%s' is invalid.",
				argv[i]
			);
			return EINVAL;
		}
	}

	// Check the parameters we have parsed.
	if (simdata and recdata)
	{
		Logging(kHLTLogError,
			"AliHLTMUONRecHitsSource::DoInit",
			"Invalid arguments",
			"Cannot have both -simdata and -recdata set."
		);
		return EINVAL;
	}
	
	if (not simdata and not recdata)
	{
		Logging(kHLTLogError,
			"AliHLTMUONRecHitsSource::DoInit",
			"Missing arguments",
			"Must have either -simdata or -recdata specified."
		);
		return EINVAL;
	}
	
	if (not chamberWasSet)
	{
		Logging(kHLTLogInfo,
			"AliHLTMUONRecHitsSource::DoInit",
			"Setting Parameters",
			"No chambers were selected so we will publish for all chambers."
		);
		for (Int_t i = 0; i < AliMUONConstants::NTrackingCh(); i++)
			fServeChamber[i] = true;
	}
	
	// Now we can initialise the data interface objects and loaders.
	if (simdata)
	{
		Logging(kHLTLogDebug,
			"AliHLTMUONRecHitsSource::DoInit",
			"Data interface",
			"Loading simulated GEANT hits with AliMUONSimData."
		);
		
		try
		{
			fSimData = new AliMUONSimData("galice.root");
		}
		catch (const std::bad_alloc&)
		{
			Logging(kHLTLogError,
				"AliHLTMUONRecHitsSource::DoInit",
				"Out of memory",
				"Not enough memory to allocate AliMUONSimData."
			);
			return ENOMEM;
		}
		fLoader = fSimData->GetLoader();
		fLoader->LoadHits("READ");
	}
	else if (recdata)
	{
		Logging(kHLTLogDebug,
			"AliHLTMUONRecHitsSource::DoInit",
			"Data interface",
			"Loading reconstructed clusters with AliMUONRecData."
		);
		
		try
		{
			fRecData = new AliMUONRecData("galice.root");
		}
		catch (const std::bad_alloc&)
		{
			Logging(kHLTLogError,
				"AliHLTMUONRecHitsSource::DoInit",
				"Out of memory",
				"Not enough memory to allocate AliMUONRecData."
			);
			return ENOMEM;
		}
		fLoader = fRecData->GetLoader();
		fLoader->LoadRecPoints("READ");
	}
	
	fRunLoader = AliRunLoader::GetRunLoader();
	
	return 0;
}


int AliHLTMUONRecHitsSource::DoDeinit()
{
	if (fSimData != NULL)
	{
		fLoader->UnloadHits();
		delete fSimData;
		fSimData = NULL;
	}
	if (fRecData != NULL)
	{
		fLoader->UnloadRecPoints();
		delete fRecData;
		fRecData = NULL;
	}
	fRunLoader = NULL;
	fLoader = NULL;
	return 0;
}


const char* AliHLTMUONRecHitsSource::GetComponentID()
{
	return AliHLTMUONConstants::RecHitsSourceId();
}


AliHLTComponentDataType AliHLTMUONRecHitsSource::GetOutputDataType()
{
	return AliHLTMUONConstants::RecHitsBlockDataType();
}


void AliHLTMUONRecHitsSource::GetOutputDataSize(
		unsigned long& constBase, double& inputMultiplier
	)
{
	constBase = sizeof(AliHLTMUONRecHitsBlockStruct) + 1024*4*8;
	inputMultiplier = 0;
}


AliHLTComponent* AliHLTMUONRecHitsSource::Spawn()
{
	return new AliHLTMUONRecHitsSource();
}


int AliHLTMUONRecHitsSource::GetEvent(
		const AliHLTComponentEventData& evtData,
		AliHLTComponentTriggerData& trigData,
		AliHLTUInt8_t* outputPtr, 
		AliHLTUInt32_t& size,
		vector<AliHLTComponentBlockData>& outputBlocks
	)
{
	assert( fSimData != NULL or fRecData != NULL );
	assert( fRunLoader != NULL );
	assert( fLoader != NULL );

	// Check the size of the event descriptor structure.
	if (evtData.fStructSize < sizeof(AliHLTComponentEventData))
	{
		Logging(kHLTLogError,
			"AliHLTMUONRecHitsSource::GetEvent",
			"Invalid event descriptor",
			"The event descriptor (AliHLTComponentEventData) size is"
			  " smaller than expected. It claims to be %d bytes, but"
			  " we expect it to be %d bytes.",
			evtData.fStructSize,
			sizeof(AliHLTComponentEventData)
		);
		size = 0; // Important to tell framework that nothing was generated.
		return EINVAL;
	}
	
	// Use the fEventID as the event number to load, check it and load that
	// event with the runloader.
	UInt_t eventnumber = UInt_t(evtData.fEventID);
	if ( eventnumber >= UInt_t(fRunLoader->GetNumberOfEvents()) )
	{
		Logging(kHLTLogError,
			"AliHLTMUONRecHitsSource::GetEvent",
			"Bad event ID",
			"The event number (%d) is larger than the available number"
			  " of events on file (%d).",
			eventnumber,
			fRunLoader->GetNumberOfEvents()
		);
		size = 0; // Important to tell framework that nothing was generated.
		return EINVAL;
	}
	fRunLoader->GetEvent(eventnumber);
	
	// Create and initialise a new data block.
	AliHLTMUONRecHitsBlockWriter block(outputPtr, size);
	if (not block.InitCommonHeader())
	{
		Logging(kHLTLogError,
			"AliHLTMUONRecHitsSource::GetEvent",
			"Buffer too small",
			"There is not enough buffer space to create a new data block."
			  " We require at least %d bytes but the buffer is only %d bytes.",
			sizeof(AliHLTMUONRecHitsBlockWriter::HeaderType),
			block.BufferSize()
		);
		size = 0; // Important to tell framework that nothing was generated.
		return ENOBUFS;
	}
	
	if (fSimData != NULL)
	{
		Logging(kHLTLogDebug,
			"AliHLTMUONRecHitsSource::GetEvent",
			"Filling hits",
			"Filling data block with GEANT hits for event %d.",
			eventnumber
		);
		
		// Loop over all tracks, extract the hits and write them to the
		// data block.
		fSimData->SetTreeAddress("H");
		for (Int_t i = 0; i < fSimData->GetNtracks(); i++)
		{
			fSimData->GetTrack(i);
			assert( fSimData->Hits() != NULL );
			Int_t nhits = fSimData->Hits()->GetEntriesFast();
			for (Int_t j = 0; j < nhits; j++)
			{
				AliMUONHit* hit = static_cast<AliMUONHit*>(
						fSimData->Hits()->At(j)
					);
				
				// Select only hits on selected chambers.
				Int_t chamber = hit->Chamber() - 1;
				if (chamber > AliMUONConstants::NTrackingCh()) continue;
				if (not fServeChamber[chamber]) continue;
				
				// Only select hits from the given part of the plane
				if (fSelection == kLeftPlane and not (hit->Xref() < 0)) continue;
				if (fSelection == kRightPlane and not (hit->Xref() >= 0)) continue;
				
				AliHLTMUONRecHitStruct* rechit = block.AddEntry();
				if (rechit == NULL)
				{
					Logging(kHLTLogError,
						"AliHLTMUONRecHitsSource::GetEvent",
						"Buffer overflow",
						"There is not enough buffer space to add more hits."
						  " We overflowed the buffer which is only %d bytes.",
						block.BufferSize()
					);
					fSimData->ResetHits();
					size = 0; // Important to tell framework that nothing was generated.
					return ENOBUFS;
				}
				
				rechit->fX = hit->Xref();
				rechit->fY = hit->Yref();
				rechit->fZ = hit->Zref();
			}
			fSimData->ResetHits();
		}
	}
	else if (fRecData != NULL)
	{
		Logging(kHLTLogDebug,
			"AliHLTMUONRecHitsSource::GetEvent",
			"Filling hits",
			"Filling data block with reconstructed raw clusters for event %d.",
			eventnumber
		);
		
		fRecData->SetTreeAddress("RC,TC"); 
		fRecData->GetRawClusters();
		
		// Loop over selected chambers and extract the raw clusters.
		for (Long_t chamber = 0; chamber < AliMUONConstants::NTrackingCh(); chamber++)
		{
			// Select only hits on selected chambers.
			if (not fServeChamber[chamber]) continue;
			
			TClonesArray* clusterarray = fRecData->RawClusters(chamber);
			Int_t nrecpoints = clusterarray->GetEntriesFast();
			for (Int_t i = 0; i < nrecpoints; i++)
			{
				AliMUONRawCluster* cluster = static_cast<AliMUONRawCluster*>(clusterarray->At(i));
				
				// Only select hits from the given part of the plane
				if (fSelection == kLeftPlane and not (cluster->GetX() < 0)) continue;
				if (fSelection == kRightPlane and not (cluster->GetX() >= 0)) continue;
			
				AliHLTMUONRecHitStruct* rechit = block.AddEntry();
				if (rechit == NULL)
				{
					Logging(kHLTLogError,
						"AliHLTMUONRecHitsSource::GetEvent",
						"Buffer overflow",
						"There is not enough buffer space to add more hits."
						  " We overflowed the buffer which is only %d bytes.",
						block.BufferSize()
					);
					fRecData->ResetRawClusters();
					size = 0; // Important to tell framework that nothing was generated.
					return ENOBUFS;
				}
				
				rechit->fX = cluster->GetX();
				rechit->fY = cluster->GetY();
				rechit->fZ = cluster->GetZ();
			}
		}
		
		fRecData->ResetRawClusters();
	}
	else
	{
		Logging(kHLTLogError,
			"AliHLTMUONRecHitsSource::GetEvent",
			"Missing data interface",
			"Neither AliMUONSimData or AliMUONRecData were created."
		);
		size = 0; // Important to tell framework that nothing was generated.
		return EFAULT;
	}
	
	AliHLTComponentBlockData bd;
	FillBlockData(bd);
	bd.fPtr = outputPtr;
	bd.fOffset = 0;
	bd.fSize = block.BytesUsed();
	bd.fDataType = AliHLTMUONConstants::RecHitsBlockDataType();
	bd.fSpecification = 7;
	outputBlocks.push_back(bd);
	size = block.BytesUsed();

	return 0;
}


int AliHLTMUONRecHitsSource::ParseChamberString(const char* str)
{
	char* end = const_cast<char*>(str);
	long lastChamber = -1;
	do
	{
		// Parse the next number.
		char* current = end;
		long chamber = strtol(current, &end, 0);
		
		// Check for parse errors of the number.
		if (current == end)
		{
			Logging(kHLTLogError,
				"AliHLTMUONRecHitsSource::GetEvent",
				"Parse error",
				"Expected a number in the range [1..%d] but got '%s'.",
				AliMUONConstants::NTrackingCh(), current
			);
			return EINVAL;
		}
		if (chamber < 1 or AliMUONConstants::NTrackingCh() < chamber)
		{
			Logging(kHLTLogError,
				"AliHLTMUONRecHitsSource::GetEvent",
				"Parse error",
				"Got the chamber number %d which is outside the valid range of [1..%d].",
				AliMUONConstants::NTrackingCh(), chamber
			);
			return EINVAL;
		}
		
		// Skip any whitespace after the number
		while (*end != '\0' and (*end == ' ' or *end == '\t' or *end == '\r' or *end == '\n')) end++;
		
		// Check if we are dealing with a list or range, or if we are at
		// the end of the string.
		if (*end == '-')
		{
			lastChamber = chamber;
			end++;
			continue;
		}
		else if (*end == ',')
		{
			assert( 1 <= chamber and chamber <= 10 );
			fServeChamber[chamber-1] = true;
			end++;
		}
		else if (*end == '\0')
		{
			assert( 1 <= chamber and chamber <= 10 );
			fServeChamber[chamber-1] = true;
		}
		else
		{
			Logging(kHLTLogError,
				"AliHLTMUONRecHitsSource::GetEvent",
				"Parse error",
				"Could not understand parameter list '%s'. Expected '-', ','"
				  " or end of line but got '%c' at character %d.",
				str, *end, (int)(end - str) +1
			);
			return EINVAL;
		}
		
		// Set the range of chambers to publish for.
		if (lastChamber > 0)
		{
			Int_t min, max;
			if (lastChamber < chamber)
			{
				min = lastChamber;
				max = chamber;
			}
			else
			{
				min = chamber;
				max = lastChamber;
			}
			assert( min >= 1 );
			assert( max <= 10 );
			for (Int_t i = min; i <= max; i++)
				fServeChamber[i-1] = true;
		}
		lastChamber = -1;
	}
	while (*end != '\0');
	return 0;
}
