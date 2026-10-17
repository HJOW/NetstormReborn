#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 오벨리스크 보상·변환과 Deck 추첨·벽시각 난수의 전체 몸체를 격리 실행한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,VTABLE,digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ,UcError
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
POOL=0x13000000 # 보호 슬롯과 분리된 50바이트 객체 풀이다.
DECK,OUTPUT=STOP+0x400,STOP+0x580 # 실제 Deck 352바이트와 두 반환 DWORD다.
POP,DESTROY,OTHER_DESTROY=STOP+0x600,STOP+0x620,STOP+0x640 # 현재 가상 효과의 서로 다른 실행 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/gem-reward-x86.tsv' # C++와 독립인 새 원문 관측이다.
REPORT=ROOT/'cpppj/gem-reward.json' # 실제 호출 수·정밀도·경계·보호 SHA다.
ENTRIES=(0x461f00,0x4620c0,0x44f200,0x44f250,0x44f280,0x44f290,0x44f510,0x4bcab0,0x4bca20,0x4558c0,0x49a840,0x41c7a0,0x4acd10,0x425830,0x4e49c0,0x44f160) # 실제 전체 몸체16개다.
# 실제 double 입력을 두 DWORD로만 직렬화한다.
def wall(value):return struct.unpack('<II',struct.pack('<d',value))
# 입력만 생성하며 기대 선택 결과나 효과 순서는 계산하지 않는다.
def cases():
 rows=[];zero=(-3*pow(0x10003,-1,1<<32))&0xffffffff
 base=[0,0,0,1,*wall(12.5),0,112,0,0,0,1,1,0]
 # 현재 타입 표/제외 목록/게임 난수/벽시각의 독립 소비를 교차한다.
 for variant,mask,seed,time in itertools.product(range(6),(0,1,3,15,63,127),(0,1,0xffffffff,zero),(0.,12.5,1000000.125)):
  r=base.copy();r[1:6]=variant,mask,seed,*wall(time);rows.append(r)
 # 현재 QA가 유효하면 추첨·제외·레벨 조건을 다시 적용하지 않는다.
 for qa,kind,status,variant in itertools.product((0,70,71,74,75,76,65535),(112,111),(0,1,2,4),range(4)):
  r=base.copy();r[0]=1;r[6:9]=qa,kind,status;r[1]=variant;rows.append(r)
 # 직접 Deck의 signed power/합계 재계산/미개방/출력포인터와 래핑을 관측한다.
 for variant,seed,flags,power,time in itertools.product(range(8),(0,1,0xffffffff),(0,1,0x80000001),(0,1),(0.,-1.,12.5,2147483648.)):
  r=base.copy();r[0]=2;r[10:14]=variant,seed,flags,power;r[4:6]=wall(time);rows.append(r)
 # 효과가 현재 필드·다음 타입·가상 표를 바꾸는 입력이다.
 for op,mutation in itertools.product(range(3),range(1,7)):
  r=base.copy();r[0]=op;r[9]=mutation
  if mutation==1:r[1]=2
  if mutation==2:r[10]=2
  if mutation==6:r[7]=111;r[6]=76
  rows.append(r)
 return list(dict.fromkeys(tuple(r) for r in rows))

class Oracle(BaseOracle):
 """게임/OS/창을 시작하지 않고 큰 월드와 벽시각 입력만 외부 효과로 둔다."""
 def __init__(self):
  """보호 이미지를 읽기 전용 근거로 적재하고 전체 실행 범위와 자료 영역을 제한한다."""
  super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.calls=collections.Counter();self.instructions=0;self.faults=0
  self.put(VTABLE+0x90,POP);self.put(VTABLE+0x10,DESTROY);self.put(VTABLE+0x100+0x10,OTHER_DESTROY)
  self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
 def instruction(self,mu,address,size,data):
  """실제 명령을 세며 벽시각/진단/로그/생성·가상 효과의 인자를 보관한다."""
  self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<5I',mu.mem_read(sp+4,20));this=mu.reg_read(UC_X86_REG_ECX);c=self.case
  if address in ENTRIES:self.calls[f'{address:08x}']+=1
  if address==0x44f200:self.deck=this
  if address==0x460d70:
   self.event('W',c[4],c[5]);self.mu.mem_write(a[0],struct.pack('<II',*c[4:6]))
   if c[9]==3:self.put(self.deck+2,0,1)
   self.ret(a[0]);return
  if address==0x4e0620:
   assert a[2] in (0xe5,0xf6,0x104,0x10a,0x39,0x3a,0x3b,0x3c,0x3d),hex(a[2]);self.event('I',a[2])
   if c[9]==1 and a[2]==0xf6:self.put(0x541348,72);self.put(TYPES+71*500+0x94,3)
   if c[9]==6 and a[2]==0x10a:self.put(POOL+50*50+10,112,1);self.put(POOL+50*50+12,70,2)
   self.ret();return
  if address==0x4c2630:
   assert a[0]==0x1d;self.event('M',a[2],a[3])
   if c[9]==2:self.put(self.deck+0x14c,0);self.put(self.deck+0x150,999)
   self.ret();return
  if address==0x4af530:
   self.event('C',a[0],a[1]);self.put(POOL+61*50+10,a[0],1)
   if c[9]==4:self.mu.mem_write(POOL+50*50+14,struct.pack('<ff',101.5,-4.25))
   self.ret(POOL+61*50);return
  if address==POP:
   self.event('P',(this-POOL)//50,*a[:3])
   if c[9]==5:self.put(POOL+50*50,VTABLE+0x100)
   self.ret(0,12);return
  if address in (DESTROY,OTHER_DESTROY):
   self.event('D' if address==DESTROY else 'E',(this-POOL)//50,a[0]);self.put(this+11,1,1);self.ret(0,4);return
  if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'보상 미검토 실행 {address:08x}')
 def write(self,mu,access,address,size,value,data):
  """원문 명령은 스택/임시 Deck/반환값/같은 게임 난수만 쓴다."""
  if STACK-0x7f000<=address and address+size<=STACK+0x1000 or DECK<=address and address+size<=DECK+352 or OUTPUT<=address and address+size<=OUTPUT+8 or address==0x532710 and size==4:return
  raise RuntimeError(f'보상 범위 밖 쓰기 {address:08x}+{size}')
 def read(self,mu,access,address,size,value,data):
  """실제 원문은 보호 이미지와 지정한 격리 자료만 읽는다."""
  spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
  if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'보상 범위 밖 읽기 {address:08x}+{size}')
 def sample(self,c,precision):
  """명시 초기 입력을 배치하고 실제 반환·효과·raw·Deck·난수를 읽는다."""
  self.case=c;self.events=[];self.deck=DECK;self.mu.mem_write(STACK-4096,b'\xa5'*4096);self.mu.mem_write(DECK,b'\xa5'*352);self.mu.mem_write(OUTPUT,b'\x55'*8)
  self.mu.mem_write(TYPES,bytes(0x20000));self.mu.mem_write(0x52f7e0,bytes(87))
  # 현재 타입과 수준은 입력이며 후보 채택/가중치 계산은 실제 PE가 수행한다.
  for index,level in enumerate((0,1,2,3,98,99)):
   self.put(TYPES+(70+index)*500+0xec,0 if c[1]==1 else 0x100);self.put(TYPES+(70+index)*500+0x94,level)
  if c[1]==2:self.put(TYPES+70*500+0x94,0xffffffff)
  if c[1]==3:self.put(TYPES+0xec,0x100);self.put(TYPES+0x94,3)
  if c[1]==4:self.put(TYPES+70*500+0xec,0x2100)
  excluded=[70+i for i in range(6) if c[2]&(1<<i)]+([0] if c[2]&64 else [])
  self.put(0x52f7e0+22,len(excluded),1);self.mu.mem_write(0x52f7e0+23,bytes(excluded))
  # 원객체와 생성객체의 전체50바이트는 독립 초기 입력으로 보관한다.
  for sid in (50,61):
   raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=c[7] if sid==50 else 70;raw[11]=c[8] if sid==50 else 0;struct.pack_into('<H',raw,12,c[6]);struct.pack_into('<ff',raw,14,98.,122.);self.mu.mem_write(POOL+sid*50,bytes(raw))
  for a,v in ((0x541348,73 if c[1]==5 else 188),(0x541218,112),(0x532710,c[3]),(0x5c8464,POOL),(0x59ab20,TYPES),(0x541264,70),(0x54125c,71),(0x541288,72),(0x541290,73)):self.put(a,v)
  self.put(0,0x87654321)
  if c[0]==2:
   entries=[(70,20,1,255,254),(71,30,1,255,0),(74,50,1,255,255)]
   if c[10]==1:entries[0]=(70,20,0,255,0)
   if c[10]==2:entries[0]=(70,20,255,255,0)
   if c[10]==3:entries=[]
   if c[10]==4:entries=[(70,0,1,255,0),(71,0,1,255,0)]
   if c[10]==5:entries=[(70,255,127,255,255)]
   if c[10]==6:entries=[(70,20,128,255,0),(71,30,1,255,0)]
   # 입력Deck만 배치하고 실제 순회·합계·선택은 원문이 계산한다.
   for i,e in enumerate(entries):self.mu.mem_write(DECK+i*5,bytes(e))
   self.put(DECK+0x14c,0xffffffff if c[10]==7 else len(entries));self.put(DECK+0x150,999 if c[10] in (1,2) else sum(e[1] for e in entries));self.put(DECK+0x154,0xffffffff);self.put(DECK+0x158,c[11]);self.put(DECK+0x15c,c[12])
   self.call(0x44f510,DECK,struct.pack('<4I',OUTPUT,OUTPUT+4 if c[13] else 0,0,0),16,precision);result=self.get(OUTPUT)
  else:
   try:
    self.call(0x461f00 if c[0]==0 else 0x4620c0,0,struct.pack('<I',5 if c[0]==0 else 50),0,precision);result=self.mu.reg_read(UC_X86_REG_EAX)
   except UcError:
    # 실제ffff QA는TYPE 표 밖을읽는462110에서중단된다. 정상반환으로세지않는다.
    assert c[0]==1 and c[6]==65535 and self.mu.reg_read(UC_X86_REG_EIP)==0x462110;self.faults+=1;result='fault'
  assert self.get(0)==0x87654321
  if result!='fault':self.returns+=1
  return str(result),';'.join(self.events) or '-',str(self.get(0x532710)),bytes(self.mu.mem_read(self.deck,352)).hex(),','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,61)),str(self.get(OUTPUT+4)),str(self.get(0x541348))

def main():
 """독립 원문을 생성하거나 기존 기대값의 바이트와 전체 입력만 감사한다."""
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
 if args.verify:
  report=json.loads(REPORT.read_text('utf-8'));rows=[l.split('\t') for l in FIXTURE.read_text('utf-8').splitlines() if l and not l.startswith('#')]
  for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
  assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'];assert len(rows)*2==report['normalReturns']+report['invalidQaReads'];assert [i for i,r in enumerate(rows) if r[1:8]!=r[8:15]]==report['precisionDifferentInputs'];print('오벨리스크 보상 원본 감사',len(rows));return
 oracles=[Oracle(),Oracle()];rows=[];different=[]
 # C++ 출력은 읽지 않고 각 정밀도의 실제 원문만 실행한다.
 for i,c in enumerate(cases()):
  try:outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
  except Exception:
   print('실패입력',i,c,'원본EIP',[hex(o.mu.reg_read(UC_X86_REG_EIP)) for o in oracles],flush=True);raise
  if outputs[0]!=outputs[1]:different.append(i)
  rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
 FIXTURE.write_text('# input result53 effects53 random53 deck53 raws53 power53 count53 result64 effects64 random64 deck64 raws64 power64 count64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
 paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/functions.tsv','tools/decomp_gem_reward.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','cpppj/tests/fixtures/gem-reward-x86.tsv')
 report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),invalidQaReads=sum(o.faults for o in oracles),invalidQaBoundary='QA65535의462110 TYPE+ec 읽기는원본의물리표밖접근이다. 호스트는같은범위입력을읽기전거부한다.',instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,wallSeedMultiplier=1013.0,boundaries='16개 전체몸체. wall460d70은호출시점double 공급경계이며진단/로그/큰생성·가상Pop·Destroy만분리한다.현재TYPEgetter/제외목록/전역및임시난수/ftol/Deck전체를실행했다. GP/retN/SEH/canary/x87/실행·읽기·쓰기범위를검사했다. 실제게임/OS/창·전체플레이검증이아니다.',sha256={p:digest(ROOT/p) for p in paths})
 REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))
if __name__=='__main__':main()
