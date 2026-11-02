#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문441965~441a44의두TAFF열거/같은FileSpec과459890을CPU로고정한다."""
import argparse
import json
from pathlib import Path
import socket
import pefile
from decomp_config_path_load import LoadOracle, FIND, CLOSE
from decomp_file_name_leaf import add_ranges
from decomp_file_name_formatter import Oracle as NameOracle, REGISTERS, RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

ENUM = ROOT/'cpppj/tests/fixtures/startup-archives-x86.tsv'  # 부모의등록/핸들순서와전체고리다.
FULL = ROOT/'cpppj/tests/fixtures/file-full-x86.tsv'  # 접두보존/무조건+2의전체BYTE다.
FRAME, NEXT = STACK-0x1000, STOP+0x300  # 준비된원문지역프레임과명시FindNextOS경계다.


class ArchiveOracle(LoadOracle):
    """실제부모/Full/FindNext/재대입/소멸자를실행하고OS/등록장치만응답한다."""
    def __init__(self):
        """기존실제FindFirst/Close경계에Next와필요한원문몸체를더한다."""
        super().__init__()
        add_ranges(self,(0x441790,0x459890,0x4597c0,0x41a540))
        pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
        for library in pe.DIRECTORY_ENTRY_IMPORT:  # 실제다음검색import한개만바꾼다.
            for item in library.imports:  # 다른OS함수를성공으로넘기지않는다.
                if item.name==b'FindNextFileA': self.put(item.address,NEXT)
        self.names=[]  # 현재명시OS가반환할순서이며정렬하지않는다.
        self.cursors={}  # 각핸들의현재파일번호다.
        self.opened=0  # 기본/CD검색핸들을서로다르게구별한다.

    def instruction(self,mu,address,size,data):
        """실제호출의이름/순서/BOOL을기록하고부모의각분기는원문그대로실행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==FIND:
            pattern,output=self.words(esp+4,2);self.events.append('find:'+self.string(pattern).hex());self.opened+=1
            handle=0x1200+self.opened if self.names else 0xffffffff
            if self.names:mu.mem_write(output+44,self.names[0]+b'\0');self.cursors[handle]=0
            self.ret(handle,8);return
        if address==NEXT:
            handle,output=self.words(esp+4,2);self.events.append('next:'+str(handle));self.cursors[handle]+=1;at=self.cursors[handle]
            if at<len(self.names):mu.mem_write(output+44,self.names[at]+b'\0')
            self.ret(int(at<len(self.names)),8);return
        if address==CLOSE:
            handle=self.words(esp+4,1)[0];self.events.append('close:'+str(handle));self.ret(1,4);return
        if address in (0x41bc20,0x41bc30):
            path=self.string(self.words(esp+4,1)[0]);self.events.append(('primary:' if address==0x41bc20 else 'secondary:')+path.hex());self.ret();return
        if address==0x41a4f0:
            self.events.append('secondary-root:'+self.string(self.words(esp+4,1)[0]).hex());self.ret();return
        if address==0x441a44:
            mu.emu_stop();return
        NameOracle.instruction(self,mu,address,size,data)

    def sample(self,root,cd,count,index,precision):
        """CWD확정뒤의실제두목록과그뒤설정경계까지살아있는핸들을관측한다."""
        self.events.clear();self.cursors.clear();self.opened=0;self.names=[b'z.tarc',b'A.tarc',b'bad.tarc'][:count];self.thrown=False
        self.mu.mem_write(RING,bytes(2560));self.put(INDEX,index);self.put(0x532728,92,1);self.put(0,0)
        self.mu.mem_write(0x544d80,root+b'\0');self.mu.mem_write(TEXT,b'' if cd is None else cd+b'\0')
        self.mu.mem_write(STACK-0x2000,bytes([0x65])*0x2000);self.mu.mem_write(FRAME+0x210,root+b'\0')
        self.mu.reg_write(UC_X86_REG_ESI,0 if cd is None else TEXT);self.mu.reg_write(UC_X86_REG_ESP,FRAME);self.mu.reg_write(UC_X86_REG_FPCW,precision)
        self.mu.emu_start(0x441965,0x441a44,count=100000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP)==0x441a44 and self.mu.reg_read(UC_X86_REG_ESP)==FRAME-4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        current=self.string(FRAME+0x108).hex();valid,handle=self.words(FRAME+0x20c,1)[0],self.words(FRAME+0x208,1)[0]
        ring=bytes(self.mu.mem_read(RING,2560)).hex();final=self.words(INDEX,1)[0]
        self.events.append('load-boundary')  # 뒤Load는실행하지않는명시수명경계이며원문소멸은그뒤다.
        self.mu.reg_write(UC_X86_REG_ECX,FRAME+0x108);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.put(STACK,STOP)
        self.mu.emu_start(0x4595f0,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and not valid
        return [','.join(self.events),current,handle,final,ring]

    def full(self,value,flag,precision):
        """FileSpec물리입력의+2와첫NUL을구별하는원문전체를실행한다."""
        self.thrown=False;self.mu.mem_write(RING,bytes(2560));self.put(INDEX,0);self.mu.mem_write(TEXT,value+b'\0')
        self.mu.mem_write(STACK-0x2000,bytes([0x65])*0x2000)
        for number,register in enumerate(REGISTERS):self.mu.reg_write(register,0x12345678+number*0x1020304)  # 실제보존레지스터에비영표식을넣는다.
        before=[self.mu.reg_read(r) for r in REGISTERS];self.mu.reg_write(UC_X86_REG_ECX,TEXT);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.put(STACK,STOP);self.put(STACK+4,flag);self.mu.reg_write(UC_X86_REG_FPCW,precision)
        self.mu.emu_start(0x459890,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+8 and self.words(INDEX,1)==(1,) and before==[self.mu.reg_read(r) for r in REGISTERS]
        assert self.mu.reg_read(UC_X86_REG_EAX)==RING+256 and self.mu.reg_read(UC_X86_REG_FPCW)==precision
        assert bytes(self.mu.mem_read(TEXT,len(value)+1))==value+b'\0'
        return bytes(self.mu.mem_read(RING+256,256)).hex()


def main():
    """최초두fixture를재작성하지않고독립재관측도지원한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--verify',action='store_true');args=p.parse_args();assert not args.output.exists()
    if not args.verify:assert not ENUM.exists() and not FULL.exists()
    o=ArchiveOracle();en=['# root_hex cd_hex_or_NULL count initial_index events current_hex handle final_index ring_hex'];fu=['# input_hex flag slot_hex']
    for root in (b'C:\\Game',b'C:/Game',b'\\\\server\\share\\Game'):  # 기본루트표기별로같은실제부모를관측한다.
        for cd in (None,b'D:\\CD',b'\\\\server\\CD'):  # CD부재는NULL이며동일한FileSpec재대입을구별한다.
            for count in (0,1,3):  # 빈검색/단일/여러파일과원문순서를고정한다.
                for index in (0,9):  # 첫순환전후의물리칸도대조한다.
                    row=o.sample(root,cd,count,index,0x027f);assert row==o.sample(root,cd,count,index,0x037f)
                    en.append('\t'.join([root.hex(),'NULL' if cd is None else cd.hex(),str(count),str(index)]+[str(x) or '-' for x in row]))
    for value in (b'.\\a',b'C:\\d\\file',b'\\\\s\\d\\f',b'ab',b'a\0Z\0',b'\0xy\0',b'.\\'+b'x'*253,b'\x81:\\f'):  # flag0은접두검사없이실제+2주소부터복사한다.
        for flag in (0,1):  # 정상모드두개를따로고정한다.
            row=o.full(value,flag,0x027f);assert row==o.full(value,flag,0x037f);fu.append(f'{value.hex()}\t{flag}\t{row}')
    for path,lines in ((ENUM,en),(FULL,fu)):  # 기존최초출력은검증만한다.
        payload='\n'.join(lines)+'\n'
        if args.verify:assert path.read_text('utf-8')==payload
        else:path.write_text(payload,encoding='utf-8',newline='\n')
    report=dict(unit=736,date='2026-11-02',host=socket.gethostname(),lastFullDecompilation={'host':'HJOW-Athlon','date':'2026-10-23'},archiveRows=len(en)-1,fullRows=len(fu)-1,observations=(len(en)+len(fu)-2)*2,
        boundary='실제441965~441a44/Full/FileSpec/재대입/Next/CRT와별도실제dtor. OS FindFirst/Next/Close와등록/보조root설정은명시경계. CWD확정전/뒤Load/전체부모반환ABI는범위밖',gameExecutions=0,multiplayerTests=0,
        sha256={str(x.relative_to(ROOT)):digest(x) for x in (ROOT/'originals/Netstorm.exe',Path(__file__),ENUM,FULL)})
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:report[k] for k in ('archiveRows','fullRows','observations')}))


if __name__=='__main__':main()
