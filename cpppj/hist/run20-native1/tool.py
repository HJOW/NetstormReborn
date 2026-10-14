#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 Gun 방향 및 Factory 예약/상태를 두 보호 PE 명령으로 독립 관찰한다."""
import csv
import itertools
import json
import struct
from decomp_assetregular_oracle import AssetRegularOracle, ROOT, TYPES, CODES, STACK, STOP, CALLER, TEMP, UPDATE, VTABLE, sha, bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

SPEC = {  # 새 함수의 실제 진입점과 mutable windArcher 전역이다.
    'originals': (0x4d4d00, 0x4549c0, 0x475290, 0x4ad0f0, 0x541288),
    'originalCD': (0x443110, 0x47ac20, 0x42b230, 0x4ace00, 0x51cb74),
}
FIXTURE = ROOT / 'cpppj/tests/fixtures/campaign-frame-x86.tsv'  # 이번 입력의 새 원본 기대값이다.
REPORT = ROOT / 'cpppj/campaign-frame.json'  # 기존 원본 관찰 수와 분리한 근거다.


class CampaignFrameOracle(AssetRegularOracle):
    """현재 조회/난수/Advance/Level을 실행하고 표시 요청만 명시 관찰한다."""

    def __init__(self, edition):
        """기존 독립 실행기에 같은 PC의 새 함수와 Level 범위를 추가한다."""
        super().__init__(edition)
        self.paths = []  # 이번 실행이 추가로 의존하는 export 원문이다.
        folders = ['campaign-frame', 'squid-level']  # 새 wrapper와 실제 Level 함수다.
        if edition == 'originalCD':
            folders += ['squid-level2', 'squid-level3']
        # 불연속 몸체만 허용하고 사이의 명령/OS는 계속 거부한다.
        for folder in folders:
            self.paths += [ROOT / f'extracted/{folder}/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
            with self.paths[-1].open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp, delimiter='\t'):
                    self.entries.add(int(row['entry'], 16))
                    for part in row['ranges'].split(';'):
                        low, high = (int(v, 16) for v in part.split('-'))
                        self.allowed[low-self.base:high+1-self.base] = b'\x01'*(high+1-low)
        self.put(VTABLE+0x80, SPEC[edition][3])
        self.put(SPEC[edition][4], 140)

    def on_write(self, mu, access, address, size, value, data):
        """Level의 실제 대상 상태 BYTE 하나만 추가 허용한다."""
        if address == self.slot(50)+11 and size == 1:
            return
        super().on_write(mu, access, address, size, value, data)

    def update(self, sid, flags):
        """현재 표시 요청에서 frame과 Level BYTE를 함께 기록한다."""
        self.events.append(f'U:{sid}:{flags}:{self.frame(sid)}:{self.read(self.slot(sid)+11,1)}')

    def sample(self, kind, values):
        """입력만 구성하고 결과/raw/난수/효과는 실제 정상 반환에서 얻는다."""
        patch = self.stride == 50  # 판본별 frame/extra 폭이다.
        self.case = dict(mutation=0)  # 명시 효과가 부모를 변경하지 않는 이번 입력이다.
        self.events = []
        self.setup([(50,82,0,0,bits(20),bits(20),1,1,0,1)])
        self.put(self.slot(50), VTABLE)
        self.put(self.p['random'], 0x12345678)
        base = TYPES+82*self.type_stride  # 합성 TYPE 입력은 파일을 변경하지 않는다.
        self.put(base+0x9c, 10)
        pattern = 'A'*8+'B'*8+'C'*8+'D'*8+'E'*8+'F'*8  # FrameTable 입력 자체다.
        self.mu.mem_write(CODES+82*256, b''.join(bytes((ord(s),65,i,0)) for i,s in enumerate(pattern)))
        self.put(base+0x124, CODES+82*256)
        self.mu.mem_write(base+0x130, struct.pack('<16i',0,8,16,24,32,40,*([-1]*10)))
        self.mu.mem_write(base+0x170, struct.pack('<16i',8,8,8,8,8,8,*([0]*10)))
        entry, args, floating = SPEC[self.edition][0], (), False
        if kind == 'M':
            typ, extra, graph, side, frame = values
            self.put(self.slot(50)+10, typ, 1)
            self.put(self.slot(50)+(40 if patch else 35), extra, 1)
            self.put(self.slot(50)+(30 if patch else 28), graph, 1)
            self.put(TYPES+typ*self.type_stride+0x124, CODES+typ*256)
            self.mu.mem_write(CODES+typ*256, bytes((side,65,0,0))*48)
        else:
            state, frame, event, count = values
            self.put(self.slot(50)+11, state, 1)
            if kind == 'S':
                entry, args = SPEC[self.edition][2], (event,)
            else:
                entry, args, floating = SPEC[self.edition][1], (event,count,bits(0.5)), True
        self.put(self.slot(50)+self.o['frame'], frame, self.o['frame_size'])
        self.mu.reg_write(UC_X86_REG_ECX, self.slot(50))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x037f)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),CALLER if floating else STOP,*args))
        self.mu.emu_start(entry,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+4*len(args)
        result = self.read(TEMP) if floating else self.mu.reg_read(UC_X86_REG_EAX)
        if kind == 'S':
            result = 0  # void 함수의 미정 EAX는 비교 대상이 아니다.
        raw = bytearray(self.mu.mem_read(self.slot(50),self.stride))  # 가상표만 실제 PE 값으로 정규화한다.
        struct.pack_into('<I',raw,0,0x509148 if patch else 0x503298)
        return result, self.read(self.p['random']), raw.hex(), ';'.join(self.events) or '-'


def main():
    """판본별 고유 입력만 집계하고 전체 관찰을 UTF-8/LF로 고정한다."""
    cases = [('M', v) for v in itertools.product((71,140),(0,8),(0,3,255),(0,75,76,77,78,79,80,127,128,255),(0,5))]
    cases += [('S', (s,f,v,0)) for s,f,v in itertools.product((0,4,0x5c),(0,23),(0,1,7))]
    cases += [('H', v) for v in itertools.product((0,32,64),(0,7,8,23),(1,2,4,5,99),(0,1))]
    rows, editions = [], {}  # 새 입력과 실제 반환 보고다.
    # 두 보호 PE를 각각 독립 매핑하고 정상 반환을 검사한다.
    for edition in SPEC:
        oracle = CampaignFrameOracle(edition)
        for kind, values in cases:
            rows.append((edition,kind,','.join(map(str,values)),*oracle.sample(kind,values)))
        paths = set(oracle.body_paths+oracle.paths)  # 이번 실제 실행 범위 원문이다.
        editions[edition] = dict(samples=len(cases),returns=len(cases),assertions=oracle.assertions,
            exports={p.relative_to(ROOT).as_posix():sha(p) for p in paths},nativeCalls=dict(oracle.native_calls))
        print(edition, len(cases), '정상 반환', flush=True)
    FIXTURE.write_text('# 격리 원본 PE, 현재 방향/Factory. 표시 요청은 명시 외부 경계.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),editions=editions,
        fixtureSha=sha(FIXTURE),toolSha=sha(__file_bytes()),protected={e:sha(ROOT/e/('Netstorm.exe' if e=='originals' else 'NETSTORM.EXE')) for e in SPEC},
        limitations=['합성TYPE/슬롯. 실제 방향/Handle/Level 명령과 기존 난수/Advance를 실행했다.',
            'Frame 지정과 표시 요청은 기존 명시 경계다. 큰upgrade/상위상태입자/GUI/실제게임완주는 검증하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def __file_bytes():
    """현재 실행 도구 경로의 전체 바이트를 해시한다."""
    return ROOT / 'tools/decomp_campaign_frame.py'


if __name__ == '__main__':
    main()
