#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 Detach의 실제 일반 finder와 현재 Flood·sentinel·정책 복귀를 격리 관측한다."""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path
from decomp_graph_live_checking import LiveGraphOracle, IDS, SOURCE, TABLE, STACK
from decomp_surface_oracle import ROOT, POOL, MAP, TYPES, CODES, SPOTS, node
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 원문 일반 해시의 추가 세 단계는 표면 지도/spot/Graph와 겹치지 않는 관측 입력이다.
EXTRA = 0x12000000
# Detach와 일반 finder의 검토한 몸체만 최신 전체 정밀 범위에서 허용한다.
SPECS = {
 'originals': {'entry':0x4637b0,'enabled':0x540414,'rebuild':0x562c1c,'large':0x5412e0,'clock':0x460cd0,'header':0x542508,
  'helpers':[0x462c30,0x4b24c0,0x4b16d0,0x4b1810,0x434380,0x4b2b00,0x40eaf0,0x41d9b0,0x41d8c0]},
 'originalCD': {'entry':0x45bfd0,'enabled':0x5207f8,'rebuild':0x5207fc,'large':0x51cbcc,'clock':0x401130,'header':0x5670c0,
  'helpers':[0x45b750,0x45b670,0x4ebbe0,0x4eae20,0x4eafe0,0x479d90,0x45b530,0x43fe10,0x43fa70]},
}
# 장면은 입력만 정의한다. 분할 결과나 연결 목록을 Python으로 계산하지 않는다.
SCENES = ('Normal','Lookup','Override','Missing','Inactive','Disabled')


class DetachOracle(LiveGraphOracle):
    """두 시각/진단/로그만 반환 장치로 두고 나머지는 실제 PE 명령을 실행한다."""
    def __init__(self,edition):
        """양판본 현재 전역과 네 해시를 기존 Graph 격리 메모리에 연결한다."""
        super().__init__(edition);self.detach=SPECS[edition]
        self.entries['Detach']=self.detach['entry'];self.stubs[self.detach['clock']]='detach-clock'
        with (ROOT/f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 실제 함수의 불연속 몸체 사이 다른 바이트를 실행 허용에 넣지 않는다.
        for entry in [self.detach['entry']]+self.detach['helpers']:
            # 범위 양 끝은 원문 내보내기의 inclusive 정의다.
            for part in facts[entry]['ranges'].split(';'):
                lo,hi=(int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.mem_map(EXTRA,0x10000);self.mu.mem_map(0,0x1000)
        self.write_ranges.append((0,4))  # CD 일반 finder의 격리 SEH 체인만 허용한다.
        self.bases=(MAP,EXTRA,EXTRA+0x8000,EXTRA+0xa000)
        self.mu.mem_write(self.detach['header'],struct.pack('<7I',0,1,256,*self.bases))
        self.clocks=0  # 원문 두 지역 시각 읽기의 실제 호출 순서다.

    def on_instruction(self,mu,address,size,data):
        """진단·로그·시각 경계의 명시 입력 변경 뒤 실제 caller로 복귀한다."""
        kind=self.stubs.get(address)
        if kind not in ('assert','graph-log','detach-clock'):
            return super().on_instruction(mu,address,size,data)
        esp=mu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',mu.mem_read(esp,4))[0]
        if kind=='detach-clock':
            self.events.append('t');self.clocks+=1
            destination=struct.unpack('<I',mu.mem_read(esp+4,4))[0];mu.mem_write(destination,struct.pack('<d',17.25))
            if self.clocks==1 and self.action==3:
                mu.mem_write(self.detach['rebuild'],struct.pack('<I',1));mu.mem_write(self.detach['enabled'],bytes(4))
                mu.mem_write(POOL+SOURCE*self.squid_stride+14,struct.pack('<ff',200,200))
                mu.mem_write(TABLE+3*6,struct.pack('<hhh',91,2,-321))
            elif self.clocks==1 and self.action==7:
                mu.mem_write(self.detach['rebuild'],bytes(4))
        elif kind=='assert':
            a,b,line=struct.unpack('<III',mu.mem_read(esp+4,12));self.events.append(f'a:{line}:{self.read_string(a).decode()}:{self.read_string(b).decode()}')
            if self.action in (2,5) and line==0x68:
                mu.mem_write(self.checking,bytes(4))
                if self.action==5:mu.mem_write(self.detach['enabled'],bytes(4))
        else:
            level,fmt,used=struct.unpack('<III',mu.mem_read(esp+4,12));assert self.read_string(fmt)==b'%d graphs in use\n'
            self.events.append(f'l:{level}:{used}');self.traces+=1
            if self.traces==1:
                if self.action==1:
                    # 뒤 연결과 원래 행/예약WORD/비활성 stack만 명시 입력으로 바꾼다.
                    for sid in (IDS[2],IDS[4]):mu.mem_write(POOL+sid*self.squid_stride+self.graph_offset,b'\x03')
                    mu.mem_write(TABLE+3*6,struct.pack('<hhh',91,2,-321));mu.mem_write(STACK+4095*4,struct.pack('<I',0xdeadbeef))
                    mu.mem_write(self.detach['rebuild'],struct.pack('<I',1))
                elif self.action==2:mu.mem_write(TABLE+254*6,bytes(4))
                elif self.action==4:mu.mem_write(POOL+IDS[2]*self.squid_stride+11,b'\x01')
                elif self.action==6:mu.mem_write(MAP+(20*256+22)*2,bytes(2))
                elif self.action==8:mu.mem_write(self.detach['rebuild'],bytes(4))
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)

    def observe_detach(self,scene,checking,action,rebuild):
        """원문 삭제 값8DWORD/현재 풀/표/정책만 준비하고 결과 전체를 직접 읽는다."""
        positions=[(20,20),(19,20),(21,20),(22,20),(20,21),(20,19)]
        if scene=='Lookup':positions[-1]=(20,20)
        objects=[node(sid,*positions[index],dead=int(index==0)) for index,sid in enumerate(IDS)]
        self.setup(objects);self.action=action;self.events=[];self.traces=0;self.clocks=0
        self.graph_offset=30 if self.edition=='originals' else 28
        self.mu.mem_write(EXTRA,bytes(0x10000))
        # 네 단계의 일반 해시도 원본 입력이다. 뒤 단계는 이 장면에서 비어 있다.
        for sid in range(1,32):
            if sid not in IDS:self.mu.mem_write(POOL+sid*self.squid_stride+11,b'\x01')
        records=bytearray().join(struct.pack('<hhh',0,0,100+index) for index in range(255))
        struct.pack_into('<hh',records,3*6,6,0 if scene=='Inactive' else 2)
        struct.pack_into('<hh',records,4*6,6,1);struct.pack_into('<hh',records,254*6,0 if action==5 else 0x7dfd,0 if action==5 else 0x7dfd)
        self.mu.mem_write(TABLE,bytes(records));self.mu.mem_write(STACK,struct.pack('<4096I',*(0xa0000000+index for index in range(4096))))
        # Lookup은 원천3과 위치 머리4를 구별하며 같은 위치의 원천을 next로 잇는다.
        for index,sid in enumerate(IDS):self.mu.mem_write(POOL+sid*self.squid_stride+self.graph_offset,bytes([4 if scene=='Lookup' and index else 3]))
        if scene=='Lookup':self.mu.mem_write(POOL+IDS[-1]*self.squid_stride+4,struct.pack('<H',SOURCE))
        if scene=='Missing':self.mu.mem_write(MAP+(20*256+20)*2,bytes(2))
        source_type=81 if scene=='Override' else 75
        if scene=='Override':
            typ=TYPES+11*self.stride;self.mu.mem_write(typ+0xe8,struct.pack('<II',0x800,8))
            self.mu.mem_write(typ+(0x1d4 if self.edition=='originals' else 0x1b4),struct.pack('<II',3,3))
            self.mu.mem_write(typ+0x114,struct.pack('<I',1));self.mu.mem_write(typ+0x124,struct.pack('<I',CODES+11*4));self.mu.mem_write(CODES+11*4,b'AP\x01\0')
        self.mu.mem_write(self.checking,struct.pack('<I',checking))
        # 실제 전역 pointer/정수만 입력하며 타이머/로그 외부 서비스를 실행하지 않는다.
        for key,value in (('table',TABLE),('stack',STACK),('maximum',251),('invalid',254)):
            self.mu.mem_write(self.spec[key],struct.pack('<I',value))
        for key,value in (('enabled',int(scene!='Disabled')),('rebuild',rebuild),('large',81)):
            self.mu.mem_write(self.detach[key],struct.pack('<I',value))
        if self.edition=='originals':
            self.mu.mem_write(0x5411c0,struct.pack('<I',90));self.mu.mem_write(0x541288,struct.pack('<I',140))
        self.call('Detach',[struct.unpack('<I',struct.pack('<f',20))[0]]*2+[0,0,source_type,0,0,0])
        slots=b''.join(bytes(self.mu.mem_read(POOL+sid*self.squid_stride,self.squid_stride)) for sid in IDS)
        return ['-',bytes(self.mu.mem_read(TABLE,255*6)).hex(),bytes(self.mu.mem_read(STACK,4096*4)).hex(),slots.hex(),
            str(struct.unpack('<I',self.mu.mem_read(self.checking,4))[0]),'|'.join(self.events) or '-',
            str(struct.unpack('<I',self.mu.mem_read(self.detach['enabled'],4))[0]),str(struct.unpack('<I',self.mu.mem_read(self.detach['rebuild'],4))[0])]


def main():
    """기존 증거를 덮지 않고 새 전체 관측을 기록하거나 읽기 재검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    fixture=ROOT/'cpppj/tests/fixtures/graph-detach-live-x86.tsv';report=ROOT/'cpppj/graph-detach-live-native-20261103.json'
    rows=[];binaries={}
    # 양판본/초기 분할정책/전체 checking DWORD/명시 경계 변경을 독립 입력으로 관측한다.
    for edition in SPECS:
        oracle=DetachOracle(edition);binaries[edition]=oracle.sha256
        # 초기 입력 장면을 매 호출 새로 만든다.
        for scene in SCENES:
            # 원래 정책과 콜백 뒤 정책을 구별한다.
            for rebuild in (0,1):
                # 상위비트만 켜진 checking도 비영 관문이다.
                for checking in (0,0x80000000):
                    # 진단/로그/두 시각 읽기의 서로 다른 변경이다.
                    for action in (0,1,2,3,4,5,6,7,8):
                        rows.append([edition,f'{scene}:{rebuild}',str(checking),str(action)]+oracle.observe_detach(scene,checking,action,rebuild))
    output=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(output).hexdigest()
    if args.verify:assert fixture.read_bytes()==output,'Detach 현재 원문 재관측 불일치'
    else:
        if fixture.exists() or report.exists():raise FileExistsError('기존 증거에는 --verify를 사용하십시오')
        fixture.write_bytes(output);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03',
            'binaries':binaries,'rows':len(rows),'fixtureSha256':digest,'boundary':'실제 PE Detach/일반 finder/Graph 명령·진단/로그/두 시각만 명시 반환·게임/GUI/OS/소켓0'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify}))


if __name__=='__main__':
    main()
