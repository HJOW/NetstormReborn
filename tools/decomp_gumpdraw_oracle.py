#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 Gump Draw/클립 보호 전체 몸체를 실행한다. 가상 Draw와 월드 투영만 입력 경계다.

게임/GUI/OS 실행 없이 실제 위치/CRT/클립 설정/복구, 중첩 호출, 허용 쓰기와 ABI를 관찰한다.
--smoke는 무저장 표본, --verify는 저장 근거의 SHA/입력/진입/반환/경계 집계 감사다.
"""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_gumpcollection_oracle import GumpCollectionOracle, node, raw, token, DATA, POOL, NODE_SIZE
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ROOT, BINARIES, CONTROLS, STACK, STOP, STUBS, digest, fbits,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 저장 경로/독립 입력 시드/원본 플래그와 가상 Draw 응답의 격리 진입점이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/gumpdraw-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gumpdraw-evidence.json'
SEED, CLIP_FLAG, VTABLE, VIRTUAL, RESUME=0x46631024,0x400000,DATA+0x9000,STUBS+0x600,STUBS+0x620
# 실제 Draw/화면 setter, half-open/inclusive 전역이다. 나머지 위치 정보는 앞 단계의 원본 배치를 쓴다.
SPECS={
    'originals':dict(Draw=0x466310,Set=0x4a1580,clip=0x5c7a44,vfx=0x5b5de0),
    'originalCD':dict(Draw=0x492aa0,Set=0x422e10,clip=0x552880,vfx=0x55bb1c),
}
SPECS['original1037']=dict(SPECS['originalCD'])


def plan():
    """경계/현재 플래그/가상 클립 변경/중첩/월드 부착의 완성 입력만 만든다."""
    result=[]
    # 응답은 현재 클립을 새로 설정하거나 다른 실제 Draw 루트를 한 번 호출한다. 기대 클립은 계산하지 않는다.
    def add(nodes,clip=(-11,3,40,70),mode=0,target=1,anchors=(),project_clip=None,vfx=(91,92,93,94)):
        """시나리오 입력은 모두 복사하며 가상 처리기의 새 클립도 독립 입력으로 고정한다."""
        result.append(dict(nodes=[dict(n) for n in nodes],clip=clip,vfx=vfx,mode=mode,target=target,
            change=(-7,9,17,21),anchors=list(anchors),project_clip=project_clip))
    clips=((-11,3,40,70),(0,0,0,0),(20,30,20,70),(30,40,10,20),
        (-2147483648,-2147483648,2147483647,2147483647),(0,0,-2147483648,-2147483648))
    rectangles=((2.75,8.5,12.75,9.5),(100,100,10,10),(-100,-100,10,10),
        (30,30,0,0),(10,15,-10,-20),(-.9,-1.9,.75,1.25))
    # visible/dead 및 인접 비트는 Draw 자체의 호출을 막지 않는다. 네 경계를 각각 제한하는 입력이다.
    for flags in (0,1,0x10000,0x200000,CLIP_FLAG,CLIP_FLAG|1,CLIP_FLAG|0x10000,0x800000):
        # 빈/역전/바깥/분수 Gump를 화면 클립 조합과 교차한다.
        for x,y,w,h in rectangles:
            # 현재 논리/VFX 전역이 서로 다를 때 보존/재설정 차이도 관찰한다.
            for clip in clips:
                add([node(0,x=fbits(.75),y=fbits(-.25)),node(1,flags=flags,parent=0,
                    x=fbits(x),y=fbits(y),width=fbits(w),height=fbits(h))],clip=clip)
    # 가상 처리기가 클립을 바꾸는 경우 flagless는 유지, 보호 분기는 저장 영역으로 복구한다.
    for flags in (0,CLIP_FLAG):
        # 각 경계 상태에서도 정상 반환은 끝까지 실행한다.
        for clip in clips: add([node(0),node(1,flags=flags,parent=0)],clip=clip,mode=1)
    rng=random.Random(SEED)
    # 다섯 부모의 float32 저장/CRT 절삭과 signed 범위의 clip을 함께 관찰한다.
    for index in range(80):
        nodes=[node(0,x=fbits(.75),y=fbits(-.75))]
        # 일반 부모 위치의 누적 반올림을 드러내는 큰/분수 좌표를 사용한다.
        for level in range(1,6):
            nodes.append(node(level,parent=level-1,flags=CLIP_FLAG if index%2 else 0,
                x=fbits(rng.choice((1,16777216,-16777216))*rng.randrange(-8,9)/3),
                y=fbits(rng.randrange(-500,501)/7),width=fbits(rng.randrange(-30,101)/9),height=fbits(rng.randrange(-30,101)/11)))
        add(nodes,target=5,mode=index%2,clip=clips[index%len(clips)])
    # 부모/자식 각각 임시 클립 유무를 바꿔 실제 중첩 루트의 저장/복구 순서를 확인한다.
    for flags in (0,CLIP_FLAG):
        # 자식의 visible/dead 여부와 관계없이 명시 가상 처리기가 실제 Draw를 호출한다.
        for child_flags in (0,CLIP_FLAG,CLIP_FLAG|0x10000):
            # 화면 밖 자식도 가상 Draw 경계에 도달한다.
            for x in (-100,4.75,100):
                # 서로 다른 부모 클립에서 자식이 보는 현재 영역을 관찰한다.
                for clip in clips[:4]:
                    add([node(0),node(1,flags=flags,parent=0,x=fbits(2.75),y=fbits(8.5)),
                        node(2,flags=child_flags,parent=1,x=fbits(x),y=fbits(.75))],clip=clip,mode=2)
    # 부착 자신/부착 부모의 int→float 판본 차이와 좌표 감김을 포함한다.
    for point in ((30,-20),(16777217,-16777217),(2147483647,-2147483648)):
        # 플래그가 없으면 월드 투영도 호출하지 않는다.
        for clipped in (False,True):
            nodes=[node(0,x=fbits(.75),y=fbits(-.75)),node(1,flags=0x2000|(CLIP_FLAG if clipped else 0),parent=0,
                x=fbits(-1.9),y=fbits(.9)),node(2,flags=CLIP_FLAG if clipped else 0,parent=1,
                x=fbits(-16777216),y=fbits(16777216))]
            # 자신/자식 Draw와 가상 클립 변경을 나눠 검사한다.
            for target in (1,2):
                # 투영 중 클립이 바뀌면 그 뒤의 보호 객체가 새 영역을 저장해야 한다.
                for project_clip in (None,(5,7,21,31)):
                    add(nodes,target=target,mode=1,anchors=[(1,*point)],project_clip=project_clip)
    return result


def encoded(case):
    """C++ 재생용 입력 열을 직렬화한다. raw 노드의 가상 표 주소만 입력에 추가한다."""
    nodes=[]
    # 같은 완성 raw 바이트를 C++와 PE에 전달한다.
    for index,n in enumerate(case['nodes']):
        value=bytearray(raw(n));struct.pack_into('<I',value,0,VTABLE);nodes.append(f'{token(index)}={value.hex()}')
    return [str(token(case['target'])),str(case['mode']),','.join(map(str,case['clip'])),','.join(map(str,case['vfx'])),
        ','.join(map(str,case['change'])),','.join(map(str,case['project_clip'])) if case['project_clip'] else '-',
        ';'.join(nodes),';'.join(','.join(map(str,a)) for a in case['anchors']) or '-']


class GumpDrawOracle(GumpCollectionOracle):
    """공유 PE/쓰기 검사기에 Draw 전용 허용 범위와 명시 가상 응답을 연결한다."""
    def __init__(self,edition):
        """같은 원본 위치 배치를 재사용하지만 실행 허용 범위는 이번 내보내기로 전부 교체한다."""
        super().__init__(edition);self.d.update(SPECS[edition]);self.allowed=[];self.entry_names={}
        self.extra=[ROOT/f'extracted/gumpdraw/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # 함수 사이의 주소는 실행하지 않으며 불연속 구간마다 원본 마지막 바이트까지 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # Ghidra가 내보낸 몸체만 실제 명령으로 실행한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.put(VTABLE+0x24,VIRTUAL);self.mu.mem_write(VIRTUAL,b'\xc3');self.mu.mem_write(RESUME,b'\xc3')

    def clips(self):
        """실제 두 전역의 현재 바이트를 순서대로 읽는다."""
        return struct.unpack('<4i',self.mu.mem_read(self.d['clip'],16))+struct.unpack('<4i',self.mu.mem_read(self.d['vfx'],16))

    def project_change(self):
        """투영의 명시 부수 효과로 두 클립 전역을 입력 값으로 쓴다. 기대 제한 계산은 하지 않는다."""
        clip=self.case['project_clip']
        if clip:
            self.mu.mem_write(self.d['clip'],struct.pack('<4i',*clip))
            self.mu.mem_write(self.d['vfx'],struct.pack('<4i',clip[0],clip[1],clip[2]-1,clip[3]-1))

    def on_instruction(self,mu,address,size,data):
        """가상 Draw 응답은 실제 setter 또는 중첩 Draw로 돌아가고 나머지는 원본 명령을 실행한다."""
        if address==VIRTUAL:
            esp,ecx=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
            self.observed.append(','.join(map(str,(ecx,*self.clips()))));self.boundaries['virtual']+=1
            if self.u32(0)==0xffffffff: raise RuntimeError('가상 Draw 중 SEH 보호 없음')
            # 최상위 가상 호출만 입력 모드를 실행해 재귀 응답이 무한히 반복되지 않게 한다.
            mode=self.case['mode'] if ecx==token(self.case['target']) else 0
            if mode:
                self.pending.append((esp,mode))
                args=self.case['change'] if mode==1 else (token(2),)
                call_esp=esp-4*(len(args)+1)
                mu.mem_write(call_esp,struct.pack('<'+'I'*(len(args)+1),RESUME,*(v&0xffffffff for v in args)))
                mu.reg_write(UC_X86_REG_ESP,call_esp);mu.reg_write(UC_X86_REG_EIP,self.d['Set'] if mode==1 else self.d['Draw']);return
            mu.reg_write(UC_X86_REG_EAX,0x12345678);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if address==RESUME:
            esp,mode=self.pending.pop();args=4 if mode==1 else 1
            if mu.reg_read(UC_X86_REG_ESP)!=esp-args*4: raise RuntimeError('가상 응답의 실제 cdecl 반환 오류')
            if mode==2: self.row_returns[f'{self.d["Draw"]:08x}']+=1;self.returns[f'{self.d["Draw"]:08x}']+=1
            mu.reg_write(UC_X86_REG_EAX,0x12345678);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if address==self.d['Set']:
            self.set_calls.append(','.join(map(str,struct.unpack('<4i',mu.mem_read(mu.reg_read(UC_X86_REG_ESP)+4,16)))))
        if address==self.d['project']: self.project_change()
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """전체 정상 반환/ABI/x87/SEH/노드/클립과 가상 관찰을 입력마다 독립적으로 확인한다."""
        mu,d=self.mu,self.d;self.case=case;inputs=encoded(case)
        self.entries,self.row_returns=collections.Counter(),collections.Counter()
        self.observed,self.projected,self.set_calls,self.pending=[],[],[],[]
        self.anchors={sid:(x,y,0,0) for sid,x,y in case['anchors']}
        mu.mem_write(DATA,b'\0'*0x1000);mu.mem_write(POOL,b'\0'*0x1000)
        # raw 노드는 읽기 전용 입력이며 허용되는 쓰기는 원본 위치 캐시 두 DWORD뿐이다.
        for index,item in enumerate(inputs[6].split(';')):
            address,value=item.split('=');mu.mem_write(int(address),bytes.fromhex(value))
            self.put(POOL+index*d['stride']+14,index);self.put(POOL+index*d['stride']+18,0)
        self.put(d['root'],token(0));self.put(d['pool'],POOL)
        mu.mem_write(d['clip'],struct.pack('<4i',*case['clip']));mu.mem_write(d['vfx'],struct.pack('<4i',*case['vfx']))
        pool_before=bytes(mu.mem_read(POOL,0x1000));table_before=bytes(mu.mem_read(VTABLE,0x40))
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000),(d['clip'],d['clip']+16),(d['vfx'],d['vfx']+16)]
        self.writes.extend((token(i)+108,token(i)+116) for i in range(len(case['nodes'])))
        # 비영 보존 레지스터와 독립 제어 워드로 정상 ABI 복구를 검사한다.
        for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
        # 호출자가 정하지 않는 임시 레지스터는 0이다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX): mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
        mu.mem_write(STACK,struct.pack('<2I',STOP,token(case['target'])))
        mu.emu_start(d['Draw'],STOP,timeout=30000000,count=2000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or self.pending:
            raise RuntimeError('Gump Draw 정상 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in REGISTERS.items()): raise RuntimeError('Gump Draw 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff:
            raise RuntimeError('Gump Draw x87/SEH 오류')
        self.row_returns[f'{d["Draw"]:08x}']+=1;self.returns[f'{d["Draw"]:08x}']+=1;caches=[]
        # 모든 입력 노드의 캐시 이외 바이트와 미사용 노드 저장소를 확인한다.
        for index,item in enumerate(inputs[6].split(';')):
            _,value=item.split('=');expected=bytearray.fromhex(value);actual=bytearray(mu.mem_read(token(index),NODE_SIZE))
            caches.append(f'{token(index)},'+','.join(map(str,struct.unpack_from('<2i',actual,108))))
            actual[108:116]=expected[108:116]
            if actual!=expected: raise RuntimeError('Gump Draw 입력 노드 변경')
        if bytes(mu.mem_read(DATA+len(case['nodes'])*NODE_SIZE,0x1000-len(case['nodes'])*NODE_SIZE))!=b'\0'*(0x1000-len(case['nodes'])*NODE_SIZE):
            raise RuntimeError('Gump Draw 미사용 저장소 변경')
        if bytes(mu.mem_read(POOL,0x1000))!=pool_before or bytes(mu.mem_read(VTABLE,0x40))!=table_before:
            raise RuntimeError('Gump Draw SID/가상 표 입력 변경')
        return [*inputs,';'.join(self.observed),','.join(map(str,self.clips())),';'.join(caches),','.join(self.projected) or '-',
            json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),';'.join(self.set_calls) or '-',
            json.dumps(dict(self.row_returns),sort_keys=True,separators=(',',':'))]


def generate(smoke):
    """같은 입력의 두 x87 관찰이 일치할 때만 원본 관찰을 저장한다. 기대 C++ 계산은 사용하지 않는다."""
    rows,editions=[],{};files={FIXTURE,ROOT/'tools/ghidra/gumpdraw-functions.json'}
    files.update(ROOT/'tools'/name for name in ('decomp_gumpdraw_oracle.py','decomp_gumpcollection_oracle.py','decomp_drawsort_oracle.py','decomp_process_oracle.py'))
    # 세 PE는 각각 로드하며 관찰/SHA/집계도 독립적으로 유지한다.
    for edition in BINARIES:
        oracle=GumpDrawOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::37]+indexed[-4:]
        # 실제 루트를 정상 반환까지 실행해 두 정밀도의 관찰을 비교한다.
        for index,case in indexed:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Gump Draw x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 Gump Draw/클립 보호 관찰. 가상 Draw와 월드 투영만 입력 경계.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-12',controls=list(CONTROLS),os_calls=0,assertions=0,
        boundaries=['virtual','project'],editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},
        limits=['전체 Gump Draw/위치/경계/CRT/ClipGump 생성/복구/화면 setter는 실제 원본 몸체',
            '가상 Draw와 월드 투영은 완성 입력 경계; 중첩 Draw/클립 변경은 다시 실제 원본 루트/setter를 실행',
            '유한 노드 좌표/크기와 signed DWORD clip; GUI/OS/assert/예외 전파 실행은 제외',
            '현재 clip 변경, 캐시/읽기 전용 노드/SID/가상 표/미사용 저장소와 ABI/x87/SEH/허용 실행/쓰기/SHA 감사']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장 SHA/입력/출력 폭과 실제 루트 진입/반환/가상/투영/setter 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 의존 도구/원본/내보내기/fixture의 정확한 바이트를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Gump Draw SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['boundaries']!=['virtual','project'] or report['os_calls'] or report['assertions']:
        raise RuntimeError('Gump Draw 총계/경계 오류')
    if set(report['editions'])!=set(BINARIES): raise RuntimeError('Gump Draw 판본 누락')
    # 세 판본별 완성 입력 순서와 실행 횟수를 따로 감사한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];entries,returns,boundaries=collections.Counter(),collections.Counter(),collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Gump Draw 행 수 오류')
        # 출력은 저장 관찰에서만 집계하며 기대 클립/캐시는 재계산하지 않는다.
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=17 or row[1]!=str(index) or row[2:10]!=encoded(case): raise RuntimeError('Gump Draw 입력/폭 오류')
            if len(row[11].split(','))!=8 or len(row[12].split(';'))!=len(case['nodes']): raise RuntimeError('Gump Draw 상태 출력 폭 오류')
            # 각 진입/반환의 저장 관찰은 두 정밀도에서 실행됐다.
            for address,count in json.loads(row[14]).items(): entries[address]+=count*2
            # 중첩을 포함한 실제 정상 반환을 루트 진입 수와 대조한다.
            for address,count in json.loads(row[16]).items(): returns[address]+=count*2
            if json.loads(row[16]).get(f'{SPECS[edition]["Draw"]:08x}')!=json.loads(row[14]).get(f'{SPECS[edition]["Draw"]:08x}'):
                raise RuntimeError('Gump Draw 진입/반환 차이')
            boundaries['virtual']+=len(row[10].split(';'))*2
            if row[13]!='-': boundaries['project']+=len(row[13].split(','))*2
            sets=0 if row[15]=='-' else len(row[15].split(';'))
            if sets!=json.loads(row[14]).get(f'{SPECS[edition]["Set"]:08x}',0): raise RuntimeError('Gump Draw setter 집계 오류')
        if dict(entries)!=item['entries'] or dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries']: raise RuntimeError('Gump Draw 실행 집계 오류')
    normal=sum(sum(item['returns'].values()) for item in report['editions'].values())
    print(f'gumpdraw 검증 통과: {len(rows)}개, 중첩 포함 두 정밀도 정상 반환 {normal}회, OS/assert 0')


def main():
    """무저장 표본/전체 관찰/저장 감사 중 하나만 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__': main()
