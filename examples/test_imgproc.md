---
layout: default
title: Image/video processing in BCV
tags: example
---
## {{page.title}} (test_imgproc)

This example demonstrates primitive image processing utilities. The basic features are `sepia`, `hist_eq`, `gamma-adjustment`, `vignette`, `vintage`, `modulate`, and `tint`. These are all very standard image processing utilities.
    
<br>

They can be applied to images or to videos (frame-by-frame). As an example:

    ./test_imgproc -input=../images/commute.mp4 -output=out.mp4 \
     -vignette -vignette_sigma=2 -vintage

yields the following:

<figure>
<a href="{{site.baseurl}}/website-images/imgproc_img1.jpg">
<img src="{{site.baseurl}}/website-images/imgproc_img1.jpg" alt="vintagesque"/>
</a>
<figcaption>Left: original. Right: vintage (still from a video)</figcaption>
</figure>

Of course you can do the standard sepia effect:

    ./test_imgproc -input=../images/commute.mp4 -output=out.mp4 \
     -vignette -vignette_sigma=2 -sepia

<figure>
<a href="{{site.baseurl}}/website-images/imgproc_img2.jpg">
<img src="{{site.baseurl}}/website-images/imgproc_img2.jpg" alt="sepiaesque"/>
</a>
<figcaption>Left: original. Right: sepia-esque </figcaption>
</figure>

`tilt` and `modulation` appear in [ImageMagick](http://www.imagemagick.org/) and are analogous to the versions there. Most other features are so standard that they do not need a visualization.


