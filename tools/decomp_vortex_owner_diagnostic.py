#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""신전 Owner의 missingIsland 상세/진단 직전까지 두 PE를 관측한다. 게임/OS/MP는 실행하지 않는다."""
import argparse
import hashlib
import json
import struct
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW

FIXTURE=ROOT/'cpppj/tests/fixtures/vortex-owner-diagnostic-local-x86.tsv'  # 최초 유효 관측은 덮어쓰지 않는다.
SPEC={  # 부모/ftol/island/exists/count/format/detail/assert/pool/players/stride/owner/VT/Player stride/Temple 필드다.
 'originals':(0x4dc4e0,0x4e49c0,0x46f530,0x46ee30,0x46ee50,0x4a89c0,0x436d20,0x4e0620,0x5c8464,0x59534c,50,34,0x519860,0xb4,0x7c),
 'originalCD':(0x44f2a0,0x4f161c,0x4bdc50,0x4bd4c0,0x4bd4f0,0x420560,0x4875b0,0x490040,0x5395dc,0x50f834,36,32,0x501de0,0xac,0x74),
}

class VortexDiagnosticOracle(Oracle):
    """부모와 실제 CRT ftol을 실행한다. 섬 조회/printf/상세 writer/진단은 명시 장치다."""
    def __init__(self, edition):
        """같은 PC의 정밀 함수 범위와 보호 PE를 사용한다."""
        self.s=SPEC[edition]  # 판본별 실제 주소/필드다.
        super().__init__(edition,[dict(name='vortex',**{edition:f'{self.s[0]:08x}'})],helpers=[self.s[1]])
        self.count=0;self.message=b'';self.expression='';self.file='';self.line=0;self.stopped=False;self.details=0  # 이번 입력의 명시 장치 기록이다.

    def get(self, address, width=4):
        """가상 원문 필드의 지정 폭만 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self, address, value, width=4):
        """지정 폭의 낮은 비트만 써 이웃 payload를 유지한다."""
        self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def resume(self, value=0, purge=0):
        """명시 자식의 호출 규약대로 복귀한다. 부모는 진단 진입에서 멈춘다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_EIP,self.get(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def on_instruction(self, mu, address, size, data):
        """원문이 실제로 쌓은 printf 인자와 상세/진단 순서를 읽는다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.s[2]: self.resume(7,8);return
        if address==self.s[3]: self.resume(0,4);return
        if address==self.s[4]: self.resume(self.count);return
        if address==self.s[5]:
            fmt=self.read_string(self.get(sp+8)).decode('ascii');values=[self.get(sp+12+4*i) for i in range(6)]
            signed=[v if v<0x80000000 else v-0x100000000 for v in values]
            self.message=(fmt%tuple(signed)).encode('ascii')  # 원문 형식/실제 인자의 printf만 대체하며 게임 결과를 계산하지 않는다.
            destination=self.get(sp+4);mu.mem_write(destination,self.message+b'\0');self.resume(destination);return
        if address==self.s[6]:
            assert self.read_string(self.get(sp+4))==self.message;self.details+=1;self.resume();return
        if address==self.s[7]:
            assert self.details==1;self.expression=self.read_string(self.get(sp+4)).decode('ascii');self.file=self.read_string(self.get(sp+8)).decode('ascii');self.line=self.get(sp+12);self.stopped=True;mu.emu_stop();return
        super().on_instruction(mu,address,size,data)

    def observe(self, xbits, ybits, old, new, count):
        """상세/진단 진입 전의 전체 슬롯과 아홉 신전 필드쌍을 관측한다."""
        self.count=count;self.details=0;self.message=b'';self.stopped=False;players=SCRATCH+0x2000
        self.put(self.s[8],SCRATCH-5*self.s[10]);self.put(self.s[9],players)
        raw=bytearray(self.s[10]);raw[:4]=self.s[12].to_bytes(4,'little');raw[10]=123;raw[self.s[11]]=old;raw[14:22]=struct.pack('<II',xbits,ybits);self.mu.mem_write(SCRATCH,bytes(raw))
        self.mu.mem_write(players,bytes(9*self.s[13]))
        for player in range(9):  # 원본은 이전 소유자의 최초 신전/복사DWORD만 지운다.
            self.put(players+player*self.s[13]+self.s[14],5);self.put(players+player*self.s[13]+self.s[14]+4,0x12345678)
        self.mu.reg_write(UC_X86_REG_FPCW,0x37f)
        try:
            self.call('vortex',[new],SCRATCH,4)
        except RuntimeError:
            if not self.stopped: raise
        assert self.stopped
        fields=','.join(str(self.get(players+p*self.s[13]+self.s[14]+4*f)) for p in range(9) for f in range(2))
        return [self.edition,','.join(map(str,(xbits,ybits,old,new,count))),self.message.hex(),self.expression,self.file,str(self.line),bytes(self.mu.mem_read(SCRATCH,self.s[10])).hex(),fields]

def main():
    """최초 관측 또는 같은 입력의 불변 재관측만 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();rows=[]
    for edition in SPEC:  # 양판본의 missingIsland 진단 접두만 관측한다.
        oracle=VortexDiagnosticOracle(edition)
        for xy in ((0x42220000,0x42490000),(0xc1a60000,0x43964000),(0x4f000000,0x4f800000),(0x7fc00000,0x7f800000)):  # 양수/음수/하위DWORD 감김/무효 FISTP 입력이다.
            for old in (0,3,8):  # 무소유와 실제 이전 Player의 필드 제거를 구분한다.
                for new in (2,0xffffffff):  # 상세 문자열의 signed 새owner를 구분하며 실제 소유자 쓰기 전 진단에서 멈춘다.
                    for count in (5,0xffffffff):  # 현재 count DWORD의 signed 표기다.
                        rows.append(oracle.observe(*xy,old,new,count))
    rendered='# 신전Owner missingIsland 진단96행; 실제부모/ftol, 나머지명시장치; 진단진입중단; 게임/MP없음\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==rendered;print(f'verified {len(rows)} local Vortex diagnostic inputs');return
    assert not FIXTURE.exists(),'최초 관측 덮어쓰기 금지'
    FIXTURE.write_bytes(rendered.encode('utf-8'))
    paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_vortex_owner_diagnostic.py',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    (ROOT/'cpppj/vortex-owner-diagnostic-native-20261106.json').write_text(json.dumps({'unit':964,'rows':len(rows),'scope':'실제 신전 부모/CRT ftol, 섬/printf/상세/진단은 명시장치. printf는 원문 형식과 실제 인자만 포맷. 진단 진입까지이며 정상 ret4/전체 부모/게임/MP 실행 아님.','sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'froze {len(rows)} local Vortex diagnostic inputs')

if __name__=='__main__':
    main()
