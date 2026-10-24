"""보호 GIF를 원본 LZW 본체로 해독하여 독립 색 번호 관측을 보존한다."""
from pathlib import Path
import hashlib,json,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_BLOCK
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_GDTR,UC_X86_REG_CS,UC_X86_REG_SS,UC_X86_REG_DS,UC_X86_REG_ES

ROOT=Path(__file__).resolve().parents[1];PE=ROOT/'originals/Netstorm.exe'  # 보호 파일을 읽기만 한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/background-gif-assets-x86.tsv';REPORT=ROOT/'cpppj/background-gif-assets-native-20261024.json'  # 최초 관측은 새 파일로만 저장한다.
SOURCE=0x2000000;PANE=0x2200000;STOP=0x2201000;STACK=0x3000000  # 독립 파일/출력 장치/스택의 주소다.

def decode(path,data=None):
    """실제407171과406ef7을 실행하고406939 행 복사 장치만 관측한다."""
    if data is None:data=path.read_bytes()
    pe=pefile.PE(str(PE));cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
    cpu.mem_map(0,0x1000);cpu.mem_map(SOURCE,0x200000);cpu.mem_write(SOURCE,data);cpu.mem_map(PANE,0x10000);cpu.mem_map(STACK-0x10000,0x20000)
    cpu.mem_write(PANE+0x2000,struct.pack('<QQQ',0,0x00cf9a000000ffff,0x00cf92000000ffff));cpu.reg_write(UC_X86_REG_GDTR,(0,PANE+0x2000,23,0));cpu.reg_write(UC_X86_REG_CS,8)
    for register in (UC_X86_REG_SS,UC_X86_REG_DS,UC_X86_REG_ES):cpu.reg_write(register,16)  # VFX push/pop ES 뒤에도 같은32비트 평면 스택을 유지한다.
    dimensions=[];pixels=bytearray();seen=[];blocks=0  # 원본의 출력 행과 실제 기본 블록 진입을 관측한다.
    def read(address,count=1):
        """장치 호출의 현재 DWORD 인자를 읽는다."""
        return struct.unpack('<'+'I'*count,cpu.mem_read(address,4*count))
    def hook(current,address,size,unused):
        """게임 계산은 원본에 맡기고 행 복사와 반환 경계만 처리한다."""
        if address==STOP:current.emu_stop();return
        if address==0x406939:
            sp=current.reg_read(UC_X86_REG_ESP);ret,pane,y,row,width=read(sp,5)
            assert pane==PANE and width==dimensions[0] and y<dimensions[1]
            pixels[y*width:(y+1)*width]=current.mem_read(row,width);seen.append(y)
            current.reg_write(UC_X86_REG_ESP,sp+20);current.reg_write(UC_X86_REG_EIP,ret);return
    def block(current,address,size,unused):
        """기본 블록마다 미선언 호출을 막으며 LZW 원문은 후킹하지 않는다."""
        nonlocal blocks
        blocks+=1
        if address not in (STOP,0x406939) and not (0x406d90<=address<0x407118 or 0x407171<=address<0x4071ab):raise RuntimeError('미선언 Native 경계 '+hex(address))
    cpu.hook_add(UC_HOOK_BLOCK,block);cpu.hook_add(UC_HOOK_CODE,hook,begin=STOP,end=STOP);cpu.hook_add(UC_HOOK_CODE,hook,begin=0x406939,end=0x406939)
    def call(address,args):
        """원문의 stdcall 스택과 실제 RET를 사용한다."""
        cpu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));cpu.reg_write(UC_X86_REG_ESP,STACK)
        try:cpu.emu_start(address,STOP+1,count=100000000)
        except Exception as error:raise RuntimeError(f'{path.name} EIP={cpu.reg_read(UC_X86_REG_EIP):08x} ESP={cpu.reg_read(UC_X86_REG_ESP):08x}') from error
        assert cpu.reg_read(UC_X86_REG_EIP)==STOP and cpu.reg_read(UC_X86_REG_ESP)==STACK+4+4*len(args)
        return cpu.reg_read(UC_X86_REG_EAX)
    packed=call(0x407171,[SOURCE]);dimensions.extend([packed>>16,packed&65535]);width,height=dimensions;assert width and height
    pixels.extend(b'\xcd'*(width*height));call(0x406ef7,[PANE,SOURCE]);assert len(seen)==height and sorted(seen)==list(range(height))
    fingerprint=14695981039346656037
    for value in pixels:  # 실제 Native 출력 바이트만 FNV1a64로 축약한다.
        fingerprint=((fingerprint^value)*1099511628211)&0xffffffffffffffff
    return dict(path=path.relative_to(ROOT/'originals').as_posix(),width=width,height=height,fnv1a64=f'{fingerprint:016x}',pixelSha256=hashlib.sha256(pixels).hexdigest(),sourceSha256=hashlib.sha256(data).hexdigest(),rows=len(seen),basicBlockEntries=blocks)

def main():
    """C++ 해독기를 호출하지 않고 보호 GIF 전체 최초 관측 또는 읽기 검증을 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon'
    assert hashlib.sha256(PE.read_bytes()).hexdigest()=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    rows=[]
    for path in sorted((ROOT/'originals/d').iterdir(),key=lambda value:value.name.lower()):  # 실제 보호 GIF 전체를 각각 새 CPU에서 실행한다.
        if path.suffix.lower()!='.gif':continue
        row=decode(path);rows.append(row);print(json.dumps(row),flush=True)
    content='path\twidth\theight\tfnv1a64\tpixelSha256\tsourceSha256\trows\tbasicBlockEntries\n'+''.join('\t'.join(str(row[key]) for key in ('path','width','height','fnv1a64','pixelSha256','sourceSha256','rows','basicBlockEntries'))+'\n' for row in rows)
    if '--verify' in sys.argv:assert FIXTURE.read_text('utf-8')==content
    else:
        FIXTURE.open('x',encoding='utf-8',newline='\n').write(content)
        report=dict(host=socket.gethostname(),gameExecuted=False,bodies=['00407171','00406ef7','00406d90..00407117'],deviceBoundary='00406939 실제 LZW 행의 출력 복사만 관측',peSha256=hashlib.sha256(PE.read_bytes()).hexdigest(),fixtureSha256=hashlib.sha256(content.encode()).hexdigest(),cases=rows)
        with REPORT.open('x',encoding='utf-8',newline='\n') as output:json.dump(report,output,ensure_ascii=False,indent=2);output.write('\n')
    print(json.dumps(dict(rows=len(rows),verified='--verify' in sys.argv)),flush=True)
if __name__=='__main__':main()
