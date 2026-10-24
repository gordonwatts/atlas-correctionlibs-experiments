"""Schema-v2 builders. These preserve interpolation, not histogram step values."""
import math

def formula(expression, variables, parameters=()):
    return dict(nodetype="formula", expression=expression, parser="TFormula",
                variables=list(variables), parameters=list(parameters))

# Cache axis checks during export; many tables share the same edges.
from functools import lru_cache
from bisect import bisect_right

@lru_cache(maxsize=256)
def _uniform_axis(edges):
    """Return (n, low, high) only when finite-double bin membership is preserved.

    Evenly spaced decimal edges alone are insufficient: arithmetic indexing
    can round a value just below an edge into the next bin. The arithmetic
    is monotone; testing each edge and its adjacent representable values
    checks the transitions against upper_bound. Keep unsafe axes explicit.
    This matches CorrectionLib's current division-then-multiplication order.
    """
    values=tuple(float(x) for x in edges)
    if len(values)<2 or not all(math.isfinite(x) for x in values):return None
    low,high=values[0],values[-1];n=len(values)-1;span=high-low
    if not math.isfinite(span) or span<=0:return None
    if any(a>=b for a,b in zip(values,values[1:])):return None
    for i,x in enumerate(values):
        expected=low+i*span/n
        if abs(x-expected)>4*max(math.ulp(x),math.ulp(expected)):return None
    for edge in values:
        for x in (math.nextafter(edge,-math.inf),edge,math.nextafter(edge,math.inf)):
            if low<=x<high and int(n*((x-low)/span))!=bisect_right(values,x)-1:return None
    return n,low,high

def binning(name, edges, content, flow="error"):
    edges=[float(e) if not isinstance(e,str) else e for e in edges]
    uniform=None if flow=="wrap" else _uniform_axis(tuple(edges))
    if uniform is not None:
        n,low,high=uniform
        edges=dict(n=n,low=low,high=high)
    return dict(nodetype="binning", input=name, edges=edges, content=list(content), flow=flow)

def transform(name, rule, content):
    return dict(nodetype="transform", input=name, rule=rule, content=content)

def clamp(name, low, high, content):
    return transform(name, formula("min(max(x,[0]),[1])", [name], [low,high]), content)

def linear(axis, values, name, inverse=False):
    """TH1::Interpolate: linear between bin centres, constant beyond them."""
    if len(axis)!=len(values) or not axis: raise ValueError("invalid linear interpolation")
    if len(axis)==1: return 1./values[0] if inverse else float(values[0])
    content=[float(values[0])]
    for i in range(len(axis)-1):
        expr="[0]+(x-[1])*(([2]-[0])/([3]-[1]))"
        if inverse: expr="1/("+expr+")"
        content.append(formula(expr,[name],[values[i],axis[i],values[i+1],axis[i+1]]))
    content.append(float(values[-1]))
    if inverse: content[0]=formula("1/[0]",[name],[values[0]]);content[-1]=formula("1/[0]",[name],[values[-1]])
    return binning(name,["-inf",*axis,"inf"],content)

def bilinear(xs,ys,values,xname,yname,inverse=False):
    if len(xs)==1: return linear(ys,values[0],yname,inverse)
    if len(ys)==1: return linear(xs,[row[0] for row in values],xname,inverse)
    if not xs or not ys: raise ValueError("empty histogram axis")
    rows=[]
    for i in range(len(xs)-1):
        row=[]
        for j in range(len(ys)-1):
            # ROOT's rectangle interpolation, with equivalent algebra.
            expr="([0]*([5]-x)*([7]-y)+[1]*(x-[4])*([7]-y)+[2]*([5]-x)*(y-[6])+[3]*(x-[4])*(y-[6]))/(([5]-[4])*([7]-[6]))"
            if inverse: expr="1/("+expr+")"
            row.append(formula(expr,[xname,yname],[values[i][j],values[i+1][j],values[i][j+1],values[i+1][j+1],xs[i],xs[i+1],ys[j],ys[j+1]]))
        rows.append(binning(yname,ys,row,"clamp"))
    node=binning(xname,xs,rows,"clamp")
    return clamp(xname,xs[0],xs[-1],clamp(yname,ys[0],ys[-1],node))

def correction(name,inputs,data,description=""):
    generic=[]; indices={}
    def pool(node):
        if isinstance(node,list): return [pool(x) for x in node]
        if not isinstance(node,dict): return node
        if node.get("nodetype")=="formula" and node.get("parameters"):
            key=(node["expression"],tuple(node["variables"]))
            if key not in indices:
                indices[key]=len(generic)
                generic.append(dict(nodetype="formula",expression=key[0],parser="TFormula",variables=list(key[1])))
            return dict(nodetype="formularef",index=indices[key],parameters=node["parameters"])
        return {k:pool(v) for k,v in node.items()}
    result=dict(name=name,description=description,version=1,
                inputs=[dict(name=n,type=t) for n,t in inputs],
                output=dict(name="value",type="real"),data=pool(data))
    if generic: result["generic_formulas"]=generic
    return result

def piecewise_coeff(edges,coeff,name):
    if len(coeff)!=len(edges): raise ValueError("residual intercept/slopes length mismatch")
    running=coeff[0]; cells=[]
    for i in range(len(edges)-1):
        cells.append(formula("[0]+[1]*(x-[2])",[name],[running,coeff[i+1],edges[i]]))
        running+=(edges[i+1]-edges[i])*coeff[i+1]
    return binning(name,edges,cells)
