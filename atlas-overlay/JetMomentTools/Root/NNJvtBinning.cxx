/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/
#include "JetMomentTools/NNJvtBinning.h"
#include "nlohmann/json.hpp"
#include "xAODBase/IParticle.h"

#include <algorithm>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>

namespace {
    static constexpr float GeV = 1e3; // AnalysisBase has no SystemOfUnits.h
}

namespace JetPileupTag {

    void to_json(nlohmann::json &j, const NNJvtBinning &binning) {
        j = nlohmann::json{{"ptbin_edges", binning.ptEdges}, {"etabin_edges", binning.etaEdges}};
    }

    void from_json(const nlohmann::json &j, NNJvtBinning &binning) {
        j.at("ptbin_edges").get_to(binning.ptEdges);
        j.at("etabin_edges").get_to(binning.etaEdges);
        // pT values are stored in GeV but we should use CLHEP values wherever possible
        for (float &value : binning.ptEdges)
            value *= GeV;
    }

    void to_json(nlohmann::json &j, const NNJvtCutMap &cutMap) {
        to_json(j, cutMap.edges);
        std::map<std::string, float> cuts;
        for (std::size_t ptIdx = 0; ptIdx < cutMap.cutMap.size(); ++ptIdx)
            for (std::size_t etaIdx = 0; etaIdx < cutMap.cutMap.at(ptIdx).size(); ++etaIdx)
                cuts["(" + std::to_string(ptIdx) + ", " + std::to_string(etaIdx) + ")"] =
                        cutMap.cutMap.at(ptIdx).at(etaIdx);
        j["cuts"] = cutMap;
    }

    void from_json(const nlohmann::json &j, NNJvtCutMap &cutMap) {
        j.get_to(cutMap.edges);
        cutMap.cutMap.resize(cutMap.edges.ptEdges.size() - 1);
        for (auto &v : cutMap.cutMap)
            v.resize(cutMap.edges.etaEdges.size() - 1);
        std::regex expr(R"(\((\d+),\s*(\d+)\))");
        for (const auto &[bin, cut] : j["cuts"].get<std::map<std::string, float>>()) {
            std::smatch sm;
            if (!std::regex_match(bin, sm, expr))
                throw std::invalid_argument("Invalid bin descriptor: " + bin);
            cutMap.cutMap.at(std::stoi(sm[1])).at(std::stoi(sm[2])) = cut;
        }
    }

    NNJvtBinning NNJvtBinning::fromJSON(std::istream &is) {
        nlohmann::json j;
        is >> j;
        return j.get<NNJvtBinning>();
    }

    std::string NNJvtBinning::toJSON() const {
        return nlohmann::json(*this).dump();
    }

    bool
    NNJvtBinning::operator()(float pt, float eta, std::size_t &ptBin, std::size_t &etaBin) const {
        ptBin = std::distance(
                ptEdges.begin(), std::lower_bound(ptEdges.begin(), ptEdges.end(), pt));
        etaBin = std::distance(
                etaEdges.begin(), std::lower_bound(etaEdges.begin(), etaEdges.end(), eta));
        // 0 => below the lowest bin edge, size() => above the highest bin edge
        // Use lowest pt bin for any jets with lower pt
        if (ptBin == 0)
            ptBin = 1;
        if (ptBin == ptEdges.size())
            ptBin = SIZE_MAX;
        else
            ptBin -= 1;
        if (etaBin == 0 || etaBin == etaEdges.size())
            etaBin = SIZE_MAX;
        else
            etaBin -= 1;
        
        return ptBin != SIZE_MAX && etaBin != SIZE_MAX;
    }

    bool NNJvtBinning::operator()(
            const xAOD::IParticle &particle, std::size_t &ptBin, std::size_t &etaBin) const {
        return this->operator()(particle.pt(), particle.eta(), ptBin, etaBin);
    }

    NNJvtCutMap NNJvtCutMap::fromJSON(std::istream &is) {
        nlohmann::json j;
        is >> j;
        return j.get<NNJvtCutMap>();
    }

    std::string NNJvtCutMap::toJSON() const {
        return nlohmann::json(*this).dump();
    }

    float NNJvtCutMap::operator()(float pt, float eta) const {
        std::size_t ptBin, etaBin;
        if (!edges(pt, eta, ptBin, etaBin))
            return -1;
        return this->operator()(ptBin, etaBin);
    }

    float NNJvtCutMap::operator()(const xAOD::IParticle &particle) const {
        std::size_t ptBin, etaBin;
        if (!edges(particle, ptBin, etaBin))
            return -1;
        return this->operator()(ptBin, etaBin);
    }

    float NNJvtCutMap::operator()(std::size_t ptBin, std::size_t etaBin) const {
        return cutMap.at(ptBin).at(etaBin);
    }
} // namespace JetPileupTag