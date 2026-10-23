#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""NSInfo의 실제 일반Attach와 meta 관문을 기존 격리CPU 장치로 독립 관측한다."""
import hashlib
import itertools
import json
import socket
from pathlib import Path
import decomp_hud_dock_geometry as raw
from unicorn.x86_const import UC_X86_REG_EIP

ROOT=Path(__file__).resolve().parents[1]  # 보호입력과 새 산출물의 공통루트다.
FIXTURE=ROOT/'cpppj/tests/fixtures/hud-nsinfo-attach-x86.tsv'  # 입력6열/실제관측8열이다.
REPORT=ROOT/'extracted/cpp-hud-nsinfo-attach-20261023-native.json'  # 원문/재사용장치와 새관측을 식별한다.
PREFIX=(0x47e9d0,0x47ea71)  # 일반부착과 두 관문까지의 실제 본체다.
EPILOGUE=(0x47efdb,0x47efe5)  # 분기합류의 실제 pop/ret16이다.
META_LAYOUT=0x47ea71  # 여기부터의 전체메타맵은 별도 명시 효과 경계다.
BASE_GLOBAL_COUNT=len(raw.GLOBALS)  # 기존 순수장치/화면전역 전체는 보존해 함께 비교한다.
raw.RANGES={'reflow':PREFIX,'epilogue':EPILOGUE,'move':(0x464d10,0x464d56)}  # 실제본체와 호출되지 않는 공통Move 가상표주소다.
raw.GLOBALS.update({'metamap':0x594fc8,'playerKnown':0x595344})  # 같은 현재 두전역을 호출 위치에서 읽는다.
raw.FIELDS['background']=0xa0  # 콜백에서 바뀔 수 있는 실제 NSInfo 배경 포인터다.


class NsInfoOracle(raw.Oracle):
    """NSInfo 일반기계어는 실제실행하고 미복원메타맵의 진입만 관측한다."""
    def instruction(self,mu,address,size,data):
        """일반분기가 합류하는 RET와 명시후속메타맵을 구별한다."""
        if address==META_LAYOUT:
            self.event('metamapLayout',[1])
            if not self.stopped:mu.reg_write(UC_X86_REG_EIP,EPILOGUE[0])
            return
        super().instruction(mu,address,size,data)


def make_case(name,width,height,scale,metamap,known,mutation='-',parent=0):
    """입력raw와 명시외부효과만 만들고 기대기하값은 만들지 않는다."""
    data,globals_,put=raw.defaults()
    put(0,0xa0,raw.node(1));put(0,0x3c,raw.bits(width));put(0,0x40,raw.bits(height))
    put(1,0x38,raw.bits(scale));put(2,0x38,raw.bits(scale*0.75))
    globals_.extend((metamap,known))
    return dict(zip(raw.INPUTS,(name,'reflow',data.hex(),':'.join(map(str,globals_)),':'.join(map(str,(raw.bits(-2.25),raw.bits(7.75),parent,1))),mutation)))


def cases():
    """현재배경 교체/크기/배율/두관문/중단을 독립입력으로 분해한다."""
    rows=[]
    for width,height,scale,mode,known in itertools.product((-1.25,0,80.125,16777216),(0,75.25,768),(0.5,1,1.25),(0,1),(0,1)):
        rows.append(make_case('normal%04d'%len(rows),width,height,scale,mode,known))  # 일반기하와 관문조합을 모두 관측한다.
    effects=(
        'baseAttach1/node0/background/'+str(raw.node(2)),
        'push1/node0/background/'+str(raw.node(2)),
        'push1/node0/baseWidth/'+str(raw.bits(97.375)),
        'push1/node0/baseHeight/'+str(raw.bits(46.875)),
        'push1/node1/scale/'+str(raw.bits(1.75)),
        'attach1/global/metamap/1;attach1/global/playerKnown/1',
        'attach1/global/metamap/0',
        'baseAttach1/control/stop/1',
        'push1/control/stop/1',
        'attach1/control/stop/1',
        'metamapLayout1/control/stop/1',
    )  # 외부장치의 명시변경과 실제 부분쓰기 경계다.
    for effect in effects:
        rows.append(make_case('mutation%04d'%len(rows),80.125,75.25,1.25,1,1,effect,parent=3))  # 기대출력과 무관한 현재조회 입력이다.
    return rows


def main():
    """보호원문을 고정하고 두x87 정밀도와 명시범위를 새파일로 저장한다."""
    if socket.gethostname().lower()!='vm-w11-codex':raise RuntimeError('현재/최종디컴파일 PC 불일치')
    if hashlib.sha256(raw.PE_PATH.read_bytes()).hexdigest()!=raw.PE_SHA:raise RuntimeError('원문SHA 불일치')
    oracle=NsInfoOracle();inputs=cases();rows=[];differences=0
    for case in inputs:
        observed=[]
        for control in raw.CONTROLS:
            output=oracle.run(case,control);observed.append(output);rows.append([case[key] for key in raw.INPUTS]+output)  # 제어어별 독립실행이다.
        if observed[0][:-2]!=observed[1][:-2]:differences+=1
    FIXTURE.write_text('\t'.join(raw.INPUTS+raw.OUTPUTS)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    report={'host':socket.gethostname(),'decompileHost':'VM-W11-Codex','decompileDate':'2026-10-23','peSha256':raw.PE_SHA,'schema':list(raw.INPUTS+raw.OUTPUTS),'globals':raw.GLOBALS,'nodeFields':raw.FIELDS,'nodeAddress':raw.NODES,'nodeStride':raw.STRIDE,'nodeBytes':0x11c,'nodes':8,'cases':len(inputs),'rows':len(rows),'completed':sum(row[6] for row in rows),'precisionDifferences':differences,'nativeInstructions':sum(row[-2] for row in rows),'crtInstructions':sum(row[-1] for row in rows),'calls':dict(oracle.calls_total),'writes':dict(oracle.writes),'fixtureSha256':hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),'toolSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'deviceToolSha256':hashlib.sha256(Path(raw.__file__).read_bytes()).hexdigest(),'boundaries':{hex(k):v for k,v in raw.BOUNDARIES.items()},'scope':['0047e9d0~0047ea71 및0047efdb합류RET실제실행','일반기본Attach/배경Push/배경Attach만외부공통Gump장치','meta&&known진입시47ea71이후전체메타맵은명시metamapLayout경계이며본문미실행','모든입력/출력폭은기존HUDoracle와같은8*0x11c;NSInfo실제확장+ a0~d4만제품사용','생성자/PlayerInfo/BattleInfo/전체NSInfo및GUI완성의검증아님']}
    report['boundaries'][hex(META_LAYOUT)]='metamapLayout'
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({key:report[key] for key in ('cases','rows','completed','precisionDifferences','nativeInstructions','crtInstructions','fixtureSha256')},ensure_ascii=False))


if __name__=='__main__':main()
