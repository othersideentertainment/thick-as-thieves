import maya.cmds as cmds
import Select

def SetupJointLabels(side,color):
   selected = Select.Type('joint')
   for each in selected:
      cmds.setAttr ((each + '.side'), side) ## set Side == Center (0) Left (1) Right (2)
      cmds.setAttr ((each + '.overrideColor'), color) ## color index 5: navy (center), 31/30: red (left), 15/28: blue (right)
      cmds.setAttr ((each + '.overrideEnabled'), 1)    
      cmds.setAttr ((each + '.type'), 18) #set type Other
      cmds.setAttr ((each + '.otherType'), each, type='string')
      cmds.setAttr(("" + each + ".drawLabel"), 0)
      cmds.refresh()

def SetJointColor(obj, int): ## color index 5: navy (center), 31/30: red (left), 15/28: blue (right)
   cmds.setAttr((obj + '.overrideColor'), int)  
   cmds.setAttr((obj + '.overrideEnabled'), 1)
   cmds.refresh()

def SetJointSide(obj, int):  ## set Side == Center (0) Left (1) Right (2)
   cmds.setAttr((obj + '.side'), int) 

def SetJointLabelType(obj):
   cmds.setAttr((obj + '.type'), 18)  # set type Other
   cmds.setAttr((obj + '.otherType'), obj, type='string')

def SetupJoints(sel,color,side):
   for each in sel:
      SetJointColor(each,color)
      SetJointSide(each,side)
      SetJointLabelType(each)

def SetupSelectedJoints():
   selected = Select.Type('joint')
   leftJoints = []
   rightJoints = []
   for i in selected:
      splitNames = i.split('_')
      suffix = splitNames[-1]
      if suffix == 'l':
         selected.remove(i)
         leftJoints.append(i)
      if suffix == 'r':
         selected.remove(i)
         rightJoints.append(i)
   
   for j in selected:
      SetupJoint(j,5,0)
   for l in leftJoints:
      SetupJoint(l,31,1)
   for r in rightJoints:
      SetupJoint(r,15,2)
      
def DisplayJointLabel():
   selected = Select.Type('joint')
   for each in selected:
      cmds.setAttr(("" + each + ".drawLabel"), 0)


