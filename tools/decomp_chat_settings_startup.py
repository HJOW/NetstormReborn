#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Chat 초기 Subscriber별 설정 조회와 미션 Init의 현재ID 복사 뒤 서식 보존을 관측한다."""
import argparse
import csv
import json
import socket
from decomp_chat_manager_prefix import Oracle as BaseOracle,ROOT,CURRENT,SUBSCRIBER,digest
from unicorn.x86_const import UC_X86_REG_ESP

LOGIN=0x54265c  # 미션 Init이 읽는 원본 Login Subscriber DWORD다.


class Oracle(BaseOracle):
    """실제42e050/42e000과4c3355~3361을 실행하며 Config만 명시 저장값으로 응답한다."""
    def __init__(self,saved):
        """원본 PE 초기값과 독립 설정표/조회 순서만 보관한다."""
        super().__init__();self.saved=saved  # 실제 ReadString 경계가 반환할 독립 저장값 표다.
        self.keys=[]  # 원문이 실제 요청한 signed Subscriber 설정 키 순서다.
        self.micro=[]  # 실제 미션 Subscriber 복사 명령의 진입 주소다.
        self.initial=self.words(SUBSCRIBER,1)[0]  # 원본 PE의 초기My이며 C++ 결과에서 가져오지 않는다.

    def instruction(self,mu,address,size,data):
        """설정 응답은 입력이며 조회 키/복사 대상/미션ID 저장은 원문이 결정한다."""
        if 0x4c3355<=address<0x4c3362:
            self.micro.append(hex(address));return
        if address==0x441300:
            key,target=self.words(mu.reg_read(UC_X86_REG_ESP)+4,2);name=self.string(key).decode('ascii');self.keys.append(name)
            value=self.saved.get(name);self.event('read','R:'+name+':'+str(value))
            if value is not None: mu.mem_write(target,value.encode('ascii')+b'\0')
            self.ret(int(value is not None));return
        super().instruction(mu,address,size,data)

    def observe(self,subscriber,control):
        """초기화 뒤 실제 ID 복사만 실행하고 기존 manager의 서식이 바뀌는지 관측한다."""
        case=(1,0,2,1,3,subscriber,0,3,31,1,0,0,'none')
        self.sample(case,control);assert self.status=='normal' and self.initial==0xffffffff;current=self.words(CURRENT,1)[0]
        before={'subscriber':self.words(SUBSCRIBER,1)[0],'front':self.string(current+0x50).decode('ascii'),'back':self.string(current+0x14).decode('ascii')}
        self.put(LOGIN,1);self.mu.emu_start(0x4c3355,0x4c3362,count=10)
        after={'subscriber':self.words(SUBSCRIBER,1)[0],'front':self.string(current+0x50).decode('ascii'),'back':self.string(current+0x14).decode('ascii')};assert self.micro==['0x4c3355','0x4c335b','0x4c335c']
        return {'inputSubscriber':subscriber,'saved':self.saved,'peInitialMy':self.initial,'readKeys':self.keys,'beforeMissionCopy':before,'afterMissionCopy':after,'sameManager':self.words(CURRENT,1)[0]==current,'missionInstructions':self.micro}


def main():
    """독립 두 정밀도의 실제 결과를 새 경로와 현재 근거 파일에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True);args=parser.parse_args();output=ROOT/args.output;output.mkdir(parents=True,exist_ok=False)
    saved=({}, {'1chatFormat2':'~g'}, {'-1chatFormat2':'~g'}, {'1chatFormat2':'~r','-1chatFormat2':'~g'})  # 초기ID/미션ID의 두 저장 키를 구분하는 입력이다.
    values=[(subscriber,settings) for subscriber in (0xffffffff,1) for settings in saved]  # 같은 설정표를 서로 다른 현재ID로 읽는다.
    first=[Oracle(settings).observe(subscriber,0x27f) for subscriber,settings in values]
    replay=[Oracle(settings).observe(subscriber,0x37f) for subscriber,settings in values];assert first==replay
    for name,observed in (('first.json',first),('replay.json',replay)):  # 최초 관측 파일을 덮어쓰지 않는다.
        (output/name).write_text(json.dumps(observed,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_chat_manager_prefix.py','tools/decomp_chat_settings_startup.py')
    with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}  # 같은PC 정밀 함수표의 실제 참조를 읽는다.
    local_calls=facts[0x4b63d0]['calls'].split(',');assert '42e050' not in local_calls and '42e050' in facts[0x4b4480]['calls'].split(',')
    report={'unit':614,'host':socket.gethostname(),'lastFullDecompilation':'HJOW-Athlon / 2026-10-23','cases':len(values),'observations':2*len(values),'differences':[],'observed':first,'staticLocalLoginCalls':local_calls,'staticChatInitializerCallers':facts[0x42e050]['callers'].split(','),'sourceSha256':{name:digest(ROOT/name) for name in paths},'scope':'격리x86의 실제Chat 초기화/생성자와 미션Init ID복사3명령이다. Config/할당/명령등록/CRT는 기존 명시 경계다. 원본전체startup/게임GUI/MP를 실행한 표본은 아니다. 싱글로그인은4b63d0이며4b4480의Chat재초기화와구별한다.'}
    (ROOT/'cpppj/chat-settings-native-20261030.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'cases':len(values),'observations':2*len(values),'differences':[]}))


if __name__=='__main__':
    main()
