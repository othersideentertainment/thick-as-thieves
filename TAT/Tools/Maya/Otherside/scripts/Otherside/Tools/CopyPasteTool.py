####################################
# Copy and Paste Keys
# This script will store keyframes on selected objects.
# Maya allows you to paste the keyframes in a separate file given the same objects are present.
# There is an additional button specific for 1P arm joints.
# TODO: Add fields for namespaces and suffixes to avoid the duplicate button.
####################################
import maya.cmds as cmds

objToCopy = []

# function for buttons to run any function and hold as many arguments needed
def buttonWrapper(fn, *args, **kwargs):
    def wrapped(_):
        fn(*args, **kwargs)

    return wrapped

# find the time range based on the INNER time slider controls. Replace minTime/maxTime with ast/aet if needed
def getTimeRange(*args):
    startTime = cmds.playbackOptions(minTime=1, q=1)
    endTime = cmds.playbackOptions(maxTime=1, q=1)

    return (startTime, endTime)

# copy the keys based on your selection, returns True/False for copyKeysAndStoreTimeRange
def copyKeys(startFrame, endFrame):
    selected = cmds.ls(sl=1)

    for i in range(len(objToCopy)) :
        objToCopy.remove(objToCopy[0])

    if len(objToCopy) == 0 :
        if selected :
            for each in selected :
                objToCopy.append(each)
            print 'Copied keys for the following objects:'
            print objToCopy
            cmds.copyKey(t=(startFrame,endFrame), iub=1)
            return True
        else :
            cmds.warning('You have nothing selected.')
            cmds.warning('You must first Select Objects and then press Copy Selection')
            return False

# paste keys from clipboard
# This works across different Maya files! Assumes selection is in the same order as the Copied Keys
def pasteKeys(startFrame, endFrame, str):
    cmds.select(cl=1)
    cmds.playbackOptions(minTime=startFrame, maxTime=endFrame)
    for each in objToCopy:
        if cmds.objExists(each+str) :
            cmds.select(each+str, add=1)
        else :
            cmds.warning('Object not found. Aborted.')
            print 'Missing: ' + each+str

    x = cmds.ls(sl=1)
    print 'Pasting Keys for:'
    print x
    cmds.pasteKey(t=(startFrame,endFrame), o="replace", iub=1)
    print 'Keys pasted successfully.'

# get the time range and store it in the UI, then copy keys
# NOTE: was previously using global values to store time range, however those values might change based on the maya file you have open.
# Storing it in the UI at the time of copy ensures we have updated the time range, and that Maya won't override it upon opening a new file
def copyKeysAndStoreTimeRange(txtField, txtFieldStart, txtFieldEnd):
    timeRange = getTimeRange()
    startTime = timeRange[0]
    endTime = timeRange[1]

    getCopyKeys = copyKeys(startTime, endTime)

    if getCopyKeys != False :
        cmds.textField(txtField, edit=True, text=('Copied Frames: ' + str(int(startTime)) +'-' + str(int(endTime))))
        cmds.textField(txtFieldStart, edit=True, text=startTime)
        cmds.textField(txtFieldEnd, edit=True, text=endTime)
    else :
        cmds.textField(txtField, edit=True, text=('Nothing selected.'))

#get the stored time ranges and paste keys in that range
def pasteKeysInTimeRange(txtFieldStart, txtFieldEnd, str):
    startTime = cmds.textField(txtFieldStart, q=True, text=True)
    endTime = cmds.textField(txtFieldEnd, q=True, text=True)
    pasteKeys(startTime, endTime, str)


def createUI():
    windowName = 'Copy/Paste Tool'
    if cmds.window(windowName, exists=True):
        cmds.deleteUI(windowName, window=True)

    window = cmds.window(windowName, title=windowName, iconName='Copy Keys', widthHeight=(200,200))

    #give the layout a name, needed to reference parent later
    masterLayout = cmds.columnLayout(adjustableColumn=True)

    timeRangeTextField = cmds.textField(editable=False, text='Select objects. Press Copy Selected')

    # new row with columns
    cmds.rowLayout(numberOfColumns=2)
    # elements of each column in the row, used for storing start time range. Hidden from user.
    startTimeTextField = cmds.textField(editable=False, text='0.0', vis=False)
    endTimeTextField = cmds.textField(editable=False, text='1.0', vis=False)

    # new row, set parent of row to masterLayout so we only have 1 column again
    cmds.setParent(masterLayout)
    cmds.button(label='Copy Selected', command=buttonWrapper(copyKeysAndStoreTimeRange, timeRangeTextField, startTimeTextField, endTimeTextField))
    cmds.button(label='Paste', command=buttonWrapper(pasteKeysInTimeRange, startTimeTextField, endTimeTextField, ''))
    cmds.button(label='Paste+1P', command=buttonWrapper(pasteKeysInTimeRange, startTimeTextField, endTimeTextField, '_1p'))
    #cmds.button(label='Test', command=buttonWrapper(copyKeysAndStoreTimeRange, timeRangeTextField, startTimeTextField, endTimeTextField))
    #cmds.button(label='Test2', command=buttonWrapper(pasteKeysFromTimeRange_1p, startTimeTextField, endTimeTextField))
    cmds.text( label='\n1. Open source Maya file.\n2. Select objects.\n3. Press Copy Selected Button.\n4. Open target Maya file. \n5. Press Paste Button.',wordWrap=1)
    cmds.showWindow( window )
