#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제건설확정+메시지생성의전체512바이트와가변길이최종화를격리관찰한다."""
import csv
import json
import struct
import sys
from decomp_constructionconfirm_oracle import ConfirmOracle,confirm_case,PLAIN,NOISLAND,ISLAND,BRIDGE,ROOT,CONTROLS
from decomp_owner_oracle import STACK,STOP,PLAYERS,digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

# 별도최종화검사의패킷입력주소와최종자료경로다.
PACKET=PLAYERS+0x1000
FIXTURE=ROOT/'cpppj/tests/fixtures/construction-packet-x86.tsv'
REPORT=ROOT/'cpppj/construction-packet.json'


class Oracle(ConfirmOracle):
    """기존실제Confirm에서메시지생성경계만실제몸체로확장한다."""
    def __init__(self):
        """PATCH의실제헤더생성과가변길이helper범위를정밀함수표에서선택한다."""
        super().__init__('originals');del self.stubs[self.c['message']]
        facts={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 보호PE의두작은실제몸체이외에새명령을허용하지않는다.
        for address in (0x4df590,0x40a800):
            self.entries.add(address)
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """헤더생성은실행하고최종화의count255진단만관찰뒤계속한다."""
        if address==self.c['message']:
            message,length,protocol,capacity,buffer=self.args(5);assert capacity==512;self.buffer=buffer
        if address==self.spec['assert_report']:
            self.events.append('Assert');self.ret();return
        super().on_instruction(mu,address,size,data)

    def stub_send_notice(self):
        """실제송신호출위치에서전체임시버퍼를복사한뒤기존통지관찰도유지한다."""
        message,=self.args(1);self.packet=bytes(self.mu.mem_read(message,512));super().stub_send_notice()

    def sample(self,case,header):
        """입력헤더와원본요청을전체Confirm몸체에넘겨실제필드/SID/잔존값을얻는다."""
        self.mu.mem_write(0x52edea,bytes(header[:2]));self.mu.mem_write(0x544c58,struct.pack('<I',header[2]))
        observed=self.confirm(case,CONTROLS[0]);assert observed[1]==1
        size=self.packet[26];sids=struct.unpack_from('<'+'H'*size,self.packet,27)
        keys=('type','x','y','argument','direction','player','flags','quality','timeLow','timeHigh')
        return ['Write',','.join(str(case[k]) for k in keys),','.join(map(str,header)),','.join(map(str,sids)) or '-',self.packet.hex(),'-']

    def finalize(self,count,sending):
        """현재버퍼의최종화만실제thiscall로실행하여전체512바이트/진단을관찰한다."""
        self.events=[];raw=bytearray((i*37+11)&255 for i in range(512));raw[26]=count;self.mu.mem_write(PACKET,bytes(raw))
        self.write_ranges=[(PACKET,PACKET+512)];self.prepare_registers(CONTROLS[0]);self.mu.reg_write(UC_X86_REG_ECX,PACKET)
        self.mu.mem_write(STACK,struct.pack('<II',STOP,sending));self.mu.emu_start(0x40a800,STOP,count=1000);self.check(CONTROLS[0],STACK+8)
        return ['Finalize',str(sending),str(count),raw.hex(),bytes(self.mu.mem_read(PACKET,512)).hex(),';'.join(self.events) or '-']


def main():
    """원본자료를생성하거나현재구현/fixture와의SHA연결을재실행없이감사한다."""
    if '--verify' in sys.argv:
        report=json.loads(REPORT.read_text(encoding='utf-8'));assert all(digest(ROOT/p)==v for p,v in report['sha256'].items())
        assert len(FIXTURE.read_text(encoding='utf-8').splitlines())-1==report['observations'];print('verified',report['observations']);return
    oracle=Oracle();rows=[]
    # decoder결과가한칸/아홉칸/여러다리패턴/빈목록인실제확정요청이다.
    for typ,argument,default in ((PLAIN,0xffffffff,3),(NOISLAND,0,3),(ISLAND,1,3),(BRIDGE,0,3),(BRIDGE,25,3),(PLAIN,0,0xffffffff)):
        for direction in (0,2,6):
            for header in ((63,1,0),(128,255,3),(255,128,65535),(1,7,65536)):
                case=confirm_case(type=typ,argument=argument,default=default,direction=direction,player=0x1ff,flags=0x1234,quality=255,timeLow=0x89abcdef,timeHigh=0x7ff80000)
                rows.append(oracle.sample(case,header))
    # 최종화0은진단하지않고현재WORD도보존한다.255는진단뒤537길이를쓴다.
    for count in (0,1,9,19,20,254,255):
        for sending in (0,1):rows.append(oracle.finalize(count,sending))
    with FIXTURE.open('w',encoding='utf-8',newline='') as fp:
        writer=csv.writer(fp,delimiter='\t',lineterminator='\n');writer.writerow(('operation','input','header','sids_or_before','packet','events'));writer.writerows(rows)
    paths=['originals/Netstorm.exe','tools/decomp_construction_packet.py','tools/decomp_constructionconfirm_oracle.py','tools/decomp_constructionplace_oracle.py','tools/decomp_owner_oracle.py',
        'extracted/constructionconfirm/originals/creation.c','extracted/constructionconfirm/originals/functions.tsv','extracted/refined/originals/functions.tsv',
        'cpppj/src/o/RawConstructionPacket.h','cpppj/src/o/RawConstructionPacket.cpp','cpppj/tests/ConstructionPacketTests.cpp','cpppj/tests/fixtures/construction-packet-x86.tsv']
    REPORT.write_text(json.dumps(dict(host='VM-W11-Codex',date='2026-10-15',observations=len(rows),write=72,finalize=14,normalReturns=len(rows),
        limits=['Confirm/Canon/512BYTE초기화/SID목록/모든필드와가변길이helper를실제명령으로실행','Factory/전송/자기통지/기록기및count255assert는명시관찰경계','가변길이최종화는실제소켓송신과별도호출하며네트워크/GUI실행0'],sha256={p:digest(ROOT/p) for p in paths if (ROOT/p).exists()}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('observed',len(rows))


if __name__=='__main__':main()
