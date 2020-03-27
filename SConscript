from building import *

cwd  = GetCurrentDir()

path  = [cwd + '/joylink']
path += [cwd + '/joylink/joylink']
path += [cwd + '/joylink/auth']
path += [cwd + '/joylink/json']
path += [cwd + '/joylink/list']
path += [cwd + '/joylink/config']
if GetDepend(['JOYLINK_USING_SOFTAP']):
    path += [cwd + '/joylink/softap']

src  = Glob('joylink/joylink/*.c')
SrcRemove(src, "joylink/joylink/test.c")
SrcRemove(src, "joylink/joylink/joylink_cloud_log.c")
SrcRemove(src, "joylink/joylink/joylink_dev_active.c")

src += Glob('joylink/auth/*.c')
SrcRemove(src, "joylink/auth/test.c")

src += Glob('joylink/list/*.c')
SrcRemove(src, "joylink/list/test.c")

src += Glob('joylink/json/joylink_json.c')
src += Glob('joylink/json/joylink_json_sub_dev.c')

src += Glob('joylink/config/*.c')
SrcRemove(src, "joylink/config/joylink_config_handle.c")
SrcRemove(src, "joylink/config/joylink_smart_config.c")
SrcRemove(src, "joylink/config/joylink_thunder_slave_sdk.c")

# sample files
if GetDepend(['JOYLINK_USING_SAMPLES_BAND']):
    src += Glob('samples/joylink_sample_band.c')

if GetDepend(['JOYLINK_USING_SAMPLES_HEATER']):
    src += Glob('samples/joylink_sample_heater.c')

# smartconfig files
if GetDepend(['JOYLINK_USING_SMARTCONFIG']):
    src += Glob('joylink/config/joylink_smart_config.c')

# thunder slave files
if GetDepend(['JOYLINK_USING_THUNDER_SLAVE']):
    src += Glob('joylink/config/joylink_thunder_slave_sdk.c')
    
# softap files
if GetDepend(['JOYLINK_USING_SOFTAP']):
    src += Glob('joylink/joylink/joylink_cloud_log.c')
    src += Glob('joylink/joylink/joylink_dev_active.c')

CPPDEFINES = ['__RT_THREAD__']
CPPDEFINES += ['_GET_HOST_BY_NAME_']

if GetDepend(['JOYLINK_USING_SOFTAP']):
    CPPDEFINES += ['_IS_DEV_REQUEST_ACTIVE_SUPPORTED_']

group = DefineGroup('joylink', src, depend = ['PKG_USING_JOYLINK'], CPPPATH = path, CPPDEFINES = CPPDEFINES)

list = os.listdir(cwd)
for item in list:
    if os.path.isfile(os.path.join(cwd, item, 'SConscript')):
        group = group + SConscript(os.path.join(item, 'SConscript'))

Return('group')
