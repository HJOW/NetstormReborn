#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""선택 Fort 소비자와 같은 초기 자료에서 기존 실제 Reset/PlaceAll의 전체 출력을 관찰한다."""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path
from decomp_islandbuilder_oracle import IslandBuilderOracle,BUILDER,RECORDS,CODES,CHUNK_BYTES,ISLAND_BYTES
from decomp_owner_oracle import ROOT,TYPES,STOP,STACK,digest
from decomp_priestgeometry_oracle import GEOMETRY
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import UC_X86_REG_ECX

# 새 부모534와 분리한 기존 Builder 소비자 골든의 파일과 실제 Reset 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/fortworldload-builder-consumer-x86.tsv'
REPORT=ROOT/'cpppj/fortworldload-builder-consumer.json'
RESETS={'originals':0x46edf0,'originalCD':0x4bd450}
# C++ 선택 소비자가 공급하는 물리 Territory120BYTE와 같은 puzzlePiece 코드 입력이다.
TERRITORY=bytes((3,1,0x11,7,0xaa,0xbb))+bytes(114)
PIECES=b''.join(bytes((side,ord('P'),number,0)) for side in range(ord('A'),ord('P')+1) for number in range(11))


class BuilderConsumerOracle(IslandBuilderOracle):
    """기존 완료 몸체만 실행하며 새 부모나 지형/Farm의 완료 수를 추가하지 않는다."""
    def __init__(self,edition):
        """현재 PC의 기존 Reset 및 CD 기록 Reset 범위를 기존 실제 배치 범위에 연결한다."""
        super().__init__(edition)
        paths=[ROOT/f'extracted/island-runtime/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.exports.extend(paths)
        entries={RESETS[edition]}
        if edition=='originalCD':entries.add(0x4bd3f0)
        with paths[1].open(encoding='utf-8-sig') as stream:
            selected=[row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16) in entries]
        assert len(selected)==len(entries)
        # 허용 범위는 실제 내보낸 불연속 명령이며 임의 바깥 helper는 실행하지 않는다.
        for row in selected:
            self.entries.add(int(row['entry'],16))
            for span in row['ranges'].split(';'):
                low,high=(int(value,16) for value in span.split('-'))
                self.allowed[low-0x400000:high+1-0x400000]=b'\1'*(high+1-low)
        self.reset_ranges={row['entry']:row['ranges'] for row in selected}

    def run_consumer(self,control):
        """zero 입력에 실제 Reset 후 기본 ChunkMap/같은 코드/레코드로 PlaceAll을 실행한다."""
        self.kind='FortBuilderConsumer';self.values=();self.events=[];self.assertions=0
        self.native=collections.Counter();self.primary_calls=collections.Counter();mu=self.mu;s=self.s
        mu.mem_write(s['world'],struct.pack('<III',1,16,16)+(bytes((127,0))+bytes(10))*256)
        mu.mem_write(s['islands'],bytes(ISLAND_BYTES))
        self.prepare_registers(control);mu.reg_write(UC_X86_REG_ECX,s['islands'])
        mu.mem_write(STACK,struct.pack('<I',STOP));mu.emu_start(RESETS[self.edition],STOP,count=600000);self.check(control,STACK+4)
        before_islands=bytes(mu.mem_read(s['islands'],ISLAND_BYTES))
        before_chunks=bytes(mu.mem_read(s['world']+12,CHUNK_BYTES-12))
        self.call(s['constructor'],[],control);self.call(s['attach'],[s['world']],control)
        mu.mem_write(RECORDS,TERRITORY);self.put(self.spec['globals']['types'],TYPES)
        # 현재 puzzlePiece 전역과 불변 물리 코드는 C++ 소비자 입력과 동일하다.
        for address,value in zip(GEOMETRY[self.edition]['patterns'],(107,82,94,157,131,129,140,142),strict=True):self.put(address,value)
        typ=TYPES+107*self.type_stride;mu.mem_write(typ,bytes(self.type_stride))
        self.put(typ+0x114,len(PIECES)//4);self.put(typ+0x124,CODES);mu.mem_write(CODES,PIECES)
        if self.edition=='originals':self.put(0x5e4794,0)
        self.call(s['all'],[RECORDS,0,20],control)
        after_chunks=bytes(mu.mem_read(s['world']+12,CHUNK_BYTES-12));after_islands=bytes(mu.mem_read(s['islands'],ISLAND_BYTES))
        add=0x46ee60 if self.edition=='originals' else 0x4bd500
        bounds=0x46f190 if self.edition=='originals' else 0x4bd900
        return [TERRITORY.hex(),PIECES.hex(),before_chunks.hex(),before_islands.hex(),after_chunks.hex(),after_islands.hex(),
            str(self.primary_calls[f"{s['place']:08x}"]),str(self.native[f'{add:08x}']),str(self.native[f'{bounds:08x}']),str(self.assertions)]


def generate():
    """두 소비자 입력의 전체 바이트와 두 x87의 정상 ABI를 별도 보고서로 기록한다."""
    if REPORT.exists() or FIXTURE.exists():raise RuntimeError('소비자 골든 덮어쓰기 금지')
    rows=[];editions={};paths={Path(__file__).resolve(),ROOT/'tools/decomp_islandbuilder_oracle.py',ROOT/'tools/decomp_owner_oracle.py',
        ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/decomp_canontype_oracle.py'}
    # 재사용한 생산 몸체 전체만 연결하며 기존 native 완료 계수를 중복 합산하지 않는다.
    for name in ('RawIslandBuilder','RawIslandRuntime','ChunkMap','CanonDecoder','RiftType'):
        for extension in ('h','cpp'):paths.add(ROOT/f'cpppj/src/o/{name}.{extension}')
    paths.add(ROOT/'cpppj/src/o/CanonDecoderTables.inc')
    for edition in RESETS:
        oracle=BuilderConsumerOracle(edition);observed=[oracle.run_consumer(control) for control in CONTROLS]
        assert observed[0]==observed[1],edition
        rows.append([edition,*observed[0]]);editions[edition]=dict(consumer_unique=1,normal_unique=1,assert_unique=0,controls=list(CONTROLS),
            top_level_returns=8,native_calls=dict(oracle.native),reset_ranges=oracle.reset_ranges)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: 소비자 골든1/정상1/실제 Place/Add/Bounds={observed[0][-4:-1]}',flush=True)
    FIXTURE.write_text('# edition\tTerritory120\tphysicalPuzzleCodes\tinitialChunk3072\tinitialIsland3204\tplacedChunk3072\tplacedIsland3204\tPlaceCalls\tAddCalls\tBoundsCalls\tasserts\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');paths.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-14',consumer_unique=2,normal_unique=2,assert_unique=0,
        new_body_completion_unique=0,parent_native_unique_unchanged=534,game_runs=0,os_calls=0,editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limitations=[
        '같은 선택 입력의 기존 Reset/Builder ctor/attach/PlaceAll/Add/Bounds/CanonDecoder만 실제 실행한다. 기존1156/2018 완료를 다시 합산하지 않는다.',
        '이 두 골든은 부모534와 별도이며 Generate/Stamp/Farm/Objects/Money/Meta/Graph의 원본 실행 결과가 아니다.',
        'Farm 목표60과 실제 Retype 횟수는 다르다. MayReplace의 return1도 목표를 소비하므로 범위 검사는 연결 확인이며 Farm 전체 동일성 완료 근거가 아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """전체 초기/출력 바이트와 정확 호출수·별도 계수·현재 출처를 엄격히 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for name,value in report['files'].items():assert digest(ROOT/name)==value,name
    assert len(rows)==len({row[0] for row in rows})==report['consumer_unique']==2
    assert report['new_body_completion_unique']==0 and report['parent_native_unique_unchanged']==534
    for row in rows:
        assert len(row)==11 and row[-1]=='0'
        assert len(bytes.fromhex(row[3]))==len(bytes.fromhex(row[5]))==3072
        assert len(bytes.fromhex(row[4]))==len(bytes.fromhex(row[6]))==3204
        assert row[1]==TERRITORY.hex() and row[2]==PIECES.hex()
        assert report['editions'][row[0]]['top_level_returns']==8
    print('Fort Builder 소비자 골든 감사: 정상2/assert0, 전체3072/3204 BYTE, 새몸체완료0/부모534불변')


# 최초 실행만 새 골든을 만들고 --verify는 기존 whole 결과를 읽기만 한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true')
    verify() if parser.parse_args().verify else generate()
