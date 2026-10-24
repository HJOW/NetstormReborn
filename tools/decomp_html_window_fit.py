"""보호4c7d40 전체 배치와 실제_ftol을 독립 측정/메뉴/트리 장치 입력으로 관측한다."""
from pathlib import Path
import hashlib,json,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
ROOT=Path(__file__).resolve().parents[1];PE=ROOT/'originals/Netstorm.exe';SOURCE=ROOT/'extracted/refined/originals/Netstorm.c'  # 현재 보호 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/html-window-fit-x86.tsv';REPORT=ROOT/'cpppj/html-window-fit-native-20261024.json'  # 최초 관측을 덮어쓰지 않는다.
HEAP=0x2000000;NODE=HEAP+0x1000;CHILD=HEAP+0x10000;VTABLE=HEAP+0x20000;PUSH=HEAP+0x21000;ATTACH=HEAP+0x21100;STOP=HEAP+0x22000;STACK=0x3000000  # 부모/25자식과 가상 장치의 실제 주소다.
FIELDS=(4,0x30,0x38,0x3c,0x40,0x44,0x48,0x4c,0x50);ROOT_FIELDS=(0x21b4,0x21b8,0x21c0,0x21c4,0x21c8,0x21cc);ROOT_NAMES=('count','body','rw','rh','columns','aux')  # 배치가 쓰는 실제 현재 필드다.
GLOBALS=(0x5ca9b4,0x531860,0x531864,0x5c78f0,0x5c78f4,0x5ca9cc,0x5ca9d4,0x5ca9d8,0x5ca9d0)  # 현재 뷰 폭/출력 크기/전체 크기/지도 직사각형이다.
INPUTS=('name','control','entities','root','slots','globals','natural','widths','heights','menus','args','reference','mutation','stop');OUTPUTS=('completed','outEntities','outRoot','outSlots','outGlobals','outMenus','heightCalls','trace','stackDelta','instructions')  # 입력과 실제 관측은 별도 열이다.
def pack(values):
    """원시DWORD 입력/관측을 십진수 목록으로 보존한다."""
    return ':'.join(map(str,values))
def bits(value):
    """독립 float 입력의 비트만 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]
def value(word):
    """장치의 명시 정수→float 변환에 필요한 원시 값을 읽는다."""
    return struct.unpack('<f',struct.pack('<I',word))[0]
def signed(word):
    """하위DWORD의 signed 입력이다."""
    return struct.unpack('<i',struct.pack('<I',word&0xffffffff))[0]
def cases():
    """게임 배치 결과를 계산하지 않고 측정/크기/메뉴/변경 입력만 교차한다."""
    rows=[]
    def add(count=1,rw=400,rh=0,natural=200,heights=(16,),columns=0,scale=1,reference=0,view=600,screen=(640,480),widths=None,mutation='-',stop='-',args=None,menu_sizes=((100,60),(200,100))):
        """같은 실제26개 객체와 하위 측정 바이트를 독립 입력으로 만든다."""
        entities=[]
        for token in range(1,27):  # 부모/본문/20버튼/두메뉴/외부 부모의 현재 필드다.
            bw,bh=(0,0) if token==1 else (40,80) if token in (2,3) else menu_sizes[token-24] if token in (24,25) else (60,16)
            entities.extend([9 if token==1 else 1,0xffffffd8,bits(scale),bits(bw),bits(bh),bits(bw),bits(bh),bits(3.5),bits(-7.25)])
        root=[count,2,rw&0xffffffff,rh&0xffffffff,columns,17];slots=[index+4 if index<count else 0 for index in range(20)]
        rows.append(dict(name=str(len(rows)),entities=pack(entities),root=pack(root),slots=pack(slots),globals=pack([view,*screen,*screen,20,620,450,30]),natural=pack([bits(natural),bits(0)]),widths=pack([bits(item) for item in (widths or [60]*20)]),heights=pack([bits(item) for item in list(heights)+[heights[-1]]*80]),menus=pack([24,25] if columns>1 else [25]),args=pack(args or [bits(101),bits(203),26,0]),reference=reference,mutation=mutation,stop=stop))
    for count in (0,1,2,3,20):  # 버튼 없음/하나/공통 최대 폭·간격/원문20행이다.
        for natural in (20,200,1000):add(count=count,natural=natural)
    for width in (0,40,80,200,360,400,639,640,800,1024):add(rw=width,natural=1600,heights=(200,))  # 폭 확대/정체/뷰 제한의 입력이다.
    for heights in ((600,500,350,200,100,60),(200,),(20,),(0,),(100.5,),(300,100,100,100,100)):
        for height in (0,200,600):add(natural=1600,heights=heights,rh=height)
    for columns in (1,2,3):  # 원문은2열 부착을 하면서 현재 열수로 부모 폭을 계산한다.
        for sizes in (((100,60),(200,100)),((80,180),(320,40)),((250,30),(50,190))):add(columns=columns,menu_sizes=sizes,count=2,widths=[40,90]+[60]*18)
    for scale in (0.75,1,1.25):  # FST32 크기와 원시 부착 좌표를 분리한다.
        for reference in (0,26):add(scale=scale,reference=reference,natural=1000,heights=(200,100,80))
    for screen in ((320,200),(640,480),(1024,768),(1920,1080)):add(screen=screen,natural=2000,heights=(1000,),view=screen[0])
    for mutation in ('fitButton.count.1','fitContent.body.3','fitHeight.body.3','fitHeight.rh.500','fitHeight.view.320','push.count.1','menuAttach.columns.1','reference.rw.9'):add(count=2,natural=1600,heights=(200,100,100,100),columns=2,mutation=mutation)
    for stop in ('fitButton','push','fitContent','fitHeight','menuCurrent','menuAttach','attach','reference','attachBase','depth'):add(natural=1600,heights=(200,),columns=2,stop=stop)  # 명시 중단은 Native EH 증거가 아니다.
    return rows
class ExplicitStop(Exception):
    """선택 장치의 명시 중단을 정상 RET와 구별한다."""
class Oracle:
    """실제 상위 배치/_ftol과 명시 하위 장치를 새 CPU에서 실행한다."""
    def __init__(self):
        """보호 PE만 읽는다."""
        self.pe=pefile.PE(str(PE));self.image=self.pe.get_memory_mapped_image()
    def run(self,case,control):
        """입력 객체/전역/메뉴 스택을 실제 독립 메모리에 설치한다."""
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x400000,(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(0x400000,self.image);cpu.mem_map(0,0x1000);cpu.mem_map(HEAP,0x30000);cpu.mem_map(STACK-0x10000,0x20000);cpu.reg_write(UC_X86_REG_FPCW,control);cpu.reg_write(UC_X86_REG_FPSW,0);trace=[];instructions=0;height_calls=0;menus=list(map(int,case['menus'].split(':')));heights=list(map(int,case['heights'].split(':')));widths=list(map(int,case['widths'].split(':')));natural=list(map(int,case['natural'].split(':')))
        def read(address):
            """현재DWORD다."""
            return struct.unpack('<I',cpu.mem_read(address,4))[0]
        def write(address,word):
            """현재DWORD를 그대로 쓴다."""
            cpu.mem_write(address,struct.pack('<I',word&0xffffffff))
        def pointer(token):
            """부모/25자식의 실제 독립 주소다."""
            return NODE if token==1 else CHILD+(token-2)*0x200 if token else 0
        def alias(address):
            """실제 관측 주소를 선언된 별칭으로 바꾼다."""
            if address==0:return 0
            if address==NODE:return 1
            assert CHILD<=address<CHILD+25*0x200 and (address-CHILD)%0x200==0,hex(address)
            return (address-CHILD)//0x200+2
        def event(name,args=()):
            """실제 장치 호출 뒤에만 명시 입력의 필드 변경/중단을 수행한다."""
            trace.append(name+(':'+pack(args) if args else ''))
            if case['mutation']!='-':
                phase,field,word=case['mutation'].split('.')
                if name==phase:
                    if field=='view':write(0x5ca9b4,int(word))
                    else:write(NODE+ROOT_FIELDS[ROOT_NAMES.index(field)],pointer(int(word)) if field=='body' else int(word))
            if case['stop']==name:raise ExplicitStop()
        def hook(current,address,size,unused):
            """원문 본체/실제_ftol 밖은 명시 측정·메뉴·트리 장치만 허용한다."""
            nonlocal instructions,height_calls
            instructions+=1;sp=current.reg_read(UC_X86_REG_ESP);this=current.reg_read(UC_X86_REG_ECX)
            def arg(index):
                """현재 스택 인자다."""
                return read(sp+4+4*index)
            def returned(pop=0,eax=0):
                """실제cdecl/thiscall 반환을 수행한다."""
                current.reg_write(UC_X86_REG_EAX,eax);current.reg_write(UC_X86_REG_ESP,sp+4+pop);current.reg_write(UC_X86_REG_EIP,read(sp))
            def attach(name,pop):
                """주어진 실제 raw 좌표를 저장할 뿐 배치 결과를 계산하지 않는다."""
                write(this+0x4c,arg(0));write(this+0x50,arg(1));write(this+4,read(this+4)&~8);event(name,(alias(this),arg(0),arg(1),alias(arg(2)),arg(3)));returned(pop)
            if address==STOP:current.emu_stop();return
            if address==0x424f90:write(this+0x3c,widths[alias(this)-4]);event('fitButton',(alias(this),));returned();return
            if address==PUSH:write(this+4,read(this+4)|8);event('push',(alias(this),));returned();return
            if address==0x4b9fb0:assert arg(0)==arg(1)==0;write(this+0x3c,natural[0]);write(this+0x40,natural[1]);event('fitContent',(alias(this),));returned(8);return
            if address==0x4b9f20:assert height_calls<len(heights);write(this+0x40,heights[height_calls]);height_calls+=1;event('fitHeight',(alias(this),));returned();return
            if address in (0x4ba1b0,0x4ba230,0x4ba360):event({0x4ba1b0:'slider',0x4ba230:'scroll',0x4ba360:'request'}[address],(alias(this),arg(0)));returned(4);return
            if address==ATTACH:attach('attach',16);return
            if address==0x466750:attach('attachBase',16);return
            if address==0x475c40:assert menus;token=menus[-1];event('menuCurrent',(token,));returned(eax=pointer(token));return
            if address==0x475ca0:
                assert menus;token=menus[-1];write(pointer(token)+0x4c,bits(signed(arg(0))));write(pointer(token)+0x50,bits(signed(arg(1))));write(pointer(token)+4,read(pointer(token)+4)&~8);event('menuAttach',(token,arg(0),arg(1),alias(arg(2))));menus.pop();returned();return
            if address==0x464020:event('reference',(arg(0),));returned(eax=pointer(case['reference']));return
            if address==0x4652b0:write(this+0x30,read(this+0x30)+arg(0));event('depth',(alias(this),arg(0)));returned(4);return
            if not (0x4c7d40<=address<0x4c8346 or 0x4e49c0<=address<0x4e4a35):raise RuntimeError('미선언 Native Fit 경계 '+hex(address))
        entities=list(map(int,case['entities'].split(':')))
        for token in range(1,27):  # 관측할 모든 객체의 현재 실제 필드를 설치한다.
            address=pointer(token);write(address,VTABLE)
            for offset,word in zip(FIELDS,entities[(token-1)*9:token*9]):write(address+offset,word)
        for offset,word in zip(ROOT_FIELDS,map(int,case['root'].split(':'))):write(NODE+offset,pointer(word) if offset==0x21b8 else word)  # 현재 부모의 부가 필드다.
        for index,word in enumerate(map(int,case['slots'].split(':'))):write(NODE+0x20c4+12*index,pointer(word))  # 실제20개 child 포인터다.
        for address,word in zip(GLOBALS,map(int,case['globals'].split(':'))):write(address,word)  # 현재 실제 전역들이다.
        write(VTABLE+0x18,PUSH);write(VTABLE+0x20,ATTACH);args=list(map(int,case['args'].split(':')));args[2]=pointer(args[2]);write(STACK,STOP)
        for index,word in enumerate(args):write(STACK+4+4*index,word)  # 실제 caller의4인자다.
        cpu.reg_write(UC_X86_REG_ESP,STACK);cpu.reg_write(UC_X86_REG_ECX,NODE);cpu.hook_add(UC_HOOK_CODE,hook);completed=1
        try:cpu.emu_start(0x4c7d40,STOP+1,count=1000000);assert cpu.reg_read(UC_X86_REG_EIP)==STOP
        except ExplicitStop:completed=0
        out=[read(pointer(token)+offset) for token in range(1,27) for offset in FIELDS];root=[read(NODE+offset) for offset in ROOT_FIELDS];root[1]=alias(root[1]);slots=[alias(read(NODE+0x20c4+12*i)) for i in range(20)]
        return (completed,pack(out),pack(root),pack(slots),pack([read(address) for address in GLOBALS]),pack(menus),height_calls,';'.join(trace),cpu.reg_read(UC_X86_REG_ESP)-STACK,instructions)
def main():
    """현재 PC/보호 SHA를 고정해 최초 관측 또는 읽기 전용 재생을 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon';peSha=hashlib.sha256(PE.read_bytes()).hexdigest();sourceSha=hashlib.sha256(SOURCE.read_bytes()).hexdigest();assert peSha=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236' and sourceSha=='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'
    oracle=Oracle();lines=['\t'.join(INPUTS+OUTPUTS)];differences=[];instructions=0
    for case in cases():  # 같은 독립 입력을 두 x87 정밀도로 실행한다.
        observations=[]
        for control in (0x27f,0x37f):
            output=oracle.run(case,control);observations.append(output);instructions+=output[-1];row={**case,'control':control};lines.append('\t'.join(map(str,[row[key] for key in INPUTS]+list(output))))
        if observations[0][:-1]!=observations[1][:-1]:differences.append(case['name'])
    content='\n'.join(lines)+'\n'
    if '--verify' in sys.argv:assert FIXTURE.read_text('utf-8')==content
    else:
        FIXTURE.open('x',encoding='utf-8',newline='\n').write(content)
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',peSha256=peSha,sourceSha256=sourceSha,rows=len(lines)-1,cases=len(cases()),instructions=instructions,precisionDifferences=differences,fixtureSha256=hashlib.sha256(content.encode()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gameExecuted=False,limits=['실제4c7d40 전체/x87비교·중간FST32·실제_ftol·RET 실행','26객체/현재6필드/20child/9전역/메뉴스택은 실제 독립 메모리 관측','측정값/가상Push·Attach/현재메뉴pop/기본Attach/깊이는 명시 장치이며 전체폰트/픽셀/트리/EH 증거 아님','장치 변경/중단을 정상 완료와 구별; 명령 위치에는 장치/STOP 포함']),stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(rows=len(lines)-1,instructions=instructions,precisionDifferences=differences,verified='--verify' in sys.argv)),flush=True)
if __name__=='__main__':main()
