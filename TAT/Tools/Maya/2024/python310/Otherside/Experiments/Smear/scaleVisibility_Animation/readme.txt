scale visibility has some limitations

----Non-Zero Values
Scale cannot become '0' - this will results in errors on animation import.  
Instead we can reduce scale to .001 to make the meshes invisible in a practical sense.

----Subframe Blending
To make the transition seamlessly, step frames are required.  However, step vs interpolated is applied to the entirety of the animation and cannot be relied upon.  
Sub-frame animation blending results in visual artifacts that are noticable and unsuable.
