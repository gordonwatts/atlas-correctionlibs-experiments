/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/
#ifndef JETMOMENTTOOLS_NNJVTBINNING_H
#define JETMOMENTTOOLS_NNJVTBINNING_H

/**
 * @file JetMomentTools/NNJvtBinning.h
 *
 * Helpers for reading NN Jvt network binnings and results from an input file
 */

#include <vector>
#include <istream>
#include <string>

namespace xAOD {
    class IParticle;
}

namespace JetPileupTag {
    /// Helper struct to hold the bin edges for the NN Jvt cut maps
    struct NNJvtBinning {
        std::vector<float> ptEdges;
        std::vector<float> etaEdges;

        static NNJvtBinning fromJSON(std::istream &is);
        std::string toJSON() const;
        /**
         * @brief Get the correct bin for the provided pt/eta values
         * @param pt The pt of the object
         * @param eta The eta of the object
         * @param[out] ptBin The pt bin this belongs to, SIZE_MAX if none
         * @param[out] etaBin The eta bin this belongs to, SIZE_MAX if none
         * @return Whether or not the object falls into a bin
         */
        bool operator()(float pt, float eta, std::size_t &ptBin, std::size_t &etaBin) const;
        /**
         * @brief Get the correct bin for the provided particle
         * @param particle The particle to test
         * @param[out] ptBin The pt bin this belongs to, SIZE_MAX if none
         * @param[out] etaBin The eta bin this belongs to, SIZE_MAX if none
         * @return Whether or not the object falls into a bin
         */
        bool
        operator()(const xAOD::IParticle &particle, std::size_t &ptBin, std::size_t &etaBin) const;
    };

    /// The NNJvt cut maps
    struct NNJvtCutMap {
        NNJvtBinning edges;
        std::vector<std::vector<float>> cutMap;

        static NNJvtCutMap fromJSON(std::istream &is);
        std::string toJSON() const;
        /**
         * @brief Get the correct cut value for the provided pt/eta. Returns -1 if out of bounds
         * @param pt The pt of the object
         * @param eta The eta of the object
         * @return The cut value
         */
        float operator()(float pt, float eta) const;
        /**
         * @brief Get the correct cut value for the provided particle. Returns -1 if out of bounds
         * @param particle The particle to test
         * @return The cut value
         */
        float operator()(const xAOD::IParticle &particle) const;
        /**
         * @brief Get the correct cut value for the provided pt/eta bins. Both indices must be in
         * bounds
         * @param ptBinThe pt bin of the object
         * @param etaBin The eta bin of the object
         * @return The cut value
         */
        float operator()(std::size_t ptBin, std::size_t etaBin) const;
    };
} // namespace JetPileupTag

#endif //> !JETMOMENTTOOLS_NNJVTBINNING_H