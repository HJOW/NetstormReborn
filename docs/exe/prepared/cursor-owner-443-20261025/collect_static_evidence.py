from pathlib import Path
import json,re,hashlib,platform,subprocess
import pefile,capstone
P=Path(__file__).resolve().parent
ROOT=next(x for x in P.parents if (x/'AGENTS.md').exists() and (x/'originals/Netstorm.exe').exists())
E=P/'evidence';E.mkdir(exist_ok=True) # 출력은 스크립트가 놓인 준비 묶음에만 쓴다.
sha=lambda b:hashlib.sha256(b).hexdigest()
pePath=ROOT/'originals/Netstorm.exe';pe=pefile.PE(str(pePath));base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image();md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
cpath=ROOT/'extracted/refined/originals/Netstorm.c';c=cpath.read_text(encoding='utf-8');meta=(ROOT/'extracted/refined/originals/functions.tsv').read_text(encoding='utf-8')
rows=[] # 기존 함수표의 전체 바이트 크기를 그대로 읽는다.
for address,size in [(0x4458e0,436),(0x445aa0,137),(0x445b30,1286),(0x446e70,386)]:
 name=f'FUN_{address:08x}';m=re.search(r'^.*\b'+name+r'\([^\n]*\)\s*\n\s*\{',c,re.M);assert m,name
 start=m.start();pos=c.find('{',m.start());depth=1;end=pos+1
 while depth:
  depth+=(c[end]=='{')-(c[end]=='}');end+=1
 body=c[start:end]+'\n';out=E/f'{address:08x}-decompiled.txt';out.write_text(body,encoding='utf-8',newline='\n')
 code=image[address-base:address-base+size];ins=[{'address':f'{i.address:08x}','bytes':i.bytes.hex(),'mnemonic':i.mnemonic,'operands':i.op_str} for i in md.disasm(code,address)]
 output=E/f'{address:08x}-instructions.json';output.write_text(json.dumps(ins,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 rows.append({'entry':f'{address:08x}','bytes':size,'codeSHA256':sha(code),'metadataLine':next(x for x in meta.splitlines() if x.startswith(f'{address:x}\t')),'decompiledFile':str(out.relative_to(P)).replace('\\','/'),'decompiledSHA256':sha(out.read_bytes()),'instructionsFile':str(output.relative_to(P)).replace('\\','/'),'instructionsSHA256':sha(output.read_bytes())})
addresses={}
for name,address,width in [('previewCount',0x55a1b8,4),('previews',0x55a1d8,120),('process',0x55a274,4),('currentGump',0x55a278,4),('legalGump',0x55a27c,4),('preview',0x55a2a8,4),('snapToGrid',0x55a2d8,4),('adjustedScreenX',0x55a2f8,4),('adjustedScreenY',0x55a2fc,4),('lockDepth',0x5325c8,4),('rawMouseX',0x5cae94,4),('rawMouseY',0x5cae98,4)]:
 rva=address-base;section=next(s for s in pe.sections if s.VirtualAddress<=rva<s.VirtualAddress+max(s.Misc_VirtualSize,s.SizeOfRawData));rel=rva-section.VirtualAddress;data=image[rva:rva+width];addresses[name]={'address':f'{address:08x}','width':width,'section':section.Name.rstrip(b'\0').decode(),'sectionRelative':rel,'rawSize':section.SizeOfRawData,'virtualSize':section.Misc_VirtualSize,'virtualZeroTail':rel>=section.SizeOfRawData,'initialImageHex':data.hex()}
result={'status':'static-only; no PE/Native/CPP execution','lastDecompile':'HJOW-Athlon/2026-10-23','host':platform.node(),'command':'python extracted/cpp-cursor-owner-443-prepared/collect_static_evidence.py','peSHA256':sha(pePath.read_bytes()),'decompiledSHA256':sha(cpath.read_bytes()),'functionMetadataSHA256':sha((ROOT/'extracted/refined/originals/functions.tsv').read_bytes()),'capstoneVersion':capstone.__version__,'pefileVersion':pefile.__version__,'entries':rows,'addresses':addresses,'claims':{'table':'0055a1d8..0055a24f:30 DWORD/120BYTE, scaled index4, both cleanup loops zero30DWORD','currentCount':'00445aa0 cmp signed current0055a1b8 after each destroy callback; loop does not capture initial count','cancel':'00446e70 always invokes00445aa0(force1), then current0055a274 removal(flags0),zero30DWORD,preview0/lock1/count0; no currentGump/legalGump reset','firstMenu':'lock1/preview0 skips selected-object branch only; Activate→Prepare→497220/497580 still reached','sidHostBoundary':'raw DWORD retained; out-of-SID16 values rejected only at actual host SID consumption, not while overwriting inactive DWORD','execution':'existing fixtures are comparison inputs only; no new independent Native execution nor C++ verification performed'}}
result['harnessSHA256']=sha(Path(__file__).read_bytes()) # 재생성 스크립트 자체도 원시 SHA로 식별한다.
(P/'static-evidence.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('static entries',len(rows),'fields',len(addresses))
for name in ['previews','previewCount','snapToGrid','lockDepth']:print(name,addresses[name]['initialImageHex'])
