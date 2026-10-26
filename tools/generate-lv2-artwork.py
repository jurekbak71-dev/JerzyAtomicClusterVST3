"""Original MOD GUI knob filmstrip and static plugin-browser illustration."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math
folder=Path(__file__).resolve().parents[1]/'lv2/modgui';folder.mkdir(parents=True,exist_ok=True)
strip=Image.new('RGBA',(64*65,64))
for frame in range(65):
    draw=ImageDraw.Draw(strip);x=frame*64
    draw.ellipse((x+3,3,x+61,61),fill='#a59c8d'); draw.ellipse((x+7,7,x+57,57),fill='#f2ead8')
    angle=math.radians(-225+frame/64*270)
    draw.line((x+32+14*math.cos(angle),32+14*math.sin(angle),x+32+24*math.cos(angle),32+24*math.sin(angle)),fill='#3d2531',width=3)
strip.save(folder/'knob.png')
# Browser artwork; not a screenshot of MODEP running on physical hardware.
im=Image.new('RGB',(420,650),'#211244');draw=ImageDraw.Draw(im)
draw.rounded_rectangle((2,2,417,647),radius=22,outline='#c9a247',width=3)
font=ImageFont.load_default(size=22);small=ImageFont.load_default(size=12)
draw.text((210,23),'JERZY ATOMIC CLUSTER',font=font,fill='#ffd05b',anchor='mt')
labels=['VOL','BLEND','SPEED','ATOMS','SMOOTH','FLAT','BPM SYNC','DIVISION','PATTERN','STEREO','BPM','TAP']
for i,label in enumerate(labels):
    x=76+(i%3)*133; y=130+(i//3)*122
    im.paste(strip.crop((32*64,0,33*64,64)),(x-32,y-32),strip.crop((32*64,0,33*64,64)))
    draw.text((x,y-53),label,font=small,fill='#fff3d4',anchor='mt')
draw.ellipse((183,591,237,645),fill='#bcbab2',outline='#777267',width=5)
im.save(folder/'screenshot.png');im.resize((194,300)).save(folder/'thumbnail.png')
print('Generated original MOD artwork (65-frame knob strip and browser illustrations).')
