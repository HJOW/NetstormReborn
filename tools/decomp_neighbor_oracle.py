#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""flag 0 첫 연결 이웃과 실제 탐색을 사용하는 다리 끝 칸 변환을 세 PE에서 대조한다.

게임/OS/창을 실행하지 않는다. 탐색 생성자·일반 Begin/Next·파생 필터·기하는 실제 명령이다.
기존 bridgeevent 실행기의 생성·삭제·소유자·Pop·표면 알림 대체는 유지한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path

from decomp_bridgeevent_oracle import (EventOracle,ROOT,SPECS,CONTROLS,BRIDGE,NEWBORN,BRIDGE_TYPE,
    OTHER_TYPE,VTABLE,TYPES,CODES,POOL,CAPACITY,STACK_BASE,base_case,float_bits,frame_codes,
    case_columns,result_text,digest)
from unicorn import UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 기존 실행기와 겹치지 않는 네 해시·spot·독립 탐색기 영역이다.
MAP,SPOTS,FINDER=0x13000000,0x13030000,0x13048000
# 판본별 원본 해시 헤더·spot·풀 개수·보드 크기와 flag 0 생성자 진입점이다.
SPACE={'originals':(0x542508,0x5c7c44,0x5c847c,0x531928,0x4b23e0),
       'originalCD':(0x5670c0,0x52fe48,0x5395f4,0x52e9a8,0x4ebad0)}
SPACE['original1037']=SPACE['originalCD']
# 단계별 버킷 크기와 새 결과 파일이다. 원본 상수 005424f8 / CD 0052d598.
SCALES=(1,2,4,16)
FIXTURE=ROOT/'cpppj/tests/fixtures/neighbor-x86.tsv'
REPORT=ROOT/'cpppj/recovery-neighbor-evidence.json'


class NeighborOracle(EventOracle):
    """첫 이웃의 입력 대체를 제거하고 실제 일반 해시 경로를 실행한다."""
    def __init__(self,edition):
        """검토한 함수 몸체·네 해시·spot·예외 체인 메모리를 연결한다."""
        super().__init__(edition)
        header,spot,count,board,entry=SPACE[edition]
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],Find=entry))
        self.stubs={address:name for address,name in self.stubs.items() if name!='finder'}
        self.mu.mem_map(MAP,0x50000)
        self.mu.mem_map(0,0x1000)
        self.mu.hook_add(UC_HOOK_MEM_INVALID,self.invalid_memory)
        self.bases=[]
        address=MAP
        # 실제 원본 해시 배열의 입력만 만든다. 기대 결과는 기계어가 계산한다.
        for scale in SCALES:
            self.bases.append(address)
            address+=(256//scale)**2*2
        self.map_end=address
        self.mu.mem_write(header,struct.pack('<7I',0,1,256,*self.bases))
        self.mu.mem_write(spot,struct.pack('<I',SPOTS))
        self.mu.mem_write(count,struct.pack('<I',CAPACITY))
        self.mu.mem_write(board,struct.pack('<I',256))
        self.pending=None
        self.first=0
        self.finder_calls=0
        # 일반 탐색·연결·발자국과 교차 기하의 읽기 전용 내보내기를 재사용한다.
        for suffix in ('','/geometry'):
            paths=[ROOT/f'extracted/graphremove/{edition}{suffix}/{name}' for name in ('creation.c','functions.tsv')]
            self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 불연속 함수 몸체만 실행을 허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'))
                        self.allowed.append((low,high+1))

    def invalid_memory(self,mu,access,address,size,value,data):
        """미설정 전역/입력 때문에 잘못된 주소를 읽으면 실행 위치와 함께 중단한다."""
        raise RuntimeError(f'허용하지 않은 메모리 접근: {self.edition} EIP={mu.reg_read(UC_X86_REG_EIP):08x} '
            f'주소={address:08x}+{size}')

    def on_write(self,mu,access,address,size,value,data):
        """CD 예외 체인과 독립 finder 외에는 기존 제한된 슬롯/스택 쓰기만 허용한다."""
        if address==0 and size==4:return
        if FINDER<=address and address+size<=FINDER+0x200:return
        super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """생성자 진입/복귀에서 임시 프레임과 실제 +0x34를 관찰하되 실행은 대체하지 않는다."""
        if self.pending and address==self.pending[0]:
            self.first=struct.unpack('<I',mu.mem_read(self.pending[1]+0x34,4))[0]
            self.events.append(f'I:{self.first}')
            self.pending=None
        if address==SPACE[self.edition][4]:
            esp=mu.reg_read(UC_X86_REG_ESP)
            ret,sid,flags=struct.unpack('<III',mu.mem_read(esp,12))
            frame=int.from_bytes(mu.mem_read(self.slot(sid)+self.o['frame'],self.o['frame_size']),'little')
            self.events.append(f'F:{sid}:{frame}:{flags}')
            self.pending=(ret,mu.reg_read(UC_X86_REG_ECX))
            self.finder_calls+=1
        super().on_instruction(mu,address,size,data)

    def prepare_space(self,case,nodes,spots):
        """시나리오의 실제 raw 객체·타입·프레임·해시 체인을 입력한다."""
        self.prepare(case)
        self.first=0
        self.pending=None
        self.mu.mem_write(MAP,bytes(self.map_end-MAP))
        self.mu.mem_write(SPOTS,bytes(65536))
        # node 필드: SID/type/state/extra/x비트/y비트/폭/높이/flag1/flag2/frame/단계.
        for sid,typ,state,extra,xb,yb,width,height,f1,f2,frame,level in nodes:
            slot=self.slot(sid)
            if sid!=BRIDGE:
                self.mu.mem_write(slot,bytes(self.stride))
                self.mu.mem_write(slot,struct.pack('<I',VTABLE))
            self.mu.mem_write(slot+10,bytes([typ,state]))
            self.mu.mem_write(slot+14,struct.pack('<II',xb,yb))
            self.mu.mem_write(slot+(40 if self.spec['patch'] else 35),bytes([extra]))
            self.mu.mem_write(slot+self.o['frame'],frame.to_bytes(self.o['frame_size'],'little'))
            base=TYPES+typ*self.type_stride
            self.mu.mem_write(base+0xe8,struct.pack('<II',f1,f2))
            foot=0x1d4 if self.spec['patch'] else 0x1b4
            self.mu.mem_write(base+foot,struct.pack('<II',width,height))
            codes=CODES+(typ-BRIDGE_TYPE)*0x400
            self.mu.mem_write(codes,frame_codes(case['variant']))
            self.mu.mem_write(base+0x114,struct.pack('<I',16))
            self.mu.mem_write(base+0x124,struct.pack('<I',codes))
            x,y=(int(struct.unpack('<f',struct.pack('<I',value))[0]) for value in (xb,yb))
            bucket=self.bases[level]+((y//SCALES[level])*(256//SCALES[level])+x//SCALES[level])*2
            head=struct.unpack('<H',self.mu.mem_read(bucket,2))[0]
            self.mu.mem_write(slot+4,struct.pack('<H',head))
            self.mu.mem_write(bucket,struct.pack('<H',sid))
        # spot 입력은 체인 순서/접합 판단과 별도로 지정한다.
        for x,y,value in spots:self.mu.mem_write(SPOTS+y*256+x,bytes([value]))

    def neighbor(self,case,nodes,spots,control):
        """실제 생성 직후 첫 반환 번호만 관찰한다. 이후 Next는 호출하지 않는다."""
        self.prepare_space(case,nodes,spots)
        self.mu.mem_write(FINDER,bytes(0x200))
        try:self.call('Find',[BRIDGE,0],FINDER,8,control,returns_float=False)
        except RuntimeError as error:
            raise RuntimeError(f'{self.edition} Find 중단 EIP={self.mu.reg_read(UC_X86_REG_EIP):08x}') from error
        self.pending=None
        return struct.unpack('<I',self.mu.mem_read(FINDER+0x34,4))[0]

    def event(self,case,nodes,spots,control):
        """실제 이벤트→임시 프레임→실제 첫 이웃→후속 효과의 관찰 결과다."""
        self.prepare_space(case,nodes,spots)
        bits=self.call('Handler',[case['event'],0,case['payload']],self.slot(BRIDGE),12,control)
        return [result_text(bits),';'.join(self.events) or '-',
            bytes(self.mu.mem_read(self.slot(BRIDGE),self.stride)).hex(),
            bytes(self.mu.mem_read(self.slot(NEWBORN),self.stride)).hex()]


def inputs():
    """단계·체인·소수 좌표·큰 발자국·dead/buried/내부·프레임 차이의 입력 격자다."""
    result=[]
    # 입력 위치와 프레임을 격자로 정한다. 이웃 결과/끝 방향은 Python에서 계산하지 않는다.
    for index,(variant,frame,position,layout) in enumerate(itertools.product(range(3),(0,1,11),
            ((20.0,21.0),(20.75,21.9),(1.0,1.0),(254.0,254.0)),range(8))):
        x,y=position
        case=base_case(variant=variant,frame=frame,x=float_bits(x),y=float_bits(y),parent=127,word=index&0xffff,
            flag=0x10,payload=float_bits(float((int(y)<<8)|int(x))),authority=0 if layout==7 else 1)
        nodes=[(BRIDGE,BRIDGE_TYPE,0,case['flag'],float_bits(x),float_bits(y),1,1,0x800,4,frame,0)]
        coords=[(max(1,x-1),y),(min(255,x+1),y),(x,max(1,y-1)),(x,min(255,y+1)),(min(255,x+2),y)]
        # 5개 후보를 서로 다른 타입·해시 단계에 넣고 한 후보는 기준점보다 넓은 발자국을 가진다.
        for i,(ax,ay) in enumerate(coords):
            nodes.append((70+i,OTHER_TYPE+i,2 if layout==1 and i<3 else 0,8 if layout==2 and i%2==0 else 0,
                float_bits(ax),float_bits(ay),2 if i==4 else 1,1,0 if layout==3 and i<3 else 0x800,
                4,2 if layout==4 else 0,(i+layout)%4))
        if layout==5:nodes.reverse()
        spots=[(int(x),int(y),8)] if layout==6 else [(int(coords[0][0]),int(coords[0][1]),8)] if layout==3 else []
        result.append((case,nodes,spots))
    return result


def packed(values):
    """입력 레코드를 TSV 한 칸에 보존한다."""
    return ';'.join(','.join(map(str,row)) for row in values) or '-'


def generate(smoke=False):
    """두 x87 정밀도의 관찰이 같을 때만 저장하고 모든 대체 경계·SHA를 기록한다."""
    rows=[]
    oracles={edition:NeighborOracle(edition) for edition in SPECS}
    cases=inputs()
    if smoke:cases=cases[:3]+cases[64:67]
    # 같은 입력을 세 PE에서 독립 실행한다. 판본 결과를 서로 복사하지 않는다.
    for edition,oracle in oracles.items():
        for case,nodes,spots in cases:
            first=[oracle.neighbor(case,nodes,spots,control) for control in CONTROLS]
            event=[oracle.event(case,nodes,spots,control) for control in CONTROLS]
            if first[0]!=first[1] or event[0]!=event[1]:raise RuntimeError(f'x87 정밀도 차이: {edition}')
            rows.append(['First',edition,packed(nodes),packed(spots),case['variant'],first[0]])
            rows.append(['Event',edition,*case_columns(case),packed(nodes),packed(spots),*event[0]])
        print(f'{edition}: {len(cases)} 첫 이웃 + {len(cases)} 실제 탐색 이벤트 입력 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# flag 0 첫 연결 이웃 및 실제 탐색을 사용하는 다리 이벤트. 원본 게임 실행 없음.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={path for oracle in oracles.values() for path in oracle.exports}
    paths.update((Path(__file__),ROOT/'tools/decomp_bridgeevent_oracle.py',FIXTURE))
    report=dict(total=len(rows),cases=dict(collections.Counter(row[0] for row in rows)),fpu_controls=list(CONTROLS),
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        editions={edition:dict(binary=oracle.spec['binary'],sha256=oracle.sha256,assertions=oracle.assertions,
            finder_calls=oracle.finder_calls,substitutions=dict(oracle.stub_calls)) for edition,oracle in oracles.items()},
        limits=['flag 0의 첫 결과만 관찰한다. flag 8/후속 Next 커서는 이 fixture의 대상 밖이다',
            '지도 안 정상 좌표/발자국과 합성 프레임 코드·일반 해시 등록 입력',
            '이벤트의 생성/삭제/소유자/Pop/표면 알림은 기존 외부 효과 대체다. 실제 월드나 게임 루프를 증명하지 않는다'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """파일·원본 PE의 SHA, 행 수와 탐색 대체가 없음을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError(f'SHA 불일치: {path}')
    counts=collections.Counter(line.split('\t',1)[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines()
        if line and not line.startswith('#'))
    if dict(counts)!=report['cases'] or sum(counts.values())!=report['total']:raise RuntimeError('행 수 불일치')
    for edition,data in report['editions'].items():
        if digest(ROOT/data['binary'])!=data['sha256']:raise RuntimeError(f'PE SHA 불일치: {edition}')
        if data['assertions'] or data['substitutions'].get('finder') or not data['finder_calls']:
            raise RuntimeError(f'탐색 실행 증거 오류: {edition}')
    print(f'neighbor 감사 통과: {report["total"]}개')


def main():
    """기대값 생성·읽기 전용 감사·파일을 쓰지 않는 도구 점검을 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
