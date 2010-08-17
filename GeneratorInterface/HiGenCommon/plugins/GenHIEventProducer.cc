// -*- C++ -*-
//
// Package:    GenHIEventProducer
// Class:      GenHIEventProducer
// 
/**\class GenHIEventProducer GenHIEventProducer.cc yetkin/GenHIEventProducer/src/GenHIEventProducer.cc

Description: <one line class summary>

Implementation:
<Notes on implementation>
*/
//
// Original Author:  Yetkin Yilmaz
//         Created:  Thu Aug 13 08:39:51 EDT 2009
// $Id: GenHIEventProducer.cc,v 1.4 2010/08/15 19:33:09 dmoon Exp $
//
//


// system include files
#include <memory>
#include <string>
#include <iostream>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "SimDataFormats/HiGenData/interface/GenHIEvent.h"

#include "HepMC/HeavyIon.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "SimGeneral/HepPDTRecord/interface/ParticleDataTable.h"
using namespace std;

//
// class decleration
//

class GenHIEventProducer : public edm::EDProducer {
    public:
        explicit GenHIEventProducer(const edm::ParameterSet&);
        ~GenHIEventProducer();

    private:
        virtual void produce(edm::Event&, const edm::EventSetup&);
        std::vector<std::string> hepmcSrc_;
        edm::ESHandle < ParticleDataTable > pdt;

        double ptCut_;
        bool _DoGetData;

};

//
// constants, enums and typedefs
//


//
// static data member definitions
//

//
// constructors and destructor
//
GenHIEventProducer::GenHIEventProducer(const edm::ParameterSet& iConfig)
{
    produces<edm::GenHIEvent>();
    hepmcSrc_ = iConfig.getParameter<std::vector<std::string> >("generators");
    ptCut_ = iConfig.getParameter<double> ("ptCut"); // ptCut added
    _DoGetData = iConfig.getParameter<bool> ("DoGetData"); // ptCut added
    //ptCut_ = iConfig.getParameter<double> >("ptCut"); // ptCut added
}


GenHIEventProducer::~GenHIEventProducer()
{

    // do anything here that needs to be done at desctruction time
    // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to produce the data  ------------
    void
GenHIEventProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
    using namespace edm;

    _DoGetData = false;

    if(_DoGetData){if(!(pdt.isValid())) iSetup.getData(pdt);}

    double b = -1;
    int npart = -1;
    int ncoll = 0;
    int nhard = 0;
    double phi = 0;

    int nCharged = 0;
    int nChargedMR = 0;
    int nChargedPtCut = 0; // NchargedPtCut bym
    int nChargedPtCutMR = 0; // NchargedPtCutMR bym

    double meanPt = 0;
    double meanPtMR = 0;
    double EtMR = 0; // Normalized of total energy bym
    double TotEnergy = 0; // Total energy bym

    for(size_t ihep = 0; ihep < hepmcSrc_.size(); ++ihep){
        Handle<edm::HepMCProduct> hepmc;
        iEvent.getByLabel(hepmcSrc_[ihep],hepmc);

        const HepMC::GenEvent* evt = hepmc->GetEvent();
        HepMC::GenEvent::particle_const_iterator begin = evt->particles_begin();
        HepMC::GenEvent::particle_const_iterator end = evt->particles_end();
        for(HepMC::GenEvent::particle_const_iterator it = begin; it != end; ++it){
            if((*it)->status() != 1) continue;
            int pdg_id = (*it)->pdg_id();
            const ParticleData * part = pdt->particle(pdg_id );
            int charge = static_cast<int>(part->charge());

            if(charge == 0) continue;
            float pt = (*it)->momentum().perp();
            float eta = (*it)->momentum().eta();
            float energy = (*it)->momentum().e(); // energy bym
            //float energy = (*it)->momentum().energy(); // energy bym
            nCharged++;
            meanPt += pt;
            // Get the total energy bym
            if(fabs(eta)<1.0){
                TotEnergy += energy;
            }
            if(pt>ptCut_){
                nChargedPtCut++;
                if(fabs(eta)<0.5){
                    nChargedPtCutMR++;
                }
            }
            // end bym

            if(fabs(eta) > 0.5) continue;
            nChargedMR++;
            meanPtMR += pt;
        }

        const HepMC::HeavyIon* hi = evt->heavy_ion();

        if(hi){
            ncoll = ncoll + hi->Ncoll();
            nhard = nhard + hi->Ncoll_hard();
            int np = hi->Npart_proj() + hi->Npart_targ();
            if(np >= 0){
                npart = np;
                b = hi->impact_parameter();
                phi = hi->event_plane_angle();
            }
        }
    }
    // Get the normalized total energy bym
    if(TotEnergy != 0){
        EtMR = TotEnergy/2;
    }

    if(nChargedMR != 0){
        meanPtMR /= nChargedMR;
    }
    if(nCharged != 0){
        meanPt /= nCharged;
    }

    std::auto_ptr<edm::GenHIEvent> pGenHI(new edm::GenHIEvent(b,
                npart,
                ncoll,
                nhard,
                phi, 
                nCharged,
                nChargedMR,
                meanPt,
                meanPtMR,								
                EtMR, // bym
                nChargedPtCut, // bym
                nChargedPtCutMR  // bym
                ));

    iEvent.put(pGenHI);

}

//define this as a plug-in
DEFINE_FWK_MODULE(GenHIEventProducer);
