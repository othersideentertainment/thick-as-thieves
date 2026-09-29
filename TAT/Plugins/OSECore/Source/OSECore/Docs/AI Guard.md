# Guard Behavior

## Purpose

Behavior for AI guard to run while tasked to guard position.  It should allow the guard to become alerted all the way through to attacking enemies.  It should recover back to the guarding state.  This eventually will need to be fancier and more contextual.  For example guard a door vs guard and object etc.  Right now just going to support guarding a position.

## Design Reference

- https://docs.google.com/document/d/1B-Cy3CxVPV4JHwkFtbhhGBJJHQ_vRet54NbJw8NJP-E/edit


## Requirements

- Position to guard
- Behavior tree implementation of guard behavior
- Goal to handle restoring behavior if something else comes up
- Script API for tasking and configuring guard
- Smart object rules for finding and using nearby smart objects
- Logic for facing desired direction while standing guard
- Idles and fidgets for guard to look alive

## Implementation

- The only way to get the guard goal is to be given it via script command.  For testing purposed this will likely be from the level blue print.  In game this could be any blue print with access to the actor to be tasked.
- The guard behavior will consist of idles with a smart object reasoner to select nearby relevant guard smart objects
- The behavior will support interrupting and restoring the actor to his guard area after using a smart object
- If the guard is alerted or responds to a stim he will leave the guard behavior.  The guard will return to the guard behavior when the guard goal is chosen again.  Upon returning to the guard behavior the guard will move back into his guard position and resume his regularly scheduled guard behaviors
