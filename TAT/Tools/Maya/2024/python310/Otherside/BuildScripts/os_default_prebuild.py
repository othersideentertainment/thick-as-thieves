import maya.cmds as cmds

def create_ik_bones():
    #ikbones
    ikbones = {}
    ikbones["ik_foot_root"] = cmds.createNode("joint", n="ik_foot_root")
    ikbones["ik_foot_l"] = cmds.createNode("joint", n="ik_foot_l")
    ikbones["ik_foot_r"] = cmds.createNode("joint", n="ik_foot_r")
    ikbones["ik_hand_root"] = cmds.createNode("joint", n="ik_hand_root")
    ikbones["ik_hand_gun"] = cmds.createNode("joint", n="ik_hand_gun")    
    ikbones["ik_hand_l"] = cmds.createNode("joint", n="ik_hand_l")    
    ikbones["ik_hand_r"] = cmds.createNode("joint", n="ik_hand_r")            
    #hierarchy
    #|-foot
    ikbones["ik_foot_root"] = cmds.parent(ikbones["ik_foot_root"], "root")[0]
    ikbones["ik_foot_k"] = cmds.parent(ikbones["ik_foot_l"], ikbones["ik_foot_root"])[0]
    ikbones["ik_foot_r"] = cmds.parent(ikbones["ik_foot_r"], ikbones["ik_foot_root"])[0]    
    #|-hand
    ikbones["ik_hand_root"] = cmds.parent(ikbones["ik_hand_root"], "root")[0]
    ikbones["ik_hand_gun"] = cmds.parent(ikbones["ik_hand_gun"], ikbones["ik_hand_root"])[0]
    ikbones["ik_hand_l"] = cmds.parent(ikbones["ik_hand_l"], ikbones["ik_hand_gun"])[0]      
    ikbones["ik_hand_r"] = cmds.parent(ikbones["ik_hand_r"], ikbones["ik_hand_gun"])[0]      
    #bones
    targets = {}    
    targets["hand_l"] = "hand_l"
    targets["hand_r"] = "hand_r"
    targets["foot_l"] = "foot_l"
    targets["foot_r"] = "foot_r"
    #constraints
    cmds.parentConstraint( targets["hand_r"], ikbones["ik_hand_gun"])
    cmds.parentConstraint( targets["hand_l"], ikbones["ik_hand_l"])
    cmds.parentConstraint( targets["hand_r"], ikbones["ik_hand_r"])
    cmds.parentConstraint( targets["foot_l"], ikbones["ik_foot_l"])
    cmds.parentConstraint( targets["foot_r"], ikbones["ik_foot_r"])            

#run the setup
if cmds.objExists("ik_foot_root") == False:
    create_ik_bones()