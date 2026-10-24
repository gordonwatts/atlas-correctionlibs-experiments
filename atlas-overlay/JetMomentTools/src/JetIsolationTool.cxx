/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// JetIsolationTool.cxx 

#include "JetMomentTools/JetIsolationTool.h"
#include "xAODCaloEvent/CaloCluster.h"
#include "FourMomUtils/xAODP4Helpers.h"
//#include <sstream>
#include <format>

using std::string;
using fastjet::PseudoJet;

namespace jet {

  namespace JetIsolation {
  
    // **************************************************************************
    // **************************************************************************
    using FourMom_t = TLorentzVector;

    TVector3 unitVector(const FourMom_t& v){
      TVector3 v3 = v.Vect();
      return (1/v3.Mag())*v3;
    }
    
    /// IsolationCalculator : base class for isolation calculations
    /// Implementations of this class encapsulate all the needed calculations inside the
    /// calcIsolationVariables(jet, constituents) function, where 'constituents' is expected to
    /// be a vector a consituent NOT containing the jet's constitents.
    class IsolationCalculator {
    public:

      /// Define the available isolation variables
      enum Kinematics {
        Pt, PtPUsub, SumPt, Par, Perp, P
      };

      /// names for isolation variables. Must match EXACTLY the enum.
      static constexpr std::array<std::string_view, 6> s_kname = {"Pt","PtPUsub" , "SumPt", "Par", "Perp", "P"};      

      /// Holds the 4-vector of all constituents contributing to isolation.
      struct IsolationResult {        
        FourMom_t isoP ;
        double isoSumPt = 0.0;
	double isoArea = 0.;
      };
      
      virtual ~IsolationCalculator() = default;

      
      virtual string baseName() const {return "";}
      virtual IsolationCalculator * clone(const xAOD::Jet* ) const {return nullptr;}
      virtual void copyFrom( const IsolationCalculator* o, const xAOD::Jet*){
        m_kinematics = o->m_kinematics;
      }
      
        
      /// Compute the isolation 4-momentum from jet and jet inputs.
      /// It is assumed the caller has already removed jet constituents from the input list.
      virtual IsolationResult jetIsolation(const xAOD::Jet*, std::vector<const xAOD::IParticle*> & ) const {
        return {};
      }

      bool scheduleKinematicCalculation(const std::string& kname){
	auto it = std::ranges::find(s_kname, kname);
	if( it != s_kname.end( )){
	  m_kinematics.push_back(static_cast<Kinematics>(it-s_kname.begin()) );
	  return true;
	}
        return false;
      }


      /// Implement the calculation of isolation variables for this jet.
      /// The vector<IParticle*> nearbyConstit is expected to contain all the constituents which can fall in the isolation area AND which are not constituent of the jet.
      virtual std::vector<float> 
      calcIsolationVariables(const xAOD::Jet* jet, std::vector<const xAOD::IParticle*>& nearbyConstit) const {
        IsolationResult result = jetIsolation(jet, nearbyConstit);
	std::vector<float> calcVector;
	float pt = jet->jetP4(xAOD::JetConstitScaleMomentum).pt();
	static const SG::AuxElement::Accessor<float> areaAcc("ActiveArea");
	float jetArea=0;
	for(auto k : m_kinematics){
	  float v=-1;
          switch( k ) {
          case Pt:
	    v = result.isoP.Pt() / pt; break ;
	  case PtPUsub:
	    jetArea= areaAcc(*jet);	    
	    v = (result.isoP.Pt() - (result.isoArea-jetArea)*m_rho)/ (pt-jetArea*m_rho);
	    break;
          case Perp:
            v = result.isoP.Vect().Dot(unitVector(jet->p4())) /pt; break;
          case SumPt:
            v = result.isoSumPt; break;
          case Par:
            v = result.isoP.Vect().Perp(unitVector(jet->p4())) /pt ; break;
          case P:
	    v = result.isoP.Vect().Mag()/pt; break ;
	  }
	  calcVector.push_back(v);
        }
        return calcVector;
      }

      virtual
      std::vector<std::string> calculationNames() const {
	std::vector<std::string> v;
	v.reserve(m_kinematics.size());
	for(auto k : m_kinematics) {
	  v.emplace_back(baseName() + string(s_kname[k]));
	}
	return v;	
      }

      
      void dump() const {
        std::cout << "Isolation calculator "<< baseName() <<std::endl;
	for(const auto& n : calculationNames()){
	  std::cout << "   - "<< n << std::endl; 
	}
      }

      void setEventDensity(float rho){
	m_rho=rho;
      }
      
    protected:
      /// kinematics isolation variables to be computed
      std::vector<Kinematics> m_kinematics;
      /// Value of the event density in case it is needed.
      float m_rho=-9999.; // initialized to obviously wrong value.
    };
    

    template<typename ISOCRITERIA>
    class IsolationCalculatorT : public IsolationCalculator {
    public:

      IsolationCalculatorT(double param=0.) : m_iso(param) { }

      virtual IsolationResult
      jetIsolation(const xAOD::Jet* jet, std::vector<const xAOD::IParticle*> &nearbyConstit) const {
        IsolationResult result;
	double rap = jet->rapidity();
	double phi = jet->phi();
	for(const xAOD::IParticle* constit:nearbyConstit){
	  if ( m_iso.inIsolationArea(rap, phi, constit) ) {
	    result.isoP     += constit->p4();
            result.isoSumPt += constit->pt();
          }    
        }
	result.isoArea = m_iso.isoArea();
        return result;
      }
      
      virtual string baseName() const {return m_iso.name();}

      virtual IsolationCalculator * clone(const xAOD::Jet* j) const {
        IsolationCalculatorT* isoT= new IsolationCalculatorT();
        isoT->m_iso = m_iso;
        isoT->m_iso.setup(j);
        isoT->copyFrom(this,j);
        return isoT;
      }

      ISOCRITERIA m_iso;
    };
    
    
    /// \class IsolationAreaBase 
    /// Defines a zone from which constituents will contribute to the isolation of a jet.
    /// In most cases, the zone is simply a cone which radius depends on the jet radius.
    struct IsolationAreaBase {
      IsolationAreaBase(double p, const string &n) : m_parameter(p), m_name(n){}
      
      bool inIsolationArea(double rap, double phi, const xAOD::IParticle* part) const {
	// we use eta rather than rapidity for constituents, because the later can generate FPE (presumably in very low pt PFlow constit)
        double dr2 = xAOD::P4Helpers::deltaR2(rap, phi, part->rapidity(), part->phi());
        return dr2 < m_deltaRmax2;
      }
      
      string name() const {
        std::ostringstream oss; oss << m_name << int(10*m_parameter);
        return oss.str();        
      }

      double isoArea() const{
	return m_deltaRmax2*M_PI;
      }
      
      double m_parameter;
      string m_name;
      double m_deltaRmax2 = 0.0;
      
    }; 
    
      
    
    // here we define a short cut to declare implementations of IsolationAreaBase classes.
    // In most cases, only a definition of the max deltaR is enough to specify the area, hence these shortcuts. 
    // 1st parameter : class name
    // 2nd parameter : code defining the max deltaR. Can use the variable 'param' representing the size parameter of this IsolationCalculator
    // 3rd parameter (optional) : (re)-declaration of additional methods.
    /// See below for example
#define ISOAREA( calcName, deltaRcode , additionalDecl )  struct calcName : public IsolationAreaBase { \
        calcName(double p) : IsolationAreaBase(p, #calcName){}          \
        virtual void setup(const xAOD::Jet* j)  {double jetRadius=j->getSizeParameter();  double param = m_parameter; m_deltaRmax2=deltaRcode ; m_deltaRmax2*=m_deltaRmax2; (void)(param+jetRadius);}  additionalDecl }


    ISOAREA( IsoKR , jetRadius*param, )  ;
    ISOAREA( IsoDelta, jetRadius+param, )  ;
    ISOAREA( IsoFixedCone, param, )  ; 
    ISOAREA( IsoFixedArea, sqrt(jetRadius*jetRadius+param*M_1_PI) , )  ;

    // For Iso6To8 we need to redefine inIsolationArea
    ISOAREA( Iso6To8, 0.8 ,  bool inIsolationArea(double rap, double phi, const xAOD::IParticle* constit)const ;  )  ;
    bool Iso6To8::inIsolationArea(double rap, double phi, const xAOD::IParticle* constit) const {
      double dr2 = xAOD::P4Helpers::deltaR2(rap, phi, constit->rapidity(), constit->phi());
      return ( (dr2<0.8*0.8) && (dr2>0.6*0.6) );
    }

 
    IsolationCalculator *createCalulator(const std::string& n, double parameter){

      if( n == "IsoKR" )        return new  IsolationCalculatorT<IsoKR>( parameter);      
      if( n == "IsoDelta" )     return new  IsolationCalculatorT<IsoDelta>( parameter);      
      if( n == "IsoFixedCone" ) return new  IsolationCalculatorT<IsoFixedCone>( parameter);      
      if( n == "IsoFixedArea" ) return new  IsolationCalculatorT<IsoFixedArea>( parameter);      
      if( n == "Iso6To8" )      return new  IsolationCalculatorT<Iso6To8>( parameter);

      return nullptr;
    }


  } // namespace JetIsolation

  /// helper 
  void colonSplit(const string &in, string &s1, string &s2, string &s3){
    s2=""; s3="";
    std::stringstream str(in);
    std::getline(str, s1, ':' );
    std::getline(str, s2, ':');
    std::getline(str, s3);
  }


} // namespace jet 

using namespace jet::JetIsolation;

//**********************************************************************

JetIsolationTool::JetIsolationTool(const string& name) 
  : asg::AsgTool(name) {
  declareInterface<IJetDecorator>(this);
}

//**********************************************************************

JetIsolationTool::~JetIsolationTool() = default;

//**********************************************************************

StatusCode JetIsolationTool::initialize() {
  ATH_MSG_DEBUG ("Initializing " << name() << "...");

  // a temporary map of isolation calculator
  std::map<string, IsolationCalculator*> calcMap;

  size_t nmom = m_isolationCodes.size();
  // decode each isolation calculations as entered by properties
  for ( size_t i=0; i<nmom; ++i ) {
    string isocriteria, param_s, kinematic;
    // decode the string passed as a property :
    jet::colonSplit(m_isolationCodes[i], isocriteria, param_s, kinematic);
    if( (param_s.empty())||(kinematic.empty())) {
      ATH_MSG_ERROR(" Can't extract parameter values or kinematic code from "<<m_isolationCodes[i]);
      return StatusCode::FAILURE;
    }
    string calcId = isocriteria + param_s;
    // isocriteria + param_s defines a calculator which can then calculate several kinematics.
    // check if this (isocriteria + param_s ) calculator has already been defined. If not do it now.
    IsolationCalculator*& isoC = calcMap[calcId];
    if ( isoC == nullptr ) {
      isoC = createCalulator( isocriteria, std::stod(param_s)*0.1 );
    }
    if( isoC == nullptr ) {
      ATH_MSG_ERROR(" Unkown isolation criteria "<< isocriteria << "  from "<< m_isolationCodes[i] );
      return StatusCode::FAILURE;
    }
    // add this kinematic for this calculator 
    bool ok = isoC->scheduleKinematicCalculation( kinematic);
    if(!ok) {
      ATH_MSG_ERROR(" Unkown isolation kinematic "<< kinematic << "  from "<< m_isolationCodes[i] );
      return StatusCode::FAILURE;
    }
  }

  if(m_jetContainerName.empty()) {
    ATH_MSG_ERROR("JetIsolationTool needs to have its input jet container configured!");
    return StatusCode::FAILURE;
  }
  
  // Fill the iso calculator vector from the map
  // Also fill DecorHandleKeyArrays at the same time
  int ncalc=0;
  for ( const auto& pair : calcMap ){ 
    m_isoCalculators.push_back(pair.second); 
    ATH_MSG_DEBUG("Will use iso calculation : "<< pair.second->baseName() << "  and variables :" );

    for(const auto& kname: pair.second->calculationNames()){
      ATH_MSG_DEBUG("      -->  : "<< kname );
      m_decorKeys.push_back(std::format("{}.{}", m_jetContainerName.value(), kname));
      ncalc++;
    }
  }

  
  ATH_MSG_INFO("Initialized JetIsolationTool " << name());
  if ( m_inputConstitKey.empty() ) {
    ATH_MSG_ERROR("  No input pseudojet collection supplied");
    return StatusCode::FAILURE;
  } else {
    ATH_MSG_INFO("  Input pseudojet collection: " << m_inputConstitKey.key());
    ATH_CHECK(m_inputConstitKey.initialize());
  }
  ATH_MSG_INFO("  Isolation calculations: " << m_isolationCodes);
  ATH_MSG_DEBUG("Total num calculations="<<ncalc<< "  ndecorations="<<m_decorKeys.size());
  
  ATH_CHECK(m_decorKeys.initialize());

  if(!m_rhoKey.empty()){
    ATH_CHECK(m_rhoKey.initialize());
    ATH_MSG_INFO(" Using EventDensity from: " << m_rhoKey.key());
  }
  
  return StatusCode::SUCCESS;
}

//**********************************************************************

StatusCode JetIsolationTool::decorate(const xAOD::JetContainer& jets) const {

  
  ATH_MSG_DEBUG("Modifying jets in container with size " << jets.size());
  if ( jets.empty() ) return StatusCode::SUCCESS;

  // Fetch the input pseudojets.
  auto inputConstits = SG::makeHandle(m_inputConstitKey);
  ATH_MSG_DEBUG("Retrieved input count is " << inputConstits->size());

  // adapt the calculators to these jets (radius, input type, etc...)
  // Since we're in a const method, we must create a copy of the calculators we own.
  // We use the 1st jet, assuming all others in the container are similar.
  std::vector<IsolationCalculator*> calculators; // the adapted calculators.
  for( const IsolationCalculator * calc : m_isoCalculators ){
    IsolationCalculator * cloned = calc->clone( jets[0] );
    cloned->setEventDensity(0);
    calculators.push_back( cloned );
  }

  // Retrieve and set EventDensity if needed
  if(!m_rhoKey.empty()){
    double rho=0;
    SG::ReadHandle<xAOD::EventShape> rhRhoKey(m_rhoKey);
    if (! rhRhoKey->getDensity( xAOD::EventShape::Density, rho ) ) {
      ATH_MSG_FATAL("Could not retrieve xAOD::EventShape::Density from xAOD::EventShape "<< m_rhoKey.key());
      return StatusCode::FAILURE;
    }
    ATH_MSG_DEBUG("Rho = "<< rho);
    for(  IsolationCalculator * calc : calculators ) calc->setEventDensity(rho);
  }

  // This will hold  all the constituents around a jet which are not constituents of the jet
  std::vector<const xAOD::IParticle*> nearbyC;
  nearbyC.reserve( inputConstits->size() ); 
  const static SG::AuxElement::ConstAccessor<char> PVMatchedAcc("matchedToPV");
  // Loop over jets in this collection.
  for (const xAOD::Jet* jet : jets ) {

    nearbyC.clear();
    size_t nRejected=0;
    const std::vector< ElementLink< xAOD::IParticleContainer > >& constitEL = jet->constituentLinks();

    // Fill nearbyC: for now we take ALL constituents  which not constituents of the jet.
    //  this is a lot and it may render subsequent iso calculation slower. If so it
    // could be useful to use a fast look-up map to preselect constituents close to the jet (ex: within 2*jetRadius)
    for(const xAOD::IParticle *part: (*inputConstits)){
      if((part->e()<=0) || ( PVMatchedAcc.isAvailable(*part) && !PVMatchedAcc(*part) )){
	nRejected++;
	continue;
      }
      bool found = std::any_of(constitEL.begin(), constitEL.end(), [&part](const auto& link) { return part == *link; });
      if(!found){
	nearbyC.push_back(part);	
      }      
    }
    if ( (nearbyC.size() + jet->numConstituents()+nRejected) != inputConstits->size() ) {
      ATH_MSG_WARNING("Inconsistent number of jet constituents found in jet.");
    }

    ATH_MSG_DEBUG(jet->index()<< "  # outside jet: " << nearbyC.size() << ", # rejected :"<< nRejected
		  << ", in jet: "<< jet->numConstituents()
		  << ", total: "<< inputConstits->size() );
		      
    
    // loop over calculators, calculate isolation given the close-by particles not part of the jet.
    int c=0;
    for(auto * isoCalc:calculators){
      // perform all calculations with this isoCalc
      std::vector<float> results = isoCalc->calcIsolationVariables(jet, nearbyC);
      // decorate the jet with the calculated values
      for( float value: results ) {
	SG::WriteDecorHandle<xAOD::JetContainer, float> decorHandle(m_decorKeys[c++]);
	ATH_MSG_DEBUG(" decor "<< decorHandle.decorKey()  << "  " << value << "  "<<c);
	decorHandle(*jet) = value;
      }
    }    
  }

  // clear calculators :
  for ( IsolationCalculator* calc : calculators ) {
    delete calc;
  }

  ATH_MSG_DEBUG("done");
  return StatusCode::SUCCESS;
}


//**********************************************************************

StatusCode JetIsolationTool::finalize() {
  ATH_MSG_DEBUG ("Finalizing " << name() << "...");
  for( size_t i=0;i<m_isoCalculators.size(); i++){
    delete m_isoCalculators[i];
  }
  return StatusCode::SUCCESS;
}

