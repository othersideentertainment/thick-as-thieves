import imp
# rigging
import Otherside.Rigging
import Otherside.Rigging.RigUtility
import Otherside.Rigging.Controller
import Otherside.Rigging.RigNode
import Otherside.Rigging.RigPose
import Otherside.Rigging.Marker
import Otherside.Rigging.SpaceSwitch
import Otherside.Rigging.SpaceSwitch2
import Otherside.Rigging.UtilityNodes
import Otherside.Rigging.Vector
import Otherside.Rigging.Retarget
import Otherside.Rigging.CharacterDefinition
# UI
import Otherside.Rigging.UI.Widgets
# control rig
import Otherside.Rigging.ControlRig.ControlRigBase
import Otherside.Rigging.ControlRig.Placement
import Otherside.Rigging.ControlRig.Arm
import Otherside.Rigging.ControlRig.Finger
import Otherside.Rigging.ControlRig.Hand
import Otherside.Rigging.ControlRig.Head
import Otherside.Rigging.ControlRig.Leg
import Otherside.Rigging.ControlRig.HindLeg
import Otherside.Rigging.ControlRig.HindLegSpring
import Otherside.Rigging.ControlRig.Shoulder
import Otherside.Rigging.ControlRig.Torso
import Otherside.Rigging.ControlRig.ToeSet
import Otherside.Rigging.ControlRig.PropChain
import Otherside.Rigging.ControlRig.Prop
import Otherside.Rigging.ControlRig.PropTwoHand
import Otherside.Rigging.ControlRig.Shadow
import Otherside.Rigging.ControlRig.GameIK
import Otherside.Rigging.ControlRig.Camera
import Otherside.Rigging.ControlRig.Character
# tools
import Otherside.Rigging.Tools.AnimTools
import Otherside.Rigging.Tools.TransferAnimation
import Otherside.Rigging.Tools.TransferPose
import Otherside.Rigging.Tools.LinkRootSpace
import Otherside.Rigging.Tools.CharacterLister
import Otherside.Rigging.Tools.CharacterDefinitionImporter
import Otherside.Rigging.Tools.CharacterDefinitionExporter
import Otherside.Rigging.Tools.ConvertUpAxis
# Retargeting
import Otherside.Rigging.Retargeting.RetargetFromFile
import Otherside.Rigging.Retargeting.BatchRetargetFromFile
# debug
import Otherside.Rigging.Debug.RigBuilder
import Otherside.Rigging.Debug.RetargetAnimationEditor
import Otherside.Rigging.Debug.CopyPoseEditor
# setup
import Otherside.Rigging.Setup
import Otherside.Rigging.Setup.PageBase
import Otherside.Rigging.Setup.PageBuildControlRig
import Otherside.Rigging.Setup.PageCharacterDefinition
import Otherside.Rigging.Setup.PageRigPose
import Otherside.Rigging.Setup.PageMarkerLayout
import Otherside.Rigging.Setup.PageSkeletonMap
import Otherside.Rigging.Setup.PageSpaceMap
import Otherside.Rigging.Setup.PageExportCharacterDefinition
import Otherside.Rigging.Setup.SetupWizard
import Otherside.Rigging.Setup.CharacterizeWizard


# rigging
imp.reload(Otherside.Rigging)
imp.reload(Otherside.Rigging.RigUtility)
imp.reload(Otherside.Rigging.Controller)
imp.reload(Otherside.Rigging.RigPose)
imp.reload(Otherside.Rigging.RigNode)
imp.reload(Otherside.Rigging.Marker)
imp.reload(Otherside.Rigging.SpaceSwitch)
imp.reload(Otherside.Rigging.SpaceSwitch2)
imp.reload(Otherside.Rigging.UtilityNodes)
imp.reload(Otherside.Rigging.Vector)
imp.reload(Otherside.Rigging.Retarget)
imp.reload(Otherside.Rigging.CharacterDefinition)
# ui
imp.reload(Otherside.Rigging.UI.Widgets)
# control rig
imp.reload(Otherside.Rigging.ControlRig.ControlRigBase)
imp.reload(Otherside.Rigging.ControlRig.Placement)
imp.reload(Otherside.Rigging.ControlRig.Arm)
imp.reload(Otherside.Rigging.ControlRig.Finger)
imp.reload(Otherside.Rigging.ControlRig.Hand)
imp.reload(Otherside.Rigging.ControlRig.Head)
imp.reload(Otherside.Rigging.ControlRig.Leg)
imp.reload(Otherside.Rigging.ControlRig.HindLeg)
imp.reload(Otherside.Rigging.ControlRig.HindLegSpring)
imp.reload(Otherside.Rigging.ControlRig.Shoulder)
imp.reload(Otherside.Rigging.ControlRig.Torso)
imp.reload(Otherside.Rigging.ControlRig.ToeSet)
imp.reload(Otherside.Rigging.ControlRig.PropChain)
imp.reload(Otherside.Rigging.ControlRig.Prop)
imp.reload(Otherside.Rigging.ControlRig.PropTwoHand)
imp.reload(Otherside.Rigging.ControlRig.Shadow)
imp.reload(Otherside.Rigging.ControlRig.GameIK)
imp.reload(Otherside.Rigging.ControlRig.Camera)
imp.reload(Otherside.Rigging.ControlRig.Character)
# tools
imp.reload(Otherside.Rigging.Tools.AnimTools)
imp.reload(Otherside.Rigging.Tools.ConvertUpAxis)
imp.reload(Otherside.Rigging.Tools.LinkRootSpace)
imp.reload(Otherside.Rigging.Tools.CharacterDefinitionImporter)
imp.reload(Otherside.Rigging.Tools.CharacterDefinitionExporter)
imp.reload(Otherside.Rigging.Tools.CharacterLister)
# Retargeting
imp.reload(Otherside.Rigging.Tools.TransferAnimation)
imp.reload(Otherside.Rigging.Tools.TransferPose)
imp.reload(Otherside.Rigging.Retargeting.RetargetFromFile)
imp.reload(Otherside.Rigging.Retargeting.BatchRetargetFromFile)
# debug
imp.reload(Otherside.Rigging.Debug.RigBuilder)
imp.reload(Otherside.Rigging.Debug.RetargetAnimationEditor)
imp.reload(Otherside.Rigging.Debug.CopyPoseEditor)
# setup
imp.reload(Otherside.Rigging.Setup)
imp.reload(Otherside.Rigging.Setup.PageBase)
imp.reload(Otherside.Rigging.Setup.PageBuildControlRig)
imp.reload(Otherside.Rigging.Setup.PageCharacterDefinition)
imp.reload(Otherside.Rigging.Setup.PageMarkerLayout)
imp.reload(Otherside.Rigging.Setup.PageRigPose)
imp.reload(Otherside.Rigging.Setup.PageSkeletonMap)
imp.reload(Otherside.Rigging.Setup.PageSpaceMap)
imp.reload(Otherside.Rigging.Setup.PageExportCharacterDefinition)
imp.reload(Otherside.Rigging.Setup.SetupWizard)
imp.reload(Otherside.Rigging.Setup.CharacterizeWizard)

print("Otherside.Rigging Rebuild Complete")