import sys
import os
import importlib
import time
import maya.cmds as cmds


class PymelInstall():

    SITE_ROOT = os.path.abspath(__file__).split('python310')[0].replace('\\','/')

    pymelVersionDict =  {
                        '2024':'1.4.0',
                        '2023':'1.3.1'
                        }
    
    def  __init__(self):
        self.update()
        # if self.pymelCheck() is False:
            # self.installPymel()
    
    def update(self):
        self.currentMayaVersion = self.getCurrentMayaVersion()
        self.targetPymelVersion = self.getTargetPymelVersion(self.currentMayaVersion)
        self.currentPymelVersion = self.getCurrentPymelVersion()
        self.currentPythonVersion = self.getCurrentPythonVersion()
        self.currentMayapyExe = self.getCurrentMayapyExe()
        self.installPath = self.SITE_ROOT + 'install/'
        self.logPath = self.SITE_ROOT + 'share/logs/'
    
    def getCurrentMayaVersion(self):
        return cmds.about(version=True)
    
    def getTargetPymelVersion(self, mayaVersion):
        return self.pymelVersionDict[mayaVersion.split('.')[0]]
      
    def getCurrentPymelVersion(self):
        
        try:
            return importlib.metadata.version('pymel')
        except:
            return None        
 
    def getCurrentPythonVersion(self):
        return sys.version.split('(')[0]
         
    def getCurrentMayapyExe(self):
        """
        returns full path to mayapy.exe with forward slashes
        """
        mayaExe = sys.executable
        mayapyExe = mayaExe.replace('maya.exe', 'mayapy.exe').replace('\\','/')
        if os.path.isfile(mayapyExe):
            return mayapyExe
                    
    def hasPymel(self):
        """
        returns True if any version of pymel is installed
        """
        try:
            import pymel
            return True
        except ImportError:
            return False     

    def hasTargetPymel(self):
        """
        returns True if the version of pymel indicated in pymelVersionDict is installed
        """
        try:
            return self.currentPymelVersion == self.targetPymelVersion
        except:         
            return False
        
         
    def pymelCheck(self, verbose=True):
        """
        returns True only if the installed version of Pymel matches the 
        version given in pymelVersionDict
        """
        self.update()
        if verbose:
            print('Checking for Pymel...')
        if self.hasPymel() is True:
            currentPymel = self.currentPymelVersion
            targetPymel = self.targetPymelVersion
            if currentPymel is not None:
                if verbose:
                    print(f'\tFound Pymel version {currentPymel}')
                if currentPymel == targetPymel:
                    if verbose:
                        print('\tPymel is current!')
                    return True
        
        if verbose:        
            print('\tPymel is not current!')
        return False
                

    def writeBatFile(self):
        installpath = self.installPath
        logPath = self.logPath
        mayapy = self.currentMayapyExe
        bat = self.installPath + 'pymelInstall.bat'
        log = logPath + 'pymelInstall.log'
        logbs = log.replace('/','\\')
        temp = logPath + 'pymelInstall.temp'
        arg = f'pymel=={self.targetPymelVersion}'
        pymel = mayapy.replace('bin/mayapy.exe', 'Python/Lib/site-packages/pymel/__init__.py')
        
        contents = '@echo OFF\n'
        contents+= 'echo Installing Pymel, please wait...\n'
        
        contents+= f'cd {mayapy.replace("/mayapy.exe","")}\n'
        contents+= f'mayapy -m pip install {arg}  1> {log} 2>&1 \n'  #redirect output
        contents+= f'type "{logbs}" \n'  # print output back to screen (this needed backslashes in the path to work)
        contents+= f'echo this is a temp file and can be deleted > {temp} \n'
        
        contents+= f'if exist "{pymel}" (\n'
        contents+= 'echo.\n'
        contents+= 'echo | set /p=[92mSuccess^^! Pymel was installed^^![0m\n'
        contents+= 'echo.\n'
        contents+= ') else (\n'
        contents+= 'echo.\n'
        contents+= 'echo | set /p=[91mError: Pymel was NOT installed successfully^^![0m\n'
        contents+= 'echo.\n'
        contents+= ')\n'
        contents+= 'pause\n'
        contents+= 'exit'
            
        if not os.path.isdir(logPath):
            os.mkdir(logPath)
            
        try:
            with open(bat, 'w') as f:
                f.write(contents)
                return bat
        except:
            raise
            
    
    def installPymel(self):
        """
        for maya versions 2023 and above
        launches a cmd shell and requests elevated privileges,
        then runs a .bat file to install pymel 
        uses command "mayapy -m pip install pymel==(version)"        
        """
        import ctypes
        import subprocess
        
        if self.currentMayaVersion < '2023':
            cmds.confirmDialog(title='Install Pymel?', 
                            message='For Maya2022 and earlier, please install pymel from the Maya installer!', 
                            button='OK')
            return
        
        msg = ''
        proceed = False
        targetPymel = self.targetPymelVersion
        if self.hasPymel() is True:
            #there is a version of pymel already
            currentPymel = self.currentPymelVersion
            if currentPymel is not None:
                if currentPymel == targetPymel:
                    cmds.error('Pymel is already installed!')
                elif currentPymel < targetPymel:
                    msg = f'Your Pymel is version {currentPymel} and needs to be updated to version {targetPymel}!'
                elif currentPymel > targetPymel:
                    msg = f'Your Pymel is version {currentPymel} and is ahead of the required version {targetPymel}!'
                
            cd = cmds.confirmDialog(title='Install Pymel?', message=msg, button=['Install', 'Cancel'],
                                defaultButton='Install', dismissString='Cancel')
            if cd == 'Install':
                proceed = True
        else:
            #no pymel installed, so let's install it
            proceed = True

        if proceed:        
            print ('Installing Pymel...')

            installpath = self.installPath
            logPath = self.logPath
            bat = self.installPath + 'pymelInstall.bat'
            #log = self.logPath + 'pymelInstall.log'
            temp = self.logPath + 'pymelInstall.temp'
            pymel = self.currentMayapyExe.replace('bin/mayapy.exe', 'Python/Lib/site-packages/pymel/__init__.py')
            
            bat = self.writeBatFile()
            if not os.path.isfile(bat):
                raise FileNotFoundError
            
            if os.path.isfile(temp):
                os.remove(temp)
            
            # contents = f'cd {mayapy.replace("/mayapy.exe","")}\n'
            # contents+= f'mayapy -m pip install {arg} 1> {log} 2>&1'                        
                
            #command = f'/k "{mayapy}" -m pip install {arg} > "{filepath}/pymelInstall.txt"'
            
            
            
            command = f'/k "{bat}"'
            #print (command)
            
            # run cmd shell but ask for elevated privileges
            ret = ctypes.windll.shell32.ShellExecuteW(
                    None,  #handle to parent window
                    u"runas",  #verb
                    u"cmd.exe",  #file on which verb acts
                    command,  #parameters
                    None,  #working directory (default is cwd)
                    1  #show window normally
                )
                
            # this attempt installed to user folder, not maya folder
            # ret = subprocess.run(['mayapy', '-m', 'pip', 'install', 'pymel'])
            # print(ret)
            
            # hack bc asking for elevated permissions means losing the ability to parse return info
            # instead, we write a temp file when install is done.  We check for this new file before proceeding.
            # to prevent locking up, we abort after waiting 30 seconds and assume it failed.
            elapsed=0
            while not os.path.isfile(temp):
                time.sleep(1)
                elapsed+=1
                if elapsed > 30:
                    cmds.confirmDialog(title='Install Pymel?', 
                                        message='Pymel did not install correctly, please see Script Editor for details!', 
                                        button='OK')
                    if os.path.isfile(temp):
                        os.remove(temp)
                    return
            if ret:
                self.update()
                if self.hasTargetPymel():
                    cmds.confirmDialog(title='Install Pymel?', 
                                    message='Pymel installed successfully!',
                                    button='OK')
                    if os.path.isfile(temp):
                        os.remove(temp)
                    import Otherside.Pipe.Startup as Startup
                    importlib.reload(Startup)
                else:
                    cmds.confirmDialog(title='Install Pymel?', 
                                    message='Pymel did not install correctly, please see Script Editor for details!', 
                                    button='OK')

    def uninstall(self):
        import ctypes
                
        mayapy = self.currentMayapyExe
        command = f'/k "\"{mayapy}\" -m pip uninstall pymel "'      
        
        #print (command)
        
        # run cmd shell but ask for elevated privileges
        ret = ctypes.windll.shell32.ShellExecuteW(
                None,  #handle to parent window
                u"runas",  #verb
                u"cmd.exe",  #file on which verb acts
                command,  #parameters
                None,  #working directory (default is cwd)
                1  #show window normally
            )
        
        print ('Pymel Uninstalled!  Restart Maya or run this in the Script Editor:')
        print ('import importlib')
        print ('import Otherside.Pipe.Startup as Startup')
        print ('importlib.reload(Startup)')


def uninstall():
    PymelInstall().uninstall()