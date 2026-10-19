import json
import os
from pathlib import Path
os.environ.setdefault('MPLCONFIGDIR','/tmp/atomic-reference-mpl')
os.environ.setdefault('XDG_CACHE_HOME','/tmp/atomic-reference-cache')
import numpy as np
from scipy.io import wavfile
from scipy import signal
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

import argparse
parser=argparse.ArgumentParser(description='Measure a reference WAV; requires NumPy, SciPy, Matplotlib')
parser.add_argument('input',type=Path)
parser.add_argument('--output',type=Path,default=Path('/workspace/atomic-reference-analysis'))
args=parser.parse_args()
root=args.output;root.mkdir(parents=True,exist_ok=True)
sr,x=wavfile.read(args.input)
if np.issubdtype(x.dtype,np.integer):
    x=x.astype(np.float32)/float(max(abs(np.iinfo(x.dtype).min),np.iinfo(x.dtype).max))
else:
    x=x.astype(np.float32)
correlation=float(np.corrcoef(x[:,0],x[:,1])[0,1]) if x.ndim==2 and x.shape[1]==2 else None
if x.ndim==2:x=x.mean(axis=1)
if sr!=22050:raise SystemExit('Resample WAV to 22050 Hz before analysis')
nfft,hop=4096,256
f,t,z=signal.stft(x,fs=sr,window='hann',nperseg=nfft,noverlap=nfft-hop,boundary=None,padded=False)
amplitude=np.abs(z).astype(np.float32)
del z
mask=(f>=60)&(f<=8000)
freq=f[mask]; a=amplitude[mask]
power=a*a
energy=np.sum(power,axis=0)
frame_db=10*np.log10(np.maximum(energy,1e-15))
active=frame_db>max(-55,float(np.max(frame_db)-45))
relative_db=20*np.log10(np.maximum(a,1e-10)/np.maximum(a.max(axis=0),1e-10))
local=(a[1:-1]>a[:-2])&(a[1:-1]>=a[2:])&(relative_db[1:-1]>-20)
peaks=local.sum(axis=0)
distribution=power/np.maximum(energy,1e-20)
entropy=np.exp(-np.sum(distribution*np.log(np.maximum(distribution,1e-20)),axis=0))
sorted_power=np.sort(power,axis=0)
top5=np.sum(sorted_power[-5:],axis=0)/np.maximum(energy,1e-20)
norm=a/np.maximum(np.sqrt(energy),1e-20)
flux=np.zeros(len(t)); flux[1:]=np.maximum(0,1-np.sum(norm[:,1:]*norm[:,:-1],axis=0))
flux[(~active)|np.r_[True,~active[:-1]]] = 0.
strong=np.argmax(a,axis=0)
dominant=freq[strong]
q=np.arange(len(t)); valid=(strong>0)&(strong<len(freq)-1)
k=strong[valid]; cols=q[valid]
logs=np.log(np.maximum(a,1e-20))
offset=.5*(logs[k-1,cols]-logs[k+1,cols])/(logs[k-1,cols]-2*logs[k,cols]+logs[k+1,cols]+1e-20)
dominant[valid]+=np.clip(offset,-.5,.5)*(sr/nfft)

chunks=[]
for start in np.arange(0,len(x)/sr-15,5):
    m=(t>=start)&(t<start+15)&active
    if m.sum()<500: continue
    item={'start':float(start),'end':float(start+15),'active_fraction':float(np.mean(active[(t>=start)&(t<start+15)])),
          'median_spectral_peaks':float(np.median(peaks[m])),
          'median_effective_bins':float(np.median(entropy[m])),
          'median_top5_energy':float(np.median(top5[m])),
          'flux_median':float(np.median(flux[m])), 'flux_p95':float(np.quantile(flux[m],.95)),
          'level_range_db':float(np.quantile(frame_db[m],.95)-np.quantile(frame_db[m],.05))}
    chunks.append(item)
selections={
  'fewest_spectral_peaks':min(chunks,key=lambda i:i['median_spectral_peaks']),
  'highest_concentration':max(chunks,key=lambda i:i['median_top5_energy']),
  'richest_spectrum':max(chunks,key=lambda i:i['median_spectral_peaks']),
  'largest_fast_changes':max(chunks,key=lambda i:i['flux_p95']),
  'smallest_changes_with_multiple_peaks':min([c for c in chunks if c['median_spectral_peaks']>=3],key=lambda i:i['flux_median'])}
summary={'duration_seconds':len(x)/sr,'sample_rate_analysis':sr,'fft_size':nfft,'hop_samples':hop,
         'window_seconds':nfft/sr,'hop_seconds':hop/sr,'frequency_bin_hz':sr/nfft,
         'input_peak_dbfs':float(20*np.log10(np.max(np.abs(x)))),
         'active_frame_fraction':float(active.mean()),
         'last_active_frame_seconds':float(t[active][-1]),
         'stereo_correlation':correlation,
         'selected_sections':selections,
         'limitations':['No knob positions or bypass markings available from MP3.',
                       'No paired isolated dry/wet input; this is not a measured transfer function.',
                       'Spectral peak counts are not a measurement of the pedal ATOMS knob.',
                       'Abrupt input note changes and edits can resemble algorithm refreshes.',
                       'Spectral concentration depends on instrument, blend, source and encoding.']}
(root/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(root/'windows.json').write_text(json.dumps(chunks,indent=2)+'\n')
np.savez_compressed(root/'features.npz',time=t,frame_db=frame_db,peaks=peaks,effective_bins=entropy,top5=top5,flux=flux,dominant_hz=dominant,active=active)

# Compact logarithmic spectrogram, without writing a large mesh into the PDF.
logf=np.geomspace(60,8000,400)
indices=np.searchsorted(f,logf)
stride=8
image=20*np.log10(np.maximum(amplitude[indices,::stride],1e-7))
fig,axes=plt.subplots(4,1,figsize=(16,11),sharex=True,gridspec_kw={'height_ratios':[3,1,1,1]},layout='constrained')
axes[0].imshow(image,aspect='auto',origin='lower',extent=[t[0],t[-1],0,399],vmin=-65,vmax=-10,cmap='magma')
ticks=np.array([100,200,500,1000,2000,5000,8000]); axes[0].set_yticks(np.log(ticks/60)/np.log(8000/60)*399,labels=[str(v) for v in ticks]); axes[0].set_ylabel('Frequency (Hz)')
axes[0].set_title('Original reference demo — measurement overview (no knob settings inferred)')
axes[1].plot(t[::4],frame_db[::4],linewidth=.5); axes[1].set_ylabel('STFT level (dB)')
axes[2].plot(t[::4],np.where(active,peaks,np.nan)[::4],linewidth=.6,color='teal'); axes[2].set_ylabel('Spectral peaks\nwithin 20 dB')
axes[3].plot(t[::4],flux[::4],linewidth=.5,color='purple'); axes[3].set_ylabel('Spectral change'); axes[3].set_xlabel('Time (seconds)')
for ax in axes: ax.grid(alpha=.15)
fig.savefig(root/'overview.png',dpi=140)
plt.close(fig)
for label,section in selections.items():
    start=section['start']; m=(t>=start)&(t<start+15)
    fig,ax=plt.subplots(figsize=(13,4),layout='constrained')
    spec=20*np.log10(np.maximum(amplitude[indices][:,m],1e-7))
    ax.imshow(spec,aspect='auto',origin='lower',extent=[start,start+15,0,399],vmin=-65,vmax=-10,cmap='magma')
    ax.set_yticks(np.log(ticks/60)/np.log(8000/60)*399,labels=[str(v) for v in ticks]); ax.set_xlabel('Time (seconds)'); ax.set_ylabel('Hz'); ax.set_title(label.replace('_',' ')+' — reference audio')
    fig.savefig(root/(label+'.png'),dpi=140); plt.close(fig)
print(json.dumps(summary,indent=2))
