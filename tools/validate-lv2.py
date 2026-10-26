"""Validate discovery and public ports through the real Lilv LV2 host scanner."""
import ctypes as C
from ctypes.util import find_library
import os
from pathlib import Path
import sys
bundle=Path(sys.argv[1]).resolve()
schemas=Path('/workspace/toolchains/lv2/usr/lib/lv2')
os.environ['LV2_PATH']=str(bundle.parent)+(':'+str(schemas) if schemas.is_dir() else '')
lib=C.CDLL(find_library('lilv-0') or 'liblilv-0.so.0')
def fn(name,restype,args):
    f=getattr(lib,name);f.restype=restype;f.argtypes=args;return f
P=C.c_void_p;S=C.c_char_p;U=C.c_uint32
world_new=fn('lilv_world_new',P,[]);world_free=fn('lilv_world_free',None,[P]);load=fn('lilv_world_load_all',None,[P])
new_uri=fn('lilv_new_uri',P,[P,S]);free_node=fn('lilv_node_free',None,[P]);as_string=fn('lilv_node_as_string',S,[P])
plugins=fn('lilv_world_get_all_plugins',P,[P]);get=fn('lilv_plugins_get_by_uri',P,[P,P])
count=fn('lilv_plugin_get_num_ports',U,[P]);getport=fn('lilv_plugin_get_port_by_index',P,[P,U])
symbol=fn('lilv_port_get_symbol',P,[P,P]);isa=fn('lilv_port_is_a',C.c_bool,[P,P,P]);verify=fn('lilv_plugin_verify',C.c_bool,[P])
world=world_new();load(world);uri=new_uri(world,b'https://jurekbak71-dev.github.io/JerzyAtomicCluster#effect')
try:
    plugin=get(plugins(world),uri);assert plugin and verify(plugin),'Lilv plugin verification failed'
    names=['in_l','in_r','out_l','out_r','events','volume','blend','speed_ms','atoms','smooth','flat','enabled','sync','division','pattern','stereo','bpm','tap','latency']
    assert count(plugin)==len(names)
    for i,expected in enumerate(names):
        port=getport(plugin,i);assert as_string(symbol(plugin,port)).decode()==expected
    for filename in ['manifest.ttl','atomic-cluster.ttl','presets.ttl','modgui.ttl','modgui/icon.html','modgui/style.css','modgui/knob.png','modgui/screenshot.png','modgui/thumbnail.png','atomic_cluster.so']:
        assert (bundle/filename).is_file(),filename
    html=(bundle/'modgui/icon.html').read_text()
    for name in ['in_l','in_r','out_l','out_r','volume','blend','speed_ms','atoms','smooth','flat','sync','division','pattern','stereo','bpm','tap']:
        assert f'mod-port-symbol="{name}"' in html,name
    print('Lilv verified URI and 19 stable ports; MOD GUI audio jacks, controls and all assets present.')
finally:
    free_node(uri);world_free(world)
