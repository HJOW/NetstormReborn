#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""SunArcher71의sunDisc83/effect0 생성/프레임을기존실제두Missile PE로별도관찰한다."""
import argparse
import csv
import itertools
import json
import struct
import subprocess
from pathlib import Path
from decomp_gunmissile_oracle import MissileOracle,SPEC,OBJECT,KEYS as BASE_KEYS,inputs as base_inputs,bits
from decomp_owner_oracle import ROOT,TYPES,STACK,digest

# 새단위격자의메타데이터와05단위baseline의구체적인commit lineage다.
KEYS=(*BASE_KEYS,'range','firingSound','impactSound')
BASELINE='4e532fd'
FIXTURE=ROOT/'cpppj/tests/fixtures/archermissile-x86.tsv'
REPORT=ROOT/'cpppj/recovery-archermissile-evidence.json'


def inputs():
    """실제Archer 분기에필요한사거리/사운드와corner/표적상태/이동을독립교차한다."""
    rows=[]
    # 캐논hotspot입력의재방문을제외하고Archer가쓰는실제포구좌표만방문한다.
    for original in base_inputs():
        if original['kind']==0 and any(original[name] for name in ('hotx','hoty','typehotx','typehoty')):continue
        for range_value,firing_sound,impact_sound in itertools.product((9.0,40.0),(0,123),(0,456)):
            value=dict(original);value.update(range=bits(range_value),firingSound=firing_sound,impactSound=impact_sound);rows.append(value)
    return rows


class ArcherMissileOracle(MissileOracle):
    """이미감사한동일PE몸체에새Archer 타입/발사메타데이터를독립입력으로넣는다."""
    def run_body(self,case):
        """기존실행기가준비한독립객체를첫실제명령직전에Archer 입력으로설정한다."""
        self.prepared=False;return super().run_body(case)

    def on_instruction(self,mu,address,size,data):
        """현재Archer 입력과group0자료경계를적용하고실제Missile 명령을그대로실행한다."""
        p=self.p
        if not self.prepared and address==(p['ctor'] if self.case['kind']==0 else p['frame']):
            self.prepared=True;source=TYPES+71*self.type_stride;mu.mem_write(self.slot(50)+10,bytes([71]));mu.mem_write(self.slot(90)+10,bytes([83]));mu.mem_write(source,bytes(self.type_stride));mu.mem_write(source+0x44,struct.pack('<I',self.case['range']));mu.mem_write(source+0x70,struct.pack('<I',self.case['firingSound']));mu.mem_write(TYPES+83*self.type_stride,bytes(self.type_stride));mu.mem_write(TYPES+83*self.type_stride+0x110,struct.pack('<I',1));mu.mem_write(TYPES+83*self.type_stride+0x74,struct.pack('<I',self.case['impactSound']))
            if self.case['kind']==0:
                mu.mem_write(STACK+12,struct.pack('<III',bits(35),83,0))
            else:mu.mem_write(OBJECT+(48 if self.edition=='originals' else 32),struct.pack('<I',bits(35)))
        if address==p['group']:
            # 실제SunArcher .type의group0은현재타입자료query의명시경계다.
            from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)


def baseline_hashes():
    """05 commit의동결된Missile 소스blob을현재수정파일과별도로감사한다."""
    import hashlib
    paths=('cpppj/src/o/RawMissile.cpp','cpppj/src/o/RawMissile.h','cpppj/tests/fixtures/gunmissile-x86.tsv')
    return {path:hashlib.sha256(subprocess.check_output(['git','show',BASELINE+':'+path],cwd=ROOT)).hexdigest() for path in paths}


def generate():
    """새Archer 고유관찰과현재소스SHA/05 baseline lineage를따로저장한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_gunmissile_oracle.py',ROOT/'tools/decomp_owner_oracle.py',FIXTURE,ROOT/'cpppj/src/o/RawMissile.cpp',ROOT/'cpppj/src/o/RawMissile.h',ROOT/'cpppj/tests/ArcherMissileTests.cpp',ROOT/'tools/ghidra/gunmissile-functions.json',ROOT/'tools/ghidra/gunmissile-helper-functions.json'}
    for edition in SPEC:
        oracle=ArcherMissileOracle(edition)
        # 각새입력에서실제동일Missile ctor/Frame의정상반환을기록한다.
        for case in inputs():
            try:result=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=oracle.returns,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),assertions=oracle.assertions,instructions=oracle.instructions);files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,flush=True)
    FIXTURE.write_text('# 실제두PE SunArcher71/sunDisc83/speed35/effect0 Missile 별도관찰. 자료/자산/공간/효과는명시경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},baseline=dict(commit=BASELINE,sha256=baseline_hashes()),metadata=dict(firingType=71,displayType=83,speed=35,effectType=0,displayDirections=1),os_calls=0,limitations=['05 썬캐논296관찰을변경하거나새누적으로중복계산하지않는다.','실제SunArcher Missile 숫자몸체와명시query/자산/공간/사운드/피해/삭제경계를대조한다.','실제Archer 발사각애니메이션과시각충돌효과/표적피해하위는별도단위다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """현재파일SHA/05 baseline blob/새격자의고유입력과정상반환을감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if report['total']!=len(rows) or report['baseline']['sha256']!=baseline_hashes():raise RuntimeError('Archer Missile 행수/baseline lineage 오류')
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Archer Missile SHA 불일치 '+path)
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if item['returns']!=len(selected) or item['assertions'] or len({tuple(row[1:17]) for row in selected})!=len(selected):raise RuntimeError('Archer Missile 고유입력/정상반환 오류')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
