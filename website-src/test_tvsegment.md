## test_tvsegment

This example is about (multi-label) image segmentation, specifically about a convex relaxation of the Potts model. The problem to find a set of regions `R_1`, ..., `R_K` which form a partition of the image domain. This can be formulated as:

<img class="eqn" src="website-images/tvsegment0.png" alt="tvsegment eqn"/>

The first term penalizes the boundary length. The second term is some prior on region assignments.

A common convex relaxation of this problem is:

<img class="eqn" src="website-images/tvsegment1.png" alt="tvsegment eqn"/>

where `u_1`,...,`u_K` are indicators of a particular region, which can be recovered as `R_i = {x: u_i(x) = 1}`. The first term is the weighted TV regularization (relaxation of boundary perimeters). The probability simplex constraint is the relaxation of the requirement that only one region must be "active" at a pixel.

__Unary term__: In the examples below, the unary term is the Euclidean distance between the image intensity at a pixel and a prototype image intensity `c_l` (obtained by kmeans)

<img class="eqn" src="website-images/tvsegment3.png" alt="tvsegment unary"/>

In practice one should use something more sophisticated, e.g. using local statistics.

__TV weighing__: The edges are weighed by image similarity as follows:

<img class="eqn" src="website-images/tvsegment2.png" alt="tvsegment weights"/>

This is customary. The parameter `beta` can be used to control weights' range. Also, as customary `beta` is pre-multiplied by the average intensity difference estimated over the entire image.

__Optimization__: The problem is solved using first-order primal-dual algorithm (see references). It is highly parallelizable, but the current implementation does not attempt to do that.

### Examples

A sample call:

    ./test_tvsegment --input images/valley.jpg --output out.png --max-iters 100 --beta 20 --lambda .1 --num-clusters 5 --isotropic 1

Output:

    kmeans: n pts: 307200, dim: 3, K: 5
    learned cluster centers:
    K = 0: 0.994683 0.967045 0.920650 
    K = 1: 0.184025 0.233376 0.295402 
    K = 2: 0.847665 0.576018 0.339771 
    K = 3: 0.941981 0.763595 0.604728 
    K = 4: 0.335889 0.397112 0.460750 
    lambda: 0.100000
    beta: 20.000000
    num-clusters: 5
    isotropic: 1
    max-iters: 100
    image size: 480x640x3
    TV segmentation took: 7117.339111 ms
    Wrote the result to 'out.png'

Below are results for segmentation with varying number of clusters, and varying TV regularization penalty.
<figure>
<a href="website-images/tvsegment_variedK.jpg">
<img src="website-images/tvsegment_variedK.jpg" alt="tv segmentation, varied number of clusters"/>
</a>
<figcaption>Segmentation with varying number of clusters. Left: input image. Right: segmentation with `K = 5, 10, 20`</figcaption>
</figure>

<figure>
<a href="website-images/tvsegment_variedlambda.jpg">
<img src="website-images/tvsegment_variedlambda.jpg" alt="tv segmentation, varied TV regularization"/>
</a>
<figcaption>Segmentation with 3 clusters and varying TV regularization.. Left: input image. Right: segmentation with `lambda = 0.01, 0.1, 0.5`. Note that as `lambda -> 0`, segmentation becomes essentially k-means clustering on the image intensities.</figcaption>
</figure>

<figure>
<a href="website-images/tvsegment_ex3.jpg">
<img src="website-images/tvsegment_ex3.jpg" alt="tv segmentation"/>
</a>
<figcaption>Top: original image. Bottom: segmentation with `K=5`</figcaption>
</figure>

### References
* Antonin Chambolle, Thomas Pock, "A first-order primal-dual algorithm for convex problems with applications to imaging", Journal of Mathematical Imaging and Vision, 2011.

* Thomas Pock, Antonin Chambolle, Daniel Cremers, Horst Bischof, "A convex relaxation approach for computing minimal partitions", CVPR 2009.

