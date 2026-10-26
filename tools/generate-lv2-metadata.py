"""Generate standard LV2 metadata and MOD HTML controls from the public port order."""
from pathlib import Path
import math
root=Path(__file__).resolve().parents[1]; folder=root/'lv2'; folder.mkdir(exist_ok=True)
uri='https://jurekbak71-dev.github.io/JerzyAtomicCluster#effect'
prefix='''@prefix atom: <http://lv2plug.in/ns/ext/atom#> .
@prefix doap: <http://usefulinc.com/ns/doap#> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .
@prefix lv2: <http://lv2plug.in/ns/lv2core#> .
@prefix mod: <http://moddevices.com/ns/mod#> .
@prefix modgui: <http://moddevices.com/ns/modgui#> .
@prefix pg: <http://lv2plug.in/ns/ext/port-groups#> .
@prefix pprops: <http://lv2plug.in/ns/ext/port-props#> .
@prefix pset: <http://lv2plug.in/ns/ext/presets#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix resize: <http://lv2plug.in/ns/ext/resize-port#> .
@prefix time: <http://lv2plug.in/ns/ext/time#> .
@prefix units: <http://lv2plug.in/ns/extensions/units#> .
@prefix urid: <http://lv2plug.in/ns/ext/urid#> .
'''
ports=[]
def audio(symbol,name,direction,group,side,optional=False):
    extra=f'pg:group <{uri}/{group}> ; lv2:designation pg:{side}'
    if optional: extra+=' ; lv2:portProperty lv2:connectionOptional'
    ports.append((symbol,name,f'lv2:{direction}Port, lv2:AudioPort',extra))
def control(symbol,name,default,low,high,extra=''):
    props=f'lv2:default {default:.9g} ; lv2:minimum {low:.9g} ; lv2:maximum {high:.9g}'
    ports.append((symbol,name,'lv2:InputPort, lv2:ControlPort',props+(' ; '+extra if extra else '')))
def enumeration(labels):
    return 'lv2:portProperty lv2:integer, lv2:enumeration ; lv2:scalePoint '+', '.join(f'[ rdfs:label "{s}" ; <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> {i} ]' for i,s in enumerate(labels))
audio('in_l','Input L / Mono','Input','input','left')
audio('in_r','Input R','Input','input','right',True)
audio('out_l','Output L','Output','output','left')
audio('out_r','Output R','Output','output','right')
ports.append(('events','Host tempo / position','lv2:InputPort, atom:AtomPort','atom:bufferType atom:Sequence ; atom:supports time:Position ; resize:minimumSize 4096 ; lv2:portProperty lv2:connectionOptional'))
control('volume','Volume',0,-60,6,'units:unit units:db')
control('blend','Blend',100,0,100,'units:unit units:pc')
control('speed_ms','Speed (Free)',2000*(1/750)**.4,2000/750,2000,'units:unit units:ms ; lv2:portProperty pprops:logarithmic')
control('atoms','Atoms',16,1,64,'lv2:portProperty lv2:integer, pprops:logarithmic')
control('smooth','Sharp / Smooth',0,0,1,enumeration(['Sharp','Smooth'])+' ; lv2:portProperty lv2:toggled')
control('flat','Flat dynamics',0,0,1,'lv2:portProperty lv2:integer, lv2:toggled')
control('enabled','Enabled',1,0,1,'lv2:designation lv2:enabled ; lv2:portProperty lv2:integer, lv2:toggled')
control('sync','BPM Sync',0,0,1,'lv2:portProperty lv2:integer, lv2:toggled')
control('division','Sync division',2,0,4,enumeration(['1/16','1/8','1/4','1/2','1 bar']))
control('pattern','Atom pattern',0,0,2,enumeration(['Cluster','Arpeggio','Random harmonic']))
control('stereo','Stereo width',100,0,100,'units:unit units:pc')
control('bpm','BPM / fallback',120,20,400,'units:unit units:bpm ; lv2:designation time:beatsPerMinute')
control('tap','Tap BPM (Sync)',0,0,1,'lv2:portProperty lv2:integer, lv2:toggled, pprops:trigger')
ports.append(('latency','Latency','lv2:OutputPort, lv2:ControlPort','lv2:designation lv2:latency ; lv2:portProperty lv2:integer, lv2:reportsLatency ; units:unit units:frame'))
blocks=[]
for i,(symbol,name,types,extra) in enumerate(ports):
    blocks.append(f'[ a {types} ; lv2:index {i} ; lv2:symbol "{symbol}" ; lv2:name "{name}" ; {extra} ]')
text=prefix+f'''\n<{uri}/input> a pg:StereoGroup, pg:InputGroup ; lv2:name "Stereo input" ; lv2:symbol "input" .
<{uri}/output> a pg:StereoGroup, pg:OutputGroup ; lv2:name "Stereo output" ; lv2:symbol "output" .
<{uri}> a lv2:Plugin, lv2:SpectralPlugin ;
    doap:name "Jerzy Atomic Cluster" ;
    doap:maintainer [ a foaf:Person ; foaf:name "Jerzy Audio" ] ;
    mod:brand "Jerzy" ; mod:label "Atomic Cluster" ;
    lv2:minorVersion 2 ; lv2:microVersion 1 ;
    lv2:requiredFeature urid:map ;
    lv2:optionalFeature lv2:hardRTCapable ;
    rdfs:comment "Harmonic atoms, stereo orbits, arpeggio and random patterns. BPM sync follows host time:Position; without host tempo use the BPM control. Right input disconnected duplicates the mono input. Latency: 2048 samples." ;
    lv2:port '''+',\n        '.join(blocks)+' .\n'
(folder/'atomic-cluster.ttl').write_text(text)
manifest=prefix+f'\n<{uri}> a lv2:Plugin ; lv2:binary <atomic_cluster.so> ; rdfs:seeAlso <atomic-cluster.ttl>, <modgui.ttl> .\n'
presets=[('stereo-cluster','Stereo Cluster',dict(atoms=32,smooth=1,stereo=100,pattern=0,sync=0)),
         ('sharp-arp','Sharp Arpeggio — BPM Sync',dict(atoms=24,smooth=0,stereo=100,pattern=1,sync=1,division=1)),
         ('random-pad','Smooth Random Harmonics',dict(atoms=48,smooth=1,stereo=100,pattern=2,sync=1,division=2))]
contents=prefix
for slug,label,values in presets:
    pu=uri+'/preset/'+slug
    manifest+=f'<{pu}> a pset:Preset ; lv2:appliesTo <{uri}> ; rdfs:label "{label}" ; rdfs:seeAlso <presets.ttl> .\n'
    # Full control snapshots prevent omitted values inheriting host state.
    full={symbol:float(extra.split('lv2:default ')[1].split(' ;')[0]) for symbol,_,types,extra in ports if types=='lv2:InputPort, lv2:ControlPort'}
    full.update(values)
    contents+=f'\n<{pu}> a pset:Preset ; lv2:appliesTo <{uri}> ; rdfs:label "{label}" ; lv2:port '+', '.join(f'[ lv2:symbol "{k}" ; pset:value {v} ]' for k,v in full.items())+' .\n'
(folder/'manifest.ttl').write_text(manifest); (folder/'presets.ttl').write_text(contents)
(folder/'modgui.ttl').write_text(prefix+f'''\n<{uri}> modgui:gui [
    modgui:resourcesDirectory <modgui> ; modgui:iconTemplate <modgui/icon.html> ;
    modgui:stylesheet <modgui/style.css> ; modgui:label "Atomic Cluster" ;
    modgui:screenshot <modgui/screenshot.png> ; modgui:thumbnail <modgui/thumbnail.png> ;
    modgui:brand "Jerzy" ; modgui:model "boxy" ; modgui:panel "12-knobs" ;
    modgui:color "purple" ; modgui:knob "cream" ;
    modgui:port '''+', '.join(f'[ lv2:index {i} ; lv2:symbol "{symbol}" ; lv2:name "{name}" ]' for i,(symbol,name,types,_) in enumerate(p for p in ports if p[2]=='lv2:InputPort, lv2:ControlPort'))+' ] .\n')
gui=folder/'modgui'; gui.mkdir(exist_ok=True)
html='<div class="atomic-pedal mod-pedal atomic-instance{{{cns}}}"><div mod-role="drag-handle" class="mod-drag-handle"></div><div class="atomic-title">JERZY<br>ATOMIC CLUSTER</div><div class="atomic-controls">'
for symbol,name,types,_ in ports:
    if types!='lv2:InputPort, lv2:ControlPort' or symbol=='enabled': continue
    html+=f'<div class="atomic-control"><div class="atomic-label">{name}</div><div class="atomic-knob mod-knob-image" mod-role="input-control-port" mod-port-symbol="{symbol}" mod-widget="film"></div><div class="atomic-value" mod-role="input-control-value" mod-port-symbol="{symbol}"></div></div>'
html+='</div><div class="atomic-footswitch" mod-role="bypass"></div><div class="atomic-led on" mod-role="bypass-light"></div><div class="mod-plugin-author"><span class="mod-plugin-brand">JERZY</span></div></div>'
html=html[:-6]+"<div class=\"mod-pedal-input\">"
for symbol,label in [('in_l','L / MONO'),('in_r','R')]:
    html+=f'<div class="atomic-jack mod-input mod-input-disconnected" title="{label}" mod-role="input-audio-port" mod-port-symbol="{symbol}"><div class="mod-pedal-input-image"></div></div>'
html+='</div><div class="mod-pedal-output">'
for symbol,label in [('out_l','L'),('out_r','R')]:
    html+=f'<div class="atomic-jack mod-output mod-output-disconnected" title="{label}" mod-role="output-audio-port" mod-port-symbol="{symbol}"><div class="mod-pedal-output-image"></div></div>'
html+='</div></div>'
(gui/'icon.html').write_text(html+'\n')
(gui/'style.css').write_text('''.atomic-pedal{width:420px;min-height:650px;background:#211244;border:3px solid #b6974b;border-radius:22px;color:#fff3d4;position:relative;padding:18px;box-sizing:border-box;font-family:sans-serif}.atomic-title{text-align:center;font-weight:bold;font-size:25px;color:#ffd05b;padding:8px}.atomic-controls{display:grid;grid-template-columns:repeat(3,1fr);gap:14px 8px}.atomic-control{text-align:center}.atomic-label{font-size:12px;min-height:28px}.atomic-knob{width:64px;height:64px;margin:auto;background-image:url(/resources/knob.png{{{ns}}});background-repeat:no-repeat;background-size:auto 64px}.atomic-value{font-size:11px;min-height:18px;margin-top:4px}.atomic-footswitch{width:48px;height:48px;margin:16px auto 0;border-radius:50%;background:#b8b6b0;border:6px solid #797772;cursor:pointer}.atomic-led{height:10px;width:10px;background:#71db91;border-radius:50%;margin:8px auto}.atomic-pedal .mod-drag-handle{height:24px;position:absolute;top:0;left:0;right:0}.atomic-pedal .mod-pedal-input{position:absolute;left:-14px;top:290px}.atomic-pedal .mod-pedal-output{position:absolute;right:-14px;top:290px}.atomic-jack{width:24px;height:24px;margin-bottom:35px;border:3px solid #ab974b;background:#14101d;border-radius:50%;box-sizing:border-box}.atomic-led.off{background:#343039}.mod-plugin-author{text-align:center;font-size:10px;color:#d7ad54}\n''')
print(f'Generated {len(ports)} LV2 ports, three complete presets and MOD GUI metadata.')
