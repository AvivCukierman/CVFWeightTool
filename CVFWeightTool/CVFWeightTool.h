#ifndef CVFWeightTool_CVFWeightTool_H
#define CVFWeightTool_CVFWeightTool_H

#include <EventLoop/Algorithm.h>
#include "EventLoopAlgs/NTupleSvc.h"
#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"

#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"
#include "TRegexp.h"
#include <fstream>
#include <iostream>

#ifndef __CINT__
#include "xAODCore/ShallowCopy.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODTracking/VertexContainer.h"
#include "xAODJet/JetContainer.h"
#include "xAODJet/JetAuxContainer.h"
#include "xAODCaloEvent/CaloClusterContainer.h"
#include "xAODJet/JetConstituentVector.h"
#endif

#include "xAODTracking/VertexContainer.h"
#include "JetCalibTools/JetCalibrationTool.h"
#include "fastjet/PseudoJet.hh"
#include "fastjet/JetDefinition.hh"

#include "PATInterfaces/SystematicRegistry.h"

#include <TTree.h>

//ASG:
#include "AsgTools/AsgTool.h"

//class CVFWeights : public EL::Algorithm
class CVFWeightTool : virtual public asg::AsgTool 
{
  // put your configuration variables here as public variables.
  // that way they can be set directly from CINT and python.
public:
  std::string m_inputContainer = "CaloCalTopoClusters",
              m_outputContainer = "CVFClusters",
              m_trackContainer = "InDetTrackParticles";

  bool m_debug = false;
  bool m_doLC = false;
  float m_ptthreshold = -1;

  const float pi = 3.14159265;
  const float twopi = pi*2;

  // Constructor with parameters:
  CVFWeightTool(const std::string& name);

  // Destructor:
  virtual ~CVFWeightTool();
   

  // methods used in the analysis
  virtual StatusCode setCVF(const xAOD::CaloClusterContainer*, const xAOD::TrackParticleContainer*, std::vector<float>&);
  float delta_phi(float, float );

private:
  //xAOD::TEvent *m_event; //!
  //xAOD::TStore *m_store;  //!
  std::vector<fastjet::PseudoJet> clusters; //!
  struct PJcomp;

public:
  // this is a standard constructor
  CVFWeightTool ();

  // these are the functions inherited from AsgTool
  virtual StatusCode initialize ();
  virtual StatusCode execute ();
  virtual StatusCode finalize ();

  // this is needed to distribute the algorithm to the workers
  ClassDef(CVFWeightTool, 1);
};

#endif
