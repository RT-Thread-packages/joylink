from building import *
import rtconfig
import os

src  = []
LIBS = ['']
LIBPATH = ['']
cwd  = GetCurrentDir()

lib_path = os.path.join(cwd, 'libs')
LIBPATH += [lib_path]

if rtconfig.CROSS_TOOL == 'gcc':
    LIBS += ['libjoylink_2.0.19.1_armcm3_gcc']
elif rtconfig.CROSS_TOOL == 'keil':
    LIBS += ['libjoylink_2.0.19.1_armcm3_keil']

path  = [cwd + '/inc']
path += [cwd + '/inc/joylink']
path += [cwd + '/inc/auth']
path += [cwd + '/inc/json']
path += [cwd + '/inc/list']
path += [cwd + '/inc/config']
path += [cwd + '/inc/softap']

# sample files
if GetDepend(['JOYLINK_USING_SAMPLES_BAND']):
    src += Glob('samples/joylink_sample_band.c')

if GetDepend(['JOYLINK_USING_SAMPLES_HEATER']):
    src += Glob('samples/joylink_sample_heater.c')

CPPDEFINES = ['__RT_THREAD__']
CPPDEFINES += ['_GET_HOST_BY_NAME_']

if GetDepend(['JOYLINK_USING_SOFTAP']):
    CPPDEFINES += ['_IS_DEV_REQUEST_ACTIVE_SUPPORTED_']

group = DefineGroup('joylink', src, depend = ['PKG_USING_JOYLINK'], CPPPATH = path, LIBS = LIBS, LIBPATH = LIBPATH, CPPDEFINES = CPPDEFINES)

list = os.listdir(cwd)
for item in list:
    if os.path.isfile(os.path.join(cwd, item, 'SConscript')):
        group = group + SConscript(os.path.join(item, 'SConscript'))

Return('group')
