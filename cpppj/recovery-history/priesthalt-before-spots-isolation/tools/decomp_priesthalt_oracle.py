#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 Halt와 공통 Carrier Halt의 실제 몸체를 제한 x86으로 관찰한다. 게임/GUI 실행0이다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle,ROOT,POOL,HEAP,VTABLE,bits,sha
from decomp_bridgedecay_oracle import TYPES,SPOTS,FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

UNPOP,POP,FALL=HEAP+0xe100,HEAP+0xe110,HEAP+0xe120 # 격리된 가상 공간/낙하 효과의 명시 입력 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priesthalt-x86.tsv' # 기존 CarrierCheck/HP 관찰을 재합산하지 않는 새 상위 입력이다.
REPORT=ROOT/'cpppj/recovery-priesthalt-evidence.json' # 원본 몸체/외부 효과/전체 raw/정밀도 결과의 별도 보고서다.
SPEC={
    'originals':dict(halt=0x491bb0,common=0x42a5f0,check=0x426fc0,pickup=0x429ed0,transfer=0x42a0d0,battle=0x594fbc,boss=0x540bc4,quarter=0x5ca8f0,carrier=0x5412c8,other=0x541344),
    'originalCD':dict(halt=0x40c950,common=0x4e45c0,check=0x4e50d0,pickup=0x4e7980,transfer=None,battle=0x540a20,boss=0x540a2c,quarter=0x511c90,carrier=0x51cbb4,other=None),
} # 실제 사제 +38과 공통 접두/가상+cc/기본 타입 전역이다.
COORDINATES=((20.75,21.9),(20.000099182128906,21.000099182128906),(254.0,254.0)) # 지면bias와 공간복귀bias의 경계를 구분하는 입력이다.

def inputs():
    """지면/절반HP/권한/최초좌표와 공통 검색·판본 반환 차이의 정상 입력만 만든다."""
    values=[]
    # 사제 지면 모드의 실제 분기와 Unpop 후 좌표 변이를 교차한다.
    for battle,boss,hp,ground,coordinate,change in itertools.product((0,1),(0,1),(0,99,100,200),(0,6,8),range(3),(0,1)):
        x,y=COORDINATES[coordinate]
        values.append(dict(kind='Halt',battle=battle,boss=boss,hp=hp,ground=ground,x=bits(x),y=bits(y),change=change,genus=0x210000,neighbor=0,reply=0,fall=0))
    # CommonHalt의 실제 +cc와 특정 운반체 helper의 반환 규약을 교차한다.
    for genus,ground,fall,neighbor,reply in itertools.product((0,0x10000,0x210000),(0,6),(0,1),(0,156,187),(0,1,10,12)):
        values.append(dict(kind='Common',battle=0,boss=1,hp=200,ground=ground,x=bits(20.75),y=bits(21.9),change=0,genus=genus,neighbor=neighbor,reply=reply,fall=fall))
    return values

class HaltOracle(FinderOracle):
    """같은 기존 네 단계 finder와 새 상위/HP/지면 몸체만 실행한다."""
    def __init__(self,edition):
        """현재 내보낸 몸체와 독립 공간/낙하/포획 callback 주소만 허용한다."""
        super().__init__(edition)
        self.h=SPEC[edition];self.stubs={};self.entries={};self.events=[];self.returns=0
        paths=[ROOT/f'extracted/carrierarrival/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.body_paths.extend(paths)
        with paths[1].open(encoding='utf-8') as stream:
            # 원본 불연속 몸체의 허용 범위 밖 코드는 실행하지 않는다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.spec=dict(self.spec,entries=dict(Halt=self.h['halt'],Common=self.h['common']))
        self.boundaries={UNPOP:'U',POP:'P',FALL:'F',self.h['pickup']:'C'}
        if self.h['transfer']:self.boundaries[self.h['transfer']]='N'
        self.put(VTABLE+0x48,UNPOP);self.put(VTABLE+0x90,POP);self.put(VTABLE+0xcc,self.h['check']);self.put(VTABLE+0xc8,FALL)

    def put(self,address,value,width=4):
        """합성 입력과 명시 가상 효과의 저장 필드만 지정 폭으로 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def read(self,address,width=4):
        """명시 callback의 인자와 원본 출력 필드를 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """실제 상위/검색/지면/HP는 실행하며 큰 가상/포획 하위 효과만 명시 입력으로 기록한다."""
        boundary=getattr(self,'boundaries',{}).get(address)
        if address in self.entries:self.native_calls[self.entries[address]]+=1
        if not boundary:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);purge=0;result=0
        self.stub_calls[boundary]+=1
        if boundary=='U':
            assert this==self.slot(50) and self.read(sp+4)==0;purge=4;self.events.append('U:50:0')
            if self.case['change']:self.put(this+14,bits(30.75));self.put(this+18,bits(31.9))
        elif boundary=='P':
            assert this==self.slot(50) and self.read(sp+12)==0;purge=12;x,y=self.read(sp+4),self.read(sp+8)
            self.events.append(f'P:50:{x}:{y}:0');self.put(this+14,x);self.put(this+18,y)
        elif boundary=='F':
            assert this==self.slot(50);result=self.case['fall'];self.events.append(f'F:50:{result}')
        else:
            # CD004e7980은 cdecl SID이며 패치 두 helper는 thiscall 후보 포인터다.
            sid=self.read(sp+4) if self.edition=='originalCD' else self.sid_of(this)
            assert sid==60;result=self.case['reply'];self.events.append(f'{boundary}:{sid}:{result}')
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_halt(self,value,control):
        """정상 반환/스택/SEH/x87와 전체 부모 raw 및 명시 요청 순서를 관찰한다."""
        self.case=value;self.mu.mem_write(TYPES,bytes(256*self.type_stride));self.mu.mem_write(POOL,bytes(80*self.stride))
        objects=[(50,158,0,0,value['x'],value['y'],1,1,0,1)]
        if value['neighbor']:objects.append((60,value['neighbor'],0,0,bits(20.2),bits(21.3),1,1,0,1))
        self.setup(objects)
        raw=self.slot(50);self.put(raw+26,value['hp'],4 if self.stride==50 else 2);self.put(TYPES+158*self.type_stride,200);self.put(TYPES+158*self.type_stride+0xec,value['genus'])
        self.put(self.h['battle'],value['battle']);self.put(self.h['boss'],value['boss']);self.put(self.h['quarter'],0);self.put(self.h['carrier'],156)
        if self.h['other']:self.put(self.h['other'],187)
        self.mu.mem_write(SPOTS,bytes(65536));x,y=(struct.unpack('<f',struct.pack('<I',value[name]))[0] for name in ('x','y'))
        self.put(SPOTS+int(y+struct.unpack('<f',struct.pack('<I',0x3f7ff972))[0])*256+int(x+struct.unpack('<f',struct.pack('<I',0x3f7ff972))[0]),value['ground'],1)
        self.spec=dict(self.spec,entries=dict(self.spec['entries']))
        # 원본 호출 도구가 실제 정상 반환/스택/SEH/x87 복구를 확인한다.
        result=self.call(value['kind'],(),raw,0,control)
        self.returns+=1
        return [str(result),';'.join(self.events) or '-',bytes(self.mu.mem_read(raw,self.stride)).hex()]

def main(arguments):
    """명시 CLI에서 초기 실행 또는 기존 결과 전체 SHA의 검증을 수행한다."""
    if arguments.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        # 당시 원본/몸체/도구/생산/fixture를 현재 전체 바이트와 대조한다.
        for name,value in report['files'].items():assert sha(ROOT/name)==value,name
        print('사제 Halt 현재 근거 감사:',report['total']);return
    cases=inputs();cases=cases[::47] if arguments.smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE,ROOT/'tools/ghidra/carrierarrival-functions.json'}
    # 같은 입력의 두 x87 결과를 관찰하며 두 번 실행한 횟수는 고유 관찰에 합산하지 않는다.
    for edition in SPEC:
        oracle=HaltOracle(edition)
        for index,input_value in enumerate(cases):
            # CD에는 타입187이 없으므로 특정 타입과 다른 후보는 실제 범위 끝170으로 준비한다.
            value=dict(input_value,neighbor=170) if edition=='originalCD' and input_value['neighbor']==187 else input_value
            result=oracle.run_halt(value,FPU_CONTROLS[0]);second=oracle.run_halt(value,FPU_CONTROLS[1]);assert result==second
            rows.append([edition,json.dumps(value,sort_keys=True,separators=(',',':')),*result])
            if index%80==0:print(edition,index,len(cases),flush=True)
        editions[edition]=dict(returns=oracle.returns,normal_inputs=len(cases),native_calls=dict(oracle.native_calls),external_calls=dict(oracle.stub_calls))
        files.update(oracle.body_paths);files.add(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# edition inputJSON result effects rawParent\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.update(ROOT/f'cpppj/src/o/RawPriestHalt{suffix}' for suffix in ('.h','.cpp'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),smoke=arguments.smoke,game_runs=0,editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},scope='사제+38/공통Halt와 실제+cc/MaxHP/finder. 가상Unpop/Pop/낙하 및 큰 포획·전달helper의 반환은 명시 외부 입력이다. 전체 포획/RPC/실제 사제 공간·걷기/Arrival는 후속이며 기존CarrierCheck/HP 관찰은 재합산0.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')

# 명시 호출에서만 새 관찰을 수행하며 원본 게임/GUI를 실행하지 않는다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    main(parser.parse_args())
