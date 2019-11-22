Jizhong Wang
CSCI-420
Assignment-2
Nov, 1, 2019

To run:
Inside "assign2" folder, type "make" into the terminal command line. Then type "./assign2 track.txt" to see the output. Open "track.txt" file to modify which spline file to use. All the files could be found in the folder "splines".

I applied skybox texture of a furture theme to the cubic background. In the implementation, the change of u in each step is set to 0.001.I figured out the camera movement by first computing tangent for each coordinate in the step with the formula, then compute all the normals and binormals with N0 calculated from an arbitrary vector. I set the camera position with an offset to the spline position to mimic the roller coaster. 

Extras:
I have implemented double rails.
I have placed master yoda somewhere in the scene. Go find it out!


