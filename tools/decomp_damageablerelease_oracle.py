#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 세 PE의 Damageable 해방 구간을 실행한다. 원본 게임/OS/창 실행은 없다.

좌표·중심·스냅·genus·contained 순회·WORD 쓰기는 실제 명령이다.
사제 자리 탐색/생성·일반 생성·가상 소유자/Pop·표면 검사와 기존 효과/소리만 기록 대체한다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_damageablepredestroy_oracle import DamageableOracle, PRE, CONTROLS, IDS, SPOTS, bits
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, STOP, TARGET, digest, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

# 별도 근거와 새 객체 관찰 슬롯, 합성 가상 표/함수 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/damageablerelease-x86.tsv'
REPORT=ROOT/'cpppj/recovery-damageablerelease-evidence.json'
NEW_IDS=(100,101,102,103)
VTABLE,OWNER,POP=STOP+0x100,STOP+0x300,STOP+0x400
# 실제 하위 경계와 현재 전역 주소다. bridge WORD 쓰기는 대체하지 않는다.
RELEASE={
 'originals':dict(spawn=0x44b2e0,create=0x4af530,notify=0x4214a0,allowed=0x542640,bridge=0x5411a0,
    center=0x4adea0,snap=0x41d7a0,word=0x4ac220),
 'originalCD':dict(spawn=0x461430,create=0x4ab390,notify=0x448c10,allowed=0x5178c4,bridge=0x51ca8c,
    center=0x4aef30,snap=0x440a80,word=0x4abb00),
}
RELEASE['original1037']=dict(RELEASE['originalCD'])


def inputs():
    """조건의 기대 결과 없이 좌표·발자국·공간·현재 전역의 독립 입력을 교차한다."""
    cases=[]
    # 사제/일반/다리·후보 free/dead 상태·하위 0x20 비트·필터 제외를 함께 공급한다.
    chains=('60,6,1,61,158,32;61,6,2,62,82,32;62,6,8,63,90,32;63,6,0,0,65535,0',
        '60,6,0,61,90,32;61,6,1,62,158,32;62,46,8,63,82,32;63,6,4,0,82,32',
        '60,6,9,61,82,32;61,6,0,62,90,288;62,6,2,63,158,32;63,6,8,0,90,0')
    # 공간 셀은충분히 넓은 같은 값 영역으로 만들되 미세 경계 입력을 별도로 둔다.
    for foot,pos,battle,allowed,spot,genus in itertools.product(((1,1),(2,4),(3,3)),
        ((20,21),(20.00001,21.00001),(20.0001,21.0001),(20.75,21.9)),(0,1),(0,0x40480002,0xffffffff),(0,2,6,16,22),(0,0x80400000)):
        index=len(cases)
        marks=';'.join(f'{x},{y},{spot}' for x in range(18,23) for y in range(18,24))
        cases.append(dict(state=2,extra=0,boss=1,type=6,priest=158,flags=0,x=bits(pos[0]),y=bits(pos[1]),
            marks=marks,head=60,nodes=chains[index%3],style=0,footx=foot[0],footy=foot[1],genus=genus,battle=battle,allowed=allowed,change=0))
    # 콜백 중 현재 후보 WORD/next·부모 좌표·타입 전역/공간·다음 후보의 내용을 바꾼다.
    for i in range(32):
        case=dict(cases[(i*23)%len(cases)],battle=1,allowed=0xffffffff,change=1+i%2,
            marks='20,21,6;21,22,6;19,19,6;19,20,6;20,20,6',flags=(0,0x800,0x300000,0x300800)[(i//2)%4])
        cases.append(case)
    return cases


class ReleaseOracle(DamageableOracle):
    """기존 접두 실행기에 실제 해방 코드와 하위 효과 경계만 더한다."""
    def __init__(self,edition):
        """읽기 전용 내보내기 범위와 합성 가상 호출 표를 준비한다."""
        super().__init__(edition);self.r=RELEASE[edition];self.region_entries=0
        paths=[ROOT/f'extracted/damageablerelease/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with paths[1].open(encoding='utf-8') as fp:
            # Ghidra가 정한 실제 몸체 범위만 실행한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.exports+=paths
        self.mu.mem_write(VTABLE+0x74,struct.pack('<I',OWNER));self.mu.mem_write(VTABLE+0x90,struct.pack('<I',POP))

    def prepare_release(self):
        """기존 실행기가 raw를 준비한 직후 추가 입력만 설정한다."""
        case=self.case;self.created=0;self.changed=False
        self.mu.mem_write(self.g['battle'],struct.pack('<I',case['battle']))
        self.mu.mem_write(self.r['allowed'],struct.pack('<I',case['allowed']));self.mu.mem_write(self.r['bridge'],struct.pack('<I',82))
        # 부모와 후보의 genus/발자국은 타입 표의 실제 필드에 기록한다.
        for typ,genus in ((160,case['genus']),(158,0x200000),(82,2),(90,0x40000000)):
            address=TYPES+typ*self.type_stride;self.mu.mem_write(address,bytes(self.type_stride))
            self.mu.mem_write(address+0xec,struct.pack('<I',genus))
            offset=0x1d4 if self.edition=='originals' else 0x1b4
            self.mu.mem_write(address+offset,struct.pack('<ii',case['footx'] if typ==160 else 1,case['footy'] if typ==160 else 1))
        self.mu.mem_write(self.slot(TARGET)+10,bytes((160,)))
        # WORD는 소유자 바이트와 별개인 값이며 상위 비트도 유지한다.
        for sid in IDS[1:]:self.mu.mem_write(self.slot(sid)+12,struct.pack('<H',0x8100+sid))
        for sid in NEW_IDS:
            self.mu.mem_write(self.slot(sid),bytes([0xcd])*self.stride)
            self.write_ranges.append((self.slot(sid)+12,self.slot(sid)+14))

    def mutate_release(self,marker):
        """첫 생성 경계에서만 입력 변경을 공급하며 판단/기대 계산을 하지 않는다."""
        change=self.case['change']
        if not change or self.changed:return
        self.changed=True
        if change==1:
            self.mu.mem_write(self.slot(60)+12,struct.pack('<H',0xfedc));self.mu.mem_write(self.slot(60)+4,bytes(2))
            self.set_coords(23.25,24.75);self.mu.mem_write(self.p['boss'],bytes(4))
            self.mu.mem_write(self.slot(61)+18,struct.pack('<HH',158,32));self.mu.mem_write(self.r['allowed'],struct.pack('<I',0xffffffff))
        else:
            self.mu.mem_write(self.g['battle'],bytes(4));self.mu.mem_write(self.r['allowed'],bytes(4))
            self.mu.mem_write(self.p['type'],struct.pack('<I',46));self.mu.mem_write(self.slot(62)+10,bytes((46,)))
            self.mu.mem_write(self.r['bridge'],struct.pack('<I',90));self.mu.mem_write(SPOTS+21*256+20,bytes((16,)))

    def on_instruction(self,mu,address,size,data):
        """구간 건너뛰기 없이 실행하며 명시한 하위 함수만 기록한다."""
        if address==self.p['entry'] and not self.ready:self.prepare_release();self.ready=True
        if address==self.p['release']:
            self.region_entries+=1;OwnerOracle.on_instruction(self,mu,address,size,data);return
        esp=mu.reg_read(UC_X86_REG_ESP);r=self.r
        if address==r['spawn']:
            x,y,kind,word=struct.unpack('<4I',mu.mem_read(esp+4,16));self.events.append(f'P:{x}:{y}:{kind}:{word}')
            self.stub_calls['P']+=1;self.mutate_release('P');self.ret();return
        if address==r['create']:
            kind,flags=struct.unpack('<II',mu.mem_read(esp+4,8));sid=NEW_IDS[self.created];self.created+=1
            if flags:raise RuntimeError('생성 flags 오류')
            raw=bytearray([0xcd]*self.stride);raw[:4]=struct.pack('<I',VTABLE);raw[10]=kind;raw[11]=0
            self.mu.mem_write(self.slot(sid),bytes(raw));mu.reg_write(UC_X86_REG_EAX,self.slot(sid))
            self.events.append(f'N:{kind}:{flags}:{sid}');self.stub_calls['N']+=1;self.mutate_release('N');self.ret();return
        if address==r['notify']:
            sid=struct.unpack('<I',mu.mem_read(esp+4,4))[0];self.events.append(f'S:{sid}');self.stub_calls['S']+=1;self.ret();return
        if address in (OWNER,POP):
            pointer=mu.reg_read(UC_X86_REG_ECX);sid=(pointer-self.slot(0))//self.stride
            if sid not in NEW_IDS or pointer!=self.slot(sid):raise RuntimeError('가상 호출 this 오류')
            if address==OWNER:
                owner=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
                if owner:raise RuntimeError('해방 소유자 오류')
                self.events.append(f'O:{sid}:{owner}');self.stub_calls['O']+=1;self.ret(4)
            else:
                x,y,flags=struct.unpack('<3I',mu.mem_read(esp+4,12))
                if flags:raise RuntimeError('해방 Pop flags 오류')
                self.events.append(f'L:{sid}:{x}:{y}:{flags}');self.stub_calls['L']+=1;self.ret(12)
            return
        super().on_instruction(mu,address,size,data)

    def run(self,case,mode,control):
        """전체 반환/보존 검사를 유지하며 생성 슬롯과 현재 전역을 추가 관찰한다."""
        self.ready=False;result=super().run(case,mode,control)
        globals_out=','.join(str(struct.unpack('<I',self.mu.mem_read(address,4))[0]) for address in (self.g['battle'],self.r['allowed'],self.r['bridge']))
        return result+[globals_out,'|'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in NEW_IDS)]


def generate(smoke=False):
    """두 x87 정밀도와 세 PE의 실제 관찰을 독립 fixture로 저장한다."""
    cases=inputs();rows=[];editions={}
    paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/damageablerelease-functions.json',FIXTURE}
    if smoke:cases=cases[::37]
    for edition in SPECS:
        oracle=ReleaseOracle(edition)
        # 직접 Damageable와 실제 Carrier 두 전체 경로를 실행한다.
        for case in cases:
            for mode in ('D','C'):
                values=[oracle.run(case,mode,control) for control in CONTROLS]
                if values[0]!=values[1]:raise RuntimeError('해방 x87 정밀도 차이')
                rows.append([edition,mode,*[case[key] for key in ('state','extra','boss','type','priest','flags','x','y','marks','head','nodes','style','footx','footy','genus','battle','allowed','change')],*values[0]])
        editions[edition]=dict(cases=2*len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions,regions_executed=oracle.region_entries)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 해방 {2*len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 해방 좌표/타입/WORD/순회. 생성·가상 소유자/Pop·표면 검사 등 하위 함수만 대체.\n# edition mode state extra boss type priest flags x y marks head nodes style footx footy genus battle allowed change globals events slots releaseGlobals newSlots\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',normal_return=True,os_calls=0,controls=list(CONTROLS),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['해방 중간 구간 대체 없음; 중심/스냅/genus/WORD/contained 순회 실제 실행',
            '사제 자리 탐색/생성·일반 생성·가상 소유자/Pop·표면 검사와 기존 효과/소리/공통 body는 함수 진입 대체',
            '일반 Pop·생성 자산의 공간 수명·GUI/소리 출력 완성을 뜻하지 않음']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·정상 반환·실제 구간/WORD 실행 및 각 외부 호출 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    if len(rows)!=report['total'] or not report['normal_return'] or report['os_calls'] or report['controls']!=list(CONTROLS):raise RuntimeError('해방 근거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];p=PRE[edition];r=RELEASE[edition];n=len(selected)
        if n!=item['cases'] or item['assertions'] or item['native_calls'].get(f'{p["entry"]:08x}')!=2*n:raise RuntimeError('실제 진입 오류')
        expected_regions=2*sum(not(int(row[3])&9) and bool(int(row[4])) and not(int(row[7])&0x800) for row in selected)
        if item['substitutions'].get('R-region',0) or item['regions_executed']!=expected_regions or not expected_regions:raise RuntimeError('해방 구간 대체/진입 오류')
        if item['native_calls'].get(f'{p["carrier"]:08x}')!=n or item['substitutions'].get('B')!=2*n:raise RuntimeError('Carrier/공통 호출 오류')
        for key in ('center','snap','word'):
            if not item['native_calls'].get(f'{r[key]:08x}'):raise RuntimeError(f'실제 보조 함수 누락: {key}')
        for marker in ('C','X','A0','A1','F','B','T','P','N','S','O','L'):
            count=2*sum(sum(part.split(':')[0]==marker for part in row[21].split(';')) for row in selected)
            if count!=item['substitutions'].get(marker,0):raise RuntimeError(f'하위 경계 수 오류: {marker}')
        if item['native_calls'].get(f'{r["word"]:08x}')!=item['substitutions'].get('N'):raise RuntimeError('생성 후 실제 WORD 복사 누락')
        if any(item['substitutions'].get(marker)!=item['substitutions'].get('N') for marker in ('O','L')):raise RuntimeError('생성 뒤 가상 호출 누락')
        if any(item['native_calls'].get(f'{r[key]:08x}',0) for key in ('spawn','create','notify')):raise RuntimeError('하위 대체/실제 실행 혼합')
        if any(item['native_calls'].get(f'{p[key]:08x}',0) for key in ('collapse','explosion','at','free','base','found')):raise RuntimeError('기존 하위 대체/실제 실행 혼합')
    print(f'damageablerelease 검증 통과: {len(rows)}개')


def main():
    """무저장 표본/전체 생성/저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
