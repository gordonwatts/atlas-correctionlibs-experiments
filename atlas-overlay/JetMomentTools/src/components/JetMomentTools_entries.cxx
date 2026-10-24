#include "JetMomentTools/JetCaloEnergies.h"
#include "JetMomentTools/JetCaloQualityTool.h"
#include "JetMomentTools/JetCaloQualityToolFE.h"
#include "JetMomentTools/JetWidthTool.h"
#include "JetMomentTools/JetVertexFractionTool.h"
#include "JetMomentTools/JetVertexTaggerTool.h"
#include "JetMomentTools/JetVertexNNTagger.h"
#include "JetMomentTools/JetForwardJvtTool.h"
#include "JetMomentTools/JetForwardPFlowJvtTool.h"
#include "JetMomentTools/JetBalancePFlowJvtTool.h"
#include "JetMomentTools/JetTrackMomentsTool.h"
#include "JetMomentTools/JetTrackSumMomentsTool.h"
#include "JetMomentTools/JetClusterMomentsTool.h"
#include "JetMomentTools/JetPtAssociationTool.h"
#include "JetMomentTools/JetLArHVTool.h"
#include "JetMomentTools/JetOriginCorrectionTool.h"
#include "JetMomentTools/JetECPSFractionTool.h"
#include "JetMomentTools/JetConstitFourMomTool.h"
#include "JetMomentTools/JetQGTaggerVariableTool.h"
#include "JetMomentTools/JetEMScaleMomTool.h"
#include "JetMomentTools/JetDRTrackAssocTool.h"
#include "JetMomentTools/JetConstituentFrac.h"
#include "JetMomentTools/JetGroomMRatio.h"
#include "JetMomentTools/BoostedJetTaggerTool.h"
#include "JetMomentTools/JetNumConstitTool.h"

#ifndef XAOD_STANDALONE
#include "JetMomentTools/JetIsolationTool.h"
#include "JetMomentTools/JetVoronoiMomentsTool.h"
#endif

#ifndef XAOD_ANALYSIS
#include "JetMomentTools/JetBadChanCorrTool.h"
#include "../JetCaloCellQualityTool.h"
#endif

// Project include(s).
#include "AsgTools/AsgComponentFactories.h"

DECLARE_COMPONENT( JetCaloEnergies )
DECLARE_COMPONENT( JetCaloQualityTool )
DECLARE_COMPONENT( JetCaloQualityToolFE )
DECLARE_COMPONENT( JetWidthTool )
DECLARE_COMPONENT( JetVertexFractionTool )
DECLARE_COMPONENT( JetVertexTaggerTool )
DECLARE_COMPONENT( JetPileupTag::JetVertexNNTagger )
DECLARE_COMPONENT( JetForwardJvtTool )
DECLARE_COMPONENT(JetForwardPFlowJvtTool)
DECLARE_COMPONENT(JetBalancePFlowJvtTool)
DECLARE_COMPONENT( JetTrackMomentsTool )
DECLARE_COMPONENT( JetTrackSumMomentsTool )
DECLARE_COMPONENT( JetClusterMomentsTool )
DECLARE_COMPONENT( JetPtAssociationTool )
DECLARE_COMPONENT( JetLArHVTool )
DECLARE_COMPONENT( JetOriginCorrectionTool )
DECLARE_COMPONENT( JetECPSFractionTool )
DECLARE_COMPONENT( JetConstitFourMomTool )
DECLARE_COMPONENT( JetQGTaggerVariableTool )
DECLARE_COMPONENT( JetEMScaleMomTool )
DECLARE_COMPONENT( JetDRTrackAssocTool )
DECLARE_COMPONENT( JetConstituentFrac )
DECLARE_COMPONENT( JetGroomMRatio)
DECLARE_COMPONENT( BoostedJetTaggerTool )
DECLARE_COMPONENT( JetNumConstitTool )

#ifndef XAOD_STANDALONE
DECLARE_COMPONENT( JetIsolationTool )
DECLARE_COMPONENT( JetVoronoiMomentsTool )
#endif

#ifndef XAOD_ANALYSIS
DECLARE_COMPONENT( JetBadChanCorrTool )
DECLARE_COMPONENT( JetCaloCellQualityTool )
#endif
