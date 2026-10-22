#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Wind12단계의 실제 중심/원주/헤더/Pop 계산과 콜백 뒤 현재 조회를 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_wind_aviary_frame import Oracle as BaseOracle,FIELDS,POOL,TYPE,SHAPE,ROOT,STOP
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW

FIXTURE=ROOT/'cpppj/tests/fixtures/wind-circle-frame-x86.tsv'  # 기존1607행과 구별한 실제 좌표 원문이다.
REPORT=ROOT/'cpppj/wind-circle-frame-20261023.json'  # 현재 원본·호스트·정밀도 근거다.


def bits(value):
    """float 입력을 비트로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """실제 소환 수·signed 헤더·발자국·소유자/헤더 뒤 현재 자료 변화를 교차한다."""
    rows=[]
    for count in (2,4,6):
        for index,geometry,mutation,extra in itertools.product(range(count),range(3),range(4),(0,1)):
            x,y,fx,fy,step,height,hot=((50.25,50.5,1,1,44.,72,21),(6.,3.,3,5,-2.75,-7,33),(255.,.25,5,3,0.0625,32767,-32768))[geometry];rows.append((count,index,bits(x),bits(y),fx,fy,bits(step),height,hot,mutation,extra))  # 기대 좌표는 계산하지 않는다.
    return rows


class Oracle(BaseOracle):
    """기존 전체 프레임의 가상 경계는 유지하고 중심·원주 및 TYPE 발자국 계산을 실제로 실행한다."""
    def __init__(self,control):
        """추가 세 함수의 실제 불연속 범위를 실행 허용 목록에 더한다."""
        super().__init__(control);old=self.allowed;self.allowed=[];ranges(self,(0x41e020,0x4adea0,0x49a900));self.geometryRanges=self.allowed;self.allowed=old+self.geometryRanges;self.didSetup=False;self.returns=0

    def instruction(self,mu,address,size,data):
        """현재 원문에 실제 생성자/헤더 콜백 입력을 적용하고 나머지는 원문 명령으로 실행한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(sp+4,8);a=self.values
        if address==0x45f0a0 and not self.didSetup:
            self.didSetup=True;self.put(POOL+50*50+14,a[2]);self.put(POOL+50*50+18,a[3]);self.put(POOL+60*50+40,a[10],1);self.put(FIELDS+68,a[0]);self.put(FIELDS+72,a[1]);self.put(SHAPE+0x14,a[6]);self.put(SHAPE+0x1a,a[7],2);self.put(SHAPE+0x1e,a[8],2)
            for kind,at in ((133,TYPE),(139,TYPE+0x400),(138,TYPE+0x800)):
                self.put(at+0x1d4,a[4]+(2 if kind==139 else 0));self.put(at+0x1d8,a[5]+(1 if kind==139 else 0));self.put(at+0xe0,0x40800000)  # 생성자 뒤에도 현재 종류별 발자국/속도를 읽는다.
        if address in (0x4ac550,0x49a840):
            kind=self.mu.mem_read(ecx+10,1)[0] if address==0x4ac550 else args[0];self.ret({133:TYPE,139:TYPE+0x400,138:TYPE+0x800}[kind]);return
        if address==0x419850:
            kind=self.mu.mem_read(POOL+60*50+10,1)[0];self.events.append(f'Header,{kind},{args[0]},{args[1]}')
            if a[9]==3:self.put(FIELDS+68,7);self.put(FIELDS+72,1);self.put(FIELDS+64,51);self.put(POOL+50*50+14,bits(61.75));self.put(POOL+50*50+18,bits(62.5))
            self.ret(SHAPE,purge=8);return
        if any(lo<=address and address+size<=hi for lo,hi in self.geometryRanges):self.total+=1;return
        super().instruction(mu,address,size,data)
        if address==STOP+0x200:
            if a[9]==1:self.put(POOL+60*50+10,139,1)
            if a[9]==2:self.put(POOL+50*50+10,139,1)

    def sample(self,values):
        """원문 Frame을 독립 초기 입력에서 실행하고 전체80BYTE/세 슬롯/순서를 반환한다."""
        self.values=values;self.didSetup=False;row=super().run(('F',(12,0.,99.,0,1,1,133,0,0,0)));self.returns+=1
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        return tuple(row[2:])


def main():
    """새 전체 프레임 원문과 기존 자료의 읽기 전용 검증을 구별한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for p,h in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/p)==h,p  # C++ 결과로 fixture를 갱신하지 않는다.
        print('Wind 원주 전체 프레임 원문 SHA 확인');return
    first,second=Oracle(0x27f),Oracle(0x37f);rows=[];different=[]
    for values in cases():
        a=first.sample(values);b=second.sample(values)
        if a!=b:different.append(','.join(map(str,values)))
        rows.append((','.join(map(str,values)),*a))  # 원본CRT 기본53비트만 제품 대조에 사용한다.
    FIXTURE.write_text('# input fields80 objects150 effects\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_wind_circle_frame.py','tools/decomp_wind_aviary_frame.py','tools/decomp_local_mission_transport.py','tools/decomp_cursor_position.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=first.returns+second.returns,nativeInstructions=first.total+second.total,precisionDifferentInputs=different,sha256={p:digest(ROOT/p) for p in paths},limitations=['45f0a0 전체12단계/4adea0·49a900·41e020은 실제 명령이다. 생성·소유자·TYPE/헤더 제공·Pop·비행 생성자/부착은 명시 큰 경계다.','기본PC53 결과를 사용하고64비트 차이를 별도로 기록한다. 전체25종/GUI 플레이 완료 근거는 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Wind 원주 전체',len(rows),facts['normalReturns'],'정밀도 차이',len(different))


if __name__=='__main__':main()
