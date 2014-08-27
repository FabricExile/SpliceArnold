Fabric Splice for SolidAngle Arnold
===================================
A Fabric Splice integration for Arnold.

Fabric Splice for Arnold allows you to make use of the Fabric Core inside of Arnold and use KL to perform computations inside of Arnold using a custom node.

Repository Status
=================

This Splice interation is not officially maintained by Fabric Software. 

The last known version of Arnold compatible is 4.0.16.1.

The last known version of Fabric Splice compatible is 1.13.0.

Supported platforms
===================

To date all three major platforms (windows, linux, osx) are supported, if you build the thirdparty dependencies for the corresponding platform.

Building
========

A scons (http://www.scons.org/) build script is provided. Fabric Splice for Arnold depends on
* A static build of boost (http://www.boost.org/), version 1.55 or higher.
* A dynamic build of Fabric Core (matching the latest version).
* The SpliceAPI repository checked out one level above (http://github.com/fabric-engine/SpliceAPI)

Fabric Splice for Arnold requires a certain folder structure to build properly. You will need to have the SpliceAPI cloned as well on your drive, as such:

    SpliceAPI
    Applications/SpliceArnold

You can use the bash script below to clone the repositories accordingly:

    git clone git@github.com:fabric-engine/SpliceAPI.git
    mkdir Applications
    cd Applications
    git clone git@github.com:fabric-engine/SpliceArnold.git
    cd SpliceArnold
    scons

To inform scons where to find the Fabric Core includes as well as the thirdparty libraries, you need to set the following environment variables:

* FABRIC_BUILD_OS: Should be the type of OS you are building for (Windows, Darwin, Linux)
* FABRIC_BUILD_ARCH: The architecture you are building for (x86, x86_64)
* FABRIC_BUILD_TYPE: The optimization type (Release, Debug)
* FABRIC_SPLICE_VERSION: Refers to the version you want to build. Typically the name of the branch (for example 1.13.0)
* FABRIC_CAPI_DIR: Should point to Fabric Engine's Core folder.
* BOOST_DIR: Should point to the boost root folder (containing boost/ (includes) and lib/ for the static libraries).
* ARNOLD_INCLUDE_DIR: The include folder of the SolidAngle Arnold API. (for example: C:\ThirdParty\Arnold\4.0.16.1\include)
* ARNOLD_LIB_DIR: The main folder of the SolidAngle Arnold API. (for example: C:\ThirdParty\Arnold\4.0.16.1\lib)
* ARNOLD_VERSION: The Arnold version to use. (for example: 4.0.16.1)

The temporary files will be built into the *.build* folder, while the structured output files will be placed in the *.stage* folder.

To perform a build you can just run

    scons all -j8

To clean the build you can run

    scons clean

License
==========

The license used for this DCC integration can be found in the root folder of this repository.
