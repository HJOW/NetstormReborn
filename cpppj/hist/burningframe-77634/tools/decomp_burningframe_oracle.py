#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 BurningFrame→캡처/Sources/Append/dirty를 두 보호 PE에서 직접 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_shape_to_buffer_oracle import ShapeToBufferOracle,BUFFER
from decomp_dudespawn_oracle import TYPES
from decomp_postpop_oracle import ARENA
from decomp_sid_oracle import POOL,SidOracle
from decomp_owner_oracle import ROOT,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 실제 frame/capture/timer/float난수/바람/viewport모드 주소다.
SPEC={
 'originals':dict(frame=0x424530,capture=0x423d50,source=0x424110,append=0x423e80,timer=0x418430,new=0x4e4391,free=0x4e4354,stop=0x4a97e0,clock=0x55b4d0,pixels=0x59a924,multiplier=0x59a934,camera=0x59a938,wind=0x5e18a8,inset=0x595130,random=0x532710,checking=0x5e4794),
 'originalCD':dict(frame=0x4a60e0,capture=0x4a5c70,source=0x4a5d30,append=0x4a60a0,timer=0x45e2b0,new=0x4f1650,free=0x4f1a50,stop=0x4386f0,clock=0x5484a8,pixels=0x565c3c,multiplier=0x565c4c,camera=0x565c50,wind=0x51fbf8,inset=0x5209bc,random=0x5308ec,checking=0x540a24),
}
# 기존 격리 GDT/버퍼/population과 겹치지 않는 실제144바이트 객체/행/입자다.
OBJECT,ROWS,PARTICLES=ARENA+0xb000,ARENA+0xc000,ARENA+0xd000
KEYS=('capacity','stage','off','visible','pattern','seed','now','wind','depth','frame','zoom','pixelScale','victim')
FIXTURE=ROOT/'cpppj/tests/fixtures/burningframe-x86.tsv'
REPORT=ROOT/'cpppj/burningframe.json'


def fbits(value):
    """현재float 입력의 실제 저장 비트를 반환한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def dbits(value):
    """현재double 시각을 정밀도 손실 없이 정수 문자열로 저장한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def sample(**kwargs):
    """현재 객체/시각/입자 입력만 정의하며 프레임 결과를 계산하지 않는다."""
    case=dict(zip(KEYS,(12,2,0,1,0,0,dbits(1.13),fbits(0),-11,0,65536,fbits(1),50)));case.update(kwargs);return case


def inputs():
    """초기 실제캡처/좌표단계·visible전환·난수발생·현재입자와 두판본life판정을 교차한다."""
    cases=[sample(capacity=0,stage=0)]
    # 단계0은 현재 보호 셰이프 프레임/고정16비트 배율의 실제캡처 전체를 실행한다.
    for frame,zoom in itertools.product((0,7,64),(65536,98304)):cases.append(sample(stage=0,frame=frame,zoom=zoom))
    # 단계1의 실제 좌표/깊이는 기존 표시 버퍼의 범위와 별개로 결정된다.
    for frame,depth,scale in itertools.product((0,7,64),(-11,34),(0.75,1.0)):cases.append(sample(stage=1,frame=frame,depth=depth,pixelScale=fbits(scale)))
    # 현재 stage2만 사용해 같은 later-stage분기를 중복 입력으로 세지 않는다.
    for off,visible,pattern,seed,now,wind in itertools.product((0,1),(0,1),range(4),(0,0x12345678),(1.0,1.13,1.3),(0,0.03)):
        cases.append(sample(off=off,visible=visible,pattern=pattern,seed=seed,now=dbits(now),wind=fbits(wind)))
    cases.extend((sample(victim=0),sample(victim=65535)))
    return cases


def particles(case):
    """세 명시 현재입자 패턴과 source this+74 표식만 공급한다."""
    result=[]
    # life NaN/inf도 좌표는 유한하며 원본판정 뒤 변화 전체를 관찰한다.
    for index in range(case['capacity']):
        source=case['pattern']==1 and index<2
        x,y=((2,3),(7,3))[index] if source else (25.125+index,30.375+index)
        life=fbits(9999) if source else (fbits(5.25) if index==0 else fbits(0.1) if index==1 else 0) if case['pattern']==2 else (0x7fc00001,0xffc00001,0x7f800000,0x80000000)[index] if case['pattern']==3 and index<4 else 0
        result.append((fbits(x),fbits(y),fbits(0.1),fbits(-1.25),life,OBJECT+0x74 if source else 0))
    return result


class BurningFrameOracle(ShapeToBufferOracle):
    """실제 현재캡처/Sources/입자/dirty를 실행하며 heap만 명시 경계로 공급한다."""
    def __init__(self,edition):
        """현재 정밀 main 및 별도CD helper 범위만 추가한다."""
        super().__init__(edition);self.b=SPEC[edition];self.frameActive=False;self.heap=[];self.frameCalls=collections.Counter();self.boundaries=collections.Counter()
        names=['burning-process','burning-sources-helper']
        if edition=='originalCD':names.extend(('burning-process-particle-helper','burning-sources-flag-helper','burning-sources-maxhp-helper'))
        # 내보낸 각 실제 함수의 불연속 범위만 허용하며 공백/전체코드를 추가하지 않는다.
        for name in names:
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """실제 하위도달을 기록하며 allocator만 두 분리버퍼로 공급한다."""
        if hasattr(self,'b') and self.frameActive:
            for name in ('frame','capture','source','append','timer','stop'):
                if address==self.b[name]:self.frameCalls[name]+=1
            if address==self.b['new']:
                esp=mu.reg_read(UC_X86_REG_ESP);count=self.read(esp+4);target=BUFFER if not self.heap else ROWS;maximum=0x2000 if not self.heap else 0x1000
                if count>maximum or len(self.heap)>1:raise RuntimeError('Burning frame 격리heap 요청범위 오류')
                self.heap.append(count);self.boundaries['heap']+=1;ret=self.read(esp);mu.reg_write(UC_X86_REG_EAX,target);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
            if address==self.b['free']:raise RuntimeError('Burning frame 현재격자의 미관찰 재캡처 free')
        SidOracle.on_instruction(self,mu,address,size,data)

    def setup(self,case):
        """명시 현재TYPE와 실제보호SHP·현재render환경 및 원본객체 의미필드를 입력한다."""
        self.frameActive=False;self.captureActive=False;self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0));self.flat_segments();mu=self.mu;b=self.b;patch=self.edition=='originals';self.heap=[];self.caseCalls=collections.Counter();before=self.frameCalls.copy()
        raw=bytearray(self.stride);struct.pack_into('<I',raw,0,self.p['vtable']);raw[10]=70;struct.pack_into('<2f',raw,14,20.75,21.9);struct.pack_into('<i' if patch else '<h',raw,26,10);raw[35 if patch else 33]=case['depth']&255
        if patch:struct.pack_into('<i',raw,36,case['frame'])
        else:raw[34]=case['frame']
        mu.mem_write(POOL+50*self.stride,bytes(raw));mu.mem_write(self.c['scale'],struct.pack('<I',case['zoom']));mu.mem_write(self.c['extra'],bytes(4));mu.mem_write(b['checking'],bytes(4));mu.mem_write(b['clock'],struct.pack('<Q',case['now']));mu.mem_write(b['pixels'],struct.pack('<2I',case['pixelScale'],case['pixelScale']));mu.mem_write(b['camera'],struct.pack('<2f',0.25,0.5));mu.mem_write(b['multiplier'],struct.pack('<f',0.3));mu.mem_write(b['wind'],struct.pack('<I',case['wind']));mu.mem_write(b['inset'],bytes(4));mu.mem_write(b['random'],struct.pack('<I',case['seed']));mu.mem_write(OBJECT,bytes(144));mu.mem_write(PARTICLES,bytes(150*24));mu.mem_write(ROWS,bytes(0x1000));mu.mem_write(BUFFER,bytes(0x2000))
        for offset,value in ((0x20,1234),(0x24,case['capacity']),(0x2c,PARTICLES),(0x40,fbits(123)),(0x44,fbits(-456)),(0x6c,50),(0x80,case['off'])):mu.mem_write(OBJECT+offset,struct.pack('<I',value))
        mu.mem_write(OBJECT+0x30,struct.pack('<2d',1.0,0.123456789));mu.mem_write(OBJECT+0xc,struct.pack('<4f',12.25,13.75,30.5,35.5) if case['visible'] else struct.pack('<4f',1100.25,1100.75,1120.5,1125.5));self.write_ranges.extend(((self.c['context'],self.c['context']+20),(BUFFER,BUFFER+0x2000),(ROWS,ROWS+0x1000),(PARTICLES,PARTICLES+150*24),(b['random'],b['random']+4)));mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);self.frameActive=True
        if case['capacity'] and case['stage']:
            self.invoke(b['capture'],this=OBJECT)
        mu.mem_write(OBJECT+0x54,struct.pack('<I',case['stage']));mu.mem_write(OBJECT+0x6c,struct.pack('<I',case['victim']))
        if case['capacity'] and case['stage']:
            values=particles(case)
            for index,value in enumerate(values):mu.mem_write(PARTICLES+index*24,struct.pack('<6I',*value))
            mu.mem_write(OBJECT+0x28,struct.pack('<I',min(case['capacity'],2) if case['pattern']==1 else min(case['capacity'],3) if case['pattern']==2 else min(case['capacity'],4) if case['pattern']==3 else 0));mu.mem_write(OBJECT+0x7c,struct.pack('<I',1 if case['pattern'] else 0))
            if case['pattern']==1:
                rows=bytearray((self.read(OBJECT+0x5c)+self.read(OBJECT+0x60))//5)
                for value in values:
                    if value[5]:
                        x,y=struct.unpack('<2f',struct.pack('<2I',value[0],value[1]));row=(int(x)+int(y))//5
                        if row<len(rows):rows[row]=1
                mu.mem_write(ROWS,bytes(rows))
        self.frameStart=before

    def run_frame(self,case):
        """정상RET·스택·x87과 전체 현재입자/마스크/시각/dirty표·난수상태를 관찰한다."""
        self.setup(case);mu=self.mu;context=bytes(mu.mem_read(self.c['context'],20));self.invoke(self.b['frame'],this=OBJECT);self.frameActive=False
        if bytes(mu.mem_read(self.c['context'],20))!=context:raise RuntimeError('Burning frame 캡처 문맥복원 오류')
        data=bytearray(mu.mem_read(PARTICLES,case['capacity']*24))
        # 원본 source 포인터0/this+74만 같은 C++bool 의미로 정규화한다.
        for index in range(case['capacity']):
            value=struct.unpack_from('<I',data,index*24+20)[0]
            if value not in (0,OBJECT+0x74):raise RuntimeError('Burning frame source 임의포인터')
            struct.pack_into('<I',data,index*24+20,int(value!=0))
        width,height=struct.unpack('<2i',mu.mem_read(OBJECT+0x5c,8));image=bytes(mu.mem_read(BUFFER,width*height)) if self.read(OBJECT+0x58) else b'';rowbytes=bytes(mu.mem_read(ROWS,(width+height)//5)) if self.read(OBJECT+0x78) else b'';self.caseCalls=self.frameCalls-self.frameStart
        return [bytes(mu.mem_read(OBJECT+0xc,16)).hex(),str(struct.unpack('<i',mu.mem_read(OBJECT+0x20,4))[0]),str(self.read(OBJECT+0x28)),bytes(mu.mem_read(OBJECT+0x30,24)).hex(),str(self.read(OBJECT+0x54)),bytes(mu.mem_read(OBJECT+0x5c,16)).hex(),image.hex() or '-',rowbytes.hex() or '-',str(self.read(OBJECT+0x7c)),str(self.read(OBJECT+0x80)),data.hex() or '-',str(self.read(self.b['random'])),*map(str,self.dirty_output()),','.join(map(str,self.heap)) or '-',json.dumps(dict(self.caseCalls),sort_keys=True,separators=(',',':'))]


def generate(smoke=False):
    """새 실제frame 입력만 저장하며 기존Capture164/Sources652/Append564를 새고유수로 더하지 않는다."""
    rows=[];editions={};files={Path(__file__)}
    # 각 판본의 보호 PE에서 같은 명시입력 한 번씩을 관찰한다.
    for edition in SPEC:
        oracle=BurningFrameOracle(edition);cases=inputs();cases=(cases[0],cases[1],sample(pattern=1),sample(pattern=3)) if smoke else cases
        for case in cases:
            try:result=oracle.run_frame(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
            if smoke:print(json.dumps(dict(edition=edition,input=case,output=result),ensure_ascii=False),flush=True)
        editions[edition]=dict(returns=len(cases),assertions=0,actual_calls=dict(oracle.frameCalls),boundaries=dict(oracle.boundaries),frame_replaced=0,capture_replaced=0,sources_replaced=0,append_replaced=0);files.update(oracle.exports);files.add(oracle.shapeFile);files.add(ROOT/edition/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE'));files.add(ROOT/edition/'netstorm.tarc');print(edition,editions[edition],flush=True)
    if smoke:return
    # 현 소비자와 모든 실행기 전체 원문을 현재SHA로 연결한다.
    for name in ('tools/decomp_shape_to_buffer_oracle.py','tools/decomp_dudespawn_oracle.py','tools/decomp_postpop_oracle.py','tools/decomp_display_oracle.py','tools/decomp_pop_oracle.py','tools/decomp_creation_oracle.py','tools/decomp_sid_oracle.py','tools/decomp_oracle.py','tools/decomp_owner_oracle.py','cpppj/src/o/RawBurningFrame.h','cpppj/src/o/RawBurningFrame.cpp','cpppj/src/client/BurningFrameCapture.h','cpppj/src/client/BurningFrameCapture.cpp','cpppj/src/o/RawBurningSources.h','cpppj/src/o/RawBurningSources.cpp','cpppj/src/o/RawBurningVisual.h','cpppj/src/o/RawBurningVisual.cpp','cpppj/src/client/RawShapeToBuffer.h','cpppj/src/client/RawShapeToBuffer.cpp','cpppj/src/client/VFXDraw.cpp','cpppj/src/client/Renderer.cpp','cpppj/tests/BurningFrameTests.cpp'):files.add(ROOT/name)
    FIXTURE.write_text('# 실제 BurningFrame→보호SHP ShapeToBuffer→Sources/Append/dirty. heap만명시경계·소리0실제helper·현재stage/입자는명시입력.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},cpp_tests_completed=0,os_calls=0,limitations=['새실제Frame와실제캡처/타이머/source/Append/dirty 전체몸체를직접실행한다. 기존독립관찰164/652/564는새고유수에더하지않는다.','이미확보한capacity12입자·등록객체와현재보호DudeTYPE를명시입력으로공급하며첫palette/ctor/Kernel부착을새완료로세지않는다.','heap만 요청크기와격리반환경계다. nonzero소리/재캡처free·mappedowner/추가가상그림·실제Draw/Host삭제·캠페인월드는후속이다.','정상53비트x87에서timer elapsed는이전double그대로사용하며패치x이동은넓은vx합을유지/CD는저장float를재읽는다.','CDsignedDWORDlife와Patchorderedfloatlife를quietNaN/inf/음수0에서도구분한다. 좌표/시각은유한이며diagnostic전역0이다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """현재전체원문SHA·정상고유입력·대체0·heap분류를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('BurningFrame SHA불일치 '+name)
    if len(rows)!=report['total'] or report['os_calls']:raise RuntimeError('BurningFrame 정상수/OS 분류오류')
    for edition,data in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=data['returns'] or len({tuple(row[1:14]) for row in selected})!=len(selected) or data['assertions'] or any(data[name] for name in ('frame_replaced','capture_replaced','sources_replaced','append_replaced')) or set(data['boundaries'])-{'heap'}:raise RuntimeError('BurningFrame 고유/대체/경계 오류')
    print(json.dumps(dict(verified=True,total=len(rows),cpp_completed=False),ensure_ascii=False))


# 명시CLI에서 새원본격자와현재출처를 생성하거나감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
