---
layout: default
title: SLIC superpixels
tags: example
---
## {{page.title}} (test_slic)

This example uses SLIC to produce a segmentation, and writes a few results. Here is a sample call:

    ./test_slic -input=../images/kings.jpg -K=3000 -M=5 -num_iters=5 -max_levels=2

and the output:

    Took 286.743000 ms to superpixellize
    Writing visualization of segmentation to out.png
    Writing averaged image to out_avg.png

This generates a segmentation with 3000 superpixels, a geometric weight of 5, in 5 iterations. It performs optimization at two levels, and downsamples each level by a factor of 2.

<figure>
<a href="{{site.url}}/website-images/slic_example.jpg"><img src="{{site.url}}/website-images/slic_example.jpg" alt="slic example"/></a>
<figcaption>
Left: input image. Middle: superpixel boundaries. Right: visualization using the mean color for each region. 
</figcaption>
</figure>


### Notes

* SLIC is essentially k-means on LAB+xy components. Optimization in LAB space is preferred, but doing integer kmeans in RGB space is faster (saves a few hundred milliseconds). This is what we do here. The implementation assumes the input image is RGB and its components are of type `uint8`.

* To produce a reasonable segmentation, one needs at least 5 iterations; ~10 is preferrable. For large images, this is somewhat expensive (~40 ms per iteration for a 640x480 image). These iterations are needed to reliably estimate the center of a superpixel in RGB-xy space. This quantity can be estimated much faster using a downsampled image (since downsampling, when done correctly, is essentially averaging). To leverage on this, our implementation performs a multi-scale SLIC segmentation on an image pyramid. This is done *only* to improve speed; results are actually a bit worse. 
 
### References

* Radhakrishna Achanta, Appu Shaji, Kevin Smith, Aurelien Lucchi, Pascal Fua, and Sabine Susstrunk, "SLIC Superpixels Compared to State-of-the-art Superpixel Methods", IEEE Transactions on Pattern Analysis and Machine Intelligence, 2012.

* Radhakrishna Achanta, Appu Shaji, Kevin Smith, Aurelien Lucchi, Pascal Fua, and Sabine Susstrunk, "SLIC Superpixels", EPFL Technical Report, 2010.
