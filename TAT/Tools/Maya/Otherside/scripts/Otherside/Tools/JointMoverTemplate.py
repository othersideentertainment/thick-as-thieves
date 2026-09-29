import maya.cmds as cmds
from functools import partial

class JointMoverTemplateEditor():
    @staticmethod
    def get_mover_list():    
        mover_list = []
        node_list = cmds.ls(type="transform")
        for node in node_list:
            if "_mover" in node:
                mover_list.append(node)    
        return mover_list
    
    @staticmethod
    def is_attr_writable(attr_path):
        writable = True
        if cmds.listConnections(attr_path, s=True,d=False) != None:
            writable = False
        elif cmds.getAttr(attr_path, lock=True):
            writable = False
        return writable
            
    @staticmethod   
    def save_mover_layout(filepath):
        mover_list = JointMoverTemplateEditor.get_mover_list()    
        layout = ""
        attr_list = [
            "translateX", 
            "translateY", 
            "translateZ", 
            "rotateX", 
            "rotateY", 
            "rotateZ", 
            "scaleX", 
            "scaleY", 
            "scaleZ"]        
        for mover in mover_list:
            for attr in attr_list:
                path = mover+"."+attr
                if JointMoverTemplateEditor.is_attr_writable(path):            
                    value = cmds.getAttr(path)       
                    line = path+" "+str(round(value,4))   
                    layout += line+"\n"
        f = open(filepath, "w")
        f.write(layout)
        f.close()
        print "JointMoverTemplate saved: "+filepath 
    
    @staticmethod    
    def load_mover_layout(filepath):
        f = open(filepath, "r")
        for line in f:
            element = line.split(' ')
            if len(element) == 2:
                path = element[0]
                value = float(element[1])
                if cmds.objExists(path):      
                    if JointMoverTemplateEditor.is_attr_writable(path):
                        cmds.setAttr(path, value)
        f.close()
        print "JointMoverTemplate loaded: "+filepath    
    
    
    @staticmethod       
    def show():
        windowName = "joint_mover_template_window"
        widgets = {}
        #close pre-existing window
        if (cmds.window(windowName, exists=True)):
            cmds.deleteUI( windowName, window=True )   
        if (cmds.windowPref(windowName, exists=True)):
            cmds.windowPref(windowName, r=True)   
         
        cmds.window( windowName, title="JointMover")    
        cmds.columnLayout("main", rs=5, co=["both",10])  
        cmds.text(l="Joint Mover Templates", width=200, align="center")
        cmds.button(l="Save Layout", width=200, c=partial(JointMoverTemplateEditor.save))
        cmds.button(l="Load Layout", width=200, c=partial(JointMoverTemplateEditor.load))
        cmds.text(l="")
        cmds.setParent("..")
        cmds.showWindow(windowName)
        
    @staticmethod    
    def load(*args):
        startDir = cmds.workspace(q=True, active=True)
        filename = cmds.fileDialog2(ds=2, fm=1, ff="*.jmt", dir=startDir)
        if filename != None:
            JointMoverTemplateEditor.load_mover_layout(filename[0])

    @staticmethod
    def save(*args):
        startDir = cmds.workspace(q=True, active=True)
        filename = cmds.fileDialog2(ds=2, fm=0, ff="*.jmt",dir=startDir)
        if filename != None:
            JointMoverTemplateEditor.save_mover_layout(filename[0])

def show(*args):
    JointMoverTemplateEditor.show()
