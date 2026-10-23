# Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration

"""
                                                                      
 JetResponseToolsConfig: A helper module for configuring jet moment
 tools, in support of JetRecConfig.JetModConfig. 
 Author: TJ Khoo                                                      


IMPORTANT : all the getXYZTool(jetdef, modspec) functions are meant to be used as callback from the main JetRecConfig module 
when we need to convert definitions of tools into the actual tools. At this point the functions are invoked as 
 func(jetdef, modspec)
Hence they have jetdef and modspec arguments even if not needed in every case.
"""

from AthenaConfiguration.ComponentFactory import CompFactory

def getJetResponseTool(jetdef, modspec=''):

    from JetRecConfig.JetDefinition import buildJetAlgName

    label = modspec
    prefix, suffix = {
        'R4TruthLabel': ('',''),
        'R4TruthDressedWZLabel': ('','DressedWZ'),
        'R4InTimeTruthLabel': ('InTime','')
    }[modspec]
    truthJetAlg = prefix+buildJetAlgName(jetdef.algorithm, jetdef.radius)+'Truth'+suffix+'Jets'

    jetPtAssociation = CompFactory.JetResponseTool(
        'jetResponse',
        JetMatchedTruthJetName = f'{label}_MatchedTruthJet',
        TruthJetContainer = truthJetAlg,
        )

    return jetPtAssociation