<h1><img alt="VAureaNox" src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_LOGO.png" width=420></h1>
Experimental CPU C++ Pathtracer for Signed Distance Fields

Started as an university project for the Computer Graphics course @ "Sapienza University of Rome" held by professor/researcher [***Fabio Pellacini***](https://github.com/xelatihy), i kept working on it , adding and improving features as i kept learning more about rendering and about (signed) distance fields.

It uses [***Yocto/GL***](https://github.com/xelatihy/yocto-gl) library, written by [***Fabio Pellacini***](https://github.com/xelatihy) , for utility functions (3d transforms, mathematic utilities , matrices etc..) 

Relies on ***C++17*** feature set.

VAureaNox is structured in 2 parts:

A "Switch" part, responsable for initialization,Pre-render tasks, thread initialization, switching between implemented renderers and selecting/parsing scene (basic external scenes support is done)

A "Renderer" part : the selected renderer processes the scene

<br />
<h4>
  <b>Features</b>
  <hr />
</h4>
<pre>
Implemented <b>LVCBdpt</b> (Light vertex cache - Bidirectional Pathtracer) with MIS

Framework for other "renderers"
Dedicated configs and scenes file format (.vnxs - Still WIP)

Conductor | Dielectric - Materials
Dedicated fresnel functions (not using Schlick's)
Dielectrics implemented as a dual layer material: Diffuse + (Glossy\[GGX\]/Smooth)

Emissive materials (Blackbody : temperature + "power")
Lambertian Diffuse and variant derived from Ashikhmin for dielectrics
Smooth - Rough Specular (GGX)
Smooth - Glossy Dielectric (GGX)
Transmissive - Refractive Materials (Roughness is still not used, TODO)


Materials with fixed or variable ior (Dispersion) , using Sellmeier Coefficients, 
accounting for ray wavelength (Sensitive values, as wavelength(s), stored by Ray)

Basilar support for Participating media (Homogeneous | Heterogenous), 
using Delta Tracking , accounting for absorption and scattering,
currently only uniform phase function.

Materials can be attached with a mutator (only on the source code side), 
which allows to change the evaluated material at runtime , 
accounting also for normals and object loc/world pos...

</pre>

The enforced floating point precision is ***double*** : Dealing with SDFs is, by nature, very error prone precision wise , using ***float*** (before version 0.0.7) lead to many lingering (yet , "show changing") precision errors . Errors that would grow a lot in non trivial scenes (lots of SDFs, non distance preserving operators , etc...). 

Switching to doubles reduced this kind of errors to an acceptable level while not giving a noticeable slowdown, indeed the increased precision made the whole sphere tracing process a bit faster (more faithfull distance estimation), and reduced false "missing intersection" phenomena.

<br />
<h4>
  <b>SDFs</b>
  <hr />
</h4>

Features a selection of SDF volumes (prefer calling this way...see below) and operators to combine/deform/displace them.
(Many primitives distance functions are taken from Inigo Quilez "https://www.iquilezles.org/" website, an awesome resource for SDF based work)

To assemble scene, utility functions are provided to univocally select items in the scene graph and manipolate them.

Sphere tracing: Uses a non naive approach to sphere tracing , using a self crafted technique (influenced by sphere overrelaxing techniques : [***erleuchtet.org***](http://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf) and [***fractalforums.com***](http://www.fractalforums.com/3d-fractal-generation/enhanced-sphere-tracing-paper/) ) , which both improves rendering speed and precision.
Expecially it allows to trace against non C1 continuous and non lipschitz continuous distance functions reducing artifacts by a lot.
It can trace inside volumes natively.
Naive sphere tracing doesn't have a good time in this cases ( basically : it gets stuck, oversteps, or it returns a null intersection, depending on implementation).
For this purpose i use a double distance system (dist and vdist)

The distance function is calculated just once, the 2 distance values are obtained by making different assumptions on the operators.

For example (shallow explaination) : union operator considers the abs(cur_dist)&lt;abs(other_dist) to determine the nearest object, the abs is to allow internal volume march. For the "vdist" distance , it just considers cur_dist&lt;other_dist , to effectively determine "the object the ray is currently inside". This value needs to be saved along other "v" params in order for the behaviour to cascade to other operators in the scene tree).

<br />
<h4>
  <b>Algorithms comparrison</b>
  <hr />
</h4>

The scene used for test is made with a large use of twist operator and other "non distance conserving" operators combinations, which makes the distance field not lipschitz continuous.

<p>Red : missing intersection with dist&lt;0.0 (inside)</p>
<p>Green : missing intersection with dist&gt;0.0 (outside)</p>
<p>Blue : missing intersection with dist&gt;-ray_tmin && dist&lt;0.0 (algorithms enforce dist&gt;0.0 && dist&lt;ray_tmin to accept intersection if started outside | dist&lt;0.0 && dist&gt;-ray_tmin if started inside)</p>

<i>{settings : ray_tmin = 0.0001; ray_tmax = 10000.0; normal_eps = 0.0001; max_march_iters = 512; ray_samples = 32;}</i>

<table>
  <tr>
    <th>"Naive" algorithm</th>
    <th>Custom "Relaxed" algorithm</th>
    <th>Custom "Enhanced" algorithm</th>
    <th>#</th>
  </tr>
  <tr>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_pt_naive_512mms_32spp_bf1_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_pt_relaxed_512mms_32spp_bf1_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_pt_enhanced_512mms_32spp_bf1_9.png" width="200"></td>
    <td><i>Twist factor : 1.9</i></td>
  </tr>
  <tr>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_iters_naive_512mms_bf1_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_iters_relaxed_512mms_bf1_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_iters_enhanced_512mms_bf1_9.png" width="200"></td>
    <td><i>Iterations View</i></td>
  </tr>
    <tr>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_pt_naive_512mms_32spp_bf9_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_pt_relaxed_512mms_32spp_bf9_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_pt_enhanced_512mms_32spp_bf9_9.png" width="200"></td>
      <td><i>Twist factor : 9.9</i></td>
  </tr>
  <tr>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_iters_naive_512mms_bf9_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_iters_relaxed_512mms_bf9_9.png" width="200"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/intersect_iters_enhanced_512mms_bf9_9.png" width="200"></td>
    <td><i>Iterations View</i></td>
  </tr>
</table>

Notice the amount of artifacts for the naive algorithm , caused by rays "overstepping" the boundary of the volume and then going further inside reaching the maximum number of iterations (at this point are considered as a "miss") , the same happens along the spiral surface: the holes seen on the whole surface are caused by the same phenomena.

Also, the horizon line seems to be "lower" compared to the Custom algorithms render, that's because the naive algorithm reaches the max number of iterations much faster (Custom algorithms use overrelaxing).

Finally, the custom algorithms run about 15% faster on average than the naive one.


***TODO*** : Each algorithm can march inside volumes natively, by the way custom algorithms will not use the overrelaxing solution if "tracing inside" (determined by initial "vdist"), thus falling back to standard behaviour.

Work is in progress on finding a way to apply the improved behaviour also in this case.

<br />
<h4>
  <b>Images</b>
  <hr />
</h4>

<p align="center">
  4096 spp , Spheres filled Cornell box , with rough specularly patterned rear wall
  <img width="648" src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/bdpt_4096spp_spheres2.png">
</p>

<p align="center">
  512 spp , Cornell box with distorted sdf (with mutator and twist)
  <img width="648" src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/Bdpt_512spp_complex.png">
</p>

<p align="center">
  512 spp , Cornell box with diamond
  <img width="648" src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/Bdpt_512spp_diamond.png">
</p>


<p align="center">
  When it still was a raytracer (Final image for the course)
  <img width="648" src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VRising_spider_1920x1080.png">
</p>

