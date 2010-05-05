OgreRecast Demo

This demo still has a few bugs, and will still crash on occasion, but in general thats because of my nasty debug code and some silly
things that I did, the recast code, and the integration in to it of Ogre, are fine.

You will need to have CEGUI 0.7.1 and a minimum of Ogre3D 1.7

Controls

W 		- move camera forward
A		- move camrera left
D		- move camera right
S		- move camera backward
R.Mouse - change the camera angle
L.Mouse	- select option on GUI or if a tool is selected and the mouse is over the input geometry the tool will be invoked.

While using the NavMesh Tile tool, Convex Volume tool or the OffMesh Connection tool, if you hold "shift" you can remove connection/volume or tile 
that you have previously created.

While using the NavMesh tester tool, Shift-Left Mouse Button sets the Path Start and Left Mouse Buton sets the Path End.

Other functionality is the same as the recast demo itself, which I will assume you will be familiar with. This code is based heavily
on the Original Recast Demo framework and all I have done is modify it to work with CEGUI and Ogre.