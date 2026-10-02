#!/usr/bin/env python3
"""Reads the load tables of the Vihtavuori Reloading Guide (PDF, e.g. Vihtavuori_Reloading_Guide_2026_ENG_web.pdf)
into a CSV: cartridge, test barrel, bullet, C.O.L., powder, start / maximum charge [g] and velocity [m/s].
usage: parse_vv.py <guide.pdf> [out.csv]      (needs pdftotext; keep the PDF and the CSV local, they are not ours)
The pages hold two tables side by side; each half is read separately, continued tables ("... cont.") keep the
cartridge and test barrel of their first part."""
import sys
import re, subprocess, csv, sys
PDF=sys.argv[1] if len(sys.argv) > 1 else 'vv2026.pdf'; W=498.898; H=708.661
num=r'(\d+,\d+)C?'
full=re.compile(r'^\s*(\d+,\d)\s+(\d+)\s+(.+?)\s{2,}(.+?)\s{2,}(\d+,\d)\s+(\d\.\d{3})\s+(\S+)\s+'+num+r'\s+(\d+\.\d)C?\s+(\d+)\s+(\d+)\s+'+num+r'\s+(\d+\.\d)C?\s+(\d+)\s+(\d+)\s*$')
cont=re.compile(r'^\s*([A-Z0-9][A-Za-z0-9]{2,4})\s+'+num+r'\s+(\d+\.\d)C?\s+(\d+)\s+(\d+)\s+'+num+r'\s+(\d+\.\d)C?\s+(\d+)\s+(\d+)\s*$')
out=[]
barrels={}
for page in range(1,78):
    for half in (0,1):
        t=subprocess.run(['pdftotext','-f',str(page),'-l',str(page),'-r','72','-x',str(0 if half==0 else 512),'-y','0','-W',str(512 if half==0 else 520),'-H',str(int(H)),'-layout',PDF,'-'],capture_output=True,text=True).stdout
        lines=t.splitlines()
        cart=None; barrel=None; bullet=None; namebuf=[]; inhead=False
        for l in lines:
            m=re.search(r'Test barrel:\s+(\d+)\s*mm',l)
            if m: barrel=int(m.group(1)); cart=None; namebuf=[]; inhead=True; continue
            mc=re.match(r'^\s*(\S.*?)\s+cont\.\s*$',l)
            if mc and mc.group(1).strip() in barrels:
                cart=mc.group(1).strip(); barrel=barrels[cart]; inhead=False; bullet=None; continue
            if inhead:
                if re.search(r'\bBullet\b',l) and 'Powder' in l:
                    cart=re.split(r'\s+(CAUTION|This data|WARNING|NOTE)',' '.join(namebuf))[0].strip(); barrels[cart]=barrel; inhead=False; continue
                left=re.split(r'Primers:|Cases:|NOTE:',l)[0].strip()
                if left and not left.startswith('[') and not re.match(r'^\d\)',left) and not re.match(r'^(CAUTION|This data|WARNING|NOTE|Cases|Large|Small|Bullet|FOR )',left): namebuf.append(left)
                continue
            if not cart: continue
            m=full.match(l)
            if m:
                g=m.groups(); bullet=dict(wg=g[0],wgr=g[1],mfg=g[2].strip(),type=re.sub(r'\d\)$','',g[3].strip()),col=g[4])
                pw,s_g,s_gr,s_ms,s_fps,m_g,m_gr,m_ms,m_fps=g[6:15]
            else:
                m=cont.match(l)
                if not (m and bullet): continue
                pw,s_g,s_gr,s_ms,s_fps,m_g,m_gr,m_ms,m_fps=m.groups()
            out.append(dict(cartridge=cart,barrel_mm=barrel,bullet_g=bullet['wg'].replace(',','.'),bullet_gr=bullet['wgr'],mfg=bullet['mfg'],type=bullet['type'],col_mm=bullet['col'].replace(',','.'),
                            powder=pw,start_g=s_g.replace(',','.'),start_ms=s_ms,max_g=m_g.replace(',','.'),max_ms=m_ms,page=page))
w=csv.DictWriter(open(sys.argv[2] if len(sys.argv) > 2 else 'vv2026.csv','w',newline='',encoding='utf-8'),fieldnames=list(out[0].keys())); w.writeheader(); w.writerows(out)
import collections
c=collections.Counter(r['cartridge'] for r in out)
print(len(out),'rows,',len(c),'cartridges')
for k,v in sorted(c.items()): print('  %-40s %d'%(k,v))
