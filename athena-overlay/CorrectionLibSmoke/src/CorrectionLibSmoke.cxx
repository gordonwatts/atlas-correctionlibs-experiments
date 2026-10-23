#include "AthenaBaseComps/AthAlgorithm.h"
#include "GaudiKernel/ToolHandle.h"
class CorrectionLibSmoke final:public AthAlgorithm{
 ToolHandleArray<IAlgTool> m_tools{this,"Tools",{},"Tools to initialize"};
public:
 CorrectionLibSmoke(const std::string& name,ISvcLocator* locator):AthAlgorithm(name,locator){}
 StatusCode initialize()override{
  ATH_CHECK(m_tools.retrieve());
  ATH_MSG_INFO("CorrectionLib Athena adapters initialized successfully: "<<m_tools.size()<<" tools");
  return StatusCode::SUCCESS;
 }
 StatusCode execute(const EventContext&)override{return StatusCode::SUCCESS;}
};
DECLARE_COMPONENT(CorrectionLibSmoke)
