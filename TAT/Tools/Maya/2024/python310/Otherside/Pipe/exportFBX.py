import maya.cmds as cmds
import maya.mel as mel
import os
import sys


def exportFBXFromPreset(nodes, startFrame, endFrame, output):
    """
    Generic export to fbx
    """
    
    SITE_ROOT = os.path.abspath(__file__).split('python310')[0].replace('\\','/')
    exportPreset = SITE_ROOT + '/share/fbxpresets/Export.fbxexportpreset'
    
    if not isinstance(nodes, (list, tuple)):
        nodes = [nodes]
        
    #validate
    for node in nodes:
        if not cmds.objExists(node):
            raise NameError('"{}" does not exist!'.format(node))
            
    #validate output or export will fail if path doesn't exist
    output = output.replace('\\', '/')
    outputDir = os.path.dirname(output)
    try:
        os.makedirs(outputDir)
    except OSError:
        if not os.path.isdir(outputDir):
            raise
            
    try:
        #load preset
        mel.eval('FBXLoadExportPresetFile -f "{}"'.format(exportPreset))
    except IOError:
        raise
    
    #set time range (fbx only exports the range shown in the timeline)
    cmds.playbackOptions(minTime=startFrame)
    cmds.playbackOptions(maxTime=endFrame)
    
    cmds.select(nodes)
    
    sys.__stdout__.write('Exporting {} range {}:{} to {}\n'.format(", ".join([n for n in nodes]), startFrame, endFrame, output))
    sys.__stdout__.flush() #python 3 thing
    
    print('Exporting {} range {}:{} to {}\n'.format(", ".join([n for n in nodes]), startFrame, endFrame, output))
    try:
        mel.eval('FBXExport -f "{}" -s'.format(output))
    except:
        pass

      
def pluginCheck():
    
    unknown = cmds.ls(type='unknown')
    for each in unknown:
        if each.startswith('timeSliderBookmark'):
            cmds.error('This scene has bookmarks but the timesliderBookmark plugin is unloaded.  Please load the plugin, reload the scene, and try again!')
            
            
def setFBXExportOptions(mode = 'anim'):
    """
    sets FBX Export Options.
    """
    
    #reset
    cmds.FBXResetExport()
    
    #GEOMETRY
    #smoothing groups
    cmds.FBXExportSmoothingGroups('-v', True)
    #split per-vertex normals
    cmds.FBXExportHardEdges('-v', False)
    #tangents and binormals
    cmds.FBXExportTangents('-v', False)
    #smooth mesh
    cmds.FBXExportSmoothMesh('-v', False)
    #selection set
    #
    #blind data
    #
    #convert to null objects
    cmds.FBXExportAnimationOnly('-v', False)
    #preserve instances
    cmds.FBXExportInstances('-v', False)
    #reference assets content
    cmds.FBXExportReferencedAssetsContent('-v', True)
    #triangulate
    cmds.FBXExportTriangulate('-v', False)
    
    #ANIMATION
    #use scene name
    cmds.FBXExportUseSceneName('-v', True)
    #remove single key
    #
    #quaternion interpolation mode
    # cmds.FBXExportQuaternion()
    
    if mode == 'anim':
        #bake animation
        cmds.FBXExportBakeComplexAnimation('-v', True)
        #start
        # cmds.FBXExportBakeComplexStart()
        #end
        # cmds.FBXExportBakeComplexEnd()
        #step
        # cmds.FBXExportBakeComplexStep()
        #resample all
        cmds.FBXExportBakeResampleAnimation('-v', True)
    else:
        #bake animation
        cmds.FBXExportBakeComplexAnimation('-v', False)
        #start
        # cmds.FBXExportBakeComplexStart()
        #end
        # cmds.FBXExportBakeComplexEnd()
        #step
        # cmds.FBXExportBakeComplexStep()
        #resample all
        cmds.FBXExportBakeResampleAnimation('-v', False)
    
    #Deformed Models
    #skins
    cmds.FBXExportSkins('-v', True)
    #blend shapes
    cmds.FBXExportShapes('-v', False)
    
    #Blend Shape Options
    #shape attributes
    cmds.FBXExportShapeAttributes('-v', False)
    #attribute values
    # cmds.FBXExportShapeAttributeValues()
    
    #Constant Key Reducer
    #constant key reducer
    cmds.FBXExportApplyConstantKeyReducer('-v', True)
    
    #geometry cache file(s)
    cmds.FBXExportCacheFile('-v', False)
    
    #constraints
    cmds.FBXExportConstraints('-v', False)
    #character definitions
    cmds.FBXExportSkeletonDefinitions('-v', False)
    
    #cameras
    cmds.FBXExportCameras('-v', False)
    #lights
    cmds.FBXExportLights('-v', False)
    #audio
    cmds.FBXExportAudio('-v', False)
    #embed media
    cmds.FBXExportEmbeddedTextures('-v', False)

    #Connections (export selected)
    cmds.FBXExportIncludeChildren('-v', True)
    cmds.FBXExportInputConnections('-v', False)
    
    #FBX File Format
    # cmds.FBXExportFileVersion()
    # cmds.FBXExportInAscii('-v', True)
    

def exportFBX(nodes, layers, startFrame, endFrame, output):
    """
    Generic export to fbx
    """
    
    if not isinstance(nodes, (list, tuple)):
        nodes = [nodes]
        
    #validate
    for node in nodes:
        if not cmds.objExists(node):
            raise NameError('"{}" does not exist!'.format(node))
            
    #validate output or export will fail if path doesn't exist
    output = output.replace('\\', '/')
    outputDir = os.path.dirname(output)
    try:
        os.makedirs(outputDir)
    except OSError:
        if not os.path.isdir(outputDir):
            raise
    
    #set time range (fbx only exports the range shown in the timeline)
    cmds.playbackOptions(minTime=startFrame)
    cmds.playbackOptions(maxTime=endFrame)
    
    cmds.select(nodes)
    
    sys.__stdout__.write('\tExporting {} range {}:{} to {}\n'.format(", ".join([n for n in nodes]), startFrame, endFrame, output))
    sys.__stdout__.flush() #python 3 thing
    print('Exporting {} range {}:{} to {}\n'.format(", ".join([n for n in nodes]), startFrame, endFrame, output))
    
    if layers:
        sys.__stdout__.write('\t\tExporting layers: {}\r\n'.format(', '.join(layers)))
        sys.__stdout__.flush() #python 3 thing
        print('\tExporting layers: {}\n'.format(', '.join(layers)))
        
    try:
        cmds.FBXExport('-f', output, '-s') # -f is file, -s is selected
    except:
        pass


def exportRoot(rootName, animLayers, startFrame, endFrame, output):
    """
    prepares the root chain for export to FBX
    mutes/unmutes animation layers as appropriate
    keywords None, Base and All are valid as layer names
        None and Base will only enable BaseAnimation layer
        All will enable all animation layers    
    """
    
    #validate root
    if not cmds.objExists(rootName):
        raise NameError('"{}" does not exist!'.format(rootName))
    
    #check timeSliderBookmark plugin
    pluginCheck()
    
    #is root a top node in the scene? if not, let's import the reference so we can reparent it to the world
    parents = cmds.listRelatives( rootName, allParents=True )
    if parents:
        #if root is referenced
        if cmds.referenceQuery( rootName, isNodeReferenced=1):
            #if it is in a nested reference (only checks one layer deep)
            parentRef = cmds.referenceQuery( rootName, filename=1, parent=1)
            if parentRef != cmds.file(q=1, sceneName=1):
                #import parent reference
                try:
                    cmds.file(parentRef, importReference=1)
                except Exception as e:
                    errmsg = f"Couldn't import {rootName} from ref! {parentRef} is not top-level reference!"
                    e.args += (errmsg,)
                    raise e
            #find reference file
            refFile = cmds.referenceQuery( rootName, filename=1)
            #import reference
            try:
                cmds.file(refFile, importReference=1)
            except Exception as e:
                errmsg = f"Couldn't import {rootName} in {parentRef} from ref!"
                e.args += (errmsg,)
                raise e
        #parent to world
        cmds.parent(rootName, world=1)
        
    #delete meshes?
    

    #record existing anim layer states
    animLayerStateDict = {}
    baseLayer = cmds.animLayer(query=1, root=1)
    existingAnimLayers = cmds.ls(type='animLayer')
    if len(existingAnimLayers) > 1:
        for layer in existingAnimLayers:
            if layer != baseLayer:  #this layer is always on
                animLayerStateDict[layer] = cmds.animLayer(layer, query=1, mute=1)
                
    exportedLayers = []
    if animLayers:
        #mute all layers 
        for layer in existingAnimLayers:
            if layer != baseLayer:  #this layer is always on
                cmds.animLayer(layer, edit=1, mute=1)
        #handle some keywords
        if 'none' in animLayers or 'None' in animLayers:
            pass  #since all layers are currently muted already
        elif 'base' in animLayers or 'Base' in animLayers:
            pass  #since all layers are currently muted already
        elif 'all' in animLayers or 'All' in animLayers:
            #turn all layers back on
            for layer in existingAnimLayers:
                if layer != baseLayer:  #this layer is always on
                    cmds.animLayer(layer, edit=1, mute=0)
                    exportedLayers.append(layer)
        else:
            #only unmute the specified layers
            for layer in animLayers:
                if cmds.animLayer(layer, query=1, exists=1):
                    cmds.animLayer(layer, edit=1, mute=0)
                    exportedLayers.append(layer)
                else:
                    cmds.warning('Specified animLayer "{}" does not exist.  Skipping'.format(layer))
    else:
        exportedLayers.extend(layer for layer, value in animLayerStateDict.items() if value == 0)  #get all unmuted layers
    
    # if exportedLayers:
        # print('Setting these layers for export: {}, {}'.format(baseLayer, (', '.join(exportedLayers))))
    # else:
        # print('Exporting current scene layers.')
    
    #exportFBX
    exportFBX(rootName, exportedLayers, startFrame, endFrame, output)
    
    #restore layers
    if animLayerStateDict:
        for key, value in animLayerStateDict.items():
            cmds.animLayer(key, edit=1, mute=value)

        
def exportSceneAnim(reload=True, batch=False):
    """
    collects the root bones in the scene and the 
    export parameters from the scene or timeline bookmarks, if any.
    output name is scenedir + Export dir +scenename + __ + root attr (if not already in scenename) + _ + bookmark name
    if keyword None is used as bookmark name, no bookmark name is appended to the exported filename.
    """
    
    if not batch:
        #warn if this seems like a bind file
        rigs = list(set(cmds.ls('Rig', recursive=1)))
        if not len(rigs):
            mode = cmds.confirmDialog(title='Export Animation?',
                          message='Warning: There is no Rig in the scene!',
                          messageAlign='left',
                          icon='question',
                          button=['Export anyway', 'Cancel'],
                          defaultButton='Export anyway',
                          dismissString='Cancel')
            if mode == 'Cancel':
                return
    
    rootBones = list(set(cmds.ls('root', recursive=1)))
    if not rootBones:
        cmds.error('There were no "root" bones found in the scene! Aborting')
    for root in rootBones[:]:
        if cmds.ls('{}.{}'.format(root, 'export')):
            if cmds.getAttr(root+'.export') == 0:
                rootBones.remove(root)
    if not rootBones:
        cmds.error('The "root" bones found in the scene are not set to export! Aborting')
    
    sceneName = cmds.file(query=1, sceneName=1)
    sceneBaseName = os.path.splitext(os.path.basename(sceneName))[0]
    
    #prompt for unsaved changes
    if not cmds.about(batch=True):
        if cmds.dgmodified():
            mode = cmds.confirmDialog(title='Save Scene?',
                                      message='Warning: Unsaved changes will be lost!',
                                      messageAlign='left',
                                      icon='question',
                                      button=['Save and Export', "Export anyway, Lose changes", 'Cancel'],
                                      defaultButton='Save and Export',
                                      dismissString='Cancel')
            if mode == 'Save and Export':
                cmds.file(save=1, force=1)
            elif mode == 'Cancel':
                return
                
    #get default scene start and end
    sceneStartFrame = cmds.playbackOptions(q=1, minTime=1)
    sceneEndFrame = cmds.playbackOptions(q=1, maxTime=1)
    
    #figure out default output
    sceneDir = os.path.dirname(sceneName)
    
    #use export settings?
    if cmds.optionVar(exists='OSE_ArtRoot'):
        artRoot = cmds.optionVar(q='OSE_ArtRoot')
    else:
        artRoot = ""
    if cmds.optionVar(exists='OSE_GameRoot'):
        gameRoot = cmds.optionVar(q='OSE_GameRoot')
    else:
        gameRoot = "" 
    
    if os.path.isdir(artRoot) and os.path.isdir(gameRoot):
        sceneOutputDir = sceneDir.replace(artRoot, gameRoot)+'/'
    else:
        sceneOutputDir = sceneDir + '/Export/'    
    
    rootExportDict = {}
 
    for root in rootBones:
        #get name from attr on root
        if cmds.ls('{}.{}'.format(root, 'exportName')):
            rootExportName = cmds.getAttr(root + '.exportName')
            if rootExportName and rootExportName not in sceneBaseName:
                rootExportDict[root] = rootExportName
            
    exportList = []
    
    #look for timeslider bookmarks
    bookmarks = cmds.ls(type='timeSliderBookmark')
    if bookmarks:
        #sort by start time
        bookmarks.sort(key=lambda bm: cmds.getAttr(bm+'.timeRangeStart'))
        for bookmark in bookmarks:
            startFrame = cmds.getAttr(bookmark+'.timeRangeStart')
            endFrame = cmds.getAttr(bookmark+'.timeRangeStop')
            bmNameString = cmds.getAttr(bookmark+'.name')
            #test name for animLayer markup
            bmName = bmNameString.split('.')[0]  #get only name
            animLayers = bmNameString.split('.')[1:]  #get only animLayer names
            #put it all together
            for root in rootBones:
                if root in rootExportDict:
                    if bmName != "None" and bmName != "":
                        output = sceneOutputDir + sceneBaseName + '__' + rootExportDict[root] + '_' + bmName + '.fbx'
                    else:
                        output = sceneOutputDir + sceneBaseName + '__' + rootExportDict[root] + '.fbx'
                else:
                    if bmName != "None" and bmName != "":
                        output = sceneOutputDir + sceneBaseName + '__' + bmName + '.fbx'
                    else:
                        output = sceneOutputDir + sceneBaseName + '.fbx'
                #check that the output name is not in the list already so we have unique exports
                for each in exportList:
                    if each[4] == output:
                        #TODO: figure out if output already ends in a number and increment
                        output = os.path.splitext(output)[0] + '_1.fbx'
                exportList.append((root, animLayers, startFrame, endFrame, output))
                
    #if there were no bookmarks, our export list is empty.  Let's add the whole timerange.
    #Note: since animLayers are specified in bookmarks, we pass [None] when there aren't any bookmarks in the scene.
    if not exportList:
        for root in rootBones:
            if root in rootExportDict:
                output = sceneOutputDir + sceneBaseName + '__' + rootExportDict[root] + '.fbx'
            else:
                output = sceneOutputDir + sceneBaseName + '.fbx'
            #check that the output name is not in the list already so we have unique exports
            for each in exportList:
                if each[4] == output:
                    #TODO: figure out if output already ends in a number and increment
                    output = os.path.splitext(output)[0] + '_1.fbx'
            exportList.append((root, None, sceneStartFrame, sceneEndFrame, output))
    
    #set fbx export options
    setFBXExportOptions('anim')

    #do the export
    # print('exports', exportList)
    for i, exportListItem in enumerate(exportList):
        if i > 0:
            #only reload the scene if we're exporting a new root
            if exportListItem[0] != exportList[i-1][0]:
                print('loading file')
                cmds.file(sceneName, force=1, open=1)
        try:
            exportRoot(*exportListItem)
        except:
            if not cmds.about(batch=True):
                cmds.confirmDialog(title='Export Errors Detected!',
                                   message='Scene will be reloaded. Please check the Script Editor for details!',
                                   messageAlign='left',
                                   icon='warning',
                                   button='OK',
                                   defaultButton='OK')
                cmds.file(sceneName, force=1, open=1)
            raise
            
    #reload scene
    if reload:
        cmds.file(sceneName, force=1, open=1)
        
    print('Scene Export Done!')
    
    
def exportSceneBind(reload=True, batch=False):
    """
    collects the root bones in the scene and exports the bind to fbx
    
    exportSet: meshA, meshB, meshC, Name
    exportSet: meshB, meshD, None
    """
    
    if not batch:
        #warn if this seems like an animation file
        rigs = list(set(cmds.ls('Rig', recursive=1)))
        if len(rigs):
            mode = cmds.confirmDialog(title='Export Bind?',
                          message='Warning: There is a Rig in the scene! Is this a bind file?',
                          messageAlign='left',
                          icon='question',
                          button=['Export anyway', 'Cancel'],
                          defaultButton='Export anyway',
                          dismissString='Cancel')
            if mode == 'Cancel':
                return
    
    rootBones = list(set(cmds.ls('root', recursive=1)))
    if not rootBones:
        cmds.error('There were no "root" bones found in the scene! Aborting')
    for root in rootBones[:]:
        if cmds.ls('{}.{}'.format(root, 'export')):
            if cmds.getAttr(root+'.export') == 0:
                rootBones.remove(root)
    if not rootBones:
        cmds.error('The "root" bones found in the scene are not set to export! Aborting')
    if len(rootBones) > 1:
        cmds.error('There is more than one "root" bone set to export! Aborting')
    root = rootBones[0]
    
    if not batch:
        if cmds.referenceQuery( root, isNodeReferenced=1):            
            mode = cmds.confirmDialog(title='Export Bind?',
                              message='Warning: The root bone is referenced! Is this a bind file?',
                              messageAlign='left',
                              icon='question',
                              button=['Export anyway', 'Cancel'],
                              defaultButton='Export anyway',
                              dismissString='Cancel')
            if mode == 'Cancel':
                return
    
    sceneName = cmds.file(query=1, sceneName=1)
    sceneBaseName = os.path.splitext(os.path.basename(sceneName))[0]
    
    #prompt for unsaved changes
    if not cmds.about(batch=True):
        if cmds.dgmodified():
            mode = cmds.confirmDialog(title='Save Scene?',
                                      message='Warning: Unsaved changes will be lost!',
                                      messageAlign='left',
                                      icon='question',
                                      button=['Save and Export', "Export anyway, Lose changes", 'Cancel'],
                                      defaultButton='Save and Export',
                                      dismissString='Cancel')
            if mode == 'Save and Export':
                cmds.file(save=1, force=1)
            elif mode == 'Cancel':
                return
                
    #get default scene start and end
    startFrame = cmds.playbackOptions(q=1, minTime=1)
    endFrame = cmds.playbackOptions(q=1, maxTime=1)
    
    #figure out default output
    sceneDir = os.path.dirname(sceneName)
    
    #use export settings?
    if cmds.optionVar(exists='OSE_ArtRoot'):
        artRoot = cmds.optionVar(q='OSE_ArtRoot')
    else:
        artRoot = ""
    if cmds.optionVar(exists='OSE_GameRoot'):
        gameRoot = cmds.optionVar(q='OSE_GameRoot')
    else:
        gameRoot = "" 
    
    if os.path.isdir(artRoot) and os.path.isdir(gameRoot):
        sceneOutputDir = sceneDir.replace(artRoot, gameRoot)+'/'
    else:
        sceneOutputDir = sceneDir + '/Export/'
            
    #get all joints from root down
    #TODO: possibly prune unweighted joint chains from export
    joints = cmds.listRelatives(root, ad=1, type="joint")
    joints.append(root)
    
    #get all meshes bound to root
    skinClusters = list(set(cmds.listConnections(joints, type="skinCluster")))
    meshList = []
    for sc in skinClusters:
        shapes = cmds.skinCluster(sc, q=1,g=1)
        for shape in shapes:
            p = cmds.listRelatives(shape, parent=True)[0]
            if p not in meshList:
                meshList.append(p)
    
    # parent root and meshes to world if not already
    for mesh in meshList:
        print(mesh)
        try:
            cmds.parent(mesh, world=1)
        except:
            pass
    try:
        cmds.parent(root, world=1)
    except:
        pass
    
    #check for mesh/joint name clash
    renamedMeshDict = {}
    for i, mesh in enumerate(meshList):
        if mesh.lower() in joints:
            cmds.warning(f'mesh named {mesh} clashes with a joint name.  Renaming...')
            meshList[i] = cmds.rename(mesh, mesh+'_export')
            #keep track of what got renamed
            renamedMeshDict[mesh] = meshList[i] #use what got assigned in case Maya auto renamed it

    exportList = []
    
    #see if custom mesh exports are specified on the "notes" attr of the root bone
    if cmds.ls('{}.{}'.format(root, 'notes')):
        notes = cmds.getAttr(root + '.notes')
    else:
        notes = ''
        
    if notes:
        for line in notes.splitlines():
            if line.startswith('exportSet:'): #custom mesh export line
                #get all comma separated mesh names from the line
                meshList = line.split(':')[1].split(',')
                if len(meshList) < 2:
                    cmds.error('exportSet needs a suffix or None keyword.  Aborting')
                    #meshList.append('None')  #tested auto adding this, but didn't like it.  better to error.
                #last entry is not a mesh, but the name to append to the export
                exportSuffix = meshList.pop().strip()
                #None or none keywords skips adding a suffix
                if exportSuffix.lower() != 'none':
                    output = sceneOutputDir + sceneBaseName + '__' + exportSuffix + '.fbx'
                else:
                    output = sceneOutputDir + sceneBaseName + '.fbx'
                #update any renamed meshes
                for i, each in enumerate(meshList):
                    if each in renamedMeshDict.keys():
                        meshList[i] = renamedMeshDict[each]
                nodes = [root] + meshList
                exportList.append((nodes, None, startFrame, endFrame, output))
        if not exportList:
            cmds.error(f'Bad notes on {root}.  Aborting')
    else:
        output = sceneOutputDir + sceneBaseName + '.fbx'
        nodes = [root] + meshList
        exportList = [(nodes, None, startFrame, endFrame, output)]
    
    #delete anim curves
    cmds.delete(all=True, channels=True)    
    
    #set fbx export options
    setFBXExportOptions('bind')

    #do the export
    print('exports', exportList)       
    for exportListItem in exportList:
        try:
            exportFBX(*exportListItem)
        except:
            if not cmds.about(batch=True):
                cmds.confirmDialog(title='Export Errors Detected!',
                                   message='Scene will be reloaded. Please check the Script Editor for details!',
                                   messageAlign='left',
                                   icon='warning',
                                   button='OK',
                                   defaultButton='OK')
                cmds.file(sceneName, force=1, open=1)
            raise
            
    #reload scene
    if reload:
        cmds.file(sceneName, force=1, open=1)
        
    print('Scene Export Done!')
           

def batch(mode, rootdir):
    """
    mode is either "anim" or "bind".
    will only export maya files under an "Anim" folder or "Skin" folder (subfolders ok).
    filenames and directories that start with an underscore are ignored.
    directories in ignoredPaths list are ignored.
    """
    import time
    if sys.version_info.major >= 3:
        start = time.perf_counter()
    else:
        start = time.clock()
    
    #validate rootdir directory
    if not os.path.isdir(rootdir):
        raise IOError('{} is not a valid directory!'.format(rootdir))
        
    ignoredPaths = ['incrementalsave', 'wip', 'old', 'ref', 'export']

    if mode == 'anim':
        do = exportSceneAnim
        folder = '\Anim\\'
        #filePrefix = 'an_'  #thinking about constraining animation batch exports to only files that start with an_
    elif mode == 'bind':
        do = exportSceneBind
        folder = '\Skin\\'
        #filePrefix = 'sk_'  #thinking about constraining bind batch exports to only files that start with sk_
    else:
        cmds.error(f'mode {mode} is not supported.  Only "anim" or "bind".  Aborting')
        
    #build a progressWindow
    bar = mel.eval('$tmp = $gMainProgressBar')
    cmds.progressBar(bar, e=1, status = 'Gathering Files... Please Wait')
    
    #collect files to operate on
    batchList = []
    #eliminate any symbolic links
    rootdir = os.path.realpath(rootdir)
    #Normalize a pathname by collapsing redundant separators and up-level references
    #On Windows, it converts forward slashes to backward slashes.
    rootdir = os.path.normpath(rootdir)
    for root, dirs, files in os.walk(rootdir):
        proceed = 1
        #skip ignored folders
        for ignored in ignoredPaths:
            if ignored in root.lower():
                proceed = 0
                break
        #skip subfolders that start with an underscore
        #this way you can still batch a top folder that starts with an underscore
        if proceed:
            if root.split("\\")[-1].startswith("_") and root != rootdir:
                proceed = 0
                break
        #only batch maya files in the appropriate folders, that don't start with an underscore
        if proceed:
            if folder in root+'\\':
                for f in files:
                    if not f.startswith("_"):
                        if f.endswith('.ma') or f.endswith('.mb'):
                            batchList.append(os.path.join(root, f))
                    
    numFiles = len(batchList)
    # print ('numFiles: ', numFiles)
    if not numFiles:
        cmds.warning('No Maya files found to batch!')
        return
    count = 1
    amount = 0
    prevamount = 0
    
    cmds.outputWindow(show=True)
    #sys.__stdout__.write('\nBatching {} files!\n'.format(numFiles))
    sys.__stdout__.write('\nStarted Batch Exporting {} files at {}!\n'.format(numFiles, time.strftime('%H:%M', time.localtime())))
    sys.__stdout__.flush()  #need to do this in 3.7
    print('\nBatching {0} files!\n'.format(numFiles))
    
    #now that we have a list of files to operate on, inject our script
    for f in batchList:
        # Check if the dialog has been cancelled
        if cmds.progressBar(bar, q=1, isCancelled=1):
            print ('Esc pressed.  Batch Aborted!\n')
            sys.__stdout__.write('Esc pressed.  Batch Aborted!\n')
            sys.__stdout__.flush()  #need to do this in 3.7
            cmds.progressBar(bar, e=1, endProgress=1)
            break
        #update progress
        amount = int((float(count) / float(numFiles)) * 100)
        # print ('count: ', count, ' amount: ', amount )
        if amount != prevamount:
            cmds.progressBar(bar, e=1, status = 'Batching: {0} of {1}'.format(count, numFiles))
            cmds.progressBar(bar, e=1, progress = amount)
        if count > 1:
            now = time.perf_counter()
            elapsed = now - start
            total = (elapsed/(count-1) * numFiles)
            eta = total - elapsed
            sys.__stdout__.write('ETA: {}.  (Estimated Total: {} )\n'.format(prettyTime(eta), prettyTime(total)))
            sys.__stdout__.flush()  #need to do this in 3.7
        #inject now
        try:
            print('*--Opening "{0}"--*'.format(f))
            sys.__stdout__.write('\n{0} of {1}\nOpening "{2}"\n'.format(count, numFiles, f))
            sys.__stdout__.flush()  #need to do this in 3.7
            
            cmds.file(f, force=1, open=1)
            #do it!
            do(reload=False, batch=True)
        except IOError as err:
            errno, strerror = err.args
            sys.__stdout__.write('I/O error on {0}({1}): {2}'.format(f, errno, strerror))
            sys.__stdout__.flush()  #need to do this in 3.7
            print('I/O error on {0}({1}): {2}'.format(f, errno, strerror))
        except RuntimeError as strerror:
            sys.__stdout__.write('RuntimeError on {0}: {1}'.format(f, strerror))
            sys.__stdout__.flush()  #need to do this in 3.7
            print('RuntimeError on {0}: {1}'.format(f, strerror))
        except:
            sys.__stdout__.write('Unexpected error on {0}: {1}'.format(f, sys.exc_info()[0]))
            sys.__stdout__.flush()  #need to do this in 3.7
            print('Unexpected error on {0}:'.format(f), sys.exc_info()[0])
            raise
        count += 1
        prevamount = amount
    cmds.progressBar(bar, e=1, endProgress=1)
    
    #file new, to clear out last file loaded
    cmds.file(force=1, new=1)
    
    if sys.version_info.major >= 3:
        end = time.perf_counter()
    else:
        end = time.clock()
    sys.__stdout__.write('Batch Retarget Done at {}!  Total time: {}\n'.format(time.strftime('%H:%M', time.localtime()), prettyTime(end - start)))
    sys.__stdout__.flush()  #need to do this in 3.7
    
    print ('Batch Export Done!')
    print('Total Batch Export Time: {}.'.format(prettyTime(end-start)))
    

def prettyTime(seconds):
    m, s = divmod(seconds, 60)
    h, m = divmod(m, 60)
    d, h = divmod(h, 24)
    output = ''
    if d:
        output += f'{int(d)} day'
        if int(d) > 1:
            output += 's'
    if h:
        output += f' {int(h)} hour'
        if int(h) > 1:
            output += 's'
    if m:
        output += f' {int(m)} minute'
        if int(m) > 1:
            output += 's'
    if s:
        output += f' {int(s)} second'
        if int(s) > 1:
            output += 's'
    
    return output    
    
    
"""
#find all roots
#if referenced, find source file name
#if namespaced, import? clear namespace?
#export timeline
#unless multipart, in which case export from bookmarks
#anim layers?  add list with layer bools?  list of anims that are true only? ignore solo.  how to update?

#add attr to root with suffix.  test output name, don't double up on suffix if already in filename.


the following syntax keeps namespaces but switches the display without them in the name of the objects :
   import maya.cmds as cmds
   myNameSpace = ('myRig:myRigRoot'.split(':'))[0]
   cmds.namespace( set=':')
   cmds.namespace( set=myNameSpace)
   cmds.namespace( rel=True )
and then to restore after export :
   cmds.namespace( set=':')
   cmds.namespace( rel=False ) 
   
   

relativeNamespace(rns)	string	create
This flag can be used with the exportSelected, exportSelectedStrict and exportAll 
operations to specify that the nodes in the exported file are to be written out 
relative to a specified namespace. This provides the ability to remove undesired 
levels of namespacing from the node names as they are exported. The relativeNamespace 
value specifies the namespace to use as the relative root for the exported nodes, and 
must be specified as an absolute namespace. Nodes in the exported file not residing within 
the specified relative namespace will be written out using absolute namespace names. 
Note: this flag cannot be used in conjunction with the preserveReferences flag.

exportSnapshotCallback(esc)	[script, string]	create
When specified alongside -ea/exportAll, es/exportSelected and -ess/exportSelectedStrict, 
this flag enables Maya to temporarily duplicate the export targets into a specified namespace 
and invoke a callback to interact with the duplicate export targets before writing the duplicate 
export targets to disk. Once written to disk, the duplicate export targets, new nodes created by the 
callback and temporary namespace are removed from the scene. Implicitly created nodes (eg. persp, top, etc.) 
are not duplicated. For the duration of the callback: 1. The specified namespace will be made current. 
2. All nodes added by the callback are tracked as a temporary export target. 3. Although the intent of the 
callback is to only operate on the duplicate export targets, there is nothing limiting the callback from 
modifying the main scene. Thus, the callback should be written with care. This flag accepts two arguments: 
a. [string] Callback to invoke prior to write to disk. b. [string] Temporary namespace to store duplicate export 
targets. This flag can only be used in conjunction with -ea/exportAll, -es/exportSelected and -ess/exportSelectedStrict. 
Note that the -pv/preview flag still only previews the contents of the export targets _prior_ to the snapshot duplication. 
It does not preview the final output after the callback. Referenced nodes are duplicated in the same manner as 
duplicating those nodes manually in the scene. Similarly, scene assembly nodes will duplicate as they would if 
manually duplicated, however scene assembly nodes have special duplication behavior, so the callback should be 
aware of these differences when anticipating the duplicate export targets.
"""