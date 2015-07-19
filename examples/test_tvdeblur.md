---
layout: default
title: Blind TV deblurring
tags: example
---
## {{page.title}} (test_tvdeblur)

<figure>
<a href="{{site.url}}/website-images/tvdeblur_mandril_cat.jpg">
<img src="{{site.url}}/website-images/tvdeblur_mandril_cat.jpg" alt="tv deblurring mandril"/>
</a>
<figcaption>Left: original image. Middle: blurred image. Right: deblurring result.</figcaption>
</figure>

This example is about blind total variation deblurring. Given a blurry image ($f$), the goal is to simultaneously find a sharp image ($u$) and the blur kernel ($k$). This can be formulated as

$$
\min_{u,k} \| u * k - f \|_2^2 + \lambda TV(u) \text{ s.t. } k\geq 0, 1^T k = 1
$$

where $*$ is the convolution operator and $TV(u)$ is the total variation penalty:

$$
TV(u) = \sum_{ij}\sqrt{ (u_{i,j+1}-u_{i,j})^2 + (u_{i+1,j}-u_{i,j})^2 }
$$

This problem is nonconvex and is solved by taking gradient steps along $u$ and $k$. The code is a reimplementation of [1] (see also their project webpage).

### Examples

For the synthetically blurred mandril image above, the call is:

    ./test_tvdeblur --lambda=0.1 --input=../images/blur/mandril_blur.png --verbosity=100

and the output is:

    Wrote the result to 'out.png'
    took 717.818 s

The implements convolutions with FFTs, and is not very particularly fast. This is the time it takes to deblur a 256x256 image.
Several additional examples are shown below.

<figure>
<a href="{{site.url}}/website-images/tvdeblur_milan_cat.jpg">
<img class="center" src="{{site.url}}/website-images/tvdeblur_milan_blur.jpg" onmouseover="this.src='{{site.url}}/website-images/tvdeblur_milan_result.jpg'" onmouseout="this.src='{{site.url}}/website-images/tvdeblur_milan_blur.jpg'" border="0" alt=""/>
</a>
<figcaption>Deblurred image appears on mouseover (and the pair is linked). The recovered blur kernel is shown in top left corner. Notice that although most of the image is clearer, errors persist near statues' heads. </figcaption>
</figure>

<figure>
<a href="{{site.url}}/website-images/tvdeblur_fish_cat.jpg">
<img class="center" src="{{site.url}}/website-images/tvdeblur_fish_blur.jpg" onmouseover="this.src='{{site.url}}/website-images/tvdeblur_fish_result.jpg'" onmouseout="this.src='{{site.url}}/website-images/tvdeblur_fish_blur.jpg'" border="0" alt=""/></a>
<figcaption>This example is difficult: ocean floor appears a vague texture. But the algorithm is able to recover some details! Since the fish is moving, its ``blur'' is different from the rest of the image. As a result, some ghosting persists.</figcaption>
</figure>


<figure>
<a href="{{site.url}}/website-images/tvdeblur_notredame_cat.jpg">
<img class="center" src="{{site.url}}/website-images/tvdeblur_notredame_blur.jpg" onmouseover="this.src='{{site.url}}/website-images/tvdeblur_notredame_result.jpg'" onmouseout="this.src='{{site.url}}/website-images/tvdeblur_notredame_blur.jpg'" border="0" alt=""/></a>
<figcaption>This example is simple. Although some ghosting persists, overall the image is clearer.</figcaption>
</figure>



### References
* "Total Variation Blind Deconvolution: The Devil is in the Details", D. Perrone and P. Favaro, CVPR, 2014 (<a href="http://www.cvg.unibe.ch/dperrone/tvdb/">project website</a>)

* T. Chan and C. K. Wong. "Total Variation Blind Deconvolution." Transactions on Image Processing, 1998.
