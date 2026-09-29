
import sys
import os
import maya.cmds as cmds
from importlib import reload
import Otherside
reload(Otherside)

#get root path with trailing slash
SITE_ROOT = os.path.abspath(__file__).split('python310')[0].replace('\\','/')

class Startup():

    @staticmethod
    def announce():
        print('OTHERSIDE ANIMATION')
        print('\tRunning from: '+SITE_ROOT)
    
    #copied from pymel
    @staticmethod 
    def appendEnv(env, value):
        # type: (str, str) -> None
        """append the value to the environment variable list

        ( separated by ':' on osx and linux and ';' on windows). skips if it
        already exists in the list

        Parameters
        ----------
        env : str
        value : str
        """
        sep = os.path.pathsep
        if env not in os.environ:
            print("\tadding", env, value)
            os.environ[env] = value
        else:
            splitEnv = os.environ[env].split(sep)
            if value not in splitEnv:
                splitEnv.append(value)
                print("\tadding", env, value)
                os.environ[env] = sep.join(splitEnv)        
     
    @staticmethod     
    def setEnvironmentPaths():
        
        #MEL
        melPath = SITE_ROOT + 'share/mel'
        if os.path.isdir(melPath):
            for root, dirs, files in os.walk(melPath):
                subPath = root.replace('\\', '/')
                Startup.appendEnv('MAYA_SCRIPT_PATH', subPath)
        
        #PLUGIN
        mayaVersion = cmds.about(version=True)
        pluginPath = SITE_ROOT + 'share/plugins/' + mayaVersion
        if os.path.isdir(pluginPath):
            for root, dirs, files in os.walk(pluginPath):
                subPath = root.replace('\\', '/')
                Startup.appendEnv('MAYA_PLUG_IN_PATH', subPath)
 
        #ICON
        iconPath = SITE_ROOT + 'share/icons'
        if os.path.isdir(iconPath):
            for root, dirs, files in os.walk(iconPath):
                subPath = root.replace('\\', '/')
                Startup.appendEnv('XBMLANGPATH', subPath)
            
        #SHELF
        shelfPath = SITE_ROOT + 'share/shelves'
        if os.path.isdir(shelfPath):
            for root, dirs, files in os.walk(shelfPath):
                subPath = root.replace('\\', '/')
                Startup.appendEnv('MAYA_SHELF_PATH', subPath)
    
    @staticmethod    
    def setEnvironmentFlags():  
        Startup.appendEnv('MAYA_NO_WARNING_FOR_MISSING_DEFAULT_RENDERER', '1')
        Startup.appendEnv('MAYA_DEBUG_ENABLE_CRASH_REPORTING', '1')

    def __init__(self):
        self.doStartup()
        
    def doStartup(self):
        self.announce()
        self.setEnvironmentPaths()
        self.setEnvironmentFlags()
        Otherside.initialize()
        
Startup()


