#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Bomb 고유41e960의 원문관문·효과 뒤 현재좌표·공통호출을 독립 실행한다."""
import argparse,itertools,json,socket,struct,collections
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,POOL,digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX
FIXTURE=ROOT/'cpppj/tests/fixtures/bomb-predestroy-x86.tsv' # 새원문이며기존기대값은수정하지않는다.
REPORT=ROOT/'cpppj/bomb-predestroy.json' # 실제정상반환/경계/보호SHA다.
# 원문입력만교차한다.
def cases():return list(itertools.product((0,1,8,9,16,255),(0,0x100000,0x100001,0xffffffff),range(4)))
class Oracle(BaseOracle):
 """큰폭발/위치소리/공통삭제만경계이며관문과좌표조회는PE가수행한다."""
 def __init__(self):
  """실제본문범위와명시raw/스택쓰기만허용한다."""
  super().__init__();self.allowed=[];ranges(self,(0x41e960,));self.instructions=0
 def instruction(self,mu,address,size,data):
  """실제효과를기록하고효과입력에따른현재raw만바꾼다."""
  self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<4I',mu.mem_read(sp+4,16));raw=POOL+50*50
  if address==0x4605d0:
   self.event('E',a[0])
   if self.case[2]==1:self.put(raw+14,0x42cb0000);self.put(raw+18,0xc0880000)
   if self.case[2]==2:self.put(raw+40,9,1);self.put(raw,0x501dd0)
   self.ret();return
  if address==0x4a9d70:
   assert self.string(a[2])=='explosion.wav';self.event('S',a[0],a[1],a[3])
   if self.case[2]==3:self.put(raw+40,0,1);self.put(raw+12,0xbeef,2)
   self.ret();return
  if address==0x4b0950:self.event('B',(mu.reg_read(UC_X86_REG_ECX)-POOL)//50,a[0]);self.ret(0,4);return
  if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'Bomb 범위밖명령 {address:x}')
 def write(self,mu,access,address,size,value,data):
  """본문의실제직접쓰기는스택뿐이다. raw변경은명시효과경계다."""
  if STACK-0x7f000<=address and address+size<=STACK+0x1000:return
  raise RuntimeError(f'Bomb 범위밖쓰기 {address:x}+{size}')
 def sample(self,c,precision):
  """원문입력raw를제공하고전체50바이트와호출순서를읽는다."""
  self.case=c;self.events=[];raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x502370);raw[10]=113;raw[11]=2;raw[40]=c[0];struct.pack_into('<ff',raw,14,98.,122.);self.mu.mem_write(POOL+50*50,bytes(raw));self.put(0x5c8464,POOL);self.put(0,0x87654321)
  self.call(0x41e960,POOL+50*50,struct.pack('<I',c[1]),4,precision);self.returns+=1;assert self.get(0)==0x87654321
  return ';'.join(self.events),bytes(self.mu.mem_read(POOL+50*50,50)).hex()
def main():
 """독립원문기대값을생성하거나현재SHA만감사한다."""
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
 if args.verify:
  r=json.loads(REPORT.read_text('utf-8'));rows=[v.split('\t') for v in FIXTURE.read_text('utf-8').splitlines() if v and not v.startswith('#')]
  for path,wanted in r['sha256'].items():assert digest(ROOT/path)==wanted,path
  assert len(rows)==r['uniqueInputs'] and len(rows)*2==r['normalReturns'] and [v[0] for v in rows]==[','.join(map(str,c)) for c in cases()] and all(v[1:3]==v[3:5] for v in rows);print('Bomb 고유전처리 원문감사',len(rows));return
 o=Oracle();rows=[];different=[]
 # 각정밀도의기대값은실제PE만실행해서읽는다.
 for i,c in enumerate(cases()):
  outputs=[o.sample(c,p) for p in (0x27f,0x37f)]
  if outputs[0]!=outputs[1]:different.append(i)
  rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
 FIXTURE.write_text('# input effects53 raw53 effects64 raw64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
 paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_bomb_predestroy.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','cpppj/tests/fixtures/bomb-predestroy-x86.tsv')
 r=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=o.returns,instructions=o.instructions,precisionDifferentInputs=different,boundaries='41e960 전체. 4605d0 폭발/4a9d70 위치소리/4b0950 큰공통전처리는명시효과경계다. GP/retN/SEH/canary/x87/실행·쓰기범위를검사했다. 게임/창/전체삭제검증이아니다.',sha256={p:digest(ROOT/p) for p in paths});REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:r[k] for k in ('uniqueInputs','normalReturns','instructions','precisionDifferentInputs')},ensure_ascii=False))
if __name__=='__main__':main()
