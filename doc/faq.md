Frequently Asked Questions
========================
This document contains the frequently asked questions to mxnet.


What is the relation between MXNet and CXXNet, Minerva, Purine2
---------------------------------------------------------------
MXNet is created in collaboration by authors from the three projects.
The project reflects what we have learnt from the past projects.
It combines important flavour of the existing projects, being
efficient, flexible and memory efficient.

It also contains new ideas, that allows user to combine different
ways of programming, and write CPU/GPU applications that are more
memory efficient than cxxnet, purine and more flexible than minerva.


What is the Relation to Tensorflow
----------------------------------
Both MXNet and Tensorflow use a computation graph abstraction, which is initially used by Theano, then also adopted by other packages such as CGT, caffe2, purine. Currently TensorFlow adopts an optimized symbolic API. While mxnet supports a more [mixed flavor](https://mxnet.readthedocs.org/en/latest/program_model.html), with a dynamic dependency scheduler to combine symbolic and imperative programming together. 
In short, mxnet is lightweight and “mixed”, with flexiblity from imperative programing, while getting similar advantages by using a computation graph to make it very fast and memory efficient. That being said, most systems will involve and we expect both systems can learn and benefit from each other.


How to Build the Project
------------------------
See [build instruction](build.md)
