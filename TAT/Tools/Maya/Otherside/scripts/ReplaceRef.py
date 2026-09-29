import maya.cmds as cmds
import os

projectPath = cmds.workspace(q=1, rd=1)

#update Game Exporter
def UpdateExportNameSpace(str):
    nodes = cmds.ls(type='gameFbxExporter')
    for x in nodes:
        cmds.setAttr(x+'.exportFilename', 'AN_' + str + '_', type='string')

def UpdateExportPath():
    nodes = cmds.ls(type='gameFbxExporter')
    filepath = cmds.file(q=1, sn=1)
    dirname = os.path.dirname(filepath)
    for x in nodes:
        cmds.setAttr(x+'.exportPath', dirname+'\Export', type='string')

#replace reference: old ref name, new ref name
def ReplaceRef(oldName,newName):
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+newName+'.mb', lr=oldName+'RN', pmt=0)
    cmds.file(projectPath+'/MayaTools/General/ART/Projects/TOW/ExportFiles/'+newName+'_Export.mb', lr=oldName+':'+oldName+'_ExportRN', pmt=0)
    #renaming nodes messes with ARTv1 ... commenting out for now
    #cmds.file( projectPath+'/MayaTools/General/ART/Projects/TOW/AnimRigs/'+newName+'.mb', e=1, namespace=newName)
    UpdateExportNameSpace(newName)
    UpdateExportPath()


#ReplaceRef('DefaultShort','Tr_MN')
