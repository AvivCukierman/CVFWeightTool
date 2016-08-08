# CVFWeightTool
This tool calculates the Cluster Vertex Fraction (CVF) of clusters and applies a weight of 0 to the cluster depending on its CVF. It makes a shallow copy container of the clusters with the pT of the clusters modified appropriately.
## Dependencies
This package makes use of [UChicago](https://github.com/UCATLAS)'s [xAODAnaHelpers](https://github.com/UCATLAS/xAODAnaHelpers) package and [Giordon Stark](https://github.com/kratsg)'s [xAODJetReclustering](https://github.com/kratsg/xAODJetReclustering) package.

## Installing
To install,
```bash
mkdir myRootCore && cd $_
rcSetup Base,2.3.41
git clone https://github.com/kratsg/xAODJetReclustering
git clone https://github.com/UCATLAS/xAODAnaHelpers
git clone https://github.com/AvivCukierman/CVFWeightTool
rc checkout_pkg atlasoff/AsgExternal/Asg_FastJet/tags
rc checkout_pkg atlasoff/AsgExternal/Asg_FastJetContrib/tags
rc find_packages
rc compile
```
(You might have to do a `rcSetup -u && rcSetup` before the final `rc compile` just because.)


## Configurations

 Property           | Type                      | Default                   | Description
:-------------------|:-------------------------:|--------------------------:|:-------------------------------------------------------------------------------------
InputContainer   | string                    |    "CaloCalTopoClusters"                       | Name of the input cluster container to be modified.
OutputContainer  | string                    |    "CVFClusters"                       | Name of the output shallow copy cluster container with modified pT.
TrackContainer  | string                    |    "InDetTrackParticles"                       | Name of the track particle container.
doLCWeights       | bool                     | false                      | Apply LC weights to clusters before CVF algorithms.
PtThreshold       | float                     | -1                      | Remove clusters with pT<=PtThreshold and CVF<=CVFThreshold. If -1, then PtTreshold -> infinity.
CVFThreshold       | float                     | 0                      | Remove clusters with pT<=PtThreshold and CVF<=CVFThreshold.
doExtrapEst        | bool                     | true                      | Track extrapolation is not available in Rootcore, only in Athena. So an estimate for track extrapolation is provided, that should be accurate for |eta|<1.6.

## Using
Add this package as a dependency in `cmt/Makefile.RootCore`.

Add a header
```c++
#include <CVFWeightTool/CVFWeightTool.h>
```

Set up the tool in the `initialize()` portion of your algorithm as a pointer

```c++
  m_CVFTool = new CVFWeightTool(m_name);
  m_CVFTool->setProperty("InputContainer", m_InputContainer);
  m_CVFTool->setProperty("OutputContainer", m_OutputContainer);
  m_CVFTool->setProperty("TrackContainer", m_TrackContainer);
  m_CVFTool->setProperty("doLCWeights", m_doLCWeights);
  m_CVFTool->setProperty("PtThreshold", m_PtThreshold);
  m_CVFTool->setProperty("CVFThreshold", m_CVFThreshold);
  m_CVFTool->initialize();
```

And then simply call `m_CVFTool->execute()` in the `execute()` portion of your algorithm to fill the TStore with the appropriate container(s). Don't forget to delete the pointer when you're done.
```c++
if(m_CVFTool) delete m_CVFTool;
```

You can then do jet clustering as normal on the output cluster container.

N.B. The `doLCWeights` option indicates whether to apply LC weights (if true) or EM weights (if false) to clusters. The output clusters have their pTs modified *in the given state*. I.e. with `doLCWeights` set to true, only the LC pT of the new cluster collection is modified, and with `doLCWeights` set to false, only the EM pT of the new cluster collection is modified. In order to set the state of the output (or any) cluster collection, the `CaloClusterChangeSignalStateList` tool should be used in your algorithm, as follows:
```c++
#include "xAODCaloEvent/CaloClusterChangeSignalState.h"
```

```c++
 const xAOD::CaloClusterContainer*             new_clusters   (nullptr);
 if(evtStore()->retrieve(new_clusters,m_OutputContainer).isFailure()) Error(APP_NAME,"Could not retrieve the CVF cluster container");

 CaloClusterChangeSignalStateList stateHelperList;
 for(auto clust: *new_clusters){
   if(m_doLCWeights) stateHelperList.add(clust,xAOD::CaloCluster::State(1)); //default is calibrated but we can make it explicit anyway
   else stateHelperList.add(clust,xAOD::CaloCluster::State(0));
   //now clust->pt() will give CVF modified pT, and jet reconstruction will use CVF modified pT
 }
```

Note that as it behaves like an `AsgTool`, the functions `setProperty()` and `initialize()` have a return type `StatusCode`.

## Track Extrapolation
Track extrapolation is currently only available in Athena. However, the performance of CVF is much better when using extrapolated tracks than when not. Because of this, in the Rootcore version, an estimate for the extrapolated track coordinates at the calorimeter entrance is given, and can be activated with the `doExtrapEst` option. Validation of the accuracy of this estimate can be found [here](https://cloud.githubusercontent.com/assets/11202460/17484749/b3fecd7c-5d8b-11e6-99b7-2453aecef903.png) and [here](https://cloud.githubusercontent.com/assets/11202460/17484748/b3f4d0ec-5d8b-11e6-9c4a-07fbe9722b85.png).

## Authors
- [Aviv Cukierman](https://github.com/AvivCukierman)

## Acknowledgements
[Giordon Stark](https://github.com/kratsg), from whom I plagiarized this readme and also whose code I used for inspiration.
