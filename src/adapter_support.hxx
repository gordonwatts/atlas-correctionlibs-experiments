#pragma once
#include "backend.hxx"
#include <cstdlib>
inline std::string atlasCorrectionBundle(){
 const char* p=std::getenv("ATLAS_CORRECTIONLIB_BUNDLE");
 if(!p||!*p)throw std::runtime_error("ATLAS_CORRECTIONLIB_BUNDLE is required in the replacement environment");
 return p;
}
inline std::shared_ptr<Nominal> atlasNominalBackend(){
 auto root=atlasCorrectionBundle();std::ifstream input(root+"/nominal-manifest.json");nlohmann::json manifest;input>>manifest;
 const auto& s=manifest.at("settings");
 return std::make_shared<Nominal>(root+"/nominal.json.gz",s.at("OffsetCorrection.DefaultMuRef").get<double>(),s.at("OffsetCorrection.DefaultNPVRef").get<double>(),s.at("MuScaleFactor").get<double>());
}
