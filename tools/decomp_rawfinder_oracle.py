#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 사각형 탐색과 다리 삭제 훅을 실제 세 PE의 제한 x86으로 대조한다.

Begin/Next·4단계 버킷·타입 발자국·CRT 절삭·기본 true 가상 필터는 원본 명령이다.
기존 삭제 훅 검사기의 destroy/fall/소리/기본 삭제·통지 대체만 유지한다.
원본 게임/OS/창을 실행하지 않으며 기존 도구·기대값을 변경하지 않는다.
"""
import argparse
import collections
import json
import struct
from pathlib import Path

from decomp_bridgeeffects_oracle import EffectsOracle,bits,sha,BRIDGE,LINK,FIRST,SECOND,FOLLOW,OTHER,VTABLE
from decomp_bridgedecay_oracle import ROOT,POOL,HEAP,MAP,CAPACITY,FPU_CONTROLS,STACK_BASE

# 판본별 실제 일반 탐색기의 두 진입점과 기본 always-true 필터 vtable이다.
SPECS={'originals':dict(Begin=0x4b16d0,Next=0x4b1810,vtable=0x502318),
       'originalCD':dict(Begin=0x4eae20,Next=0x4eafe0,vtable=0x5003d0)}
SPECS['original1037']=SPECS['originalCD']
# 분석 전용 finder 공간과 출력 경로. 기존 삭제/붕괴 fixture와 분리한다.
FINDER=HEAP+0x4000
FIXTURE=ROOT/'cpppj/tests/fixtures/rawfinder-x86.tsv'
REPORT=ROOT/'cpppj/recovery-rawfinder-evidence.json'
IDS=(BRIDGE,LINK,FIRST,SECOND,FOLLOW,OTHER)
SCALES=(1,2,4,16)


class FinderOracle(EffectsOracle):
    """기존 효과 대체를 유지하되 일반 탐색 begin/next의 대체를 제거한다."""
    def __init__(self,edition):
        """새 Ghidra 몸체 표를 더하고 실제 네 해시 배열을 연결한다."""
        super().__init__(edition)
        self.finder_spec=SPECS[edition]
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],Begin=self.finder_spec['Begin'],Next=self.finder_spec['Next']))
        self.stubs={addr:name for addr,name in self.stubs.items() if name not in ('begin','next')}
        self.body_paths.extend(ROOT/f'extracted/finder/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        # 실제 불연속 함수 몸체만 실행한다. 전체 PE를 허용하지 않는다.
        import csv
        with self.body_paths[-1].open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp,delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'))
                    self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        self.bases=[]
        address=MAP
        # 배열에 들어가는 것은 정수 SID 머리이며 Python에서 탐색 결과를 계산하지 않는다.
        for scale in SCALES:
            self.bases.append(address);address+=(256//scale)**2*2
        self.map_end=address
        self.mu.mem_write(self.g['grid']-12,struct.pack('<7I',0,1,256,*self.bases))
        self.foot_offset=self.o['foot']
        self.native_calls=collections.Counter()
        self.read_only_find=False

    def on_write(self,mu,access,address,size,value,data):
        """일반 탐색은 커서/스택/FS만 쓴다. 풀·타입·해시는 원본 명령에서도 읽기 전용이다."""
        if not getattr(self,'read_only_find',False):
            return super().on_write(mu,access,address,size,value,data)
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if address==0 and size==4:return
        if FINDER<=address and address+size<=FINDER+64:return
        raise RuntimeError(f'일반 탐색의 읽기 전용 영역 쓰기: {self.edition} {address:08x}+{size}')

    def setup(self,objects):
        """입력 슬롯·타입·등록 체인만 만든다. 반환 순서와 커서는 원본 명령으로 얻는다."""
        self.mu.mem_write(MAP,bytes(self.map_end-MAP))
        # 같은 버킷에 입력 순서대로 머리 삽입한다. 이 배열 입력은 일반 Pop 전체 검증을 뜻하지 않는다.
        for sid,typ,state,extra,xb,yb,width,height,extension,level in objects:
            raw=bytearray(self.stride);raw[10]=typ;raw[11]=state;raw[self.o['extra']]=extra
            struct.pack_into('<I',raw,0,VTABLE)
            struct.pack_into('<II',raw,14,xb,yb)
            self.mu.mem_write(self.slot(sid),bytes(raw))
            self.mu.mem_write(0x11100000+typ*self.type_stride+self.foot_offset,struct.pack('<ii',width,height))
            self.mu.mem_write(0x11100000+typ*self.type_stride+self.foot_offset+16,struct.pack('<i',extension))
            x,y=(int(struct.unpack('<f',struct.pack('<I',value))[0]) for value in (xb,yb))
            address=self.bases[level]+((y//SCALES[level])*(256//SCALES[level])+x//SCALES[level])*2
            previous=struct.unpack('<H',self.mu.mem_read(address,2))[0]
            self.mu.mem_write(self.slot(sid)+4,struct.pack('<H',previous))
            self.mu.mem_write(address,struct.pack('<H',sid))

    def snapshot(self):
        """현재 객체뿐 아니라 사각형·단계·버킷·next까지 직접 기록한다."""
        return ','.join(str(value) for value in struct.unpack('<15i',self.mu.mem_read(FINDER+4,60)))

    def find(self,objects,area,flags,mutation,control):
        """첫 결과 뒤의 동적 raw 변경과 마지막 0 반환까지 정상 호출한다."""
        self.setup(objects)
        self.mu.mem_write(FINDER,struct.pack('<16I',self.finder_spec['vtable'],*([0]*13),flags&1,flags&4))
        output=[]
        self.read_only_find=True
        self.call('Begin',area,FINDER,16,control);output.append(self.snapshot())
        # 원본 결과만 읽는다. 잘못된 무한 체인은 명령 수/반환 횟수 한도로 중단한다.
        for index in range(32):
            current=struct.unpack('<I',self.mu.mem_read(FINDER+0x34,4))[0]
            if current==0:break
            if index==0:
                # 이전 후보 next 변경은 이미 저장한 커서를 되돌리지 않아야 한다.
                if mutation==1:self.mu.mem_write(self.slot(current)+4,b'\x00\x00')
                # 아직 읽지 않은 후보의 buried/좌표/타입은 다음 호출에 반영되어야 한다.
                elif mutation==2:self.mu.mem_write(self.slot(FOLLOW)+self.o['extra'],b'\x08')
                elif mutation==3:self.mu.mem_write(self.slot(FOLLOW)+14,struct.pack('<ff',100.25,100.75))
                elif mutation==4:self.mu.mem_write(self.slot(FOLLOW)+10,bytes([objects[0][1]]))
            self.call('Next',(),FINDER,0,control);output.append(self.snapshot())
        else:raise RuntimeError('일반 탐색 반환 횟수 초과')
        self.read_only_find=False
        return ';'.join(output)

    def lifecycle(self,post,control,reverse):
        """실제 일반 탐색으로 얻은 후보를 기존 실제 pre/postDestroy가 소비한다."""
        self.prepare(case=2 if post else 1,post=post)
        # 링크 관계/type/state를 유지하면서 위치와 버킷을 등록한다.
        fields={sid:bytes(self.mu.mem_read(self.slot(sid),self.stride)) for sid in IDS}
        objects=[(sid,fields[sid][10],fields[sid][11],fields[sid][self.o['extra']],bits(20.75),bits(21.9),1,1,0,
                  0 if sid==BRIDGE else 1) for sid in IDS]
        if reverse:objects.reverse()
        self.setup(objects)
        # 원래 참조를 복구한다. next는 이번 일반 공간 등록 입력으로 보존한다.
        for sid in IDS:
            self.mu.mem_write(self.slot(sid)+8,fields[sid][8:10]);self.mu.mem_write(self.slot(sid)+12,fields[sid][12:14])
        self.before={sid:bytes(self.mu.mem_read(self.slot(sid),self.stride)) for sid in IDS}
        events=self.invoke('Post' if post else 'Pre',[0x12345678],self.slot(BRIDGE),4,control)
        return events,self.states()


def scenes():
    """범위 접촉·소수 절삭·큰 발자국·네 단계·buried와 나머지 state 조합의 입력만 만든다."""
    result=[]
    # 각 장면의 타입은 다른 발자국을 가지며 일부 객체는 같은 버킷에서 next로 이어진다.
    for variant in range(10):
        objects=[]
        for i,sid in enumerate(IDS):
            x=20+(i%3)+(0.000005 if variant%2 else 0.75)
            y=20+(i//3)+(0.99999 if variant%3 else 0.125)
            objects.append((sid,74+i,(0,1,2,4,8,6)[(variant+i)%6],8 if (variant+i)%7==0 else 0,
                            bits(x),bits(y),1+(variant+i)%5,1+(variant+2*i)%5,i%3,(variant+i)%4))
        result.append(objects)
    return result


def generate():
    """독립 기계어 출력과 입력/도구/몸체 SHA를 별도 파일로 저장한다."""
    rows=[];counts=collections.Counter();oracles={edition:FinderOracle(edition) for edition in SPECS}
    areas=[(20,20,20,20),(18,19,21,21),(21,20,21,20),(24,23,24,23),(-3,-2,1,1),(255,255,300,300),(19,19,22,22)]
    # 판본/정밀도별로 같은 입력을 실행한다. Python 참조 계산은 사용하지 않는다.
    for edition,oracle in oracles.items():
        for control in FPU_CONTROLS:
            for scene,objects in enumerate(scenes()):
                for area in areas:
                    for flags in (0,1,4,5):
                        mutation=scene%5
                        output=oracle.find(objects,area,flags,mutation,control)
                        rows.append(['Find',edition,control,flags,','.join(map(str,area)),mutation,
                                     ';'.join(','.join(map(str,obj)) for obj in objects),output])
                        counts['Find']+=1
            # 전체 보드·-1 끝 표식·상한 밖/뒤집힌 범위도 원본 Begin으로 확인한다.
            for area in [(-4,-4,-1,-1),(256,256,300,300),(23,23,20,20)]:
                objects=scenes()[0]
                for flags in (0,1,4,5):
                    output=oracle.find(objects,area,flags,0,control)
                    rows.append(['Find',edition,control,flags,','.join(map(str,area)),0,
                                 ';'.join(','.join(map(str,obj)) for obj in objects),output]);counts['Find']+=1
            for post in (False,True):
                for reverse in (False,True):
                    events,states=oracle.lifecycle(post,control,reverse)
                    rows.append(['Post' if post else 'Pre',edition,control,int(reverse),events,states]);counts['Post' if post else 'Pre']+=1
    FIXTURE.write_text('# 세 실제 PE의 일반 탐색/삭제 훅 출력. 입력과 커서는 정수, 좌표는 float 비트다.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    files={path.relative_to(ROOT).as_posix():sha(path) for oracle in oracles.values() for path in oracle.body_paths}
    # 부모 실행기의 출처를 고정하되 부모 도구/fixture 자체는 수정하지 않는다.
    for path in [Path(__file__),ROOT/'tools/decomp_bridgeeffects_oracle.py',ROOT/'tools/decomp_bridgedecay_oracle.py',
                 ROOT/'tools/ghidra/ExportCreation.java',FIXTURE]:files[path.relative_to(ROOT).as_posix()]=sha(path)
    report=dict(cases=dict(counts),total=sum(counts.values()),files=files,
        binaries={edition:oracle.sha256 for edition,oracle in oracles.items()},fpu_controls=list(FPU_CONTROLS),
        native_calls={edition:dict(oracle.native_calls) for edition,oracle in oracles.items()},
        substitutions={edition:dict(oracle.stub_calls) for edition,oracle in oracles.items()},
        assert_reports={edition:oracle.assertions for edition,oracle in oracles.items()},
        limits=['실제 네 버킷 배열/next를 직접 준비한 합성 등록 상태','정상 월드 좌표/발자국·flag 1/4와 기본 true 필터',
                '일반 탐색 원본 명령의 쓰기는 커서/스택/FS만 허용, 풀/타입/해시는 읽기 전용',
                '삭제 훅의 destroy/fall·소리·공통 pre/postDestroy·제거 통지는 대체','게임/OS/창/일반 Pop·실제 삭제/낙하 검증 아님'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'cases':dict(counts),'native_calls':report['native_calls'],'asserts':report['assert_reports']}))


def verify():
    """저장된 출처/행 수/원본 PE SHA와 대체 경계를 감사한다. 기계어를 재실행하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('출처 SHA 불일치: '+name)
    binaries={'originals':'originals/Netstorm.exe','originalCD':'originalCD/NETSTORM.EXE','original1037':'original1037/netstorm.exe'}
    for edition,name in binaries.items():
        if sha(ROOT/name)!=report['binaries'][edition]:raise RuntimeError('PE SHA 불일치: '+edition)
        if report['assert_reports'][edition]!=0:raise RuntimeError('assert 도달')
        if any(name in report['substitutions'][edition] for name in ('begin','next')):raise RuntimeError('일반 탐색 대체')
    count=collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#'))
    if dict(count)!=report['cases'] or sum(count.values())!=report['total']:raise RuntimeError('행 수 불일치')
    print('rawfinder SHA/행 수/실제 탐색 경계 확인:',report['total'])


if __name__=='__main__':
    # 재생성/감사 명령을 명시적으로 선택한다.
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate()
