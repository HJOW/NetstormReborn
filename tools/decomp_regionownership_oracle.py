#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""지역 소유 투표 전체 wrapper와 실제 좌표/지역/테마 getter를 세 PE에서 대조한다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle,ROOT,SPECS,TYPES,POOL,LISTS,STACK,STOP,digest,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,
    UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS)
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 실제 전역/함수 주소다. CD의 작업장 목록은 nearest 목록과 다르다.
SPEC={
 'originals':dict(entry=0x455380,region=0x46f530,paint=0x470900,config=0x441270,additional=0x55a70c,workshops=0x5e189c,
    islands=0x5c84bc,empty=0x50c038,isleType=0x5411dc,affected=0x55a718,revision=0x59a8b0,dirty=0x5949d4),
 'originalCD':dict(entry=0x47b3c0,region=0x4bdc50,paint=0x4bd2d0,config=0,additional=0x5670e0,workshops=0x565af0,
    islands=0x52d590,empty=0x505eb0,isleType=0x51cac8),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 분석용 지도와 합성 frameCode 포인터, wrapper 쓰기의 시작값이다.
GRID=0x14000000
FRAME=0x12345678
REVISION=0xffffffff
DIRTY=0x12345678
FIXTURE=ROOT/'cpppj/tests/fixtures/regionownership-x86.tsv'
REPORT=ROOT/'cpppj/recovery-regionownership-evidence.json'
KEYS=('scene','variant','mode','option','paintRegions','x','y','empty')
# 기대값을 포함하지 않는 SID 목록·좌표·소유자/타입 입력이다.
SCENES=[([],[]),([50],[]),([50,53],[60]),([50],[60,63]),([50,51,53],[60,61]),
    ([50,51,52,53,54,55],[60,61,62,63,64,65]),([50,50,50],[61,64]),([51,54],[60,63,64]),
    ([],[60,60,63,61]),([52,55],[62,65]),([50,51],[60,61,63,64,65]),([55,50,53,50],[65,60,60,63])]
COORDS=[(10.2,20.3),(19.000001,20.0),(19.00002,20.0),(40.5,22.5),(60.5,0.0),(-1.00002,0.0),(10.0,255.00002),(255.00002,255.0)]
OBJECT_COORDS=[(10.2,20.3),(30.000001,21.00002),(40.5,22.5),(10.00002,20.000001),(30.5,21.3),(40.000001,22.000001)]
OWNERS=[[1,2,0,1,2,8],[3,3,3,3,3,3],[0,0,8,2,2,8]]
WORKSHOP_OWNERS=[[1,2,8,1,2,8],[3,4,3,3,3,3],[2,0,8,2,0,8]]
THEMES=[0,1,2,3,0xffffffff,0]


def bits(value):
    """좌표 입력을 IEEE float DWORD로 저장한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def affected(variant):
    """중복 지역을 포함한 12바이트 입력 기록만 만든다."""
    return [7,9] if variant==0 else [7,7,9] if variant==1 else [9,9,7]


def inputs():
    """투표 장면/정밀 좌표/옵션/도장 경계의 누적 입력을 교차한다."""
    rows=[]
    # 세 가지 소유자/테마 변형과 세 mode를 같은 독립 PE 입력으로 제공한다.
    for scene,variant,mode,option,paint in itertools.product(range(len(SCENES)),range(3),(0,1,10),(0,1),(0,1)):
        x,y=COORDS[(scene+variant)%len(COORDS)]
        rows.append(dict(scene=scene,variant=variant,mode=mode,option=option,paintRegions=paint,x=bits(x),y=bits(y),empty=31 if variant==1 else 127))
    # 한 장면에서 모든 경계 좌표를 관찰하여 부동소수점 snap 차이를 검증한다.
    for x,y in COORDS:
        rows.append(dict(scene=5,variant=0,mode=0,option=1,paintRegions=0,x=bits(x),y=bits(y),empty=127))
    return rows


class RegionOracle(OwnerOracle):
    """지형 도장/정수 옵션만 대체하고 투표·재귀·getter·clear/생성자는 실제 명령이다."""
    def __init__(self,edition):
        """현재 PC의 새 함수 범위와 별도 지도 저장소를 준비한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(GRID,0x20000);self.returns=0;self.substitutions={'P':0,'C':0}
        self.exports=[ROOT/f'extracted/regionownership/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 계산한 불연속 함수 범위를 그대로 사용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 끝 주소는 포함한다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.mem_write(self.p['islands'],struct.pack('<I',GRID));self.mu.mem_write(self.p['isleType'],struct.pack('<I',83))
        # 지도는 x 구간마다 두 섬·다리·빈 칸의 실제 raw SID를 가리킨다.
        line=[100 if x<20 else 101 if x<40 else 102 if x<60 else 0 for x in range(256)]
        self.mu.mem_write(GRID,struct.pack('<65536H',*(line*256)))

    def set_affected(self,regions):
        """도장 경계가 제공한 기록의 저장소/시작/끝을 쓴다. flood 동작을 계산하지 않는다."""
        pointer=LISTS+0x400
        # 좌표는 원본이 투표에 읽지 않지만 실제 레코드 배치에 공급한다.
        for index,region in enumerate(regions):self.mu.mem_write(pointer+index*12,struct.pack('<2fI',90.5+index,91.25+index,region))
        self.mu.mem_write(self.p['affected'],struct.pack('<4I',0,pointer,pointer+12*len(regions),pointer+96))

    def on_instruction(self,mu,address,size,data):
        """getter 인자와 도장/옵션의 ABI·지역 drawer 생성자 출력을 관찰한다."""
        p=self.p;sp=mu.reg_read(UC_X86_REG_ESP)
        if address==p['region']:
            x,y=struct.unpack('<2I',mu.mem_read(sp+4,8));self.events.append(f'R:{x}:{y}')
        if address in (p['paint'],p['config']):
            if address==p['paint']:
                count=4 if self.edition=='originals' else 3;args=struct.unpack('<'+'i'*2+'I'*(count-2),mu.mem_read(sp+4,count*4))
                if self.edition!='originals':args=(*args,0)
                drawer=mu.reg_read(UC_X86_REG_ECX)
                if struct.unpack('<I',mu.mem_read(drawer,4))[0]!=FRAME:raise RuntimeError('지역 drawer frame 포인터 오류')
                if self.paint_calls==0 and struct.unpack('<4I',mu.mem_read(drawer+4,16))!=(0xc61c4000,)*4:raise RuntimeError('실제 지역 drawer 초기값 오류')
                self.events.append('P:'+':'.join(map(str,args)));self.substitutions['P']+=1;self.paint_calls+=1;purge=count*4
                if self.edition=='originals' and self.case['paintRegions'] and self.paint_calls==2:self.set_affected(affected(self.case['variant']))
            else:
                key,out=struct.unpack('<2I',mu.mem_read(sp+4,8))
                if bytes(mu.mem_read(key,10))!=b'theammode\0' or struct.unpack('<I',mu.mem_read(out,4))[0]!=1:raise RuntimeError('실제 옵션 key/초기값 오류')
                mu.mem_write(out,struct.pack('<I',self.case['option']));self.events.append(f'C:{self.case["option"]}');self.substitutions['C']+=1;purge=0
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """전체 정상 반환·보존 레지스터/x87·순서·wrapper 전역 쓰기를 직접 읽는다."""
        self.case=case;self.events=[];self.instructions=0;self.paint_calls=0;mu=self.mu;p=self.p;variant=case['variant'];patch=self.edition=='originals'
        # 실제 getter에 타입 genus·theme·frame 표와 raw 슬롯을 공급한다.
        for index in range(6):
            type_number=83+index;mu.mem_write(TYPES+type_number*self.type_stride,bytes(self.type_stride));mu.mem_write(TYPES+type_number*self.type_stride+0x98,struct.pack('<I',THEMES[(index+variant)%6]))
            for second in (0,1):
                sid=(60 if second else 50)+index;raw=bytearray(self.stride);raw[10]=type_number;raw[self.o['owner']]=(WORKSHOP_OWNERS if second else OWNERS)[variant][index]
                struct.pack_into('<2f',raw,14,*OBJECT_COORDS[index]);mu.mem_write(self.slot(sid),bytes(raw))
        mu.mem_write(TYPES+83*self.type_stride+0x124,struct.pack('<I',FRAME))
        # 섬/다리 raw +8과 genus는 좌표별 지역 getter에서 실제로 읽는다.
        for sid,type_number,genus,region in ((100,83,2,7),(101,84,0x1000002,9),(102,85,4,42)):
            raw=bytearray(self.stride);raw[10]=type_number;struct.pack_into('<H',raw,8,region);mu.mem_write(self.slot(sid),bytes(raw));mu.mem_write(TYPES+type_number*self.type_stride+0xec,struct.pack('<I',genus))
        mu.mem_write(p['empty'],struct.pack('<I',case['empty']))
        for second,key in enumerate(('additional','workshops')):
            values=SCENES[case['scene']][second];pointer=LISTS+second*0x100;mu.mem_write(pointer,struct.pack('<8I',*(values+[0xabababab]*(8-len(values)))))
            mu.mem_write(p[key],struct.pack('<3I',pointer,8,len(values)))
        self.write_ranges=[]
        if patch:
            self.set_affected(affected(variant));mu.mem_write(p['revision'],struct.pack('<I',REVISION));mu.mem_write(p['dirty'],struct.pack('<I',DIRTY));self.write_ranges=[(p['affected']+8,p['affected']+12),(p['revision'],p['revision']+4),(p['dirty'],p['dirty']+4)]
        before_pool=bytes(mu.mem_read(POOL,128*self.stride));before_lists=bytes(mu.mem_read(LISTS,0x120))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 외부 경계도 원본처럼 callee 보존 레지스터를 유지한다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ECX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.mem_write(STACK,struct.pack('<4I',STOP,case['x'],case['y'],case['mode']))
        mu.emu_start(p['entry'],STOP,count=100000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('지역 투표 cdecl 정상 반환/ESP 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()) or mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('지역 투표 보존 레지스터/x87 오류')
        if before_pool!=bytes(mu.mem_read(POOL,128*self.stride)) or before_lists!=bytes(mu.mem_read(LISTS,0x120)):raise RuntimeError('투표가 원본 raw/목록을 수정함')
        self.returns+=1;regions=[];revision=REVISION;dirty=DIRTY
        if patch:
            begin,end=struct.unpack('<2I',mu.mem_read(p['affected']+4,8))
            regions=[struct.unpack('<I',mu.mem_read(begin+i+8,4))[0] for i in range(0,end-begin,12)]
            revision=struct.unpack('<I',mu.mem_read(p['revision'],4))[0];dirty=struct.unpack('<I',mu.mem_read(p['dirty'],4))[0]
        return [';'.join(self.events),','.join(map(str,regions)) or '-',revision,dirty]


def generate(smoke=False):
    """실제 PE 출력과 현재 도구/내보내기/fixture의 SHA를 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/regionownership-functions.json',FIXTURE}
    # 세 독립 실행 파일을 두 x87 정밀도로 정상 반환까지 실행한다.
    for edition in SPECS:
        oracle=RegionOracle(edition);cases=inputs()[::43] if smoke else inputs()
        for case in cases:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('지역 투표 두 정밀도 불일치')
            rows.append([edition,*[case[k] for k in KEYS],*observed[0]])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),substitutions=oracle.substitutions,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: regionownership {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 지역 소유 투표 전체 wrapper/좌표·지역·테마 getter/재귀. 지형 flood/도장과 옵션 조회만 대체.\n# edition '+' '.join(KEYS)+' events affected revision dirty\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['실제 지역 소유 wrapper·getter·Array clear·drawer 생성자 정상 반환/ABI/x87','지형 flood/도장 내부와 theammode 옵션 조회만 명시 대체; 외부 도장 후 영향 지역 입력 제공','합성 타입 +0x98 테마/목록/지도/raw; 실제 옵션/지형 변경/미션 수명/전체 Pop/GUI 미검증','revision은 투표 wrapper 자체 증가만 관찰하며 미실행 도장 몸체 내부 증가는 포함하지 않음']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장 SHA·입력 순서·행 수·실제 호출 및 대체 사건 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 저장된 파일 해시가 변경되면 재생 근거를 거부한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if len(rows)!=3*len(inputs()) or report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('지역 소유 근거 오류')
    # 결과로부터 getter/도장/옵션 사건을 세며 투표 판단을 다시 구현하지 않는다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];n=len(selected);p=SPEC[edition]
        if n!=len(inputs()) or item['returns']!=2*n or item['assertions']:raise RuntimeError('지역 소유 반환/행 오류')
        events=[event for row in selected for event in row[9].split(';')]
        for kind in ('P','C'):
            if item['substitutions'][kind]!=2*sum(e.startswith(kind+':') for e in events):raise RuntimeError('명시 대체 사건 오류')
        if item['native_calls'].get(f'{p["region"]:08x}',0)!=2*sum(e.startswith('R:') for e in events):raise RuntimeError('실제 getter 진입 오류')
        calls=2*n if edition!='originals' else sum(e.startswith('P:') for e in events)
        if item['native_calls'].get(f'{p["entry"]:08x}',0)!=calls:raise RuntimeError('실제 wrapper/재귀 진입 오류')
        if any(item['native_calls'].get(f'{p[key]:08x}',0) for key in ('paint','config')):raise RuntimeError('외부 몸체 실행/대체 혼합')
        for row,case in zip(selected,inputs(),strict=True):
            if len(row)!=13 or row[1:9]!=[str(case[k]) for k in KEYS]:raise RuntimeError('지역 소유 입력/열 오류')
    print(f'regionownership 검증 통과: {len(rows)}개')


if __name__=='__main__':
    # 게임/GUI 없이 생성·무저장 표본·저장 감사 중 하나를 실행한다.
    parser=argparse.ArgumentParser();parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
