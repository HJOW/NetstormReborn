"""보호 PE의 트리즌 자격/효과를 격리 메모리에서 관찰하고 독립 기대값을 만든다."""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  #원본은 읽기 전용이고 새 fixture만 저장한다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL,STACK,STOP,TYPE,VT=0x2000000,0x3000000,0xf00000,0x2100000,0x11010000  #실제 OS/게임과 분리한 입력 주소다.

def sample(pe,mode,genus,kind,same,side,qa,found,mutate,candidates):
    """자격/효과 부모는 원문 명령을 실행하고 실제 큰 장치 경계만 기록한다."""
    image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
    emu=Uc(UC_ARCH_X86,UC_MODE_32);emu.mem_map(base,(len(image)+4095)&~4095);emu.mem_write(base,image)
    for address,size in [(POOL,0x20000),(TYPE,0x20000),(STACK,0x10000),(STOP,0x1000),(VT,0x1000)]:  #주소가 겹치지 않는 명시 입력 영역만 확보한다.
        emu.mem_map(address,size)
    trace=[];first=True
    def word(address,value=None):
        """현재 little endian DWORD의 전체 비트를 읽거나 쓴다."""
        if value is None:return int.from_bytes(emu.mem_read(address,4),'little')
        emu.mem_write(address,struct.pack('<I',value&0xffffffff))
    def byte(address):
        """현재 BYTE의 부호 없는 값을 읽는다."""
        return emu.mem_read(address,1)[0]
    def bits(value):
        """장치 좌표를 실제 float 저장 비트로 기록한다."""
        return struct.unpack('<I',struct.pack('<f',value))[0]
    def text(address):
        """원문 C 문자열의 첫 NUL까지만 읽는다."""
        result=bytearray()
        while byte(address):  #명시 입력/보호 PE의 실제 문자열만 순서대로 읽는다.
            result.append(byte(address));address+=1
        return bytes(result)
    def ret(value=None,pop=0):
        """현재 장치의 실제 cdecl/thiscall 규약에 따라 복귀한다."""
        esp=emu.reg_read(UC_X86_REG_ESP);dest=word(esp)
        if value is not None:emu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        emu.reg_write(UC_X86_REG_ESP,esp+4+pop);emu.reg_write(UC_X86_REG_EIP,dest)
    def genus_at(sid):
        """현재 TYPE getter 경계의 명시 행 입력을 반환한다."""
        if sid in (70,71):return 0x10040000 if byte(POOL+sid*50+10)==120 else 0
        return genus if byte(POOL+sid*50+10)==kind else 0
    def hook(uc,address,size,unused):
        """소유권/표시/펜스/검색/받침·진단/로그·플레어의 실제 인자만 기록한다."""
        nonlocal first
        esp=uc.reg_read(UC_X86_REG_ESP);this=uc.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50
        if address==0x4ac200:ret(genus_at(sid))
        elif address==0x4ad680:ret(side if side<128 else side-256)
        elif address==0x4ac550:ret(TYPE+byte(this+10)*0x244)
        elif address==0x4c2630:
            assert word(esp+4)==4;fmt=text(word(esp+8))
            if fmt.startswith(b'Set '):
                values=[word(esp+offset) for offset in (16,20,24,28)];values=[value if value<0x80000000 else value-0x100000000 for value in values]
                value=fmt%(text(word(esp+12)),*values)
            else:value=fmt
            trace.append('L:'+value.hex());ret()
        elif address in (0x457e20,0x457890):trace.append(f'F:{word(esp+4)}:{int(address==0x457890)}');ret()
        elif address==STOP+0x100:
            owner=word(esp+4);trace.append(f'O:{sid}:{word(this)}:{owner}');emu.mem_write(this+34,bytes([owner&255]))
            if mutate and first:
                emu.mem_write(POOL+50*50+34,b'\x07');emu.mem_write(POOL+60*50+10,b'\x88');word(POOL+60*50,VT+0x200)
            first=False;ret(pop=4)
        elif address==STOP+0x200:trace.append(f'R:{sid}:{word(this)}');ret()
        elif address==0x4b1fa0:
            trace.append('Q:'+':'.join(str(word(esp+4+i*4)) for i in range(3)));ret(61 if found else 0)
        elif address==0x44bd20:
            trace.append('P:'+':'.join(str(word(esp+4+i*4)) for i in range(3)));ret()
        elif address==0x4e0620:trace.append(f'D:{word(esp+12)}');ret()
        elif address==0x4202f0:
            x,y=(struct.unpack('<f',struct.pack('<I',word(esp+4+i*4)))[0] for i in range(2));assert word(esp+12)==0
            word(this+52,70 if candidates and int(x)==100 and int(y)==(199 if kind==82 else 200) else 0);ret(this,12)
        elif address==0x4b1810:word(this+52,71 if word(this+52)==70 else 0);ret(this)
        elif address==0x45a480:trace.append('V:'+':'.join(str(word(esp+4+i*4)) for i in range(5)));ret()
    word(0x5c8464,POOL)
    for number in range(188):  #현재 TYPE+4 이름은 원문 printf가 실제로 읽는다.
        emu.mem_write(TYPE+number*0x244+4,f'T{number}\0'.encode('ascii'))
    for table in (VT,VT+0x200):  #소유자 효과 뒤 바뀐 가상 표도 별도 현재 표로 만든다.
        word(table+0x74,STOP+0x100);word(table+0x88,STOP+0x200)
    for sid,typ,x,y in [(50,182,100.25,200.75),(60,kind,100.25,200.75),(61,157,101.5,201.5),(70,155 if kind==82 else 120,100,199 if kind==82 else 200),(71,121,100,199 if kind==82 else 200)]:
        #실제 게임 객체를 생성하지 않고 각 경계가 읽는50BYTE 입력만 지정한다.
        address=POOL+sid*50;word(address,VT);emu.mem_write(address+10,bytes([typ]));word(address+14,bits(x));word(address+18,bits(y));emu.mem_write(address+34,b'\x02' if sid==50 or same else b'\x01')
    emu.mem_write(POOL+50*50+12,struct.pack('<H',60 if qa else 0));emu.hook_add(UC_HOOK_CODE,hook)
    esp=STACK+0x8000;word(esp,STOP);word(esp+4,60);emu.reg_write(UC_X86_REG_ESP,esp);emu.reg_write(UC_X86_REG_ECX,POOL+50*50)
    emu.emu_start(0x41ff20 if mode=='E' else 0x41ff90,STOP,count=50000);assert emu.reg_read(UC_X86_REG_EIP)==STOP
    return [str(emu.reg_read(UC_X86_REG_EAX)) if mode=='E' else '-',str(byte(POOL+60*50+34)),';'.join(trace) or '-']

def main():
    """기대값을 제품 알고리즘으로 계산하지 않고 원본 부모의 관찰로 저장한다."""
    out=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'extracted/cpp-resume-1232';out.mkdir(parents=True,exist_ok=True);pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));inputs=[]
    for same in (0,1):  #같은 owner일 때 잘못된 상태/프레임을 추가 조회하지 않는 첫 관문이다.
        for kind in (94,157,84):  #일반/지면/고갈 역할의 세 경로를 교차한다.
            for genus in (0,4,0x10000000,0x40000,0x200000,0x10170004,0x10370004,0xffffffff):  #자격/보행 마스크의 각 조합을 원문에 입력한다.
                for side in (65,72,255):  #비H/H/음수 방향의 signed 해석을 교차한다.
                    inputs.append(('E',genus,kind,same,side,0,0,0,0))
    for kind,genus,side,found,mutate,candidates in [(94,0,72,0,0,0),(136,0,72,0,0,0),(94,0,72,0,1,0),(94,0x40000,72,0,0,0),(94,0x40000,72,1,0,0),(94,0x40000,65,1,0,0),(157,0,72,0,0,1),(157,0,65,0,0,1),(84,0,72,0,0,1),(94,0x10000000,72,0,0,1),(82,4,72,0,0,1),(94,0xffffffff,72,1,0,1)]:
        #받침 진단 정상 복귀/현재 가상 표/반복 후보의 실제 소유권 갱신을 관찰한다.
        inputs.append(('A',genus,kind,0,side,0,found,mutate,candidates))
    inputs.append(('A',0xffffffff,157,0,255,1,1,1,1))  #QA이면 다른 장치가 필요 없는 초기 반환이다.
    rows=[[*map(str,args),*sample(pe,*args)] for args in inputs]  #각 입력의 기대값은 원본 부모 관찰에서만 얻는다.
    fixture=ROOT/'cpppj/tests/fixtures/bomb-treason-x86.tsv';fixture.write_text('# mode genus kind same side qa found mutate candidates result owner effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(passed=True,cases=len(rows),fixture=str(fixture.relative_to(ROOT)),fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),originalPeSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),originalExecutions=0,multiplayerExecutions=0,dedicatedAirExecutions=0,emulatedFunctions=['0041ff20','0041ff90'],deviceBoundaries=['current TYPE/genus/orientation','virtual74/88','fence clear/rebuild','find type/point finder','support owner','returning assertion/trace','direction flare'])
    (out/'oracle.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report))

if __name__=='__main__':
    main()
