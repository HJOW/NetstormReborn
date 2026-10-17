#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""직접 좌표 빔 생성·화면 wrapper·double 지연타이머를 원본 전체 몸체로 관측한다."""
import argparse,csv,json,itertools,socket,struct
from decomp_lightning_lifetime import Oracle as PreviousOracle,ROOT,SOURCE,PALETTE,MERGED,OBJECT,PARTICLES,PATH,TYPES,SOUND,TABLE,POOL,slots,semantic,digest
from decomp_particle_gradient import STACK
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX
TIMER=0x12507000 # 타이머 원문의 실제8바이트 저장소다.
EXTRA=(0x471e70,0x471ca0,0x472770,0x418070) # 이번에 전체 실행할 네 진입점이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/lightning-screen-x86.tsv' # 기존 원문은 바꾸지 않는 새 관측이다.
REPORT=ROOT/'cpppj/lightning-screen.json' # 실제 반환·경계·정밀도·전체 원문 SHA다.
# float 입력의 비트만 만든다.
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
# 현재 19개 입력DWORD를 만들며 선택 결과나 생성 필드는 계산하지 않는다.
def cases():
 base=[0,0,0,0xffffffff,50,127,0,1,*map(bits,(1600.,0.,1600.,1342.)),0,0,bits(5.),1,1,*struct.unpack('<II',struct.pack('<d',100.123456789))]
 rows=[]
 # 직접 좌표 생성의 공유색/확장/재진입/0수명/NaN/파일명을 교차한다.
 for warm,scene,life,sound in itertools.product((0,1),range(3),(0,bits(5.),bits(-1.),0x7fc01234),(0,1)):
  c=base.copy();c[1]=warm;c[2:4]=(0,0xffffffff) if scene==0 else (40,20);c[6]=scene;c[14]=life;c[15]=sound;rows.append(c)
 # 화면 wrapper의 signed 좌표/카메라 감김과 선행좌표 보관을 검사한다.
 for camera,points,warm,mutation in itertools.product(((0,0),(100,200),(-300,400),(0x7fffffff,0x80000000)),((1600,0,1600,1342),(-50,13,0,0),(0x7fffffff,0x80000000,16,-16)),(0,1),(0,1,2)):
  c=base.copy();c[0]=1;c[1]=warm;c[2:4]=40,20;c[6]=mutation;c[8:12]=[v&0xffffffff for v in points];c[12:14]=[v&0xffffffff for v in camera];rows.append(c)
 # new104 실패 때 생성자·색·Host는 실행되지 않는다.
 for camera in ((0,0),(0x7fffffff,0x80000000)):
  c=base.copy();c[0]=1;c[12:14]=camera;c[16]=0;rows.append(c)
 # 원본418084는 float벽시각이 아니라 fadd qword다.
 for now,delay in itertools.product((0.,100.123456789,0.000001,-1000000.,1e15),(0,bits(1.5),bits(-1),0x7fc01234,0x7f800000)):
  c=base.copy();c[0]=2;c[14]=delay;c[17:19]=struct.unpack('<II',struct.pack('<d',now));rows.append(c)
 return list(dict.fromkeys(tuple(r) for r in rows))

def palette_evidence():
 """동일RGBX의기존독립원문팔레트만재사용한다. C++계산결과를읽지않는다."""
 fixture=ROOT/'cpppj/tests/fixtures/lightning-lifetime-x86.tsv';facts=json.loads((ROOT/'cpppj/lightning-lifetime.json').read_text('utf-8'))
 # 원본PE/기존관측기/원문전체의현재SHA를독립확인한다.
 for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
 rows=[r.split('\t') for r in fixture.read_text('utf-8').splitlines() if r and not r.startswith('#')]
 selected=next(r for r in rows if int(r[1])==0x27f and r[2].startswith('GIFCLOUD.COL,0,'));assert len(bytes.fromhex(selected[7]))==44
 return bytes.fromhex(selected[3]),bytes.fromhex(selected[7]),dict(fixture=fixture.relative_to(ROOT).as_posix(),row=selected[0],precision=0x27f,rgbxSha256=__import__('hashlib').sha256(bytes.fromhex(selected[3])).hexdigest(),paletteHex=selected[7],scope='동일GIFCLOUD RGBX의이미독립관측한458480 팔레트결과만명시경계로재사용한다. 신규4몸체/147입력/나머지등록·경로·소멸전체는매번실제PE를실행한다.')

class Oracle(PreviousOracle):
 """이미 검증한 팔레트/공통등록/소멸을 같은 PE에서 실행하며 새 생성 경로만 더한다."""
 def __init__(self):
  """실제 새 몸체와 timer 저장범위/읽기 감사 경계를 등록한다."""
  super().__init__();self.instructions=0;self.paletteRgbx,self.coldPalette,self.paletteFacts=palette_evidence()
  with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(r['entry'],16):r for r in csv.DictReader(stream,delimiter='\t')}
  # 새 전체 몸체의 정밀 범위만 허용한다.
  for entry in EXTRA:
   for part in facts[entry]['ranges'].split(';'):
    lo,hi=(int(v,16) for v in part.split('-'));self.allowed.append((lo,hi+1))
  self.spans=[(lo,hi+1) for lo,hi,_ in self.mu.mem_regions()];self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
 def read(self,mu,access,address,size,value,data):
  """격리 실행기가 명시적으로 만든 자료/보호이미지/스택 외 읽기는 거부한다."""
  if not any(lo<=address and address+size<=hi for lo,hi in self.spans):raise RuntimeError(f'화면빔 범위밖읽기 {address:x}+{size}')
 def write(self,mu,access,address,size,value,data):
  """8바이트 지연 타이머 이외에는 기존 실제 빔/경로 쓰기 제한을 유지한다."""
  if TIMER<=address and address+size<=TIMER+8:return
  super().write(mu,access,address,size,value,data)
 def instruction(self,mu,address,size,data):
  """할당104와double wall만 추가 장치 경계이며 나머지 생성 계산은 PE가 수행한다."""
  self.instructions+=1
  if address in EXTRA:self.calls[f'{address:08x}']+=1
  if address==0x458480:
   # 동일한과거원본입력/출력만사용하는팔레트경계다. 호출자는계속전체PE를실행한다.
   if not self.integer(0x55a758):
    self.events.extend(('N:8','N:44'));mu.mem_write(MERGED,struct.pack('<2I',11,MERGED+0x1000));mu.mem_write(MERGED+0x1000,self.coldPalette);mu.mem_write(0x55a758,struct.pack('<I',MERGED));self.calls['palette-fixture-cold-boundary']+=1
   else:self.calls['palette-warm-return-boundary']+=1
   self.ret(MERGED);return
  if address==0x460d70:
   pointer=self.integer(mu.reg_read(UC_X86_REG_ESP)+4);self.events.append('W');mu.mem_write(pointer,struct.pack('<II',*self.row[17:19]));self.ret(pointer);return
  if address==0x4e4391 and self.integer(mu.reg_read(UC_X86_REG_ESP)+4)==104:
   self.events.append('N:104');self.calls['object-new-boundary']+=1;self.ret(OBJECT if self.row[16] else 0);return
  super().instruction(mu,address,size,data)
 def sample(self,row,precision):
  """전체 생성/삭제 또는 타이머의 실제 상태와 호출 순서를 읽는다."""
  self.row=row;self.events=[];mu=self.mu
  if row[0]==2:
   mu.mem_write(TIMER,b'\xa5'*8);mu.mem_write(0,struct.pack('<I',0x87654321));self.call(0x418070,TIMER,struct.pack('<I',row[14]),4,precision);self.returns['timer']+=1;assert self.integer(0)==0x87654321
   return ('-','-','-','-','-','-', ';'.join(self.events),'-','-','-',bytes(mu.mem_read(TIMER,8)).hex())
  self.capacity,hole,self.parent,depth,self.mutate=row[2:7];hole=hole if hole<0x80000000 else hole-(1<<32)
  col=(ROOT/'originals/d/GIFCLOUD.COL').read_bytes();rgbx=b''.join(col[8+i*3:11+i*3]+b'\0' for i in range(256));assert rgbx==self.paletteRgbx;mu.mem_write(SOURCE,rgbx);self.call(0x439de0,PALETTE,struct.pack('<I',SOURCE),4,precision,PALETTE)
  mu.mem_write(OBJECT,b'\xa5'*0x68);mu.mem_write(PARTICLES,b'\x5a'*2880);mu.mem_write(PATH,b'\xa5'*64);mu.mem_write(SOUND,b'shortZap.wav\0');mu.mem_write(MERGED,struct.pack('<2I',11,MERGED+0x1000));mu.mem_write(MERGED+0x1000,struct.pack('<11I',*range(31,42)))
  # 공통등록과 공유 팔레트의 실제 초기 입력이다.
  for address,values in ((0,(0x87654321,)),(0x5c8464,(POOL,)),(0x5410bc,(19,)),(0x55a758,(MERGED if row[1] else 0,)),(0x595138,(TABLE if self.capacity else 0,self.capacity)),(0x59ab20,(TYPES,)),(0x541348,(188,)),(0x59a91c,tuple(row[12:14]))):mu.mem_write(address,struct.pack('<'+'I'*len(values),*values))
  mu.mem_write(0x59a924,struct.pack('<2f',16,11));mu.mem_write(POOL,bytes(2700))
  # 부모는 원본 깊이와 실제 등록 중 변경을 관측하기 위한 입력이다.
  for sid,typeid,x,y,z in ((0,149,1.5,1.5,255),(50,148,20.5,21.25,depth),(51,144,-7.75,8.25,127),(52,149,40.125,35.375,0)):
   raw=bytearray(50);raw[10]=typeid;raw[35]=z;struct.pack_into('<H',raw,6,1);struct.pack_into('<2f',raw,14,x,y);mu.mem_write(POOL+sid*50,bytes(raw))
  initial=slots(self.capacity,hole)
  if initial:mu.mem_write(TABLE,struct.pack('<'+'I'*len(initial),*initial))
  self.events=[]
  if row[0]==0:self.call(0x471e70,OBJECT,struct.pack('<7I',self.parent,*row[8:12],row[14],SOUND if row[15] else 0),28,precision,OBJECT);self.returns['constructor']+=1
  else:self.call(0x472770,0,struct.pack('<6I',self.parent,*row[8:12],row[14]),0,precision);self.returns['wrapper']+=1
  assert self.integer(0)==0x87654321
  if not row[16]:
   assert bytes(mu.mem_read(OBJECT,0x68))==b'\xa5'*0x68 and bytes(mu.mem_read(PATH,64))==b'\xa5'*64
   return (rgbx.hex(),'-','-','-','-',self.snapshot(),';'.join(self.events),'-',self.snapshot(),'-','-')
  raw=bytes(mu.mem_read(OBJECT,0x68));path=bytearray(mu.mem_read(PATH,64));assert struct.unpack_from('<I',path,60)[0]==OBJECT;struct.pack_into('<I',path,60,1)
  # wrapper의 원본 shortZap 리터럴 주소만 같은 의미 토큰으로 정규화한다. 실행 메모리는 바꾸지 않는다.
  copy=bytearray(raw)
  if row[0]==1:assert struct.unpack_from('<I',copy,0x60)[0]==0x50c2a4;struct.pack_into('<I',copy,0x60,SOUND)
  before=semantic(copy).hex();palette=bytes(mu.mem_read(MERGED+0x1000,44));registered,events=self.snapshot(),';'.join(self.events)
  assert raw[0x40:0x48]==b'\xa5'*8 and raw[0x4c:0x50]==b'\xa5'*4 and raw[0x64:]==b'\xa5'*4 and bytes(mu.mem_read(PARTICLES,2880))==bytes(2880)
  self.events=[];self.call(0x471df0,OBJECT,struct.pack('<I',row[7]),4,precision,OBJECT);self.returns['destructor']+=1;after=bytearray(mu.mem_read(OBJECT,0x68))
  assert after[0xc:]==raw[0xc:] and self.integer(0)==0x87654321 and bytes(mu.mem_read(MERGED+0x1000,44))==palette and bytes(mu.mem_read(SOURCE,1024))==rgbx
  if row[0]==1:struct.pack_into('<I',after,0x60,SOUND)
  return rgbx.hex(),before,path.hex(),bytes(mu.mem_read(PARTICLES,2880)).hex(),palette.hex(),registered,events,semantic(after).hex(),self.snapshot(),';'.join(self.events),'-'

def main():
 """독립 기대값을 생성하거나 기존SHA/입력·정상반환만 검사한다."""
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
 if args.verify:
  report=json.loads(REPORT.read_text('utf-8'));assert report['paletteBoundary']==palette_evidence()[2];rows=[r.split('\t') for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')]
  for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
  assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'];assert [i for i,r in enumerate(rows) if r[1:12]!=r[12:23]]==report['precisionDifferentInputs'];print('화면좌표빔 원본 감사',len(rows));return
 oracle=Oracle();rows=[];different=[]
 # 두정밀도 결과는 각각 실제 원문에서만 얻는다.
 for i,c in enumerate(cases()):
  try:outputs=[oracle.sample(c,p) for p in (0x27f,0x37f)]
  except Exception:print('실패입력',i,c,flush=True);raise
  if outputs[0]!=outputs[1]:different.append(i)
  rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
 FIXTURE.write_text('# input rgbx53 fields53 path53 particles53 palette53 registered53 events53 destroyed53 unregistered53 destroyEvents53 timer53 rgbx64 fields64 path64 particles64 palette64 registered64 events64 destroyed64 unregistered64 destroyEvents64 timer64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
 paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','originals/d/GIFCLOUD.COL','extracted/refined/originals/functions.tsv','tools/decomp_lightning_screen.py','tools/decomp_lightning_lifetime.py','tools/decomp_lightning_path.py','tools/decomp_fierytail_lifetime.py','tools/decomp_particle_gradient.py','cpppj/lightning-lifetime.json','cpppj/tests/fixtures/lightning-lifetime-x86.tsv','cpppj/tests/fixtures/lightning-screen-x86.tsv')
 report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=dict(oracle.returns),instructions=oracle.instructions,nativeCalls=dict(oracle.calls),precisionDifferentInputs=different,paletteBoundary=oracle.paletteFacts,boundaries='471e70/471ca0/472770/418070 전체와기존공통등록/경로소멸전체. 동일GIFCLOUD 팔레트는기존독립원문과입력이같음을확인해명시경계로재사용한다. new/free/Host부착및double wall공급도명시경계다. wrapper 좌표는현재camera signedDWORD감김뒤float저장이다. shortZap 리터럴/팔레트/경로/목록포인터만의미토큰화한다. GP/retN/SEH/canary/x87/실행·읽기·쓰기범위를검사했다. 전체Frame/Draw/게임창/자연플레이검증이아니다.',sha256={p:digest(ROOT/p) for p in paths})
 REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','precisionDifferentInputs')},ensure_ascii=False))
if __name__=='__main__':main()
