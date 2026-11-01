#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 TARC의 실제 세 번역 파일을 원문 Xlat 본체로 읽어 전체 수정 버퍼를 관측한다(711)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
from decomp_xlat_runtime import Oracle, OWNER, QUERY, hextext
from decomp_oracle import ROOT, STOP, STACK
from taff import TaffArchive
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP


def load_asset(native,which):
    """같은 살아 있는 CPU 호출이 시간/명령 경계에 도달하면 그 PC부터 이어 실행한다."""
    try: return native.call('load',[which,QUERY],OWNER,8)
    except RuntimeError as error:
        if str(error)!='load: 제한 내에 반환하지 않음': raise
    for _ in range(120):  #최대0.5초씩 같은 CPU 상태를 재개하며 파일/장부/스택을 다시 시작하지 않는다.
        native.mu.emu_start(native.mu.reg_read(UC_X86_REG_EIP),STOP,timeout=500000,count=200000)
        if native.mu.reg_read(UC_X86_REG_EIP)==STOP:
            assert native.mu.reg_read(UC_X86_REG_ESP)==STACK+12;return native.mu.reg_read(UC_X86_REG_EAX)
    raise RuntimeError('실제 Xlat 파일의 동일 호출이 재개 한도 안에 반환하지 않음')


def main():
    """이미 생성된 경계 입력 fixture는 보존하고 실제 자산 관측을 별도 새 fixture에 쓴다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists(),'최초 관측 보존'
    archive=TaffArchive(str(ROOT/'originals/netstorm.tarc'));entries={entry.name.replace('\\','/').strip('/').lower():entry for entry in archive.entries};native=Oracle();native.reset();rows=['# actual TARC C/L/Q; expected buffers and values from native code','C\t0'];sources=[]
    for which,name in ((3,'german'),(1,'english'),(6,'portuguese')):  #원문의 실제 시작 순서로 전체 파일을 공급한다.
        path='d/xlat.'+name;source=archive.read(entries[path]);native.files={b'd/case':source};native.mu.mem_write(QUERY,b'case\0');returned=load_asset(native,which);assert returned==1;count=0
        for bucket in range(256):  #실제 Native 해시 목록의 고유 노드 수를 읽는다.
            node=native.get(OWNER+0x1c+bucket*36)
            while node: count+=1;node=native.get(node)  #같은 버킷의 실제 next 목록을 끝까지 센다.
        pointer=native.get(OWNER+which*4);buffer=bytes(native.mu.mem_read(pointer,len(source)));rows.append(f'L\t{which}\t{source.hex()}\t{returned}\t{buffer.hex()}\t{count}');sources.append(dict(path=path,bytes=len(source),recordsAfter=count,sha256=hashlib.sha256(source).hexdigest()));print(json.dumps(sources[-1]),flush=True)
    keys=(b'',b'Loading',b'Please Wait',b'Options',b'Direct Draw / Full Screen',b'Window Mode',b'Quit Game',b'Loading\0ignored')  #실제 메뉴/로딩과 미등록/첫NUL을 같은 장부에서 조회한다.
    for language,key in itertools.product((0,1,2,3,6),keys):  #없는 언어 칸과 영어 우회도 포함한다.
        native.put(0x5435b4,language);native.mu.mem_write(QUERY,key+b'\0');pointer=native.call('translate',[QUERY],OWNER,4);rows.append(f'Q\t{language}\t{hextext(key)}\t{hextext(native.read_string(pointer))}')
    args.fixture.write_text('\n'.join(rows)+'\n','utf-8',newline='\n');record=dict(unit=711,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),observations=len(rows)-1,sources=sources,nativeBodies=['004de710','004de260','004de1e0','004de130','004de9a0','004e63e0'],explicitBoundaries='원문 경계 도구와 동일한 파일/할당/memmove 장치. 시간 경계는 같은 CPU PC에서 재개하며 재시작하지 않음',gameExecutions=0,multiplayerTests=0,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),archiveSha256=hashlib.sha256(archive.raw).hexdigest(),peSha256=native.sha256);args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))


if __name__=='__main__': main()
