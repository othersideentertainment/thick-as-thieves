'''
UTILITY NODES
'''
import maya.cmds as cmds


def createOneMinus(**kwargs):        
    nodeName = kwargs.get("n", "oneMinus")
    node = cmds.createNode("plusMinusAverage", n=nodeName)   
    cmds.addAttr(node, ln="input", min=0, max=1, dv=0, k=True)
    cmds.addAttr(node, ln="output", min=0, max=1, dv = 1)   
    cmds.setAttr(node+".operation", 2)    
    cmds.setAttr(node+".input1D[0]", 1)
    cmds.setAttr(node+".input1D[1]", 0)    
    cmds.addAttr(node, ln="input", at="float", min=0, max=1, k=True)    
    cmds.connectAttr(node+".input", node+".input1D[1]")      
    cmds.connectAttr(node+".output1D", node+".output")  
    return node


def createLerpNode(**kwargs):        
    nodeName = kwargs.get("n", "lerp")
    remap = cmds.createNode("remapValue", n=nodeName)         
    cmds.addAttr(remap, ln="valueA", at="float")
    cmds.addAttr(remap, ln="valueB", at="float")    
    cmds.addAttr(remap, ln="weight", at="float", min=0, max=1, dv=0)
    cmds.addAttr(remap, ln="output", at="float")            
    cmds.connectAttr(remap+".valueA", remap+".value[0].value_FloatValue")
    cmds.connectAttr(remap+".valueB", remap+".value[1].value_FloatValue")    
    cmds.connectAttr(remap+".weight", remap+".inputValue")     
    cmds.connectAttr(remap+".outValue", remap+".output")       
    return remap


def createDistanceNode(**kwargs):    
    nodeName = kwargs.get("n", "distance")
    parentNode = kwargs.get("p", None)
    transformA = kwargs.get("transformA", None)
    transformB = kwargs.get("transformB", None)
    pc1 = None
    pc2 = None
    #
    xfA = cmds.createNode("transform", n=nodeName+"_pointA", p=parentNode)    
    if transformA != None:
        pc1 = cmds.pointConstraint(transformA, xfA)
        if parentNode != None:
            cmds.parent(pc1, parentNode)
    #    
    xfB = cmds.createNode("transform", n=nodeName+"_pointB", p=parentNode)                    
    if transformB != None:
        pc2 = cmds.pointConstraint(transformB, xfB)  
        if parentNode != None:
            cmds.parent(pc2, parentNode)
    #
    distanceNode = cmds.createNode("distanceBetween")
    cmds.connectAttr(xfA+".rotatePivot", distanceNode+".point1")
    cmds.connectAttr(xfA+".worldMatrix[0]", distanceNode+".inMatrix1")
    cmds.connectAttr(xfB+".rotatePivot", distanceNode+".point2")
    cmds.connectAttr(xfB+".worldMatrix[0]", distanceNode+".inMatrix2")             
    return distanceNode
    
    
def createStretchNode(**kwargs):
    nodeName = kwargs.get("n", "stretch")
    #attrs collection
    node = cmds.createNode("network", n=nodeName)
    cmds.addAttr(node, ln="defaultLength", at="float", min=0, dv=1, k=True)
    cmds.addAttr(node, ln="currentLength", at="float", min=0, dv=1, k=True)
    cmds.addAttr(node, ln="weight", at="float", min=0, max=1, dv=1, k=True)
    cmds.addAttr(node, ln="stretchFactor", at="float", min=0, dv=1, k=True)
    cmds.setAttr(node+".stretchFactor", cb=True)
    #floatMath(Max)
    maxNode = cmds.createNode("floatMath", n=node+"_max")
    cmds.setAttr(maxNode+".operation", 5) 
    cmds.connectAttr(node+".defaultLength", maxNode+".floatA")
    cmds.setAttr(maxNode+".floatB", .001)
    #floatMath(Divide)
    divideNode = cmds.createNode("floatMath", n=node+"_divide")
    cmds.setAttr(divideNode+".operation", 3)
    cmds.connectAttr(node+".currentLength", divideNode+".floatA")
    cmds.connectAttr(maxNode+".outFloat", divideNode+".floatB")
    #lerp (weight)
    lerpNode = createLerpNode(n=node+"_lerpWeight")
    cmds.setAttr(lerpNode+".valueA", 1)
    cmds.connectAttr(divideNode+".outFloat", lerpNode+".valueB")
    cmds.connectAttr(node+".weight", lerpNode+".weight")
    cmds.connectAttr(lerpNode+".outValue", node+".stretchFactor")
    cmds.setAttr(node+".stretchFactor", l=True)
    #Return the asset
    return node


def createSquashNode(**kwargs):
    nodeName = kwargs.get("n", "squash")
    #Values
    node = cmds.createNode("network", n=nodeName)
    cmds.addAttr(node, ln="stretchFactor", at="float", k=True, dv=1)
    cmds.addAttr(node, ln="weight", at="float", k=True, min=0, max=1, dv=1)    
    cmds.addAttr(node, ln="squashFactor", at="float", k=True, dv=1)
    #floatMath (power / sqrt)
    sqrtNode = cmds.createNode("floatMath", n=node+"_sqrt")
    cmds.setAttr(sqrtNode+".operation", 6)
    cmds.connectAttr(node+".stretchFactor", sqrtNode+".floatA")
    cmds.setAttr(sqrtNode+".floatB", .5)
    #floatMath divide
    divideNode = cmds.createNode("floatMath", n=node+"_divide")
    cmds.setAttr(divideNode+".operation", 3)
    cmds.setAttr(divideNode+".floatA", 1)
    cmds.connectAttr(sqrtNode+".outFloat", divideNode+".floatB")
    #lerp (Weight)    
    lerpNode = createLerpNode(n=node+"_lerpWeight")
    cmds.setAttr(lerpNode+".valueA", 1)
    cmds.connectAttr(divideNode+".outFloat", lerpNode+".valueB")
    cmds.connectAttr(node+".weight", lerpNode+".weight")
    cmds.connectAttr(lerpNode+".outValue", node+".squashFactor")        
    #
    cmds.setAttr(node+".squashFactor", l=True)
    return node
