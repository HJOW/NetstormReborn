#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 두 PE의 썬 디스크 발사기 포구/TYPE 치수/충돌 상자를 격리 x86에서 실행한다."""
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, PLAYERS, TARGET, STOP, STACK, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

SPEC = {  # 실제 함수와 BoardCoord 배율/TYPE 필드의 판본 배치다.
    'originals': dict(muzzle=0x46a780,dimensions=0x49a900,bounds=0x49af10,scale=0x59a92c,foot=0x1d4),
    'originalCD': dict(muzzle=0x437390,dimensions=0x4442b0,bounds=0x444b50,scale=0x565c44,foot=0x1b4),
}
VTABLE, SHAPES, OUTPUT = PLAYERS+0x200, PLAYERS+0x1000, PLAYERS+0x5000  # 격리 입력/출력 주소다.
DEFAULT = STOP+0x360  # 가상78의 기본프레임만 명시 외부 입력1로 제공한다.
OFFSETS = ((-35,91),(20,-7),(32767,-32768),(-127,43))  # 원본 signed WORD 폭의 SHP 입력이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/projectile-geom-x86.tsv'  # 기존 Missile fixture와 분리한다.
REPORT = ROOT/'cpppj/projectile-geom.json'  # 새 고유 입력과 정상반환 근거다.


def bits(value):
    """입력 좌표의 binary32 표현만 만든다. 결과 계산은 PE가 수행한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class ProjectileGeometryOracle(OwnerOracle):
    """포구/치수/상자와 모든 하위 좌표 계산은 실제 몸체로 실행한다."""
    def __init__(self, edition):
        """같은 PC의 정확 export 범위와 합성 SHP 표를 준비한다."""
        super().__init__(edition)
        self.paths=[]  # 이번 정밀 export 원문과 범위다.
        # 다른 기존 검증 자료를 덮어쓰지 않고 추가 범위만 읽는다.
        for folder in ('projectile-geom','projectile-help'):
            self.paths += [ROOT/f'extracted/{folder}/{edition}/{name}' for name in ('creation.c','functions.tsv')]
            with self.paths[-1].open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'))
                        self.allowed.append((low,high+1))
        self.write_ranges=[(OUTPUT,OUTPUT+16)]
        self.put(VTABLE+0x78,DEFAULT)
        self.type=TYPES+71*self.type_stride  # 포구 getter가 실제 TYPE71을 조회한다.
        self.put(self.type+0xdc,SHAPES)
        self.put(self.type+0x114,4)
        # 실제 SHP 인덱스는 header 주소가 아니라 header+36의 상대주소다.
        for frame,offset in enumerate(OFFSETS):
            relative=0x100+frame*0x40
            self.put(SHAPES+8+frame*8,relative+36)
            self.mu.mem_write(SHAPES+relative+32,struct.pack('<hh',*offset))

    def put(self, address, value, width=4):
        """현재 합성 입력의 정수 바이트만 메모리에 기록한다."""
        self.mu.mem_write(address,int(value).to_bytes(width,'little',signed=value<0))

    def on_instruction(self, mu, address, size, data):
        """가상 기본프레임만 외부 입력으로 제공하며 좌표 helper는 대체하지 않는다."""
        if address==DEFAULT:
            sp=mu.reg_read(UC_X86_REG_ESP)
            ret=int.from_bytes(mu.mem_read(sp,4),'little')
            mu.reg_write(UC_X86_REG_EAX,1)
            mu.reg_write(UC_X86_REG_ESP,sp+4)
            mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def sample(self, kind, frame, extra, sx, sy, x, y, fx, fy, height):
        """하나의 명시 입력을 실행하고 출력 float/int 전체 DWORD를 그대로 얻는다."""
        spec=SPEC[self.edition]
        raw=bytearray(self.stride)
        struct.pack_into('<I',raw,0,VTABLE)
        raw[10]=71
        struct.pack_into('<II',raw,14,x,y)
        if self.edition=='originals':
            struct.pack_into('<I',raw,36,frame);raw[40]=extra
        else:
            raw[34]=frame;raw[35]=extra;self.put(0x546778,0)
        self.mu.mem_write(self.slot(TARGET),bytes(raw))
        self.mu.mem_write(spec['scale'],struct.pack('<ff',sx,sy))
        self.put(self.type+spec['foot'],fx);self.put(self.type+spec['foot']+4,fy)
        self.put(self.type+spec['foot']+16,height)
        self.mu.mem_write(OUTPUT,bytes(16))
        if kind=='M':
            entry=spec['muzzle'];this=self.slot(TARGET);args=(OUTPUT,);width=2
        elif kind=='D':
            entry=spec['dimensions'];this=self.type;args=(OUTPUT,OUTPUT+4,0);width=2
        else:
            entry=spec['bounds'];this=self.type;args=(OUTPUT,x,y);width=4
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,this)
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(entry,STOP,count=4000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==0x027f
        return ','.join(f'{v:08x}' for v in struct.unpack('<'+'I'*width,self.mu.mem_read(OUTPUT,width*4)))


def main():
    """두 판본의 새 고유240입력과 실제 함수 호출 수/원문 SHA를 고정한다."""
    positions=((0.5,10.25),(94.125,108.375),(-10000,-10000),(255.75,255.5))
    cases=[('M',f,e,sx,sy,bits(x),bits(y),1,1,0) for (x,y),f,e,(sx,sy) in itertools.product(positions,range(4),(0,1),((16,11),(3,7),(17,13)))]
    dims=((1,1,0),(3,4,7),(20,30,50))
    corners=positions+((1,1),(0,0),(256,257))
    cases += [('B',0,0,16,11,bits(x),bits(y),fx,fy,h) for (x,y),(fx,fy,h) in itertools.product(corners,dims)]
    cases += [('D',0,0,16,11,bits(94),bits(108),fx,fy,h) for fx,fy,h in dims]
    rows,editions=[],{}  # 현재 독립 입력과 판본별 실행 근거다.
    # 두 보호 PE는 원본 파일 변경 없이 각각 별도 에뮬레이터에서 실행한다.
    for edition in SPEC:
        oracle=ProjectileGeometryOracle(edition)
        for values in cases:
            rows.append((edition,*values,oracle.sample(*values)))
        editions[edition]=dict(samples=len(cases),returns=len(cases),assertions=oracle.assertions,
            peSha=oracle.sha256,exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.paths},nativeCalls=dict(oracle.native_calls))
        print(edition,len(cases),'정상 반환',flush=True)
    FIXTURE.write_text('# 실제 PE 포구/치수/상자. 기본프레임1은 명시 외부 가상78 입력; x87=027f.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),editions=editions,
        fixtureSha=digest(FIXTURE),toolSha=digest(ROOT/'tools/decomp_projectile_geom.py'),
        limitations=['합성 TYPE71/SHP signed WORD/위치 입력이며 전체 캠페인 발사 검증은 별도다.',
            '가상78 기본프레임1만 명시 외부 입력이다. 포구/SHP lookup/좌표/치수/상자/helper는 실제 몸체다.',
            '유한 좌표와 작은 정수 치수만 검증한다. NaN/정수 덧셈 overflow/범위 밖 프레임은 포함하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':
    main()
