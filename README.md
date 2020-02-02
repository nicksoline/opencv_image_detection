# opencv_image_detection

The algorithm of abandoned objects detection exploits the fact that static objects on current frame apparent 
on the original clear scene. This difference is seen on the top-left frame of the output picture. 

To get the picture algorithm takes the first frame from the input video as a reference frame and 
subtract from it the current frame of the video during whole processing pixelwise (cv::subtract()). 

As a result we get a negative image of new objects on the picture relative to the reference frame. 
The specifically preprocessed (see 3rd paragraph) picture of all new objects acts as a source of the 
following subtraction.

The left-bottom frame of the output picture is represented an output of the Gaussian mixture model 
background subtraction applied to preprocessed (see 3rd paragraph) reference and current frame sequently 
to make more differences. The model is implemented by the class cv::BackgroundSubtractorMOG2 [1, 2]. 
This step is needed to detect all moving objects on the picture regardless of its nature: while they 
are moving they are be detected.

Regarding preproccessing the pictures. Both of them (the picture to be detected all moving objects and 
the picture to be detected all new objects) are blurred with Gaussian filter (cv::GaussianBlur()) to
 eliminate an artefacts of the pictures. Then both are masked with a threshold to get sharp difference 
between pixels of interest and others. Thus we get two binary pictures containing the masks moving and 
noew objects on current frame. Now we can subtract one from the other and get new non-moving objects only. 
It can be seen on the right-bottom frame.

The last step if finding the contour of the non-moving object by function cv::findContours() [3, 4]. 
After the finding we have to be sure that the object is non-moving. To make this we compare an offset 
of the contour with some threshold and if the offset is less of it we futhermore draw the boundary box 
around the static new object on the input picture - top-right frame.




1) Z.Zivkovic, Improved adaptive Gausian mixture model for background subtraction, International Conference Pattern Recognition, 
UK, August, 2004, http://www.zoranz.net/Publications/zivkovic2004ICPR.pdf. The code is very fast and performs also shadow detection. 
Number of Gausssian components is adapted per pixel.
2) Z.Zivkovic, F. van der Heijden, Efficient Adaptive Density Estimapion per Image Pixel for the Task of Background Subtraction, 
Pattern Recognition Letters, vol. 27, no. 7, pages 773-780, 2006. The algorithm similar to the standard Stauffer&Grimson 
algorithm with additional selection of the number of the Gaussian components based on: Z.Zivkovic, F.van der Heijden, R
ecursive unsupervised learning of finite mixture models, IEEE Trans. on Pattern Analysis and Machine Intelligence, 
vol.26, no.5, pages 651-656, 2004.
3) Suzuki, S. and Abe, K., Topological Structural Analysis of Digitized Binary Images by Border Following. CVGIP 30 1, pp 32-46 (1985)
4) Teh, C.H. and Chin, R.T., On the Detection of Dominant Points on Digital Curve. PAMI 11 8, pp 859-872 (1989)
