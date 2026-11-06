#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미분류 파일 소비자42cd10의 정적 출처와 폐기된 CD 확인 프로토콜의 제품 차단 지점을 기록한다."""
import csv
import hashlib
import json
import socket
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent  # 같은 저장소의 읽기 전용 원본/정밀 자료를 사용한다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile

def digest(path):
    """원시 바이트 지문을 계산하며 파일을 실행하거나 수정하지 않는다."""
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
    """주소/호출/상수와 실제 제품 guard만 정적으로 읽고 보고서를 생성한다."""
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
    with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        functions={int(r['entry'],16):r for r in csv.DictReader(stream,delimiter='\t')}
    source=(ROOT/'extracted/refined/originals/Netstorm.c').read_text(encoding='utf-8')
    start=source.index('// ==== FUN_0042cd10 @');end=source.index('// ==== ',start+8);body=source[start:end]
    assert functions[0x42cd10]['data_ref_from']=='40c26d' and functions[0x42cd10]['call_refs']=='0'
    registration=image[0x40c26d-base:0x40c279-base];assert registration==bytes.fromhex('6810cd42006a7fe8d72a0d00')
    for literal in ('FUN_0042cc10','FUN_00440d40','FUN_0041a700','FUN_0041a7a0(local_330,local_334,4,0xffffffff)','FUN_004e2dc0(0xb,local_338)'):  # 의미 분류를 뒷받침하는 실제 부모 호출만 확인한다.
        assert literal in body,literal
    message=image[0x52eea4-base];version=image[0x52eea5-base]&0x7f;length=struct.unpack_from('<H',image,0x52eea8-base)[0]
    assert (message,version,length)==(0x80,1,8)
    product=ROOT/'cpppj/src/zacket/LocalMissionTransport.cpp';text=product.read_text(encoding='utf-8');guards=[]
    for number,line in enumerate(text.splitlines(),1):  # 실제 함수 선언과 단일/집합 수신·송신의 세 선택 위치를 기록한다.
        if 'IsRetiredCdValidationPacket' in line: guards.append({'line':number,'text':line.strip()})
    assert len(guards)==4
    tracked=['AGENTS.md','originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','cpppj/src/zacket/LocalMissionTransport.cpp','cpppj/src/client/EditorLocalTransport.cpp','cpppj/src/o/RawPacketDispatch.cpp','tools/cpp_retired_cd_validation_audit.py']
    report={'unit':965,'host':socket.gethostname(),'lastPreciseDecompile':{'host':'HJOW-Athlon','date':'2026-11-03'},'kind':'read-only PE/source inventory; no emulation/game/network/protocol tests',
        'original':{'requestType':127,'registrationParent':'0040b4b0','registrationReference':'0040c26d','registrationBytes':registration.hex(),'receive':'0042cd10','decoder':'0042cc10','cdRoot':'00440d40','seek':'0041a700','read':'0041a7a0','readBytes':4,'send':'004e2dc0','destination':11,'replyConstructor':'0040b420','replyType':message,'replyVersion':version,'replyLength':length,'purpose':'원격 지정 CD 파일/offset의4BYTE 확인 응답. 클라이언트 검증 용도 판정은 이 동작과 이전 검증 비활성화 정책을 함께 근거로 한 분류다. 고정 원격 도메인 또는 다른 검증 전체와의 동일성을 새로 증명하지 않는다.'},
        'product':{'guards':guards,'sharedBy':'LocalMissionTransport and EditorLocalTransport.DispatchFrame','genericRawPacketDispatchUnchanged':True,'newReceiveOrReplyImplemented':False,'validationSuccessSynthesized':False},
        'scopeCorrection':'785의 미분류42cd10 seek 부모는 일반 비멀티 FILE 복원 대상에서 폐기된 CD 확인 경로로 재분류한다. 원본 callback/파일 조회/응답은 제품에 연결하지 않는다.',
        'sha256':{name:digest(ROOT/name) for name in tracked},'emulations':0,'gameExecutions':0,'networkExecutions':0,'newProtocolTests':0}
    (ROOT/'cpppj/retired-cd-validation-static-20261106.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('static inventory: request 7f -> CD file read 4 -> recipient 11/reply 80; three product guard sites')

if __name__=='__main__':
    main()
