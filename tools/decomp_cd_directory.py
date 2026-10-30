#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 CD-ROM 필터/깊이2 탐색을 실제 CPU로 실행하고 OS 열거만 격리한다."""
import argparse
import csv
import hashlib
import json
import socket
from decomp_local_mission_transport import Oracle as Base, ROOT, IMAGE, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
import pefile

FIXTURE=ROOT/'cpppj/tests/fixtures/cd-directory-x86.tsv'  # 최초 전체 목적지/열거 trace 관측이다.
REPORT=ROOT/'cpppj/cd-directory-native-20261030.json'  # 원본 범위/장치 경계/도구 SHA다.
MARKER='cd.netstorm.titanic.activision'  # PE00532544의 그대로인 표시 이름이다.


class Oracle(Base):
    """드라이브/Find API와 ASCII CRT만 장치로 두고 세 원문 판단 몸체를 실행한다."""
    def __init__(self):
        """현재 PC 정밀 범위를 고정하고 사용한 Win32 IAT만 가상 주소로 바꾼다."""
        super().__init__();self.allowed=[]
        facts={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for address in (0x440d40,0x4dd340,0x4dde40):  # 원본 범위 밖의 우발적 기계어 실행은 거부한다.
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(x,16) for x in part.split('-'));self.allowed.append((low,high+1))
        self.imports={};pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
        for library in pe.DIRECTORY_ENTRY_IMPORT:  # 선택한 실제 OS 장치만 새 주소로 연결한다.
            for entry in library.imports:
                name=entry.name.decode() if entry.name else ''
                if name in ('GetLogicalDriveStringsA','GetDriveTypeA','FindFirstFileA','FindNextFileA','FindClose'):
                    target=STOP+0x100+len(self.imports)*0x10;self.imports[target]=name;self.put(entry.address,target)

    def data(self,address,handle):
        """현재 가상 디렉터리 항목을 실제 WIN32_FIND_DATAA 물리에 쓴다."""
        directory,index=self.handles[handle];name,folder=self.tree[directory][index];self.mu.mem_write(address,b'\0'*320);self.put(address,16 if folder else 0);self.mu.mem_write(address+44,name.encode()+b'\0');self.entries+=1;self.put(0x5e18b4,int(self.cancel!=0 and self.entries==self.cancel))

    def instruction(self,mu,address,size,data):
        """선택된 장치의 원래 호출 규약/출력만 제공하고 원문 분기는 대신 계산하지 않는다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,5)
        if address in self.imports:
            name=self.imports[address]
            if name=='GetLogicalDriveStringsA':
                self.trace.append('drives');raw=('\0'.join(self.roots)+'\0\0').encode();self.mu.mem_write(args[1],raw);self.ret(0 if self.enumeration_error else len(raw)-1,8);return
            if name=='GetDriveTypeA':
                root=self.string(args[0]).decode();self.trace.append('type:'+root);self.ret(self.roots[root],4);return
            if name=='FindFirstFileA':
                pattern=self.string(args[0]).decode();self.trace.append('first:'+pattern);directory=pattern[:-2]
                if not self.tree.get(directory):self.ret(0xffffffff,8);return
                handle=self.serial;self.serial+=1;self.handles[handle]=[directory,0];self.data(args[1],handle);self.ret(handle,8);return
            if name=='FindNextFileA':
                handle=args[0];directory,index=self.handles[handle];index+=1;self.trace.append('next:'+directory);self.handles[handle][1]=index
                if index>=len(self.tree[directory]):self.ret(0,8);return
                self.data(args[1],handle);self.ret(1,8);return
            if name=='FindClose':
                self.trace.append('close:'+self.handles.pop(args[0])[0]);self.ret(1,4);return
        if address==0x4e65ed:
            left,right=(self.string(p).lower() for p in args[:2]);self.ret((left>right)-(left<right));return
        if address==0x4e51f0:
            source=self.string(args[1])[:args[2]];self.mu.mem_write(args[0],source+b'\0'*(args[2]-len(source)));self.ret(args[0]);return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError(f'CD 허용 범위 밖 {address:08x}')

    def sample(self,name,roots,tree,cancel=0,capacity=255,error=False):
        """각 사례마다 목적지/핸들/현재 중단/드라이브를 독립 초기화한다."""
        self.roots=roots;self.tree=tree;self.cancel=cancel;self.enumeration_error=error;self.entries=0;self.handles={};self.serial=1;self.trace=[];self.put(0x5e18b4,0);self.mu.mem_write(IMAGE,b'seed\0'+b'\xa5'*251)
        self.put(STACK,STOP);self.put(STACK+4,IMAGE);self.put(STACK+8,capacity);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.emu_start(0x440d40,STOP,count=100000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('CD 탐색 ret/스택 불일치')
        root_text=','.join(r+'='+str(t) for r,t in roots.items());tree_text=';'.join('|'.join((d,n,str(int(f)))) for d,entries in tree.items() for n,f in entries)
        return '\t'.join((name,root_text or '-',tree_text or '-',str(cancel),str(capacity),str(int(bool(self.mu.reg_read(UC_X86_REG_EAX)))),bytes(self.mu.mem_read(IMAGE,256)).hex(),';'.join(self.trace),str(len(self.handles)),str(int(error))))


def main():
    """최초 관측은 덮어쓰지 않으며 재실행은 전체 fixture/근거 바이트를 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()):raise RuntimeError('CD 최초 관측 덮어쓰기 금지')
    roots={'C:\\':3,'D:\\':5,'E:\\':5};cases=[('none',{},{}),('fixed',{'C:\\':3},{'C:':[(MARKER,False)]}),('missing',roots,{}),('first',roots,{'D:':[(MARKER,False)],'E:':[(MARKER,False)]}),('second',roots,{'D:':[('other',False)],'E:':[(MARKER.upper(),False)]}),('nested',roots,{'D:':[('folder',True),(MARKER,False)],'D:\\folder':[(MARKER,False)]}),('deep2',roots,{'D:':[('one',True)],'D:\\one':[('two',True)],'D:\\one\\two':[(MARKER,False)]}),('deep3',roots,{'D:':[('one',True)],'D:\\one':[('two',True)],'D:\\one\\two':[('three',True)],'D:\\one\\two\\three':[(MARKER,False)]}),('dot',roots,{'D:':[('.hidden',True),('.',True),('..',True),(MARKER,False)],'D:\\.hidden':[(MARKER,False)]}),('file-no-recurse',roots,{'D:':[('folder',False)],'D:\\folder':[(MARKER,False)]}),('cancel-root',roots,{'D:':[(MARKER,False)]},1),('cancel-child',roots,{'D:':[('one',True)],'D:\\one':[(MARKER,False)]},2),('truncate',roots,{'D:':[('longfolder',True)],'D:\\longfolder':[(MARKER,False)]},0,6),('one-byte',roots,{'D:':[(MARKER,False)]},0,1),('enumeration-error',roots,{'D:':[(MARKER,False)]},0,255,True)]
    oracle=Oracle();content='# name roots tree cancel capacity found destination256Hex trace openHandles enumerationError\n'+'\n'.join(oracle.sample(*case) for case in cases)+'\n';paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_transport.py','tools/decomp_cd_directory.py');hashes={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    if args.verify:
        if FIXTURE.read_bytes()!=content.encode() or hashes!=json.loads(REPORT.read_text(encoding='utf-8'))['sha256']:raise RuntimeError('CD 원문 재관측 차이')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n');REPORT.write_text(json.dumps({'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','observations':len(cases),'ranges':oracle.allowed,'sha256':hashes,'fixtureSha256':hashlib.sha256(content.encode()).hexdigest(),'scope':'00440d40/004dd340/004dde40 실제 CPU. Win32 열거/핸들과 ASCII stricmp/strncpy는 명시 장치. 게임/창/실제 CD 드라이브 실행 없음.'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'observations':len(cases),'verified':args.verify,'differences':0}))


if __name__=='__main__':main()
