#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import('parentEnv', 'FABRIC_CAPI_DIR', 'FABRIC_SPLICE_VERSION', 'STAGE_DIR', 'FABRIC_BUILD_OS', 'FABRIC_BUILD_TYPE', 'ARNOLD_INCLUDE_DIR', 'ARNOLD_LIB_DIR','ARNOLD_VERSION', 'sharedCapiFlags', 'spliceFlags')

env = parentEnv.Clone()

arnoldFlags = {
  'CPPPATH': [
      ARNOLD_INCLUDE_DIR
    ],
  'LIBPATH': [
    ARNOLD_LIB_DIR,
  ],
  'LIBS': [
    'ai'
  ]
}

if FABRIC_BUILD_OS == 'Windows':
  arnoldFlags['CCFLAGS'] = ['/DNT_PLUGIN']
elif FABRIC_BUILD_OS == 'Linux':
  arnoldFlags['CCFLAGS'] = ['-DLINUX']

env.MergeFlags(arnoldFlags)
env.Append(CPPDEFINES = ["_SPLICE_ARNOLD_VERSION="+str(ARNOLD_VERSION[:4])])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

target = 'FabricSpliceArnold' + ARNOLD_VERSION

if FABRIC_BUILD_OS == 'Windows':
  target += '.dll'
elif FABRIC_BUILD_OS == 'Linux':
  target += '.so'
else:
  target += '.dylib'

arnoldModule = env.SharedLibrary(target = target, source = Glob('*.cpp'))

arnoldFiles = []

installedModule = env.Install(STAGE_DIR, arnoldModule)
arnoldFiles.append(installedModule[0])

arnoldFiles.append(env.Install(STAGE_DIR, env.File('license.txt')))

# also install the FabricCore dynamic library
arnoldFiles.append(env.Install(STAGE_DIR, env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.so'))))
arnoldFiles.append(env.Install(STAGE_DIR, env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dylib'))))
arnoldFiles.append(env.Install(STAGE_DIR, env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dll'))))

# install PDB files on windows
if FABRIC_BUILD_TYPE == 'Debug' and FABRIC_BUILD_OS == 'Windows':
  env['CCPDBFLAGS']  = ['${(PDB and "/Fd%s_incremental.pdb /Zi" % File(PDB)) or ""}']
  pdbSource = arnoldModule[0].get_abspath().rpartition('.')[0]+".pdb"
  pdbTarget = os.path.join(STAGE_DIR.abspath, os.path.split(pdbSource)[1])
  copyPdb = env.Command( 'copy', None, 'copy "%s" "%s" /Y' % (pdbSource, pdbTarget) )
  env.Depends( copyPdb, installedModule )
  env.AlwaysBuild(copyPdb)

alias = env.Alias('splicearnold', arnoldFiles)
spliceData = (alias, arnoldFiles)
Return('spliceData')
