#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 TYPE의 실제35클러스터를 AST로 공급해 원본 TYPE callback/후처리와 초기화 접두를 관찰한다."""
import argparse
import collections
import csv
import json
import re
import struct
from decomp_owner_oracle import OwnerOracle,ROOT,TYPES,STACK,STOP,digest
from shp import load_type_texts,read_clusters
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 원본 TYPE 초기화/전체 callback/후처리와 명시 파일·힙·현재CRT 스레드 경계다.
META={
 'originals':dict(init=0x49ebb0,parser=0x49c2e0,callback=0x49c3b0,post=0x49b0d0,new=0x4e4391,
    count=0x541348,index=0x59ab38,shape=0x59ab24,shapeCursor=0x59af78,astBase=0x59ab48,astEnd=0x59ab4c,
    fileCalls=[0x459c60,0x41ad10,0x4dd580],thread=0x4ea61d,locale=0x543efc,theme=0x592990),
 'originalCD':dict(init=0x4434d0,parser=0x446080,callback=0x4460f0,post=0x444e10,new=0x4f1650,
    count=0x51cbf0,index=0x51cc08,shape=0x51cbf4,shapeCursor=0x51cc20,astBase=0x51cc18,astEnd=0x51cc1c,
    fileCalls=[0x49c1a0,0x4bb6d0,0x4be710],thread=None,locale=0x5473e0,theme=0x530278),
}
# 기존 격리된 에뮬레이터 메모리 안의 AST·코드·스레드·SHP 관찰 저장소다.
AST,FRAMES,THREAD,LOCALE,SHAPE=0x11300000,0x11314000,0x11315000,0x11316000,0x14000000
FIXTURE=ROOT/'cpppj/tests/fixtures/archertype-x86.tsv'
REPORT=ROOT/'cpppj/recovery-archertype-evidence.json'


def float_bits(value):
    """숫자 AST 입력의 float32 비트를 만들며 원본 출력은 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """실제 무속성 입력과 방향속성/클러스터 순서의 명시 대조 입력을 구분한다."""
    # layout0은 실제 순서,1은 역순,2는 help 클러스터를 뺀 합성 대조다.
    return [(orientation,layout) for orientation in ('-',*[str(float_bits(v)) for v in (0,1,1.75,16,20,28)]) for layout in range(3)]


class ArcherTypeOracle(OwnerOracle):
    """원본 초기화와 parser callback을 실제 CRT/이름표/Run과 실행한다."""
    def __init__(self,edition):
        """현호스트 정밀 export의 범위만 추가하고 TYPE/SHP/AST 경계 저장소를 연결한다."""
        super().__init__(edition);self.mu.mem_map(0,0x1000);self.m=META[edition];self.boundaries=collections.Counter();self.prefix=0;self.normal=0;self.cursor=AST
        # TYPE 본체와 실제 group 이름표 순회 helper를 별도 출처로 연결한다.
        for name in ('archer-type','archer-type-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 함수의 실제 불연속 body만 실행을 허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.text=load_type_texts(str(ROOT/edition))['sunarcher'];self.clusters=read_clusters(self.text)
        if len(self.clusters)!=35 or re.search(r'(?im)^\s*orientations\s*=',self.text):raise RuntimeError('보호 sunArcher 실제 자료 변경')
        self.write_ranges=[(0,4),(TYPES,TYPES+0x20000),(FRAMES,FRAMES+0x1000)]
        # 실제 함수가 쓰는 포인터·AST청소·SHP커서 전역만 명시 허용한다.
        for name in ('types',):self.write_ranges.append((self.g[name],self.g[name]+4))
        for name in ('shape','shapeCursor','astEnd'):self.write_ranges.append((self.m[name],self.m[name]+4))
        self.shapeFile=ROOT/edition/'d/_shapes.shp';shapeData=self.shapeFile.read_bytes();self.mu.mem_map(SHAPE,(len(shapeData)+4095)&~4095);self.mu.mem_write(SHAPE,shapeData)
        self.sunBlock=SHAPE+8+self.read(SHAPE+4)*8
        if self.read(self.sunBlock)!=0x30312e31 or self.read(self.sunBlock+4)!=35:raise RuntimeError('보호 sunArcher SHP 블록 오류')
        if edition=='originals':
            self.mu.mem_write(THREAD+0x64,struct.pack('<I',LOCALE));self.mu.mem_write(self.m['locale'],struct.pack('<I',LOCALE));self.mu.mem_write(LOCALE,bytes(128))
        else:self.mu.mem_write(self.m['locale']+8,bytes(4))
        self.mu.mem_write(self.m['theme']+ord('s')*4,bytes(4))

    def read(self,address,width=4):
        """원본 가상 메모리의 little endian 필드를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """파일·힙·CRT스레드는 관찰 입력 경계이며 parser 진입 전 정지를 정상반환으로 세지 않는다."""
        m=self.m;esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp)
        if address==m['parser']:
            self.prefix+=1;mu.emu_stop();return
        if address==m['new']:
            request=self.read(esp+4);value=TYPES if request==self.read(m['count'])*self.type_stride else FRAMES
            if value==FRAMES and request>0x1000:raise RuntimeError('metadata 코드 할당 관찰 범위 오류')
            self.boundaries[f'{address:08x}:{request}']+=1
        elif address in m['fileCalls']:
            value=(AST,1,SHAPE)[m['fileCalls'].index(address)];self.boundaries[f'{address:08x}']+=1
        elif address==m['thread']:
            value=THREAD;self.boundaries[f'{address:08x}']+=1
        else:super().on_instruction(mu,address,size,data);return
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)

    def invoke(self,address,args=(),this=0,prefix=False):
        """실제 top body 호출과 반환/스택/x87 상태를 엄격히 검사한다."""
        mu=self.mu
        # 매 원본 호출의 비인자 레지스터와 원본53비트 x87환경을 다시 공급한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,this);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.mem_write(0,bytes(4));mu.emu_start(address,STOP,count=300000)
        if prefix:
            if mu.reg_read(UC_X86_REG_EIP)!=self.m['parser']:raise RuntimeError('TYPE 초기화 접두 정지 오류')
        else:
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('TYPE callback cdecl 반환 오류')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('TYPE x87 반환 스택 오류')
            self.normal+=1

    def allocate(self,data):
        """AST 입력을 정렬된 불변 영역에 쓴다. 실행 중 AST 쓰기는 허용하지 않는다."""
        address=self.cursor;self.mu.mem_write(address,data);self.cursor=(address+len(data)+3)&~3
        if self.cursor>=FRAMES:raise RuntimeError('TYPE AST 입력 공간 초과')
        return address

    def string(self,value):
        """원본 ASCII identifier/string의 실제 바이트 입력이다."""
        return self.allocate(value.encode('ascii')+b'\0')

    def node(self,btype,utype=0,first=0,second=0):
        """원본16바이트 AST의 btype/utype/p1/p2 구조를 공급한다."""
        return self.allocate(struct.pack('<IIII',btype,utype,first,second))

    def linked(self,nodes,utype):
        """원본 리스트 노드의 순서와 끝0을 실제 callback에 공급한다."""
        tail=0
        # 마지막 단일 항목은 그대로 두고 앞 항목만 UT 리스트로 잇는다.
        for node in reversed(nodes):tail=node if not tail else self.node(0,utype,node,tail)
        return tail

    def ast(self,orientation,layout):
        """보호 TYPE의 실제 클러스터/특수flags와 관련 group/theme 속성만 AST로 투영한다."""
        self.cursor=AST;frames=[];selected=list(self.clusters)
        if layout==1:selected.reverse()
        elif layout==2:selected=[c for c in selected if 'help' not in c[1].split()]
        # 모든 클러스터 번호는 원본의 float 숫자 AST를 거쳐 실제 CRT가 절삭한다.
        for name,flags,_images in selected:
            match=re.fullmatch(r'([A-Za-z]+)(\d+)',name)
            ident=self.node(1,0,self.string(match[1]));number=self.node(2,0,float_bits(int(match[2])));orient=self.node(4,0,ident,number)
            flagNodes=[self.node(1,0,self.string(flag)) for flag in flags.split()]
            frames.append(self.node(3,0,orient,self.linked(flagNodes,3)))
        properties=[]
        # callback 밖의 무관한 경제/효과/소리 속성은 공급하거나 완료로 주장하지 않는다.
        for name,value in (('group','archer'),('theme','sun')):
            key=self.node(1,0,self.string(name));val=self.node(10,0,self.string(value));properties.append(self.node(9,0,key,val))
        if orientation!='-':properties.append(self.node(9,0,self.node(1,0,self.string('orientations')),self.node(2,0,int(orientation))))
        text='typename sunArcher constructor\n{group="archer";theme="sun";'
        if orientation!='-':text+='orientations='+repr(struct.unpack('<f',struct.pack('<I',int(orientation)))[0])+';'
        text+='}\n'
        # C++는 같은 투영 입력문만 읽으며 원본 출력에서 특수 인덱스를 만들지 않는다.
        for name,flags,images in selected:text+=name+':'+flags+':'+':'.join('"'+file+'" #'+str(number) for file,number in images)+';\n'
        self.inputText=text.encode('ascii').hex()
        return self.linked(properties,7),self.linked(frames,2)

    def projection(self,prefix=False):
        """실제 raw TYPE와4바이트 코드/Run 배열의 의미 필드만 수집한다."""
        address=TYPES+71*self.type_stride
        values=[self.read(address+offset) for offset in (0x110,0x114,0x118,0x11c,0x120,0x128,0x12c)]
        signed=[value-(1<<32) if value>=1<<31 else value for value in values]
        count=values[1];pointer=self.read(address+0x124)
        codes='-' if prefix else bytes(self.mu.mem_read(pointer,count*4)).hex()
        runs='-' if prefix else ','.join(str(self.read(address+offset+index*4)) for offset in (0x130,0x170) for index in range(16))
        shapePointer=self.read(address+0xdc);signed.append(self.read(shapePointer+4) if shapePointer else 0)
        return [','.join(map(str,signed)),codes,runs]

    def initialize(self):
        """원본188/171 TYPE 전체 초기화를 실행하되 실제 parser 진입에서 명시 정지한다."""
        self.mu.mem_write(TYPES,bytes([0xa5])*0x20000);self.invoke(self.m['init'],prefix=True)
        self.initial=bytes(self.mu.mem_read(TYPES,0x20000));return self.projection(True)

    def run(self,orientation,layout):
        """실제 초기화 결과를 입력으로 복원하고 전체callback→전체후처리를 정상반환까지 실행한다."""
        mu=self.mu;m=self.m;mu.mem_write(TYPES,self.initial);mu.mem_write(m['index'],struct.pack('<I',2));mu.mem_write(m['shape'],struct.pack('<I',SHAPE));mu.mem_write(m['shapeCursor'],struct.pack('<I',self.sunBlock));mu.mem_write(m['astBase'],bytes(4));mu.mem_write(m['astEnd'],bytes(4));mu.mem_write(FRAMES,bytes(0x1000));properties,frames=self.ast(orientation,layout)
        self.invoke(m['callback'],(0,0,properties,frames));self.invoke(m['post'],this=TYPES+71*self.type_stride)
        return self.projection()


def generate(smoke):
    """정상 metadata 관찰과 초기화 parser 접두를 별도 기록하고 당시 자료SHA를 연결한다."""
    rows=[];editions={};files={path for path in (ROOT/'tools/decomp_archertype_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/shp.py',ROOT/'tools/taff.py',ROOT/'tools/ghidra/archer-type-functions.json',ROOT/'tools/ghidra/archer-type-helper-functions.json')}
    for edition in META:
        oracle=ArcherTypeOracle(edition);rows.append([edition,'prefix','-',0,'-',*oracle.initialize()])
        # 실제 input은 첫 행이며 나머지는 명시적인 property/순서 대조 입력이다.
        for orientation,layout in cases()[:1] if smoke else cases():
            result=oracle.run(orientation,layout);rows.append([edition,'callback',orientation,layout,oracle.inputText,*result])
        editions[edition]=dict(observations=sum(row[0]==edition and row[1]=='callback' for row in rows),normal_top_calls=oracle.normal,prefix=oracle.prefix,instructions=oracle.instructions,assertions=oracle.assertions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),type_text_sha256=__import__('hashlib').sha256(oracle.text.encode('latin-1')).hexdigest())
        files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);files.add(ROOT/edition/'netstorm.tarc');files.add(oracle.shapeFile);print(edition,editions[edition],flush=True)
    if smoke:return
    for relative in ('cpppj/src/o/RawArcherTypeData.cpp','cpppj/src/o/RawArcherTypeData.h','cpppj/tests/ArcherTypeDataTests.cpp','cpppj/src/o/RiftType.cpp','cpppj/src/o/RiftType.h','cpppj/src/o/TypeParser.cpp','cpppj/src/o/RiftTypeTable.cpp','cpppj/src/client/GameAssets.cpp',
        'cpppj/src/o/RawArcherAnimation.cpp','cpppj/src/o/RawArcherAnimation.h','cpppj/src/o/RawGunProcess.cpp','cpppj/src/o/RawGunTarget.cpp','cpppj/src/o/RawBuildingSpace.cpp','cpppj/src/o/SquidFactory.cpp','cpppj/src/o/DerivedConstructors.inc','cpppj/src/o/TypeConstructors.inc','cpppj/src/o/SquidPop.cpp','cpppj/src/o/SquidUnpop.cpp','cpppj/src/o/SquidProcess.cpp','cpppj/src/o/Kernel.cpp','cpppj/src/client/SquidRenderer.cpp','cpppj/tests/fixtures/gunarcher-x86.tsv','cpppj/recovery-gunarcher-evidence.json'):
        files.add(ROOT/relative)
    FIXTURE.write_text('# 실제 TYPE 초기화 parser 접두와 보호35클러스터 AST의 전체callback/후처리 정상반환.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',normal_observations=len(rows)-2,prefix=2,total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},os_calls=0,limitations=['초기화는 전체TYPE 대입 뒤 실제parser 진입에서 정지하는 접두이며 전체게임로딩 완료가 아니다.','callback은 실제 보호35클러스터/group/theme의 관련부분 AST를 공급한다. 전체파일 lexer/문법parser·경제/소리/효과속성 완료가 아니다.','힙/파일/현재CRT 스레드와 이전dude처리후 두 번째 실제SHP블록 커서는 명시 입력 경계다. 보호전체SHP바이트와 원본 TYPE 후처리·16문자Run·float절삭/ASCII비교는 실제자료/몸체다.','실제 무orientations+실제순서 입력2개와 합성property/순서 대조40개를 구분하며 기존1690/1888근거에 더하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """고유입력/정상반환/접두 분리와 현재전체소스/원본/자료SHA를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r for r in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if r and not r[0].startswith('#')]
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('ArcherType SHA불일치 '+path)
    if len(rows)!=44 or report['normal_observations']!=42 or report['prefix']!=2 or len({tuple(row[:4]) for row in rows})!=len(rows):raise RuntimeError('ArcherType 고유관찰/접두 오류')
    for edition,item in report['editions'].items():
        if item['observations']!=21 or item['normal_top_calls']!=42 or item['prefix']!=1 or item['assertions']:raise RuntimeError('ArcherType 원본완료/접두 오류')
    print(json.dumps(dict(verified=True,normal=42,prefix=2),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
