"""Versioned schema for the optional generic CorrectionLib GraphSet extension."""
from typing import Annotated,Literal,Union
from pydantic import BaseModel,ConfigDict,Field
class Strict(BaseModel):
 model_config=ConfigDict(extra="forbid",populate_by_name=True)
class Operation(Strict):
 inputs:list[str]=[]
 outputs:list[str]=[]
class Constant(Operation):
 op:Literal["constant"]
 value:float
class Formula(Operation):
 op:Literal["formula"]
 expression:str
class Call(Operation):
 op:Literal["call"]
 correction:str
 compound:bool=False
class GraphCall(Operation):
 op:Literal["graph"]
 graph:str
class Cast(Operation):
 op:Literal["cast"]
 mode:Literal["float32","int64"]
class Assert(Operation):
 op:Literal["assert"]
 message:str
class Gaussian(Operation):
 op:Literal["gaussian"]
 algorithm:Literal["ROOT-TRandom3-Gaus"]
class Coordinates(Operation):
 op:Literal["coordinates"]
 mode:Literal["ptetaphim_properties","ptetaphim_normalize","ptetaphim_to_cartesian","cartesian_to_ptetaphim"]
class Inference(Operation):
 op:Literal["inference"]
 model:str
 features:list[str]
 input_node:str
 output_index:int=Field(ge=0)
class Branch(Operation):
 op:Literal["if"]
 then_:list["Instruction"]=Field(alias="then")
 else_:list["Instruction"]=Field(alias="else")
Instruction=Annotated[Union[Constant,Formula,Call,GraphCall,Cast,Assert,Gaussian,Coordinates,Inference,Branch],Field(discriminator="op")]
Branch.model_rebuild()
class Program(Strict):
 inputs:list[str]
 steps:list[Instruction]
 outputs:list[str]
class GraphBundle(Strict):
 graph_schema_version:Literal[1]
 requires:Literal["correctionlib-2.7.0+graph-1"]
 correction_set_file:str
 graphs:dict[str,Program]
