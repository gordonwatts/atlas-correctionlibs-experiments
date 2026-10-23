# Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration

########################################################################
#                                                                      #
# JetJvtEfficiencyToolConfig: A helper module for configuring jet jvt  #
# selection and efficiency configurations. This way the derivation     #
# framework can easily be kept up to date with recommendations.        #
# Authors: mswiatlo, wbalunas                                          #
#                                                                      #
########################################################################

from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

def getJvtSelToolCfg(flags, jetContainer):
  """Configure the JVT selection tool"""
  acc = ComponentAccumulator()
  if "EMTopo" in jetContainer:
    # Not recommended for analysis but retained for specialized usage
    jvtSelTool = CompFactory.CP.JvtSelectionTool("JVTSelection_{0}".format(jetContainer))
    jvtSelTool.JetContainer = jetContainer
    jvtSelTool.JvtMomentName = "Jvt"
    jvtSelTool.PassFlagName = "passJvt"
    jvtSelTool.IsPFlow = False
    acc.setPrivateTools(jvtSelTool)
    return acc
  # PFlow jets
  jvtSelTool = CompFactory.CP.NNJvtSelectionTool("JVTSelection_{0}".format(jetContainer))
  jvtSelTool.JetContainer = jetContainer
  jvtSelTool.JvtMomentName = "NNJvt"
  jvtSelTool.PassFlagName = "passJvt"
  acc.setPrivateTools(jvtSelTool)
  return acc

# TODO: Efficiencies not currently computed in derivations, but if needed this can be added here.
