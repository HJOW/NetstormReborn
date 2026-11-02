#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 일반 명령 이름을 실제 원본 lookup/파서/FIFO에서 격리 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import sys

ROOT=Path('H:/T5Workspace/NetstormReborn')  # 같은 호스트의 보호 원본과 정밀 함수표는 읽기만 한다.
sys.path.insert(0,str(ROOT/'tools'))
import decomp_script_command as base
from decomp_cursor_position import ranges


class Oracle(base.Oracle):
    """현호스트 정밀 범위로 실제 이름·토큰·큐 몸체만 허용한다."""
    def __init__(self):
        """기존 원문 관측기의 큐/고리를 초기화하고 전체 함수표 범위를 사용한다."""
        base.MemoryOracle.__init__(self);self.allowed=[]
        functions=json.loads((ROOT/'tools/ghidra/script-command-functions.json').read_text('utf-8'))['editions']['originals']
        ranges(self,tuple(int(entry,16) for entry in functions)+(0x4e63e0,0x4e6fa1,0x4e6656,0x4e65ed,0x4ef510))
        self.put(base.THREAD+0x64,self.words(0x543efc,1)[0]);self.reset_ring();self.allocations=0


def main():
    """새 원시 fixture를 만들며 기존 파일은 동일 바이트인지 확인만 한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);output=parser.parse_args().output.resolve();output.mkdir(parents=True,exist_ok=True)
    oracle=Oracle();lines=['# operation source delimiter existed value next queue ring events']
    names=tuple(value for name in ('CreateInitialGeysers','BrainCrankMoney','AiParamSet') for value in (name,name.lower(),name.upper()))+('MissingAiUtilityCommand',)
    for name,argument in itertools.product(names,('0','3','section')):  # 숫자/문자열 고리와 대소문자를 같은 실제 파서로 보낸다.
        lines.append('\t'.join(oracle.run('Command',(name+','+argument+'>').encode('ascii'))))
    raw=('\n'.join(lines)+'\n').encode('utf-8');fixture=output/'ai-utility-names-x86.tsv'
    if fixture.exists():assert fixture.read_bytes()==raw
    else:fixture.write_bytes(raw)
    record=dict(unit=772,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(lines)-1,nativeLookup='004d52c0',nativeQueueParser='004c9090',nativeTable='00542b90',nativeInstructions=oracle.instructions,gameExecutions=0,multiplayerTests=0,builds=0,productWrites=0,fixtureSha256=hashlib.sha256(raw).hexdigest(),observedToolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),limitations=['실제 이름 lookup/숫자·문자열 파서/FIFO만 관측하며 세 하위 몸체나 GUI를 실행하지 않는다.'])
    (output/'names-native.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8');print(json.dumps(dict(inputs=len(lines)-1,fixtureSha256=record['fixtureSha256'])))


if __name__=='__main__':main()
