---
layout: default
title: TV denoising
tags: example
---
## {{page.title}} (test_tvdn)

This example is about total-variation denoising. Given a noisy image $u$, denoising is formulated as a convex optimization problem:

$$
\min_{x} \|x-u\|_2^2 + \lambda TV(x)
$$

where $x$ is the "clean"/denoised image. The first term ensures that the resulting image is "close" to the original $u$. The second term -- total variation -- is weighted by a penalty $\lambda$ and is used to penalize pixelwise image differences. It can be written as (anisotropic version):

$$
TV(x) = \sum_{i,j} |x_{i+1,j}-x_{i,j}|+|x_{i,j+1}-x_{i,j}|
$$

or as (isotropic version):

$$
TV(x) = \sum_{i,j} \sqrt{ |x_{i+1,j}-x_{i,j}|^2+|x_{i,j+1}-x_{i,j}|^2 }
$$

This optimization problem is convex. Here it is solved using first-order primal-dual algorithm (see references). The problem is fairly parallelizable and some parallelization is done using SSE instructions.

### Examples

A sample call:

    ./test_tvdn -input=../images/valley.jpg -lambda=5 -max_iters=500 -dx_tolerance=1e-3

Output:

    Loaded from '../images/valley.jpg'
    ...
    Wrote the result to 'out.png'
    took 322.863037 ms

Below are results for segmentation with varying $\lambda$ (TV regularization penalty).
<figure>
<a href="{{site.url}}/website-images/tvdn_variedlambda.jpg">
<img src="{{site.url}}/website-images/tvdn_variedlambda.jpg" alt="tv denoising"/>
</a>
<figcaption>Left to right: input image, $\lambda=5$, $\lambda=20$ </figcaption>
</figure>

The next example shows the effect of isotropic and anisotropic penalties (with a very large $\lambda$):

<figure>
<a href="{{site.url}}/website-images/tvdn_isoaniso.jpg">
<img src="{{site.url}}/website-images/tvdn_isoaniso.jpg" alt="tv denoising"/>
</a>
<figcaption>Left: anisotropic. Right: isotropic</figcaption>
</figure>
Anisotropic is "grid-aligned", while isotropic is not. For this reason, it typically looks a bit better. On the other hand, anisotropic penalty is usually a bit faster to compute. 

The images above are not really noisy; the algorithm was applied only to make them more stylish. Below is an actual denoising example (of huge robots).

<figure>
<a href="{{site.url}}/website-images/tvdn_noisy.jpg">
<img src="{{site.url}}/website-images/tvdn_noisy.jpg" alt="tv denoising"/>
</a>
<figcaption>Left: original. Right: denoised with $\lambda=4$</figcaption>
</figure>


### References
* Antonin Chambolle, Thomas Pock, "A first-order primal-dual algorithm for convex problems with applications to imaging", Journal of Mathematical Imaging and Vision, 2011.

* Leonid Rudin, Stanley Osher, and Emad Fatemi, "Nonlinear total variation based noise removal algorithms." Physica D: Nonlinear Phenomena, 1992.

