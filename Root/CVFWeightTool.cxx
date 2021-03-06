#include <vector>
#include <map>
#include <math.h>
#include <ios>

// event loop
#include <EventLoop/Job.h>
#include <EventLoop/Worker.h>
#include "xAODEventInfo/EventInfo.h"

#include <CVFWeightTool/CVFWeightTool.h>

#include <xAODJet/FastJetLinkBase.h>
#include "fastjet/JetDefinition.hh"
#include "fastjet/PseudoJet.hh"
#include "fastjet/Selector.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/tools/JetMedianBackgroundEstimator.hh"
#include <fastjet/tools/Subtractor.hh>
#include "xAODCaloEvent/CaloClusterContainer.h"

#include "xAODCore/ShallowAuxContainer.h"
#include "xAODCore/ShallowCopy.h"
#include "xAODJet/JetConstituentVector.h"

// EDM
#include "xAODCaloEvent/CaloCluster.h"
#include "xAODCaloEvent/CaloClusterChangeSignalState.h"
#include "xAODTruth/TruthParticleContainer.h"
#include "xAODTruth/TruthEventContainer.h"
#include "xAODTruth/TruthEvent.h"

// Infrastructure include(s):
#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"

// xAH includes
#include "xAODAnaHelpers/HelperFunctions.h"
#include "xAODAnaHelpers/tools/ReturnCheck.h"

//ANN:
//#include "ANN/ANN.h"

namespace HF = HelperFunctions;

// this is needed to distribute the algorithm to the workers
//ClassImp(CVFWeightTool)

CVFWeightTool :: CVFWeightTool(const std::string& name) :
  AsgTool(name)
{
  declareProperty("InputContainer", m_inputContainer);
  declareProperty("TrackContainer", m_trackContainer);
  declareProperty("OutputContainer", m_outputContainer);
  declareProperty("doLCWeights", m_doLC);
  declareProperty("doExtrapEst", m_doEst);
  declareProperty("doExtrap", m_doExtrap);
  declareProperty("PtThreshold", m_ptthreshold);
  declareProperty("CVFThreshold", m_cvfthreshold);
}

CVFWeightTool::~CVFWeightTool() {}

StatusCode CVFWeightTool :: initialize ()
{
  return StatusCode::SUCCESS;
}

//Have to define custom comparator for PseudoJets in order to have a map from PJs to anything
//Comparison is fuzzy to account for rounding errors
struct CVFWeightTool :: PJcomp {
  bool operator() (const std::pair<fastjet::PseudoJet, std::vector<float> >& lhsp, const std::pair<fastjet::PseudoJet, std::vector<float> >& rhsp)
  {
    fastjet::PseudoJet lhs = lhsp.first;
    fastjet::PseudoJet rhs = rhsp.first;
    return lhs.pt()>rhs.pt();
    //The comparator must be a strict weak ordering. 
  }
};

StatusCode CVFWeightTool :: execute ()
{
  const char* APP_NAME = "CVFWeightTool::execute()";

  const xAOD::CaloClusterContainer*             in_clusters   (nullptr);
  if(evtStore()->retrieve(in_clusters,m_inputContainer).isFailure()) Error(APP_NAME,"Could not retrieve the input cluster container");
  const xAOD::TrackParticleContainer*             tracks   (nullptr);
  if(evtStore()->retrieve(tracks,m_trackContainer).isFailure()) Error(APP_NAME,"Could not retrieve the input track container");
  //const xAOD::EventInfo*             eventInfo   (nullptr);
  //if(evtStore()->retrieve(eventInfo,"EventInfo").isFailure()) Error(APP_NAME,"Could not retrieve the input cluster container");
  /*int event_number = eventInfo->eventNumber(); //added
  std::cout << event_number << std::endl;*/

  std::vector<float> cvfs;
  if(m_doExtrap) Error("CVFWeightTool","Full extrapolation not yet implemented in Rootcore. Try doExtrapEst for an estimate of the track extrapolation.");
  if(m_doEst) setExtrapPhi(tracks);
  setCVF(in_clusters,tracks,cvfs);

  CaloClusterChangeSignalStateList stateHelperList;
  std::pair< xAOD::CaloClusterContainer*, xAOD::ShallowAuxContainer* > clustSC = xAOD::shallowCopyContainer( *in_clusters );
  xAOD::CaloClusterContainer* SC_clusters = clustSC.first;
  int i=0;
  //std::cout << m_outputContainer << std::endl;
  for(auto clust : HF::sort_container_pt(SC_clusters)){
    if(m_doLC) stateHelperList.add(clust,xAOD::CaloCluster::State(1)); //default is calibrated but we can make it explicit anyway
    else stateHelperList.add(clust,xAOD::CaloCluster::State(0));
    if(cvfs[i]>-1 && cvfs[i]<=m_cvfthreshold){
      if((m_ptthreshold>=0 && clust->pt() < m_ptthreshold) || (m_ptthreshold<0)){
        //std::cout << cvfs[i] << ";" << clust->pt() << std::endl;
        clust->setE(0);
      }
    }
    i++;
  }
  evtStore()->record( clustSC.first, m_outputContainer );
  evtStore()->record( clustSC.second, m_outputContainer+std::string("Aux.") );

  return StatusCode::SUCCESS;
}

StatusCode CVFWeightTool :: finalize ()
{
  return StatusCode::SUCCESS;
}

const float params[3] = { -14.6027571, -44.7818374, 540.656643};
float CVFWeightTool::ExtrapolationEstimate(float pt, float eta, float charge){
  float eptsindeltaphi = params[0]*pow(eta,4)+params[1]*pow(eta,2)+params[2]*pow(eta,0);
  float sindeltaphi = eptsindeltaphi/(pt*charge);
  if (fabs(sindeltaphi)>1.0) return -999.0; //never reaches the detector
  float deltaphi = asin(sindeltaphi);
  return deltaphi;
}

StatusCode CVFWeightTool::setExtrapPhi(const xAOD::TrackParticleContainer* tracks){
  static SG::AuxElement::Decorator< float > newphi("newphi");
  for(auto track: HF::sort_container_pt(tracks)){
    float trketa = track->eta();
    float trkphi = track->phi();
    float trkpt = track->pt();
    float trkcharge = track->charge();
    float deltaphi = ExtrapolationEstimate(trkpt,trketa,trkcharge);
    //std::cout << deltaphi << std::endl;
    if(deltaphi>-998.0) newphi(*track) = trkphi-deltaphi;
    else newphi(*track) = -999.0;
  }
}

StatusCode CVFWeightTool::setCVF(const xAOD::CaloClusterContainer* clusters, const xAOD::TrackParticleContainer* tracks, std::vector<float>& cvfs){
  static SG::AuxElement::Decorator< float > newphi("newphi");
  for(auto clust: HF::sort_container_pt(clusters)){
    //std::cout << "c: " << clust->pt() << ";" << clust->eta() << ";" << clust->phi() << std::endl;
    float cleta = clust->eta();
    float clphi = clust->phi();
    float num = 0;
    float den = 0;
    for(auto track: HF::sort_container_pt(tracks)){
      float trketa = track->eta();
      float trkphi = track->phi();
      /*float trkpt = track->pt();
      float trkcharge = track->charge();*/
      float deta = trketa-cleta;
      if (fabs(deta)>0.1) continue;
      if(m_doEst){
        //float deltaphi = ExtrapolationEstimate(trkpt,trketa,trkcharge);
        //std::cout << trkpt << ";" << trketa << ";" << trkcharge << ";" << deltaphi << std::endl;
        //std::cout << newphi(*track) << std::endl;
        trkphi = newphi(*track);
        if(trkphi<-998) continue; //track never made it to the calorimeter
      }
      float dphi = delta_phi(trkphi,clphi);
      if(fabs(dphi)>0.1) continue;
      if(pow(deta,2)+pow(dphi,2) < 0.1*0.1){
        float trkpt = track->pt();
        //std::cout << "t: " << track->pt() << ";" << track->eta() << ";" << track->phi() << std::endl;
        const xAOD::Vertex* vertex = track->vertex();
        if(vertex){
          enum xAOD::VxType::VertexType vtype = vertex->vertexType();
          if (vtype == 1){
            //PV
            num+=trkpt;
            den+=trkpt;
          }
          if (vtype == 3){
            //pileup
            den+=trkpt;
          }
          //else other kinds of vertices - ?
        }
        //else no vertex - something about z0?
      }
    }
    float cvf;
    if(den==0) cvf = -1;
    else cvf = num/den; //Rpt
    cvfs.push_back(cvf);
    //std::cout << clust->pt() << ";" << cvf << std::endl;
  }
  return StatusCode::SUCCESS;
}

const float pi = 3.14159265;
const float twopi = pi*2;
float CVFWeightTool::delta_phi(float phi1, float phi2){
  float dphi = phi1-phi2;
  if (dphi >  pi) dphi -= twopi;
  if (dphi < -pi) dphi += twopi;
  return dphi;
}

