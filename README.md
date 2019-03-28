# VAureaNox
Experimental CPU C++ Pathtracer for Signed Distance Fields

Started as an university project for the Computer Graphics course @ "Sapienza University of Rome" held by professor/researcher [***Fabio Pellacini***](https://github.com/xelatihy), i kept working on it , adding and improving features as i kept learning more about rendering and about (signed) distance fields.

It uses [***Yocto/GL***](https://github.com/xelatihy/yocto-gl) library, written by [***Fabio Pellacini***](https://github.com/xelatihy) , for utility functions (3d transforms, mathematic utilities , matrices etc..) 


VAureaNox is structured in 2 parts:

A "Switch" part, responsable for initialization,Pre-render tasks, thread initialization, switching between implemented renderers and selecting/parsing scene (basic external scenes support is done)

A "Renderer" part : the selected renderer processes the scene


Feauteres an Unidirectional-Pathtracer with MIS , NEE , support for rough and delta surfaces, both transmissives and not.

Surfaces categorizeable between Conductors and Dielectrics with dedicated Fresnel function (not using Schlick's)
Microfacet brdf for specular surfaces using GGX

Diffuse model which accounts for energy conservation, that allows for proper ( i hope...) handling of mixed specular / diffuse surfaces , especially when fresnel effect kicks in.

Refractive materials with fixed or variable ior accounting for ray wavelength (keeps track of wl/speed/frequency[at first shoot]/ior) to simulate dispersion. Material parametrizable with a fixed ior or with Sellmeier coefficients (for dispersion)

There is basilar support for homogeneous and scattering media, so far (v 0.07) only uniform phase function.

Materials can be attached with a mutator , which allows to change the evaluated material at runtime , accounting also for normals and object loc/world pos...

<br />
<h4>
  <b>SDFs</b>
  <hr />
</h4>

Features a selection of SDF volumes (prefer calling this way...see below) and operators to combine/deform/displace them.

To assemble scene, utility functions are provided to univocally select items in the scene graph and manipolate them.

Sphere tracing: Uses a non naive approach to sphere tracing , using a self crafted technique (influenced by sphere overrelaxing techniques : [***erleuchtet.org***](http://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf) and [***fractalforums.com***](http://www.fractalforums.com/3d-fractal-generation/enhanced-sphere-tracing-paper/) ) , which both improves rendering speed and precision.
Expecially it allows to trace against non C1 continuous and non lipschitz continuous distance functions reducing artifacts by a lot.
It can trace inside volumes natively.
Naive sphere tracing doesn't have a good time in this cases ( basically : it gets stuck, oversteps, or it returns a null intersection, depending on implementation).
For this purpose i use a double distance system (dist and vdist)

The distance function is calculated just once, the 2 distance values are obtained by making different assumptions on the operators.

For example (shallow explaination) : union operator considers the abs(cur_dist)<abs(other_dist) to determine the nearest object, the abs is to allow internal volume march. For the "vdist" distance , it just considers cur_dist<other_dist , to effectively determine "the object the ray is currently inside". This value needs to be saved along other "v" params in order for the behaviour to cascade to other operators in the scene tree).

<hr />

This scene is made with a large use of twist operator and other "non distance conserving" operators combinations, which makes the distance field not lipschitz continuous.

<table>
  <tr>
    <th>Standard algorithm</th>
    <th>Custom algorithm</th>
  </tr>
  <tr>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_img01.jpg" width="400"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_img00.jpg" width="400"></td>
  </tr>
</table>

<p size="10px" align="center">Iterations debug view</p>

<table>
  <tr>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_img03.jpg" width="400"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_img02.jpg" width="400"></td>
  </tr>
</table>

Notice the amount of artifacts for the standard algorithm , caused by rays "overstepping" the boundary of the volume and then going further inside reaching the maximum number of iterations (at this point are considered as a "miss") , the same happens along the spiral surface: the holes seen on the whole surface are caused by the same phenomena.

Also, the horizon line seems to be "lower" compared to the Custom algorithm render, that's because the standard algorithm reaches the max number of iterations much faster (Custom algorithm uses overrelaxing).

Finally, the custom algorithm runs about 15% faster on average than the standard one.


***TODO*** : both algorithms can march inside volumes natively, by the way custom algorithm will not use the overrelaxing solution if "tracing inside" (determined by initial "vdist"), thus falling back to standard behaviour.

Work is in progress on finding a way to apply the improved behaviour also in this case.

<br />
<h4>
  <b>Post V 0.07 changes</b>
  <hr />
</h4>

Changed system to use doubles instead of floats , this fixed a lot of issues (mostly false/missing intersections) and improved  overall rendering quality.

This scene uses a huge sphere as "floor" (***radius 3000***)

Using ***floats***, precision related artifacts appeared (intersection precision related, not renderer related) 

Using ***doubles***, first artifacts appear with a radius value of more than ***800000000000***
(Of course this depends on the host system) 

<table>
  <tr>
    <th>Floats</th>
    <th>Doubles</th>
  </tr>
  <tr>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_ParsingTest_648x480_PathTracer_spec_spp32.jpg" width="400"></td>
    <td><img src="https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_ParsingTest_648x480_PathTracer_spec_spp32_doubles.jpg" width="400"></td>
  </tr>
</table>

Preformance wise , didn't notice any drop

Other improvements include way less rays lost during emissive precalc (most of the times , 0)

<br />
<h4>
  <b>Images</b>
  <hr />
</h4>

4096 spp , dispersion enabled

![4096spp](https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_cornell_648x480_PathTracer_spec_spp4096.jpg)

8096 spp , dispersion enabled

![8096spp](https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_cornell_648x480_PathTracer_spec_spp8192.jpg)

4096 spp , dispersion enabled, partecipating sorrounding volume

![4096spp](https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VAureaNox_cornell_648x480_PathTracer_spec_spp4096_2.jpg)

When it still was a raytracer (Final image for the course)

![1spp](https://github.com/RisingDaystar/VAureaNox/blob/master/Images/VRising_spider_1920x1080.png)

