
// system include files
#include <memory>
#include <string>
#include <vector>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "FWCore/Utilities/interface/Exception.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "SimDataFormats/CaloHit/interface/PCaloHit.h"
#include "SimDataFormats/CaloHit/interface/PCaloHitContainer.h"
#include "SimDataFormats/Track/interface/SimTrack.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"
#include "SimDataFormats/Vertex/interface/SimVertex.h"
#include "SimDataFormats/Vertex/interface/SimVertexContainer.h"

#include <TH1F.h>
#include <TH1I.h>

class XtalDedxAnalysis : public edm::EDAnalyzer {

public:
  explicit XtalDedxAnalysis(const edm::ParameterSet&);
  virtual ~XtalDedxAnalysis() {}

protected:

  virtual void beginJob() {}
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
  virtual void endJob() {}

  void analyzeHits (std::vector<PCaloHit>&, 
		    edm::Handle<edm::SimTrackContainer>& , 
		    edm::Handle<edm::SimVertexContainer>&);

private:
  edm::InputTag  caloHitSource_;
  std::string    simTkLabel_;

  TH1F           *meNHit_[4], *meE1T0_[4], *meE9T0_[4], *meE1T1_[4], *meE9T1_[4];
  TH1I           *mType_;

};

XtalDedxAnalysis::XtalDedxAnalysis(const edm::ParameterSet& ps) {

  caloHitSource_   = ps.getParameter<edm::InputTag>("caloHitSource");
  simTkLabel_      = ps.getUntrackedParameter<std::string>("moduleLabelTk","g4SimHits");
  double energyMax = ps.getParameter<double>("EnergyMax");
  edm::LogInfo("XtalDedxAnalysis") << "XtalDedxAnalysis::Source "
				    << caloHitSource_ << " Track Label "
				    << simTkLabel_ << " Energy Max "
				    << energyMax;

  // Book histograms
  edm::Service<TFileService> tfile;

  if ( !tfile.isAvailable() )
    throw cms::Exception("BadConfig") << "TFileService unavailable: "
                                      << "please add it to config file";
  //Histograms for Hits
  std::string types[4] = {"total", "by dE/dx", "by delta-ray", "by bremms"};
  char name[20], title[80];
  for (int i=0; i<4; i++) {
    sprintf (name,  "Hits%d", i);
    sprintf (title, "Number of hits (%s)", types[i].c_str());
    meNHit_[i]= tfile->make<TH1F>(name, title, 5000, 0., 5000.);
    meNHit_[i]->GetXaxis()->SetTitle(title);
    meNHit_[i]->GetYaxis()->SetTitle("Events");
    sprintf (name,  "E1T0%d", i);
    sprintf (title, "E1 (Loss %s) in GeV", types[i].c_str());
    meE1T0_[i] = tfile->make<TH1F>(name, title, 5000, 0, energyMax);
    meE1T0_[i]->GetXaxis()->SetTitle(title);
    meE1T0_[i]->GetYaxis()->SetTitle("Events");
    sprintf (name,  "E9T0%d", i);
    sprintf (title, "E9 (Loss %s) in GeV", types[i].c_str());
    meE9T0_[i] = tfile->make<TH1F>(name, title, 5000, 0, energyMax);
    meE9T0_[i]->GetXaxis()->SetTitle(title);
    meE9T0_[i]->GetYaxis()->SetTitle("Events");
    sprintf (name,  "E1T1%d", i);
    sprintf (title, "E1 (Loss %s with t < 400 ns) in GeV", types[i].c_str());
    meE1T1_[i] = tfile->make<TH1F>(name, title, 5000, 0, energyMax);
    meE1T1_[i]->GetXaxis()->SetTitle(title);
    meE1T1_[i]->GetYaxis()->SetTitle("Events");
    sprintf (name,  "E9T1%d", i);
    sprintf (title, "E9 (Loss %s with t < 400 ns) in GeV", types[i].c_str());
    meE9T1_[i]= tfile->make<TH1F>(name, title, 5000, 0, energyMax);
    meE9T1_[i]->GetXaxis()->SetTitle(title);
    meE9T1_[i]->GetYaxis()->SetTitle("Events");
  }
  sprintf (name,  "PDGType");
  sprintf (title, "PDG ID of first level secondary");
  mType_ = tfile->make<TH1I>(name, title, 5000, -2500, 2500);
  mType_->GetXaxis()->SetTitle(title);
  mType_->GetYaxis()->SetTitle("Tracks");
}

void XtalDedxAnalysis::analyze(const edm::Event& e, const edm::EventSetup& ) {

  LogDebug("XtalDedxAnalysis") << "Run = " << e.id().run() << " Event = "
				<< e.id().event();

  std::vector<PCaloHit>               caloHits;
  edm::Handle<edm::PCaloHitContainer> pCaloHits;
  e.getByLabel(caloHitSource_, pCaloHits);

  std::vector<SimTrack> theSimTracks;
  edm::Handle<edm::SimTrackContainer> simTk;
  e.getByLabel(simTkLabel_,simTk);

  std::vector<SimVertex> theSimVertex;
  edm::Handle<edm::SimVertexContainer> simVtx;
  e.getByLabel(simTkLabel_,simVtx);

  if (pCaloHits.isValid()) {
    caloHits.insert(caloHits.end(),pCaloHits->begin(),pCaloHits->end());
    LogDebug("XtalDedxAnalysis") << "HcalValidation: Hit buffer "
				  << caloHits.size();
    analyzeHits (caloHits, simTk, simVtx);
  }
}

void XtalDedxAnalysis::analyzeHits (std::vector<PCaloHit>& hits,
				    edm::Handle<edm::SimTrackContainer>& SimTk,
				    edm::Handle<edm::SimVertexContainer>& SimVtx) {

  edm::SimTrackContainer::const_iterator simTrkItr;
  int nHit = hits.size();
  double e10[4], e90[4], e11[4], e91[4], hit[4];
  for (int i=0; i<4; i++)
    e10[i] = e90[i] = e11[i] = e91[i] = hit[i] = 0;
  for (int i=0; i<nHit; i++) {
    double energy    = hits[i].energy();
    double time      = hits[i].time();
    unsigned int id_ = hits[i].id();
    int    trackID   = hits[i].geantTrackId();
    int    type      = 1;
    for (simTrkItr = SimTk->begin(); simTrkItr!= SimTk->end(); simTrkItr++) {
      if (trackID == (int)(simTrkItr->trackId())) {
	int thePID = simTrkItr->type();
	if      (thePID == 11) type = 2;
	else if (thePID != -13 && thePID != 13) type = 3;
	break;
      }
    }
    hit[0]++;
    hit[type]++;
    e90[0]    += energy;
    e90[type] += energy;
    if (time < 400) {
      e91[0]    += energy;
      e91[type] += energy;
    }
    if (id_ == 22) {
      e10[0]    += energy;
      e10[type] += energy;
      if (time < 400) {
	e11[0]    += energy;
	e11[type] += energy;
      }
    }
    LogDebug("XtalDedxAnalysis") << "Hit[" << i << "] ID " << id_ << " E " 
				 << energy << " time " << time << " track "
				 << trackID << " type " << type;
  }
  for (int i=0; i<4; i++) {
    LogDebug("XtalDedxAnalysis") << "Type(" << i << ") Hit " << hit[i] 
				 << " E10 " << e10[i] << " E11 " << e11[i]
				 << " E90 " << e90[i] << " E91 " << e91[i];
    meNHit_[i]->Fill(hit[i]);
    meE1T0_[i]->Fill(e10[i]);
    meE9T0_[i]->Fill(e90[i]);
    meE1T1_[i]->Fill(e11[i]);
    meE9T1_[i]->Fill(e91[i]);
  }

  // Type of the secondary (coming directly from a generator level track)
  int nvtx=0, ntrk=0, k1=0;
  edm::SimVertexContainer::const_iterator simVtxItr;
  for (simTrkItr=SimTk->begin(); simTrkItr !=SimTk->end(); simTrkItr++) ntrk++;
  for (simVtxItr=SimVtx->begin(); simVtxItr!=SimVtx->end();simVtxItr++) nvtx++;
  LogDebug("XtalDedxAnalysis") << ntrk << " tracks and " << nvtx <<" vertices";
  for (simTrkItr=SimTk->begin(); simTrkItr != SimTk->end(); simTrkItr++,++k1) {
    LogDebug("XtalDedxAnalysis") << "Track " << k1 << " PDGId " << simTrkItr->type() << " Vertex ID " << simTrkItr->vertIndex() << " Generator " << simTrkItr->noGenpart();
    if (simTrkItr->noGenpart()) { // This is a secondary
      int vertIndex = simTrkItr->vertIndex();  // Vertex index of origin
      if (vertIndex >= 0 && vertIndex  < nvtx) {
	simVtxItr= SimVtx->begin();
	for (int iv=0; iv<vertIndex; iv++) simVtxItr++;
	int parent = simVtxItr->parentIndex(), k2 = 0;
	for (edm::SimTrackContainer::const_iterator trkItr = SimTk->begin();
	     trkItr != SimTk->end(); trkItr++, ++k2) {
	  LogDebug("XtalDedxAnalysis") << "Track " << k2 << " ID " << trkItr->trackId() << " (" << parent << ")  Generator " << trkItr->noGenpart();
	  if ((int)trkItr->trackId() == parent) { // Parent track
	    if (!trkItr->noGenpart()) { // Generator level
	      LogDebug("XtalDedxAnalysis") << "Track found with ID " << simTrkItr->type();
	      mType_->Fill(simTrkItr->type());
	    }
	    break;
	  }
	}
      }
    }
  }
}

//define this as a plug-in
DEFINE_FWK_MODULE(XtalDedxAnalysis);
