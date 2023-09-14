# Post-production effects on volumetric videos

A repository containing code developed as part of the Bachelor thesis at the University of Ljubljana, Faculty of Computer and Information Science by [Enei Sluga](https://github.com/smufa).

The accompanying repository of adapted Volumetric Path Tracer (developed by [terier](https://github.com/terier/vpt), and extended by [JernejCernelc](https://github.com/JernejCernelc/vpt)):
[https://github.com/smufa/vpt]

## Abstract
Volumetric videos are getting closer to practical use, so there is a need for tools that can create and process these videos. As part of the thesis, we developed a program that converts animated 3D Blender scenes into volumetric videos in the BVP format. We also implemented several effects that can be applied to volumes. These effects include background color replacement, blurring, and surface detection. The goal was to render the videos in VPT, but it does not yet support color videos, so we had to combine a sequence rendering implementation with an implementation that allows multiple volumes to be displayed. Finally, we implemented similar effects in VPT as shaders so that we could compare the results.

The thesis is accessible at:
[https://repozitorij.uni-lj.si/Dokument.php?id=174486]


Bibtex:
```
@thesis{Sluga2023,
    title = {Post-production effects on volumetric videos},
    author = {Sluga, Enei},
    year = {2023},
    school = {University of Ljubljana, Faculty of Computer and Information Science},
    type = {Bachelor thesis},
    address = {Ljubljana, Slovenia},
    note = {Mentor: Ciril Bohak, Co-mentor: Žiga Lesar, Language: Slovenian, Slovenian title: Postprodukcijski efekti na volumetričnih videih},
    url = {https://repozitorij.uni-lj.si/Dokument.php?id=174486}
}
```
