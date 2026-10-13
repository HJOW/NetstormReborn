#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""주변 Dude 실제finder/notice·현재Regular·거주지선택·속도/대피요청을두PE로관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle,ROOT,TYPES,POOL,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제Notice/Nearest/Point와typed조회·Regular/Path 생성경계의판본별주소다.
SPEC={
 'originals':dict(around=0x451470,nearest=0x48fd30,point=0x4505a0,masked=0x4afbe0,new=0x4e4391,attach=0x41bd20,dudeCtor=0x423b20,path=0x489080,fromform=0x4ae2d0,frame=0x4acee0,now=0x55b4d0,residences=0x59a9d8,grid=0x542508,board=0x531928,capacity=0x5c847c,pathType=0x5410d0,dudeType=0x5410a8),
 'originalCD':dict(around=0x4e8e70,nearest=0x406ef0,point=0x4e8a30,masked=0x4ac9c0,new=0x4f1650,attach=0x48f590,dudeCtor=0x4a5a50,path=0x47d150,fromform=0x4af4e0,frame=0x4acbc0,now=0x5484a8,residences=0x565ae0,grid=0x5670c0,board=0x52e9a8,capacity=0x5395f4,pathType=0x51c9c0,dudeType=0x51c998),
}
# 격리객체/코드/거주지목록과네해시배열은호스트와무관한가상메모리다.
REGULAR,PATH,DUMMY,CODES,POINT,LIST,MAP=0x11318000,0x11319000,0x1131a000,0x1131b000,0x1131c000,0x1131d000,0x13000000
SCALES=(1,2,4,16)
KEYS=('kind','dead','task','regular','dude','targetmode','path','made','speed','frame','now')
FIXTURE=ROOT/'cpppj/tests/fixtures/dudenotice-x86.tsv'
REPORT=ROOT/'cpppj/dudenotice.json'


def bits(value):
    """float입력의실제저장비트를공급한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def wide(value):
    """현재게임시각double의비트입력이다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def sample(**kwargs):
    """반환값을계산하지않는현재장면의입력기본값이다."""
    case=dict(zip(KEYS,(0,0,0x5101,0,0,0,0,1,bits(2.75),0,wide(10))));case.update(kwargs);return case


def inputs():
    """task/dead/기존Regular/Dude·거주지/기존경로·분수속도·원본double시각을교차한다."""
    rows=[]
    # 기본Around는각자연반환과실제notice가도달하는두생성요청을포함한다.
    for task,dead,regular,dude,target in itertools.product((0x5101,0x5104,0x5105,0x5107),(0,2),(0,1),(0,1),range(4)):
        rows.append(sample(task=task,dead=dead,regular=regular,dude=dude,targetmode=target))
    # 대피이동의같은endpoint/새경로/실패·속도절삭·I단계재설정과float(now)차이를포함한다.
    for path,speed,frame,made,now in itertools.product(range(3),(2.0,2.75,-2.75),(0,1),(0,1),(10.0,16777217.125)):
        rows.append(sample(regular=1,dude=1,targetmode=1,path=path,speed=bits(speed),frame=frame,made=made,now=wide(now)))
    for mode in range(4):rows.append(sample(kind=1,targetmode=mode))
    for genus in range(4):rows.append(sample(kind=2,targetmode=genus))
    # 중복된기본입력은재방문으로세지않고원본기대값계산없이첫입력만남긴다.
    return list({tuple(row[key] for key in KEYS):row for row in rows}.values())


class DudeNoticeOracle(OwnerOracle):
    """원본상위몸체/선택/숫자쓰기는직접실행하고미복원worker/path ctor는관찰경계다."""
    def __init__(self,edition):
        """현호스트두좁은export만추가하고현재원본코드/네해시/Regular상태를연결한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(0,0x1000);self.mu.mem_map(MAP,0x30000);self.boundaries=collections.Counter();self.returns=0;self.events=[]
        # 관련주몸체와각실제helper의내보낸불연속범위만실행한다.
        for name in ('gunimpact-dude','gunimpact-dude-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 현재Ghidra가실제로내보낸함수body를허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.bases=[];address=MAP
        # 실제256보드의네단계WORD버킷저장소를연결한다.
        for scale in SCALES:self.bases.append(address);address+=(256//scale)**2*2
        self.mapEnd=address;self.mu.mem_write(self.p['grid'],struct.pack('<7I',0,1,256,*self.bases));self.mu.mem_write(self.p['board'],struct.pack('<I',256));self.mu.mem_write(self.p['capacity'],struct.pack('<I',128))

    def read(self,address,width=4):
        """원본비정렬현재필드를읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def marker(self,sid,typeid,nextsid):
        """미복원Dude ctor/기존typed경로의Form효과관찰입력을같은원본체인에공급한다."""
        raw=bytearray(self.stride);raw[10]=typeid;raw[11]=8;struct.pack_into('<HH',raw,4,nextsid,0);self.mu.mem_write(self.slot(sid),bytes(raw))

    def on_instruction(self,mu,address,size,data):
        """기존Regular조회/힙/부착·새Dude/Path 요청·현재경로/Frame효과의경계만관찰한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+i*4) for i in range(8)];purge=0;value=0
        if address==p['masked']:
            self.events.append('R:50:65520:20736');value=REGULAR if self.case['regular'] else 0;purge=8
        elif address==p['new']:
            self.events.append('N:'+str(args[0]));value=REGULAR if args[0]==40 else DUMMY
        elif address==p['attach']:
            self.events.append('B:'+':'.join(map(str,args[:4])));mu.mem_write(this+4,struct.pack('<II',83,50));self.marker(83,46,self.read(self.slot(50)+6,2));mu.mem_write(self.slot(50)+6,struct.pack('<H',83));purge=16
        elif address==p['dudeCtor']:
            self.events.append('D:50:0');self.marker(84,14,self.read(self.slot(50)+6,2));mu.mem_write(self.slot(50)+6,struct.pack('<H',84));value=DUMMY;purge=8
        elif address==p['fromform']:value=PATH
        elif address==p['path']:
            self.events.append('P:'+','.join(map(str,args[:8 if self.edition=='originals' else 7]))+':'+str(self.read(TYPES+70*self.type_stride+0xe0)));value=self.case['made']
        elif address==p['frame']:
            self.events.append('F:50:'+str(args[0])+':'+str(args[1]));mu.mem_write(self.slot(50)+(36 if self.edition=='originals' else 34),struct.pack('<I',args[0]) if self.edition=='originals' else bytes([args[0]]));purge=8
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def prepare(self,case):
        """기대값없이실제원본slot/타입/Regular/거주지순서/endpoint입력만공급한다."""
        self.case=case;self.events=[];mu=self.mu;p=self.p;patch=self.edition=='originals';mu.mem_write(MAP,bytes(self.mapEnd-MAP));mu.mem_write(POOL,bytes(128*self.stride));mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(CODES,b'AP\0\0IP\0\0');mu.mem_write(POINT,bytes(8));mu.mem_write(p['now'],struct.pack('<Q',case['now']));mu.mem_write(p['dudeType'],struct.pack('<I',14));mu.mem_write(p['pathType'],struct.pack('<I',24))
        # 70현재Dude와89거주지의발자국/코드/분수속도자료다.
        for typeid,genus,width,height in ((70,0x400,1,1),(89,0x40000000,3,3)):
            typ=TYPES+typeid*self.type_stride;mu.mem_write(typ+0xec,struct.pack('<I',genus));mu.mem_write(typ+(0x1d4 if patch else 0x1b4),struct.pack('<ii',width,height));mu.mem_write(typ+0x124,struct.pack('<I',CODES));mu.mem_write(typ+0x130,bytes(4))
        if case['kind']==2:mu.mem_write(TYPES+89*self.type_stride+0xec,struct.pack('<I',(0,0x40000000,0x10000000,0x40000)[case['targetmode']]))
        mu.mem_write(TYPES+70*self.type_stride+0xe0,struct.pack('<I',case['speed']))
        # 입력객체는현재source/대피후보두개이며각field는판본원래폭이다.
        for sid,typeid,x,y,owner in ((50,70,20.75,21.9,1),(60,89,24.75,24.5,2 if case['targetmode']==2 else 1),(61,89,16.75,19.3,1)):
            raw=bytearray(self.stride);raw[10]=typeid;raw[11]=case['dead'] if sid==50 else 0;raw[34 if patch else 32]=owner;struct.pack_into('<ff',raw,14,x,y);struct.pack_into('<H',raw,12,case['task'] if sid==50 else 0);struct.pack_into('<I' if patch else '<B',raw,36 if patch else 34,case['frame'] if sid==50 else 0);mu.mem_write(self.slot(sid),bytes(raw))
        head=0
        if case['path']:
            self.marker(78,24,0);head=78;mu.mem_write(PATH,bytes(80));mu.mem_write(PATH+16,struct.pack('<I',1));mu.mem_write(PATH+20,struct.pack('<ff',25 if case['path']==1 else 100,22 if case['path']==1 else 100))
        if case['dude']:self.marker(77,14,head);head=77
        fields=bytearray(40);struct.pack_into('<II',fields,4,83,50);struct.pack_into('<dIII',fields,16,27.0,0x5104,bits(1),7);mu.mem_write(REGULAR,bytes(fields))
        if case['regular']:self.marker(83,46,head);head=83
        mu.mem_write(self.slot(50)+6,struct.pack('<H',head));mu.mem_write(self.bases[1]+((21//2)*128+20//2)*2,struct.pack('<H',50))
        residenceIds=[] if case['targetmode']==0 else [60,61] if case['targetmode']==3 else [60];mu.mem_write(LIST,struct.pack('<'+'I'*len(residenceIds),*residenceIds) if residenceIds else bytes(4));mu.mem_write(p['residences'],struct.pack('<III',LIST,2,len(residenceIds)))
        self.write_ranges=[(0,4),(self.slot(50),self.slot(50)+self.stride),(self.slot(83),self.slot(85)),(REGULAR,REGULAR+80),(DUMMY,DUMMY+160),(TYPES+70*self.type_stride+0xe0,TYPES+70*self.type_stride+0xe4),(POINT,POINT+8)]

    def run(self,case):
        """전체상위정상RET/스택/x87과실제숫자/순서/현재raw·속도원복을기록한다."""
        self.prepare(case);mu=self.mu;p=self.p;kind=case['kind'];args=[bits(20.75),bits(21.9)] if kind==0 else [bits(20.75),bits(21.9),p['residences'],1] if kind==1 else [POINT,60];entry=p['around'] if kind==0 else p['nearest'] if kind==1 else p['point']
        # 각독립호출은현재원본53비트 x87환경을공급한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,0);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.mem_write(0,bytes(4));mu.emu_start(entry,STOP,count=300000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Dude notice 원본RET/스택/x87 오류')
        self.returns+=1;regular='-'
        if case['regular'] or any(event.startswith('B:') for event in self.events):regular=','.join(map(str,(self.read(REGULAR+16,8),self.read(REGULAR+24),self.read(REGULAR+28),self.read(REGULAR+32))))
        return [str(mu.reg_read(UC_X86_REG_EAX)) if kind==1 else 'return',';'.join(self.events),bytes(mu.mem_read(self.slot(50),self.stride)).hex(),regular,str(self.read(TYPES+70*self.type_stride+0xe0)),bytes(mu.mem_read(POINT,8)).hex()]


def generate(smoke):
    """실제기계어출력만저장하고외부worker/path 생성은실제완료관찰로세지않는다."""
    cases=inputs();cases=cases[:1] if smoke else cases;rows=[];editions={};files={ROOT/'tools/decomp_dudenotice_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/gunimpact-dude-functions.json',ROOT/'tools/ghidra/gunimpact-dude-helper-functions.json'}
    for edition in SPEC:
        oracle=DudeNoticeOracle(edition)
        # 동일입력의재방문을고유사례로더하지않는다.
        for case in cases:
            try:result=oracle.run(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=oracle.returns,instructions=oracle.instructions,assertions=oracle.assertions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,flush=True)
    if smoke:return
    for path in ('cpppj/src/o/RawDudeNotice.cpp','cpppj/src/o/RawDudeNotice.h','cpppj/tests/DudeNoticeTests.cpp','cpppj/src/o/SquidProcess.cpp','cpppj/src/o/SquidProcess.h','cpppj/src/o/RawSquidFinder.cpp','cpppj/src/o/SquidPostPop.cpp','cpppj/dude-regular-reset-lineage.json'):files.add(ROOT/path)
    FIXTURE.write_text('# 실제Dude around/notice·Regular상태·거주지/대피요청. DudeProcess14/Path24 ctor와frame 효과는관찰경계다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',total=len(rows),editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},new_worker_constructors_completed=0,new_path_constructors_completed=0,os_calls=0,limitations=['일반finder/현재task/Regular숫자쓰기/거주지선택/속도2배와절삭복원·현재단계조회는원본몸체다.','힙/기존Regular조회/부착·DudeProcess14 ctor/Path24 생성/기존경로객체조회/SetFrame은명시관찰경계다.','현저한분수speed/정밀double now를공급하며C++Fixture는같은실제Host Regular와좁은ResetTask를소비한다.','기존type14/24 Form과거주지배열은명시입력이며실제worker 경제/전체경로이동을완료로세지않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """현재SHA/고유입력/정상반환과미복원worker/path 완료0을감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r for r in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if r and not r[0].startswith('#')]
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Dude notice SHA불일치 '+path)
    if len(rows)!=report['total'] or report['new_worker_constructors_completed'] or report['new_path_constructors_completed']:raise RuntimeError('Dude notice 사례/생성완료 경계 오류')
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition]
        if len({tuple(r[1:12]) for r in selected})!=len(selected) or len(selected)!=item['returns'] or item['assertions']:raise RuntimeError('Dude notice 고유사례/정상반환 오류')
    print(json.dumps(dict(verified=True,total=len(rows),worker_ctor_completed=0,path_ctor_completed=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
