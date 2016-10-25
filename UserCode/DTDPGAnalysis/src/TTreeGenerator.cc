//
// Package:    TTreeGenerator
// Class:      TTreeGenerator
// 
/**\class TTreeGenerator TTreeGenerator.cc MyTools/TTreeGenerator/src/TTreeGenerator.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Carlo BATTILANA, Mario PELLICCIONI
//         Created:  Mon Jan 11 14:59:51 CET 2010
// $Id: TTreeGenerator.cc,v 1.33 2012/07/02 16:43:36 guiducci Exp $
//
// Modificated M.C Fouz March/2016 for TwinMux and to include tracks extrapolation and times variable
// Modifications L. Guiducci July 2016 to include TwinMux output data and clean up legacy trigger information

// user include files
#include "DataFormats/CSCRecHit/interface/CSCSegmentCollection.h"

#include "DataFormats/DTDigi/interface/DTDigi.h"
#include "DataFormats/DTDigi/interface/DTDigiCollection.h"
#include "DataFormats/DTDigi/interface/DTLocalTriggerCollection.h"

#include "DataFormats/DTRecHit/interface/DTRecSegment4DCollection.h"

#include "DataFormats/GeometrySurface/interface/Cylinder.h"

#include "DataFormats/L1DTTrackFinder/interface/L1MuDTChambPhContainer.h"
#include "DataFormats/L1DTTrackFinder/interface/L1MuDTChambThContainer.h"
#include "DataFormats/L1GlobalMuonTrigger/interface/L1MuGMTReadoutCollection.h"

#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerReadoutRecord.h"

#include "DataFormats/Luminosity/interface/LumiDetails.h"
#include "DataFormats/Luminosity/interface/LumiSummary.h"

#include "CondFormats/L1TObjects/interface/L1GtTriggerMenu.h"
#include "CondFormats/DataRecord/interface/L1GtTriggerMenuRcd.h"

#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"

#include "DataFormats/TrackReco/interface/Track.h"

#include "DataFormats/Common/interface/TriggerResults.h"

#include "DataFormats/Scalers/interface/LumiScalers.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "FWCore/Common/interface/TriggerNames.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"

#include "Geometry/Records/interface/GlobalTrackingGeometryRecord.h"
#include "Geometry/CommonDetUnit/interface/GlobalTrackingGeometry.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"  // New trying to avoid crashes in the topology functions
#include "Geometry/DTGeometry/interface/DTLayer.h" // New trying to avoid crashes in the topology functions
#include "Geometry/DTGeometry/interface/DTTopology.h" // New trying to avoid crashes in the topology functions
#include <DataFormats/MuonDetId/interface/DTLayerId.h> // New trying to avoid crashes in the topology functions

#include "MagneticField/Engine/interface/MagneticField.h"

#include "TrackingTools/Records/interface/TrackingComponentsRecord.h"

#include "TrackingTools/TrajectoryState/interface/FreeTrajectoryState.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"

#include <DataFormats/RPCDigi/interface/RPCDigiCollection.h>
#include <DataFormats/RPCRecHit/interface/RPCRecHitCollection.h>
#include <DataFormats/MuonDetId/interface/RPCDetId.h>

#include "CalibMuon/DTDigiSync/interface/DTTTrigBaseSync.h"
#include "CalibMuon/DTDigiSync/interface/DTTTrigSyncFactory.h"

#include "UserCode/DTDPGAnalysis/interface/TTreeGenerator.h"
#include <iostream>
#include <vector>


TTreeGenerator::TTreeGenerator(const edm::ParameterSet& pset)
{
  // get the tTrigDBInfo
  theSync =
      DTTTrigSyncFactory::get()->create(pset.getUntrackedParameter<std::string>("tTrigMode"),
                                pset.getUntrackedParameter<edm::ParameterSet>("tTrigModeConfig"));
  
  //get parameters from the configuration file
  //names of the different event collections
  dtDigiLabel_     = pset.getParameter<edm::InputTag>("dtDigiLabel");
  dtDigiToken_     = consumes<DTDigiCollection>(edm::InputTag(dtDigiLabel_));

  dtSegmentLabel_  = pset.getParameter<edm::InputTag>("dtSegmentLabel");
  dtSegmentToken_  = consumes<DTRecSegment4DCollection>(edm::InputTag(dtSegmentLabel_));

  cscSegmentLabel_ = pset.getParameter<edm::InputTag>("cscSegmentLabel");
  cscSegmentToken_ = consumes<CSCSegmentCollection>(edm::InputTag(cscSegmentLabel_));

  dtTrigTwinMuxInLabel_    = pset.getParameter<edm::InputTag>("dtTrigTwinMuxInLabel");
  dtTrigTwinMuxThLabel_    = pset.getParameter<edm::InputTag>("dtTrigTwinMuxThLabel");
  dtTrigTwinMuxInToken_    = consumes<L1MuDTChambPhContainer>(edm::InputTag(dtTrigTwinMuxInLabel_)); 
  dtTrigTwinMux_ThToken_ = consumes<L1MuDTChambThContainer>(edm::InputTag(dtTrigTwinMuxThLabel_));

  dtTrigTwinMuxOutLabel_  = pset.getParameter<edm::InputTag>("dtTrigTwinMuxOutLabel");
  dtTrigTwinMuxOutToken_  = consumes<L1MuDTChambPhContainer>(edm::InputTag(dtTrigTwinMuxOutLabel_));

  staMuLabel_      = pset.getParameter<edm::InputTag>("staMuLabel");
  staMuToken_      = consumes<reco::MuonCollection>(edm::InputTag(staMuLabel_));

  gmtLabel_        = pset.getParameter<edm::InputTag>("gmtLabel"); // legacy
  gmtToken_        = consumes<L1MuGMTReadoutCollection>(edm::InputTag(gmtLabel_)); //legacy

  triggerTag_      = pset.getParameter<edm::InputTag>("TriggerTag");
  triggerToken_    = consumes<edm::TriggerResults>(edm::InputTag(triggerTag_));

  gtLabel_         = pset.getParameter<edm::InputTag>("gtLabel"); // legacy
  gtToken_         = consumes<L1GlobalTriggerReadoutRecord>(edm::InputTag(gtLabel_)); //legacy

  rpcRecHitLabel_  = pset.getParameter<edm::InputTag>("rpcRecHitLabel");
  rpcRecHitToken_  = consumes<RPCRecHitCollection>(edm::InputTag(rpcRecHitLabel_));

  //max size of the different saved objects (per event)
  digisSize_       = pset.getParameter<int>("dtDigiSize");
  dtsegmentsSize_  = pset.getParameter<int>("dtSegmentSize");
  cscsegmentsSize_ = pset.getParameter<int>("cscSegmentSize");
  dtltTwinMuxOutSize_     = pset.getParameter<int>("dtTrigTwinMuxOutSize");
  dtltTwinMuxInSize_ = pset.getParameter<int>("dtTrigTwinMuxInSize");
  dtltTwinMuxThSize_ = pset.getParameter<int>("dtTrigTwinMuxThSize");
  gmtSize_         = pset.getParameter<int>("gmtSize");  // legacy
  STAMuSize_       = pset.getParameter<int>("STAMuSize");
  rpcRecHitSize_   = pset.getParameter<int>("rpcRecHitSize"); 

  PrimaryVertexTag_ = pset.getParameter<edm::InputTag>("PrimaryVertexTag");
  PrimaryVertexToken_ =consumes<reco::VertexCollection>(edm::InputTag(PrimaryVertexTag_));

  beamSpotTag_      = pset.getParameter<edm::InputTag>("beamSpotTag");
  beamSpotToken_    = consumes<reco::BeamSpot>(edm::InputTag(beamSpotTag_));

  scalersSource_    = pset.getParameter<edm::InputTag>("scalersResults");
  scalersSourceToken_ = consumes<LumiScalersCollection>(edm::InputTag(scalersSource_));
       
  lumiInputTag_      = pset.getParameter<edm::InputTag>("lumiInputTag");
  lumiProducerToken_ = consumes<LumiDetails, edm::InLumi>(lumiInputTag_);

  runOnRaw_        = pset.getParameter<bool>("runOnRaw");
  runOnSimulation_ = pset.getParameter<bool>("runOnSimulation");

  localDTmuons_    = pset.getUntrackedParameter<bool>("localDTmuons","False");

  AnaTrackGlobalMu_= pset.getUntrackedParameter<bool>("AnaTrackGlobalMu","True");  // set to False when problems with tracks of global muons

   bmtfPhInputTag_ = consumes<L1MuDTChambPhContainer>(pset.getParameter<edm::InputTag>("bmtfInputPhDigis"));
   bmtfThInputTag_ = consumes<L1MuDTChambThContainer>(pset.getParameter<edm::InputTag>("bmtfInputThDigis"));
   //bmtfOutputTag_ = consumes<l1t::RegionalMuonCandBxCollection>(pset.getParameter<edm::InputTag>("bmtfOutputDigis"));
  bmtfInputTag_ = pset.getParameter<edm::InputTag>("bmtfOutputDigis");
  bmtfOutputTag_ = consumes<l1t::RegionalMuonCandBxCollection>(edm::InputTag(bmtfInputTag_));


  outFile_         = pset.getParameter<std::string>("outputFile");

  initialize_Tree_variables();

  //counters
  idigis       = 0;
  idtsegments  = 0;
  icscsegments = 0;
  idtltTwinMuxOut     = 0;
  idtltTwinMuxIn     = 0;
  idtltTwinMux_th  = 0;
  imuons       = 0;
  igmtdt       = 0; // legacy
  igmtcands    = 0; // legacy
  igtalgo      = 0; // legacy
  igttt        = 0; // legacy
  ihlt         = 0;
}

void TTreeGenerator::beginLuminosityBlock(edm::LuminosityBlock const& lumiBlock,
                                          edm::EventSetup const& context)
{
   return;
}



void TTreeGenerator::analyze(const edm::Event& event, const edm::EventSetup& context)
{

   theSync->setES(context);

   edm::ESHandle<DTGeometry> dtGeomH;
   context.get<MuonGeometryRecord>().get(dtGeomH);
   const DTGeometry* dtGeom_ = dtGeomH.product();

  //retrieve the beamspot info
  if(!localDTmuons_)     
  {
    edm::Handle<reco::BeamSpot> recoBeamSpotHandle;
    //event.getByLabel(beamSpotTag_ ,recoBeamSpotHandle);  // Doesn't work after 75X
    event.getByToken(beamSpotToken_ ,recoBeamSpotHandle);
    beamspot = *recoBeamSpotHandle; 
    
    //retrieve the luminosity
    edm::Handle<LumiScalersCollection> lumiScalers;
    //event.getByLabel(scalersSource_, lumiScalers);  // Doesn't work after 75X
    event.getByToken(scalersSourceToken_, lumiScalers);
    LumiScalersCollection::const_iterator lumiIt = lumiScalers->begin();
    lumiperblock = lumiIt->instantLumi();
  }

  //retrieve the collections you are interested on in the event
  edm::Handle<DTDigiCollection> dtdigis;
  if(runOnRaw_ && !runOnSimulation_) event.getByToken(dtDigiToken_, dtdigis);

  edm::Handle<DTRecSegment4DCollection> dtsegments4D;
  event.getByToken(dtSegmentToken_, dtsegments4D);

  context.get<GlobalTrackingGeometryRecord>().get(theTrackingGeometry);

  edm::Handle<reco::VertexCollection> privtxs;
  if(!localDTmuons_) event.getByToken(PrimaryVertexToken_, privtxs);
  
  edm::Handle<CSCSegmentCollection> cscsegments;
  event.getByToken(cscSegmentToken_, cscsegments);

  edm::Handle<L1MuDTChambPhContainer> localTriggerTwinMuxOut;
  bool hasPhiTwinMuxOut=false;
  if(runOnRaw_) hasPhiTwinMuxOut=event.getByToken(dtTrigTwinMuxOutToken_,localTriggerTwinMuxOut);

  edm::Handle<L1MuDTChambPhContainer> localTriggerTwinMuxIn;
  bool hasPhiTwinMuxIn=false;
  if(runOnRaw_) hasPhiTwinMuxIn=event.getByToken(dtTrigTwinMuxInToken_,localTriggerTwinMuxIn);

  edm::Handle<L1MuDTChambThContainer> localTriggerTwinMux_Th;
  bool hasThetaTwinMux=false;
  if(runOnRaw_) hasThetaTwinMux=event.getByToken(dtTrigTwinMux_ThToken_,localTriggerTwinMux_Th);

  edm::Handle<reco::MuonCollection> MuList;
  if(!localDTmuons_) event.getByToken(staMuToken_,MuList);

  edm::Handle<L1MuGMTReadoutCollection> gmtrc;   // legacy
  if(runOnRaw_ && !localDTmuons_) event.getByToken(gmtToken_,gmtrc); // legacy

  edm::Handle< L1GlobalTriggerReadoutRecord > gtrc; // legacy
  if(runOnRaw_ && !localDTmuons_) event.getByToken(gtToken_, gtrc); // legacy

  edm::ESHandle<L1GtTriggerMenu> menuRcd;
  context.get<L1GtTriggerMenuRcd>().get(menuRcd) ;
  const L1GtTriggerMenu* menu = menuRcd.product();

  edm::Handle<edm::TriggerResults>  hltresults;
  if(!localDTmuons_) event.getByToken(triggerToken_, hltresults); 

  edm::Handle<RPCRecHitCollection> rpcHits;
  if(!localDTmuons_) event.getByToken(rpcRecHitToken_,rpcHits);

  //clear the containers
  clear_Arrays();

  // Get the propagators
  context.get<TrackingComponentsRecord>().get("SmartPropagatorAny",propagatorAlong);
  context.get<TrackingComponentsRecord>().get("SmartPropagatorAnyOpposite", propagatorOpposite);

  //get the magnetic field
  context.get<IdealMagneticFieldRecord>().get(theBField);

  //Fill the event info block
  runnumber = event.run();
  lumiblock = event.getLuminosityBlock().luminosityBlock();
  eventNumber = event.eventAuxiliary().event();
  timestamp = event.eventAuxiliary().time().value();
  bunchXing = event.eventAuxiliary().bunchCrossing();
  orbitNum = event.eventAuxiliary().orbitNumber();

  // it's filling nothing at the moment lumiDetails->isValid() return false (30/04/2016  M.C.F) 
  if(!localDTmuons_)     
  {
     edm::Handle<LumiDetails> lumiDetails;
     event.getLuminosityBlock().getByToken(lumiProducerToken_, lumiDetails); 
     if(lumiDetails->isValid()){
       beam1Intensity = lumiDetails->lumiBeam1Intensity(bunchXing);
       beam2Intensity = lumiDetails->lumiBeam2Intensity(bunchXing);
     }
  }

  //Primary vertex
  if(!localDTmuons_)     
  {
    if((*privtxs).size() != 0){
      PV_x = (*privtxs)[0].position().x();
      PV_y = (*privtxs)[0].position().y();
      PV_z = (*privtxs)[0].position().z();
    
      PV_xxE = (*privtxs)[0].covariance(0,0);
      PV_yyE = (*privtxs)[0].covariance(1,1);
      PV_zzE = (*privtxs)[0].covariance(2,2);
      PV_xyE = (*privtxs)[0].covariance(0,1);
      PV_xzE = (*privtxs)[0].covariance(0,2);
      PV_yzE = (*privtxs)[0].covariance(1,2);
    
      PV_normchi2 = (*privtxs)[0].chi2()/(*privtxs)[0].ndof();
    
      PV_Nvtx = (*privtxs).size();
    }
    else{
      PV_x   = -999.;
      PV_y   = -999.;
      PV_z   = -999.;
      PV_xxE = -999.;
      PV_yyE = -999.;
      PV_zzE = -999.;
      PV_xyE = -999.;
      PV_xzE = -999.;
      PV_yzE = -999.;
      PV_normchi2 = -999.;
      PV_Nvtx = -999;
    }
    
    //Bmtf_Pt.push_back(-1);
    
  }

  //DIGIS
  if(runOnRaw_ && !runOnSimulation_) fill_digi_variables(dtdigis);

  //DT SEGMENTS
  fill_dtsegments_variables(dtsegments4D, dtGeom_);

  //CSC SEGMENTS
  if(!localDTmuons_) fill_cscsegments_variables(cscsegments);

  //TwinMux
  if(runOnRaw_ && hasPhiTwinMuxIn) fill_twinmuxin_variables(localTriggerTwinMuxIn);
  if(runOnRaw_ && hasPhiTwinMuxOut) fill_twinmuxout_variables(localTriggerTwinMuxOut);
  if(runOnRaw_ && hasThetaTwinMux) fill_twinmuxth_variables(localTriggerTwinMux_Th);

  //MUONS
  if(!localDTmuons_) fill_muons_variables(MuList);

  //GMT
  if(runOnRaw_ && !localDTmuons_) fill_gmt_variables(gmtrc); // legacy

  //GT
  if(runOnRaw_ && !localDTmuons_) fill_gt_variables(gtrc,menu); // legacy
    
  //HLT
  if(!localDTmuons_) fill_hlt_variables(event,hltresults);

  // RPC
  if(!localDTmuons_) fill_rpc_variables(event,rpcHits);
  
  analyzeBMTF(event);
  
  tree_->Fill();

  return;
}


void TTreeGenerator::analyzeBMTF(const edm::Event& event)
{
  //l1bmtf->Reset();

// edm::InputTag dttfSource_("BMTFStage2Digis");
//unsigned int max = 50;
  edm::Handle<L1MuDTChambPhContainer > myL1MuDTChambPhContainer;
  //iEvent.getByLabel("BMTFStage2Digis",myL1MuDTChambPhContainer);
  event.getByToken(bmtfPhInputTag_,myL1MuDTChambPhContainer);
//   if (!myL1MuDTChambPhContainer.isValid()) {
//     edm::LogInfo("L1Prompt") << "can't find L1MuDTChambPhContainer" ;
//     }
//   else l1bmtf->SetDTPH(myL1MuDTChambPhContainer, max);

  edm::Handle<L1MuDTChambThContainer > myL1MuDTChambThContainer;
  //iEvent.getByLabel("BMTFStage2Digis",myL1MuDTChambThContainer);
  event.getByToken(bmtfThInputTag_,myL1MuDTChambThContainer);
//   if (!myL1MuDTChambThContainer.isValid()) {
//     edm::LogInfo("L1Prompt") << "can't find L1MuDTChambThContainer";
//     }
//   else l1bmtf->SetDTTH(myL1MuDTChambThContainer, max);

///Output of BMTF
  int ctr = 0;
  edm::Handle<l1t::RegionalMuonCandBxCollection> mycoll;
  event.getByToken(bmtfOutputTag_,mycoll);
  //const l1t::RegionalMuonCandBxCollection& coll = *mycoll;
  if (mycoll.isValid()) {
        int firstbx = (*mycoll).getFirstBX();
        int lastbx  = (*mycoll).getLastBX() + 1;
        //cout<<firstbx<<"\t"<<lastbx<<"\t"<<(*mycoll).size()<<std::endl;
        for(int i=firstbx; i<lastbx; i++){
            //l1bmtf->SetBMTF(*mycoll, ctr, i);
  				for (auto mu = (*mycoll).begin(i); mu != (*mycoll).end(i); ++mu) {
	    		  //cout<<ctr<<endl;
		    	  ctr++;
			      Bmtf_Pt.push_back(mu->hwPt());
			      Bmtf_Eta.push_back(mu->hwEta());
			      Bmtf_FineBit.push_back(mu->hwHF());
			      Bmtf_Phi.push_back(mu->hwPhi());
		    	  Bmtf_qual.push_back(mu->hwQual());
	    		  Bmtf_ch.push_back(mu->hwSign());
    			  Bmtf_bx.push_back(i);
			      Bmtf_processor.push_back(mu->processor());
    			 // Bmtf__.Bmtf_trAddress.push_back(mu->hwTrackAddress());
			      std::map<int, int>  trAdd;
	    		  trAdd = mu->trackAddress();
		    	  int wheel = pow(-1,trAdd[0]) * trAdd[1];
    			  Bmtf_wh.push_back(wheel);
			      Bmtf_trAddress.push_back(trAdd[2]);
    			  Bmtf_trAddress.push_back(trAdd[3]);
	    		  Bmtf_trAddress.push_back(trAdd[4]);
			      Bmtf_trAddress.push_back(trAdd[5]);

    			} // for mu
    			
		        Bmtf_Size = ctr;
 	 } // for i
  } //if
  else 
      edm::LogInfo("L1Prompt") << "can't find L1MuMBTrackContainer";
  

}


void TTreeGenerator::fill_digi_variables(edm::Handle<DTDigiCollection> dtdigis)
{
  idigis = 0;
  for (DTDigiCollection::DigiRangeIterator dtLayerIdIt = dtdigis->begin(); dtLayerIdIt!=dtdigis->end(); dtLayerIdIt++){
    for (DTDigiCollection::const_iterator digiIt = ((*dtLayerIdIt).second).first;digiIt!=((*dtLayerIdIt).second).second; ++digiIt){
      if(idigis >= digisSize_) return;
      digi_wheel.push_back((*dtLayerIdIt).first.wheel());
      digi_sector.push_back((*dtLayerIdIt).first.sector());
      digi_station.push_back((*dtLayerIdIt).first.station());
      digi_sl.push_back((*dtLayerIdIt).first.superLayer());
      digi_layer.push_back((*dtLayerIdIt).first.layer());
      digi_wire.push_back((*digiIt).wire());
      digi_time.push_back((*digiIt).time());
      idigis++;
    }
  }
  return;
}

void TTreeGenerator::fill_dtsegments_variables(edm::Handle<DTRecSegment4DCollection> segments4D, const DTGeometry* dtGeom_)
{
  idtsegments = 0;
  static TVectorF dummyfloat(1); dummyfloat(0) = -999.;
  for (DTRecSegment4DCollection::id_iterator chambIt = segments4D->id_begin(); chambIt != segments4D->id_end(); ++chambIt){
    DTRecSegment4DCollection::range  range = segments4D->get(*chambIt);
    for (DTRecSegment4DCollection::const_iterator segment4D = range.first; segment4D!=range.second; ++segment4D){
      if(idtsegments >= dtsegmentsSize_) return;
      segm4D_wheel.push_back((*chambIt).wheel());
      segm4D_sector.push_back((*chambIt).sector());
      segm4D_station.push_back((*chambIt).station());
      const bool hasPhi = segment4D->hasPhi();
      const bool hasZed = segment4D->hasZed();
      segm4D_hasPhi.push_back(hasPhi);
      segm4D_hasZed.push_back(hasZed);
      segm4D_x_pos_loc.push_back(segment4D->localPosition().x());
      segm4D_y_pos_loc.push_back(segment4D->localPosition().y());
      segm4D_z_pos_loc.push_back(segment4D->localPosition().z());
      segm4D_x_dir_loc.push_back(segment4D->localDirection().x());
      segm4D_y_dir_loc.push_back(segment4D->localDirection().y());
      segm4D_z_dir_loc.push_back(segment4D->localDirection().z());
      if (hasPhi||hasZed){
	TVectorF hitExpectedPos(12);
	TVectorF hitExpectedWire(12);
	std::vector<DTWireId> wireVector;
	for (int kSL=1; kSL<4; kSL=kSL+1){
	  for (int kL=1; kL<5; kL++){
	    wireVector.push_back(DTWireId((*chambIt).wheel(),(*chambIt).station(),(*chambIt).sector(),kSL,kL,2));
	  }
	}
	int kkk=0;
	for(std::vector<DTWireId>::const_iterator wireIt = wireVector.begin(); wireIt!=wireVector.end(); ++wireIt){
	  int theExpWire=-999;
	  float theExpPos=999;
	  if((*wireIt).station() !=4 || (*wireIt).superLayer() !=2)
	    { // To avoid SL theta in MB4 
	      const DTLayer* layer = dtGeom_->layer(*wireIt);
	      float wireX = layer->specificTopology().wirePosition(wireIt->wire());
	      
	      LocalPoint wirePosInLay(wireX,0,0);
	      GlobalPoint wirePosGlob = layer->toGlobal(wirePosInLay);
	      const DTChamber* chamber = dtGeom_->chamber(wireIt->layerId().chamberId());
	      LocalPoint wirePosInChamber = chamber->toLocal(wirePosGlob);
	      //std::cout<<wirePosInChamber.x()<<" "<<wirePosInChamber.y()<<" "<<wirePosInChamber.z()<<std::endl;
	      LocalPoint segPosAtZWire = segment4D->localPosition()     + segment4D->localDirection()*wirePosInChamber.z()/cos(segment4D->localDirection().theta());
	      if ((kkk<4 || kkk>7)&&hasPhi){
		theExpPos = segPosAtZWire.x();
		theExpWire = layer->specificTopology().channel(segPosAtZWire);
	      }
	      else if ((kkk>=4 && kkk<=7) &&hasZed){
		theExpPos = segPosAtZWire.y();
		LocalPoint passPoint(-segPosAtZWire.y(),segPosAtZWire.x(),segPosAtZWire.z());
		theExpWire = layer->specificTopology().channel(passPoint);
	      }
	    }
	  hitExpectedWire[kkk] = theExpWire;
	  hitExpectedPos[kkk] = theExpPos;
	  kkk++;
	}// END iterator
	
	new ((*segm4D_hitsExpPos)[idtsegments]) TVectorF(hitExpectedPos);
	new ((*segm4D_hitsExpWire)[idtsegments]) TVectorF(hitExpectedWire);
      }
      else {
	new ((*segm4D_hitsExpPos)[idtsegments]) TVectorF(dummyfloat);
	new ((*segm4D_hitsExpWire)[idtsegments]) TVectorF(dummyfloat);
      }
      const GeomDet* geomDet = theTrackingGeometry->idToDet(segment4D->geographicalId());
      const GlobalVector point_glb = geomDet->toGlobal(segment4D->localDirection());
      segm4D_cosx.push_back(point_glb.x());
      segm4D_cosy.push_back(point_glb.y());
      segm4D_cosz.push_back(point_glb.z());
      segm4D_phi.push_back(point_glb.phi());
      segm4D_theta.push_back(point_glb.theta());
      segm4D_eta.push_back(point_glb.eta());
      if(hasPhi) fill_dtphi_info(segment4D->phiSegment(),geomDet);
      else{
	segm4D_t0.push_back(-999.);
	segm4D_vdrift.push_back(-999.);
	segm4D_phinormchi2.push_back(-999.);
	segm4D_phinhits.push_back(-999);
	new ((*segm4D_phiHits_Pos)[idtsegments])    TVectorF(dummyfloat);
	new ((*segm4D_phiHits_PosCh)[idtsegments])  TVectorF(dummyfloat);
	new ((*segm4D_phiHits_PosErr)[idtsegments]) TVectorF(dummyfloat);
	new ((*segm4D_phiHits_Side)[idtsegments])   TVectorF(dummyfloat);
 	new ((*segm4D_phiHits_Wire)[idtsegments])   TVectorF(dummyfloat);
 	new ((*segm4D_phiHits_Layer)[idtsegments])  TVectorF(dummyfloat);
	new ((*segm4D_phiHits_SuperLayer)[idtsegments])  TVectorF(dummyfloat);
	new ((*segm4D_phiHits_Time)[idtsegments])    TVectorF(dummyfloat);
	new ((*segm4D_phiHits_TimeCali)[idtsegments])    TVectorF(dummyfloat);
      }
      if(hasZed) fill_dtz_info(segment4D->zSegment(),geomDet);
      else{
	segm4D_znormchi2.push_back(-999.);
	segm4D_znhits.push_back(-999);
	new ((*segm4D_zHits_Pos)[idtsegments])      TVectorF(dummyfloat);
	new ((*segm4D_zHits_PosCh)[idtsegments])    TVectorF(dummyfloat);
	new ((*segm4D_zHits_PosErr)[idtsegments])   TVectorF(dummyfloat);
	new ((*segm4D_zHits_Side)[idtsegments])     TVectorF(dummyfloat);
	new ((*segm4D_zHits_Wire)[idtsegments])     TVectorF(dummyfloat);
	new ((*segm4D_zHits_Layer)[idtsegments])    TVectorF(dummyfloat);
	new ((*segm4D_zHits_Time)[idtsegments])     TVectorF(dummyfloat);
	new ((*segm4D_zHits_TimeCali)[idtsegments]) TVectorF(dummyfloat);
      }
      idtsegments++;
    }
  }
  return;
}

void TTreeGenerator::fill_dtphi_info(const DTChamberRecSegment2D* phiSeg, const GeomDet* chamb)
{
  std::vector<DTRecHit1D> phirecHitslist = phiSeg->specificRecHits();
  //segment information
  segm4D_t0.push_back(phiSeg->t0());
  segm4D_vdrift.push_back(phiSeg->vDrift());
  segm4D_phinormchi2.push_back(phiSeg->chi2()/phiSeg->degreesOfFreedom());
  //rechits information
  const int nphirecHits = phirecHitslist.size();
  segm4D_phinhits.push_back(nphirecHits);
  TVectorF phiPosRechits(nphirecHits);
  TVectorF phiPosChRechits(nphirecHits);
  TVectorF phiPosErrRechits(nphirecHits);
  TVectorF phiSideRechits(nphirecHits);
  TVectorF phiwireRechits(nphirecHits);
  TVectorF philayerRechits(nphirecHits);
  TVectorF phisuperlayerRechits(nphirecHits);
  TVectorF phiTimeRechits(nphirecHits);
  TVectorF phiTimeCaliRechits(nphirecHits);
  int rechitscounter = 0;
  for(std::vector<DTRecHit1D>::const_iterator recHitsIt = phirecHitslist.begin(); recHitsIt!=phirecHitslist.end(); ++recHitsIt){
    const GeomDet * layer = theTrackingGeometry->idToDet(recHitsIt->wireId().layerId());
    phiPosRechits(rechitscounter)    = recHitsIt->localPosition().x();
    phiPosChRechits(rechitscounter)  = chamb->toLocal(layer->toGlobal(recHitsIt->localPosition())).x();
    phiPosErrRechits(rechitscounter) = recHitsIt->localPositionError().xx();
    phiSideRechits(rechitscounter)   = recHitsIt->lrSide();
    phiwireRechits(rechitscounter)   = recHitsIt->wireId().wire();
    philayerRechits(rechitscounter)  = recHitsIt->wireId().layerId().layer();
    phisuperlayerRechits(rechitscounter)  = recHitsIt->wireId().layerId().superlayer();
    phiTimeRechits(rechitscounter)  = recHitsIt->digiTime();
    float ttrig = theSync->offset(recHitsIt->wireId());
    phiTimeCaliRechits(rechitscounter)  = recHitsIt->digiTime()-ttrig;
    rechitscounter++;
  }
  new ((*segm4D_phiHits_Pos)[idtsegments])    TVectorF(phiPosRechits);
  new ((*segm4D_phiHits_PosCh)[idtsegments])  TVectorF(phiPosChRechits);
  new ((*segm4D_phiHits_PosErr)[idtsegments]) TVectorF(phiPosErrRechits);
  new ((*segm4D_phiHits_Side)[idtsegments])   TVectorF(phiSideRechits);
  new ((*segm4D_phiHits_Wire)[idtsegments])   TVectorF(phiwireRechits);
  new ((*segm4D_phiHits_Layer)[idtsegments])  TVectorF(philayerRechits);
  new ((*segm4D_phiHits_SuperLayer)[idtsegments])  TVectorF(phisuperlayerRechits);
  new ((*segm4D_phiHits_Time)[idtsegments])  TVectorF(phiTimeRechits);
  new ((*segm4D_phiHits_TimeCali)[idtsegments])  TVectorF(phiTimeCaliRechits);
  return;
}

void TTreeGenerator::fill_dtz_info(const DTSLRecSegment2D* zSeg,  const GeomDet* chamb)
{
  std::vector<DTRecHit1D> zrecHitslist = zSeg->specificRecHits();
  segm4D_znormchi2.push_back(zSeg->chi2()/zSeg->degreesOfFreedom());
  //rechits information
  const int nzrecHits = zrecHitslist.size();
  segm4D_znhits.push_back(nzrecHits);
  TVectorF zPosRechits(nzrecHits);
  TVectorF zPosChRechits(nzrecHits);
  TVectorF zPosErrRechits(nzrecHits);
  TVectorF zSideRechits(nzrecHits);
  TVectorF zwireRechits(nzrecHits);
  TVectorF zlayerRechits(nzrecHits);
  TVectorF zTimeRechits(nzrecHits);
  TVectorF zTimeCaliRechits(nzrecHits);
  int rechitscounter = 0;
  for(std::vector<DTRecHit1D>::const_iterator recHitsIt = zrecHitslist.begin(); recHitsIt!=zrecHitslist.end(); ++recHitsIt){
    const GeomDet * layer = theTrackingGeometry->idToDet(recHitsIt->wireId().layerId());
    zPosRechits(rechitscounter)    = recHitsIt->localPosition().y();
    zPosChRechits(rechitscounter)  = chamb->toLocal(layer->toGlobal(recHitsIt->localPosition())).y();
    zPosErrRechits(rechitscounter) = recHitsIt->localPositionError().yy();
    zSideRechits(rechitscounter)   = recHitsIt->lrSide();
    zwireRechits(rechitscounter)   = recHitsIt->wireId().wire();
    zlayerRechits(rechitscounter)  = recHitsIt->wireId().layerId().layer();
    zTimeRechits(rechitscounter)   = recHitsIt->digiTime();
    float ttrig = theSync->offset(recHitsIt->wireId());
    zTimeCaliRechits(rechitscounter)   = recHitsIt->digiTime()-ttrig;
    rechitscounter++;
  }
  new ((*segm4D_zHits_Pos)[idtsegments])    TVectorF(zPosRechits);
  new ((*segm4D_zHits_PosCh)[idtsegments])  TVectorF(zPosChRechits);
  new ((*segm4D_zHits_PosErr)[idtsegments]) TVectorF(zPosErrRechits);
  new ((*segm4D_zHits_Side)[idtsegments])   TVectorF(zSideRechits);
  new ((*segm4D_zHits_Wire)[idtsegments])   TVectorF(zwireRechits);
  new ((*segm4D_zHits_Layer)[idtsegments])  TVectorF(zlayerRechits);
  new ((*segm4D_zHits_Time)[idtsegments])   TVectorF(zTimeRechits);
  new ((*segm4D_zHits_TimeCali)[idtsegments]) TVectorF(zTimeCaliRechits);
  return;
}

void TTreeGenerator::fill_cscsegments_variables(edm::Handle<CSCSegmentCollection> cscsegments)
{
  icscsegments = 0;
  for (CSCSegmentCollection::id_iterator chambIt = cscsegments->id_begin(); chambIt != cscsegments->id_end(); ++chambIt){
    CSCSegmentCollection::range  range = cscsegments->get(*chambIt);
    for (CSCSegmentCollection::const_iterator cscsegment = range.first; cscsegment!=range.second; ++cscsegment){
      if(icscsegments >= cscsegmentsSize_) return;
      cscsegm_ring.push_back((*chambIt).ring());
      cscsegm_chamber.push_back((*chambIt).chamber());
      cscsegm_station.push_back((*chambIt).station());
      const GeomDet* geomDet = theTrackingGeometry->idToDet(cscsegment->geographicalId());
      const GlobalVector point_glb = geomDet->toGlobal(cscsegment->localDirection());
      cscsegm_cosx.push_back(point_glb.x());
      cscsegm_cosy.push_back(point_glb.y());
      cscsegm_cosz.push_back(point_glb.y());
      cscsegm_normchi2.push_back(cscsegment->chi2()/cscsegment->degreesOfFreedom());
      cscsegm_nRecHits.push_back(cscsegment->nRecHits());
      icscsegments++;
    }
  }
  return;
}

void TTreeGenerator::fill_twinmuxout_variables(edm::Handle<L1MuDTChambPhContainer> localTriggerTwinMuxOut)
{
  idtltTwinMuxOut = 0;
  const std::vector<L1MuDTChambPhDigi>*  phTrigs = localTriggerTwinMuxOut->getContainer();
  for(std::vector<L1MuDTChambPhDigi>::const_iterator iph = phTrigs->begin(); iph != phTrigs->end() ; ++iph){
    if(idtltTwinMuxOut >= dtltTwinMuxOutSize_) break;
    if (iph->code()!=7){
      ltTwinMuxOut_wheel.push_back(iph->whNum());
      ltTwinMuxOut_sector.push_back(iph->scNum() + 1); // DTTF[0-11] -> DT[1-12] Sector Numbering
      ltTwinMuxOut_station.push_back(iph->stNum());
      ltTwinMuxOut_quality.push_back(iph->code());
      ltTwinMuxOut_rpcbit.push_back(iph->RpcBit());
      ltTwinMuxOut_bx.push_back(iph->bxNum());
      ltTwinMuxOut_phi.push_back(iph->phi());
      ltTwinMuxOut_phiB.push_back(iph->phiB());
      ltTwinMuxOut_is2nd.push_back(iph->Ts2Tag());
      idtltTwinMuxOut++;
    }
  }
  return;
}

void TTreeGenerator::fill_twinmuxin_variables(edm::Handle<L1MuDTChambPhContainer> localTriggerTwinMuxIn)
{
  idtltTwinMuxIn=0;
  const std::vector<L1MuDTChambPhDigi>*  phTrigs = localTriggerTwinMuxIn->getContainer();
  for(std::vector<L1MuDTChambPhDigi>::const_iterator iph = phTrigs->begin(); iph != phTrigs->end() ; ++iph){
    if(idtltTwinMuxIn >= dtltTwinMuxInSize_) break;
    if (iph->code()!=7){
      ltTwinMuxIn_wheel.push_back(iph->whNum());
      ltTwinMuxIn_sector.push_back(iph->scNum() + 1); // DTTF[0-11] -> DT[1-12] Sector Numbering
      ltTwinMuxIn_station.push_back(iph->stNum());
      ltTwinMuxIn_quality.push_back(iph->code());
      if (iph->Ts2Tag()==1) ltTwinMuxIn_bx.push_back(iph->bxNum()-1);
      else ltTwinMuxIn_bx.push_back(iph->bxNum());
      ltTwinMuxIn_phi.push_back(iph->phi());
      ltTwinMuxIn_phiB.push_back(iph->phiB());
      ltTwinMuxIn_is2nd.push_back(iph->Ts2Tag());
      idtltTwinMuxIn++;
    }
  }
  return;
}

void TTreeGenerator::fill_twinmuxth_variables(edm::Handle<L1MuDTChambThContainer> localTriggerTwinMux_Th)
{
  idtltTwinMux_th = 0;
  const std::vector<L1MuDTChambThDigi>*  thTrigs = localTriggerTwinMux_Th->getContainer();
  for(std::vector<L1MuDTChambThDigi>::const_iterator ith = thTrigs->begin(); ith != thTrigs->end() ; ++ith){
    if(idtltTwinMux_th >= dtltTwinMuxThSize_) break;
    ltTwinMux_thWheel.push_back(ith->whNum());
    ltTwinMux_thSector.push_back(ith->scNum() + 1); // DTTF[0-11] -> DT[1-12] Sector Numbering
    ltTwinMux_thStation.push_back(ith->stNum());
    ltTwinMux_thBx.push_back(ith->bxNum());
    unsigned short int thcode=0;
    for (int pos=0; pos<7; pos++)
      if (ith->code(pos))
      thcode=thcode | (0x1<<pos);
    ltTwinMux_thHits.push_back(thcode);
    idtltTwinMux_th++;
  }
  return;
}

void TTreeGenerator::fill_muons_variables(edm::Handle<reco::MuonCollection> MuList)
{
  imuons = 0;
  for (reco::MuonCollection::const_iterator nmuon = MuList->begin(); nmuon != MuList->end(); ++nmuon){
    if(!(nmuon->isStandAloneMuon())) continue;
    if(imuons >= STAMuSize_) break;
    const reco::TrackRef mutrackref = nmuon->outerTrack();
    STAMu_isMuGlobal.push_back(nmuon->isGlobalMuon());
    STAMu_isMuTracker.push_back(nmuon->isTrackerMuon());
    STAMu_numberOfChambers.push_back(nmuon->numberOfChambers());
    STAMu_numberOfMatches.push_back(nmuon->numberOfMatches());
    STAMu_numberOfHits.push_back(mutrackref->numberOfValidHits());
    Mu_px_mu.push_back(nmuon->px());
    Mu_py_mu.push_back(nmuon->py());
    Mu_pz_mu.push_back(nmuon->pz());
    Mu_phi_mu.push_back(nmuon->phi());
    Mu_eta_mu.push_back(nmuon->eta());
    STAMu_recHitsSize.push_back(mutrackref->recHitsSize());
    STAMu_normchi2Mu.push_back(mutrackref->chi2()/mutrackref->ndof());
    STAMu_chargeMu.push_back(mutrackref->charge());
    STAMu_dxyMu.push_back(mutrackref->dxy(beamspot.position()));
    STAMu_dzMu.push_back(mutrackref->dz(beamspot.position()));
    int segmIndex = 0;
    int segmWord = 0;
    std::vector<int> segmIndex_container;
    for (trackingRecHit_iterator recMu = mutrackref->recHitsBegin(); recMu!=mutrackref->recHitsEnd(); recMu++){
      DetId detid = (*recMu)->geographicalId(); 
      if(detid.subdetId() != MuonSubdetId::DT) continue;
      DTChamberId recChamb(detid);
      const short recWheel   = recChamb.wheel();
      const short recSector  = recChamb.sector();
      const short recStation = recChamb.station();
      //loop over the saved segments and find the position of the rechits
      //This is the quickest way to do this search: find the sector (highest number of
      //combinations), loop over the find iterator, and search for wheel and stations
      std::vector<short>::iterator sectorIt = std::find(segm4D_sector.begin(),segm4D_sector.end(),recSector);
      while(sectorIt != segm4D_sector.end()){
	segmIndex = (short) distance(segm4D_sector.begin(),sectorIt);
	if(recWheel == segm4D_wheel.at(segmIndex) && recStation == segm4D_station.at(segmIndex))
	  if(find(segmIndex_container.begin(),segmIndex_container.end(),segmIndex) == segmIndex_container.end()){
	    segmIndex_container.push_back(segmIndex);
	    segmWord |= (1 << segmIndex);
	  }
	sectorIt = std::find(sectorIt+1,segm4D_sector.end(),recSector);
      }
    }
    STAMu_segmIndex.push_back(segmWord);
    if(nmuon->isGlobalMuon() & AnaTrackGlobalMu_) { 
      // This part  gives problems in MWGR16. All the calls glbmutrackref->... give similar error  (different Product ID number)
      // RefCore: A request to resolve a reference to a product of type 'std::vector<reco::Track>' with ProductID '2:533'
      // can not be satisfied because the product cannot be found.
      // The problem is due to the fact that the Muon collections exists, 
      // vector<reco::Muon>                    "muons"                     ""                "RECO"
      // and also the track collecions for STA, 
      // but not for GLOBAL!!!
      //  vector<reco::Track>                   "globalCosmicMuons"         ""                "RECO"    
      // AnalyzeTracksFromGlobalMuons_
      // An extra variable is add  to control from python the possibility of skiping this part of the code
      //
      const reco::TrackRef glbmutrackref = nmuon->innerTrack();
      GLBMu_normchi2Mu.push_back(glbmutrackref->chi2()/glbmutrackref->ndof());
      GLBMu_dxyMu.push_back(glbmutrackref->dxy(beamspot.position()));
      GLBMu_dzMu.push_back(glbmutrackref->dz(beamspot.position()));
      
      GLBMu_numberOfPixelHits.push_back(glbmutrackref->hitPattern().numberOfValidPixelHits());
      GLBMu_numberOfTrackerHits.push_back(glbmutrackref->hitPattern().numberOfValidTrackerHits());
      
      GLBMu_tkIsoR03.push_back(nmuon->isolationR03().sumPt);
      GLBMu_ntkIsoR03.push_back(nmuon->isolationR03().nTracks);
      GLBMu_emIsoR03.push_back(nmuon->isolationR03().emEt);
      GLBMu_hadIsoR03.push_back(nmuon->isolationR03().hadEt);
    }
    else{
      GLBMu_normchi2Mu.push_back(-999.);
      GLBMu_dxyMu.push_back(-999.);
      GLBMu_dzMu.push_back(-999.);
      GLBMu_numberOfPixelHits.push_back(-999);
      GLBMu_numberOfTrackerHits.push_back(-999);
      GLBMu_tkIsoR03.push_back(-999.);
      GLBMu_ntkIsoR03.push_back(-999.);
      GLBMu_emIsoR03.push_back(-999.);
      GLBMu_hadIsoR03.push_back(-999.);
    }
    if(nmuon->isCaloCompatibilityValid()) STAMu_caloCompatibility.push_back(nmuon->caloCompatibility());
    else STAMu_caloCompatibility.push_back(-999.);
    //extrapolate the muon to the MB2
    TrajectoryStateOnSurface tsos;
    tsos = cylExtrapTrkSam(mutrackref,500.);  // track at MB2 radius - extrapolation
    if (tsos.isValid()){
      static const float pig = acos(-1.);
      const double xx = tsos.globalPosition().x();
      const double yy = tsos.globalPosition().y();
      const double zz = tsos.globalPosition().z();
      const double rr       = sqrt(xx*xx + yy*yy);
      const double cosphi   = xx/rr;
      const double abspseta = -log(tan(atan(fabs(rr/zz))/2.));
      STAMu_z_mb2.push_back(zz);
      if (yy>=0) STAMu_phi_mb2.push_back(acos(cosphi));
      else       STAMu_phi_mb2.push_back(2*pig-acos(cosphi));
      if (zz>=0) STAMu_pseta_mb2.push_back(abspseta);
      else       STAMu_pseta_mb2.push_back(-abspseta);
    }
    else{
      STAMu_z_mb2.push_back(-999.);
      STAMu_phi_mb2.push_back(-999.);
      STAMu_pseta_mb2.push_back(-999.);
    }
    imuons++;
  }
  return;
}

void TTreeGenerator::fill_gmt_variables(edm::Handle<L1MuGMTReadoutCollection> gmtrc) // legacy
{
  igmtdt = 0;
  igmtcands = 0;
  std::vector<L1MuGMTReadoutRecord> gmt_records = gmtrc->getRecords();
  for(std::vector<L1MuGMTReadoutRecord>::const_iterator igmtrr=gmt_records.begin(); igmtrr!=gmt_records.end(); igmtrr++) {
    //loop over the different subdetector cands
    for(int i=0;i<4;i++){
      std::vector<L1MuRegionalCand> cands = getBXCands(&(*igmtrr),i);
      for(std::vector<L1MuRegionalCand>::const_iterator candIt = cands.begin(); candIt!=cands.end(); ++candIt){
	if(igmtdt >= gmtSize_) break;
	if(!candIt->empty()){
	  gmt_bx.push_back((*candIt).bx());
	  gmt_phi.push_back((*candIt).phiValue());
	  gmt_eta.push_back((*candIt).etaValue());
	  gmt_pt.push_back((*candIt).ptValue());
	  gmt_qual.push_back((*candIt).quality());
	  gmt_detector.push_back(i);
	  igmtdt++;
	}
      }
    }
    std::vector<L1MuGMTExtendedCand> candsOut = igmtrr->getGMTCands();
    for(std::vector<L1MuGMTExtendedCand>::const_iterator candOutIt  = candsOut.begin(); candOutIt!=candsOut.end(); ++candOutIt){
      if(igmtcands >= gmtSize_) break;
      if(!candOutIt->empty()){
	gmt_cands_fwd.push_back((*candOutIt).isFwd());
	gmt_cands_isRpc.push_back((*candOutIt).isRPC());
	gmt_cands_bx.push_back((*candOutIt).bx());
	gmt_cands_phi.push_back((*candOutIt).phiValue());
	gmt_cands_eta.push_back((*candOutIt).etaValue());
	gmt_cands_pt.push_back((*candOutIt).ptValue());
	gmt_cands_qual.push_back((*candOutIt).quality());
	gmt_cands_ismatched.push_back((*candOutIt).isMatchedCand());
	igmtcands++;
      }
    }
  }
  return;
}

void TTreeGenerator::fill_gt_variables(edm::Handle<L1GlobalTriggerReadoutRecord> gtrr, const L1GtTriggerMenu* menu) // CB FIXME speedup // legacy
{
  igtalgo = 0;
  igttt   = 0;
  const AlgorithmMap algoMap = menu->gtAlgorithmMap();
  const AlgorithmMap ttMap   = menu->gtTechnicalTriggerMap();
  for(int ibx=0; ibx<5; ibx++) {
    DecisionWord gtDecisionWord = gtrr->decisionWord(ibx-2);
    if (!gtDecisionWord.empty()) {
      CItAlgo algoMapIt  = algoMap.begin();
      CItAlgo algoMapEnd = algoMap.end();
      for(; algoMapIt!=algoMapEnd; ++algoMapIt) {
	if( menu->gtAlgorithmResult((*algoMapIt).first, gtDecisionWord) ) {
	  // gt_algo_bit.push_back(TString((*algoMapIt).first));
	  gt_algo_bit.push_back((algoMapIt->second).algoBitNumber());
	  gt_algo_bx.push_back(ibx-2);
	  igtalgo++;
	}
      }
    }
    TechnicalTriggerWord gtTTWord = gtrr->technicalTriggerWord(ibx-2);
    if (!gtTTWord.empty()) {
      CItAlgo ttMapIt  = ttMap.begin();
      CItAlgo ttMapEnd = ttMap.end();
      for(; ttMapIt!=ttMapEnd; ++ttMapIt) {
	int bitNumber = (ttMapIt->second).algoBitNumber();
	if (gtTTWord.at(bitNumber)) {
	  //	  gt_tt_bit.push_back(TString(ttMapIt->first));
	  gt_tt_bit.push_back((ttMapIt->second).algoBitNumber());
	  gt_tt_bx.push_back(ibx-2);
	  igttt++;
	}
      }
    }
  }
  return;
}

void TTreeGenerator::fill_hlt_variables(const edm::Event &e, edm::Handle<edm::TriggerResults> hltresults)
{
  ihlt = 0; 
  const edm::TriggerNames TrigNames_ = e.triggerNames(*hltresults);
  const int ntrigs = hltresults->size();

  for (int itr=0; itr<ntrigs; itr++){
    TString trigName=TrigNames_.triggerName(itr);
    if (hltresults->accept(itr)) {
      hlt_path.push_back(trigName);
      ihlt++;
    }
  }
  return;
}

void TTreeGenerator::fill_rpc_variables(const edm::Event &e, edm::Handle<RPCRecHitCollection> rpcrechits){
  RPCRecHitCollection::const_iterator recHit;
  irpcrechits=0;
  for(recHit = rpcrechits->begin(); recHit != rpcrechits->end(); recHit++){ 
    int cls = recHit->clusterSize();
    int firststrip = recHit->firstClusterStrip();
    int bx = recHit->BunchX();
    RPCDetId rpcId = recHit->rpcId();
    int region = rpcId.region();
    int stat = rpcId.station();
    int sect = rpcId.sector();
    int layer = rpcId.layer();
    int subsector = rpcId.subsector();
    int roll = rpcId.roll();
    int ring = rpcId.ring();
    rpc_region.push_back(region);
    rpc_clusterSize.push_back(cls);
    rpc_strip.push_back(firststrip);
    rpc_bx.push_back(bx);
    rpc_station.push_back(stat);
    rpc_sector.push_back(sect);
    rpc_layer.push_back(layer);
    rpc_subsector.push_back(subsector);
    rpc_roll.push_back(roll);
    rpc_ring.push_back(ring);
    irpcrechits++;
  }
  return;
}

std::vector<L1MuRegionalCand> TTreeGenerator::getBXCands(const L1MuGMTReadoutRecord* igmtrr, const int DetectorType) const
{
  if(DetectorType == 0) return igmtrr->getDTBXCands();
  else if(DetectorType == 1) return igmtrr->getCSCCands();
  else if(DetectorType == 2) return igmtrr->getBrlRPCCands();
  else if(DetectorType == 3) return igmtrr->getFwdRPCCands();
  return igmtrr->getDTBXCands();
}


void TTreeGenerator::beginJob()
{
  outFile = new TFile(outFile_.c_str(), "RECREATE", "");
  outFile->cd();

  tree_ = new TTree ("DTTree", "CMSSW DT tree");

  //Event info
  tree_->Branch("runnumber",&runnumber,"runnumber/I");
  tree_->Branch("lumiblock",&lumiblock,"lumiblock/I");
  tree_->Branch("eventNumber",&eventNumber,"eventNumber/I");
  tree_->Branch("timestamp",&timestamp,"timestamp/F");
  tree_->Branch("bunchXing",&bunchXing,"bunchXing/I");
  tree_->Branch("orbitNum",&orbitNum,"orbitNum/I");

  //Primary vertex
  tree_->Branch("PV_x",&PV_x,"PV_x/F");
  tree_->Branch("PV_y",&PV_y,"PV_y/F");
  tree_->Branch("PV_z",&PV_z,"PV_z/F");

  tree_->Branch("PV_xxE",&PV_xxE,"PV_xxE/F");
  tree_->Branch("PV_yyE",&PV_yyE,"PV_yyE/F");
  tree_->Branch("PV_zzE",&PV_zzE,"PV_zzE/F");
  tree_->Branch("PV_xyE",&PV_xyE,"PV_xyE/F");
  tree_->Branch("PV_xzE",&PV_xzE,"PV_xzE/F");
  tree_->Branch("PV_yzE",&PV_yzE,"PV_yzE/F");

  tree_->Branch("PV_normchi2",&PV_normchi2,"PV_normch2/F");
  tree_->Branch("PV_Nvtx",&PV_Nvtx,"PV_Nvtx/F");

  //luminosity
  tree_->Branch("lumiperblock",&lumiperblock,"lumiperblock/F");
  tree_->Branch("beam1Intensity",&beam1Intensity,"beam1Intensity/F");
  tree_->Branch("beam2Intensity",&beam2Intensity,"beam2Intensity/F");

  //HLT
  tree_->Branch("hlt_path",&hlt_path,32000,-1);

  //digi variables
  tree_->Branch("digi_wheel",&digi_wheel);
  tree_->Branch("digi_sector",&digi_sector);
  tree_->Branch("digi_station",&digi_station);
  tree_->Branch("digi_sl",&digi_sl);
  tree_->Branch("digi_layer",&digi_layer);
  tree_->Branch("digi_wire",&digi_wire);
  tree_->Branch("digi_time",&digi_time);

  //DT segment variables
  tree_->Branch("dtsegm4D_wheel",&segm4D_wheel);
  tree_->Branch("dtsegm4D_sector",&segm4D_sector);
  tree_->Branch("dtsegm4D_station",&segm4D_station);

  tree_->Branch("dtsegm4D_hasPhi",&segm4D_hasPhi);
  tree_->Branch("dtsegm4D_hasZed",&segm4D_hasZed);
  tree_->Branch("dtsegm4D_x_pos_loc",&segm4D_x_pos_loc);
  tree_->Branch("dtsegm4D_y_pos_loc",&segm4D_y_pos_loc);
  tree_->Branch("dtsegm4D_z_pos_loc",&segm4D_z_pos_loc);
  tree_->Branch("dtsegm4D_x_dir_loc",&segm4D_x_dir_loc);
  tree_->Branch("dtsegm4D_y_dir_loc",&segm4D_y_dir_loc);
  tree_->Branch("dtsegm4D_z_dir_loc",&segm4D_z_dir_loc);
  tree_->Branch("dtsegm4D_cosx",&segm4D_cosx);
  tree_->Branch("dtsegm4D_cosy",&segm4D_cosy);
  tree_->Branch("dtsegm4D_cosz",&segm4D_cosz);
  tree_->Branch("dtsegm4D_phi",&segm4D_phi);
  tree_->Branch("dtsegm4D_theta",&segm4D_theta);
  tree_->Branch("dtsegm4D_eta",&segm4D_eta);
  tree_->Branch("dtsegm4D_t0",&segm4D_t0);
  tree_->Branch("dtsegm4D_vdrift",&segm4D_vdrift);
  tree_->Branch("dtsegm4D_phinormchisq",&segm4D_phinormchi2);
  tree_->Branch("dtsegm4D_phinhits",&segm4D_phinhits);
  tree_->Branch("dtsegm4D_znormchisq",&segm4D_znormchi2);
  tree_->Branch("dtsegm4D_znhits",&segm4D_znhits);

  tree_->Branch("dtsegm4D_hitsExpPos", &segm4D_hitsExpPos, 2048000,0);
  tree_->Branch("dtsegm4D_hitsExpWire", &segm4D_hitsExpWire, 2048000,0);

  //rechits info
  tree_->Branch("dtsegm4D_phi_hitsPos",&segm4D_phiHits_Pos,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsPosCh",&segm4D_phiHits_PosCh,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsPosErr",&segm4D_phiHits_PosErr,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsSide",&segm4D_phiHits_Side,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsWire",&segm4D_phiHits_Wire,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsLayer",&segm4D_phiHits_Layer,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsSuperLayer",&segm4D_phiHits_SuperLayer,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsTime",&segm4D_phiHits_Time,2048000,0);
  tree_->Branch("dtsegm4D_phi_hitsTimeCali",&segm4D_phiHits_TimeCali,2048000,0);

  tree_->Branch("dtsegm4D_z_hitsPos",&segm4D_zHits_Pos,2048000,0);
  tree_->Branch("dtsegm4D_z_hitsPosCh",&segm4D_zHits_PosCh,2048000,0);
  tree_->Branch("dtsegm4D_z_hitsPosErr",&segm4D_zHits_PosErr,2048000,0);
  tree_->Branch("dtsegm4D_z_hitsSide",&segm4D_zHits_Side,2048000,0);
  tree_->Branch("dtsegm4D_z_hitsWire",&segm4D_zHits_Wire,2048000,0);
  tree_->Branch("dtsegm4D_z_hitsLayer",&segm4D_zHits_Layer,2048000,0);
  tree_->Branch("dtsegm4D_z_hitsTime",&segm4D_zHits_Time,2048000,0);
  tree_->Branch("dtsegm4D_z_hitsTimeCali",&segm4D_zHits_TimeCali,2048000,0);

  //CSC segment variables
  tree_->Branch("cscsegm_ring",&cscsegm_ring);
  tree_->Branch("cscsegm_chamber",&cscsegm_chamber);
  tree_->Branch("cscsegm_station",&cscsegm_station);
  tree_->Branch("cscsegm_cosx",&cscsegm_cosx);
  tree_->Branch("cscsegm_cosy",&cscsegm_cosy);
  tree_->Branch("cscsegm_cosz",&cscsegm_cosz);
  tree_->Branch("cscsegm_phi",&cscsegm_phi);
  tree_->Branch("cscsegm_eta",&cscsegm_eta);
  tree_->Branch("cscsegm_normchisq",&cscsegm_normchi2);
  tree_->Branch("cscsegm_nRecHits",&cscsegm_nRecHits);
 
  //Twinmux variables
  tree_->Branch("ltTwinMuxIn_wheel",&ltTwinMuxIn_wheel);
  tree_->Branch("ltTwinMuxIn_sector",&ltTwinMuxIn_sector);
  tree_->Branch("ltTwinMuxIn_station",&ltTwinMuxIn_station);
  tree_->Branch("ltTwinMuxIn_quality",&ltTwinMuxIn_quality);
  tree_->Branch("ltTwinMuxIn_bx",&ltTwinMuxIn_bx);
  tree_->Branch("ltTwinMuxIn_phi",&ltTwinMuxIn_phi);
  tree_->Branch("ltTwinMuxIn_phiB",&ltTwinMuxIn_phiB);
  tree_->Branch("ltTwinMuxIn_is2nd",&ltTwinMuxIn_is2nd);

  tree_->Branch("ltTwinMuxOut_wheel",&ltTwinMuxOut_wheel);
  tree_->Branch("ltTwinMuxOut_sector",&ltTwinMuxOut_sector);
  tree_->Branch("ltTwinMuxOut_station",&ltTwinMuxOut_station);
  tree_->Branch("ltTwinMuxOut_quality",&ltTwinMuxOut_quality);
  tree_->Branch("ltTwinMuxOut_rpcbit",&ltTwinMuxOut_rpcbit);
  tree_->Branch("ltTwinMuxOut_bx",&ltTwinMuxOut_bx);
  tree_->Branch("ltTwinMuxOut_phi",&ltTwinMuxOut_phi);
  tree_->Branch("ltTwinMuxOut_phiB",&ltTwinMuxOut_phiB);
  tree_->Branch("ltTwinMuxOut_is2nd",&ltTwinMuxOut_is2nd);

  tree_->Branch("ltTwinMux_thWheel",&ltTwinMux_thWheel);
  tree_->Branch("ltTwinMux_thSector",&ltTwinMux_thSector);
  tree_->Branch("ltTwinMux_thStation",&ltTwinMux_thStation);
  tree_->Branch("ltTwinMux_thBx",&ltTwinMux_thBx);
  tree_->Branch("ltTwinMux_thHits",&ltTwinMux_thHits);

  //muon variables
  tree_->Branch("Mu_isMuGlobal",&STAMu_isMuGlobal);
  tree_->Branch("Mu_isMuTracker",&STAMu_isMuTracker);
  tree_->Branch("Mu_numberOfChambers_sta",&STAMu_numberOfChambers);
  tree_->Branch("Mu_numberOfMatches_sta",&STAMu_numberOfMatches);
  tree_->Branch("Mu_numberOfHits_sta",&STAMu_numberOfHits);
  tree_->Branch("Mu_segmentIndex_sta",&STAMu_segmIndex);
  tree_->Branch("Mu_px",&Mu_px_mu);
  tree_->Branch("Mu_py",&Mu_py_mu);
  tree_->Branch("Mu_pz",&Mu_pz_mu);
  tree_->Branch("Mu_phi",&Mu_phi_mu);
  tree_->Branch("Mu_eta",&Mu_eta_mu);
  tree_->Branch("Mu_recHitsSize",&STAMu_recHitsSize);
  tree_->Branch("Mu_normchi2_sta",&STAMu_normchi2Mu);
  tree_->Branch("Mu_charge",&STAMu_chargeMu);
  tree_->Branch("Mu_dxy_sta",&STAMu_dxyMu);
  tree_->Branch("Mu_dz_sta",&STAMu_dzMu);
  tree_->Branch("Mu_normchi2_glb",&GLBMu_normchi2Mu);
  tree_->Branch("Mu_dxy_glb",&GLBMu_dxyMu);
  tree_->Branch("Mu_dz_glb",&GLBMu_dzMu);
  tree_->Branch("Mu_numberOfPixelHits_glb",&GLBMu_numberOfPixelHits);
  tree_->Branch("Mu_numberOfTrackerHits_glb",&GLBMu_numberOfTrackerHits);
  tree_->Branch("Mu_tkIsoR03_glb",&GLBMu_tkIsoR03);
  tree_->Branch("Mu_ntkIsoR03_glb",&GLBMu_ntkIsoR03);
  tree_->Branch("Mu_emIsoR03_glb",&GLBMu_emIsoR03);
  tree_->Branch("Mu_hadIsoR03_glb",&GLBMu_hadIsoR03);
  tree_->Branch("STAMu_caloCompatibility",&STAMu_caloCompatibility);
  tree_->Branch("Mu_z_mb2_mu",&STAMu_z_mb2);
  tree_->Branch("Mu_phi_mb2_mu",&STAMu_phi_mb2);
  tree_->Branch("Mu_pseta_mb2_mu",&STAMu_pseta_mb2);

  //GMT  // legacy
  tree_->Branch("gmt_bx",&gmt_bx);
  tree_->Branch("gmt_phi",&gmt_phi);
  tree_->Branch("gmt_eta",&gmt_eta);
  tree_->Branch("gmt_pt",&gmt_pt);
  tree_->Branch("gmt_qual",&gmt_qual);
  tree_->Branch("gmt_detector",&gmt_detector);
  tree_->Branch("gmt_cands_fwd",&gmt_cands_fwd);
  tree_->Branch("gmt_cands_isRpc",&gmt_cands_isRpc);
  tree_->Branch("gmt_cands_bx",&gmt_cands_bx);
  tree_->Branch("gmt_cands_phi",&gmt_cands_phi);
  tree_->Branch("gmt_cands_eta",&gmt_cands_eta);
  tree_->Branch("gmt_cands_pt",&gmt_cands_pt);
  tree_->Branch("gmt_cands_qual",&gmt_cands_qual);
  tree_->Branch("gmt_cands_ismatched",&gmt_cands_ismatched);

  //GT  // legacy
  tree_->Branch("gt_algo_bit",&gt_algo_bit);
  tree_->Branch("gt_algo_bx",&gt_algo_bx);
  tree_->Branch("gt_tt_bit",&gt_tt_bit);
  tree_->Branch("gt_tt_bx",&gt_tt_bx);

  //RPC
  tree_->Branch(  "rpc_region",&rpc_region);			   
  tree_->Branch(  "rpc_clusterSize",&rpc_clusterSize);		   
  tree_->Branch(  "rpc_strip",&rpc_strip);			   
  tree_->Branch(  "rpc_bx",&rpc_bx);			   
  tree_->Branch(  "rpc_station",&rpc_station);		   
  tree_->Branch(  "rpc_sector",&rpc_sector);		   
  tree_->Branch(  "rpc_layer",&rpc_layer);			   
  tree_->Branch(  "rpc_subsector",&rpc_subsector);		   
  tree_->Branch(  "rpc_roll",&rpc_roll);			   
  tree_->Branch(  "rpc_ring",&rpc_ring);                     
  
  //counters
  tree_->Branch("Ndigis",&idigis,"Ndigis/S");
  tree_->Branch("Ndtsegments",&idtsegments,"Ndtsegments/S");
  tree_->Branch("Ncscsegments",&icscsegments,"Ncscsegments/S");
  tree_->Branch("NdtltTwinMuxOut",&idtltTwinMuxOut,"NdtltTwinMuxOut/S");
  tree_->Branch("NdtltTwinMux_th",&idtltTwinMux_th,"NdtltTwinMux_th/S");
  tree_->Branch("NdtltTwinMuxIn",&idtltTwinMuxIn,"NdtltTwinMuxIn/S");
  tree_->Branch("Nmuons",&imuons,"Nmuons/S");
  tree_->Branch("Ngmt",&igmtdt,"Ngmt/S");
  tree_->Branch("Ngmtcands",&igmtcands,"Ngmtcands/S");
  tree_->Branch("Ngtalgo",&igtalgo,"Ngtalgo/S");
  tree_->Branch("Ngttechtrig",&igttt,"Ngttt/S");
  tree_->Branch("Nhlt",&ihlt,"Nhlt/S");
  tree_->Branch("NrpcRecHits",&irpcrechits,"NrpcRecHits/S");
  
  tree_->Branch("bmtfPt", &Bmtf_Pt); 
  tree_->Branch("bmtfPt", &Bmtf_Pt);
  tree_->Branch("bmtfEta", &Bmtf_Eta);
  tree_->Branch("bmftFineBit", &Bmtf_FineBit);
  tree_->Branch("bmtfPhi", &Bmtf_Phi);
  tree_->Branch("bmtfqual", &Bmtf_qual);
  tree_->Branch("bmtfch", &Bmtf_ch);
  tree_->Branch("bmtfbx", &Bmtf_bx);
  tree_->Branch("bmtfprocessor", &Bmtf_processor);
  tree_->Branch("bmtfwh", &Bmtf_wh);
  tree_->Branch("bmtftrAddress", &Bmtf_trAddress);
  tree_->Branch("bmtfSize", &Bmtf_Size);

  return;
}

void TTreeGenerator::endJob()
{
  outFile->cd();
  tree_->Write();
  outFile->Close();

  return;
}

inline void TTreeGenerator::clear_Arrays()
{
  //digi variables
  digi_wheel.clear();
  digi_sector.clear();
  digi_station.clear();
  digi_sl.clear();
  digi_layer.clear();
  digi_wire.clear();
  digi_time.clear();

  //DT segment variables 
  segm4D_wheel.clear();
  segm4D_sector.clear();
  segm4D_station.clear();
  segm4D_hasPhi.clear();
  segm4D_hasZed.clear();
  segm4D_x_pos_loc.clear();
  segm4D_y_pos_loc.clear();
  segm4D_z_pos_loc.clear();
  segm4D_x_dir_loc.clear();
  segm4D_y_dir_loc.clear();
  segm4D_z_dir_loc.clear();
  segm4D_cosx.clear();
  segm4D_cosy.clear();
  segm4D_cosz.clear();
  segm4D_phi.clear();
  segm4D_theta.clear();
  segm4D_eta.clear();
  segm4D_t0.clear();
  segm4D_vdrift.clear();
  segm4D_phinormchi2.clear();
  segm4D_phinhits.clear();
  segm4D_znormchi2.clear();
  segm4D_znhits.clear();

  segm4D_hitsExpPos->Clear();
  segm4D_hitsExpWire->Clear();

  segm4D_phiHits_Pos->Clear();
  segm4D_phiHits_PosCh->Clear();
  segm4D_phiHits_PosErr->Clear();
  segm4D_phiHits_Side->Clear();
  segm4D_phiHits_Wire->Clear();
  segm4D_phiHits_Layer->Clear();
  segm4D_phiHits_SuperLayer->Clear();
  segm4D_phiHits_Time->Clear();
  segm4D_phiHits_TimeCali->Clear();
  segm4D_hitsExpPos->Clear();
  segm4D_hitsExpWire->Clear();

  segm4D_zHits_Pos->Clear();
  segm4D_zHits_PosCh->Clear();
  segm4D_zHits_PosErr->Clear();
  segm4D_zHits_Side->Clear();
  segm4D_zHits_Wire->Clear();
  segm4D_zHits_Layer->Clear();
  segm4D_zHits_Time->Clear();
  segm4D_zHits_TimeCali->Clear();

  //CSC segment variables 
  cscsegm_ring.clear();
  cscsegm_chamber.clear();
  cscsegm_station.clear();
  cscsegm_cosx.clear();
  cscsegm_cosy.clear();
  cscsegm_cosz.clear();
  cscsegm_phi.clear();
  cscsegm_eta.clear();
  cscsegm_normchi2.clear();
  cscsegm_nRecHits.clear();

  //TM Variables
  ltTwinMuxIn_wheel.clear();
  ltTwinMuxIn_sector.clear();
  ltTwinMuxIn_station.clear();
  ltTwinMuxIn_quality.clear();
  ltTwinMuxIn_bx.clear();
  ltTwinMuxIn_phi.clear();
  ltTwinMuxIn_phiB.clear();
  ltTwinMuxIn_is2nd.clear();

  ltTwinMuxOut_wheel.clear();
  ltTwinMuxOut_sector.clear();
  ltTwinMuxOut_station.clear();
  ltTwinMuxOut_quality.clear();
  ltTwinMuxOut_rpcbit.clear();
  ltTwinMuxOut_bx.clear();
  ltTwinMuxOut_phi.clear();
  ltTwinMuxOut_phiB.clear();
  ltTwinMuxOut_is2nd.clear();

  ltTwinMux_thWheel.clear();
  ltTwinMux_thSector.clear();
  ltTwinMux_thStation.clear();
  ltTwinMux_thBx.clear();
  ltTwinMux_thHits.clear();

  //muon variables
  STAMu_isMuGlobal.clear();
  STAMu_isMuTracker.clear();
  STAMu_numberOfChambers.clear();
  STAMu_numberOfMatches.clear();
  STAMu_numberOfHits.clear();
  STAMu_segmIndex.clear();

  Mu_px_mu.clear();
  Mu_py_mu.clear();
  Mu_pz_mu.clear();
  Mu_phi_mu.clear();
  Mu_eta_mu.clear();
  STAMu_recHitsSize.clear();
  STAMu_normchi2Mu.clear();
  STAMu_chargeMu.clear();
  STAMu_dxyMu.clear();
  STAMu_dzMu.clear();

  GLBMu_normchi2Mu.clear();
  GLBMu_dxyMu.clear();
  GLBMu_dzMu.clear();

  GLBMu_numberOfPixelHits.clear();
  GLBMu_numberOfTrackerHits.clear();

  GLBMu_tkIsoR03.clear();
  GLBMu_ntkIsoR03.clear();
  GLBMu_emIsoR03.clear();
  GLBMu_hadIsoR03.clear();

  STAMu_caloCompatibility.clear();

  STAMu_z_mb2.clear();
  STAMu_phi_mb2.clear();
  STAMu_pseta_mb2.clear();

  //GMT
  gmt_bx.clear();
  gmt_phi.clear();
  gmt_eta.clear();
  gmt_pt.clear();
  gmt_qual.clear();
  gmt_detector.clear();

  gmt_cands_fwd.clear();
  gmt_cands_isRpc.clear();
  gmt_cands_bx.clear();
  gmt_cands_phi.clear();
  gmt_cands_eta.clear();
  gmt_cands_pt.clear();
  gmt_cands_qual.clear();
  gmt_cands_ismatched.clear();

  //GT
  gt_algo_bit.clear();
  gt_algo_bx.clear();
  gt_tt_bit.clear();
  gt_tt_bx.clear();

  //HLT
  hlt_path.clear();

  // RPC rec hits
  rpc_region.clear();
  rpc_clusterSize.clear();
  rpc_strip.clear();
  rpc_bx.clear();
  rpc_station.clear();
  rpc_sector.clear();
  rpc_layer.clear();
  rpc_subsector.clear();
  rpc_roll.clear();
  rpc_ring.clear();
  
	 //Bmtf_Size.clear();
	  Bmtf_Pt.clear();
	  Bmtf_Eta.clear();
	  Bmtf_Phi.clear();
	  Bmtf_qual.clear();
	  Bmtf_ch.clear();
	  Bmtf_bx.clear();
	  Bmtf_processor.clear();
	  Bmtf_trAddress.clear();
	  Bmtf_wh.clear();
	  Bmtf_FineBit.clear();
  
  return;
}

void TTreeGenerator::initialize_Tree_variables()
{
  //Event variables
  runnumber   = 0;
  lumiblock   = 0;
  eventNumber = 0;
  timestamp   = 0.;
  bunchXing   = 0;
  orbitNum    = 0;

  PV_x = 0.;
  PV_y = 0.;
  PV_z = 0.;
  PV_xxE = 0.;
  PV_yyE = 0.;
  PV_zzE = 0.;
  PV_xyE = 0.;
  PV_xzE = 0.;
  PV_yzE = 0.;
  PV_normchi2 = 0.;
  PV_Nvtx = 0.;

  lumiperblock = 0.;
  beam1Intensity = -1.;
  beam2Intensity = -1.;

  segm4D_phiHits_Pos    = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_PosCh  = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_PosErr = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_Side   = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_Wire   = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_Layer  = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_SuperLayer  = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_Time   = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_phiHits_TimeCali   = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_hitsExpPos     = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_hitsExpWire    = new TClonesArray("TVectorF",dtsegmentsSize_);

  segm4D_zHits_Pos    = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_zHits_PosCh  = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_zHits_PosErr = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_zHits_Side   = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_zHits_Wire   = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_zHits_Layer  = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_zHits_Time   = new TClonesArray("TVectorF",dtsegmentsSize_);
  segm4D_zHits_TimeCali   = new TClonesArray("TVectorF",dtsegmentsSize_);

  return;
}

TrajectoryStateOnSurface TTreeGenerator::cylExtrapTrkSam(reco::TrackRef track, const float rho) const
{
  Cylinder::PositionType pos(0.,0.,0.);
  Cylinder::RotationType rot;
  Cylinder::CylinderPointer myCylinder = Cylinder::build(pos, rot, rho);

  FreeTrajectoryState recoStart = freeTrajStateMuon(track);
  TrajectoryStateOnSurface recoProp;
  recoProp = propagatorAlong->propagate(recoStart, *myCylinder);
  if (!recoProp.isValid()) {
    recoProp = propagatorOpposite->propagate(recoStart, *myCylinder);
  }
  return recoProp;
}

FreeTrajectoryState TTreeGenerator::freeTrajStateMuon(const reco::TrackRef track) const
{
  const GlobalPoint  innerPoint(track->innerPosition().x(),track->innerPosition().y(),track->innerPosition().z());
  const GlobalVector innerVec  (track->innerMomentum().x(),track->innerMomentum().y(),track->innerMomentum().z());  
  
  FreeTrajectoryState recoStart(innerPoint, innerVec, track->charge(), &*theBField);
  
  return recoStart;
}

//define this as a plug-in
DEFINE_FWK_MODULE(TTreeGenerator);
 
