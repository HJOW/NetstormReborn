"""보호4cf270 전체와 실제alnum/파서설정/스택·x87/RET를 독립 실행한다."""
from pathlib import Path
import hashlib,json,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW
ROOT=Path(__file__).resolve().parents[1];PE=ROOT/'originals/Netstorm.exe';SOURCE=ROOT/'extracted/refined/originals/Netstorm.c'  # 보호 입력은 읽기 전용이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/html-window-factory-x86.tsv';REPORT=ROOT/'cpppj/html-window-factory-native-20261024.json'  # 최초 근거를 덮어쓰지 않는다.
HEAP=0x2000000;NODE=HEAP+0x1000;SOURCE_BUFFER=HEAP+0x5000;FALLBACK=HEAP+0x9000;STRING_DATA=HEAP+0x10000;VTABLE=HEAP+0x20000;ATTACH=HEAP+0x21000;STOP=HEAP+0x22000;STACK=0x3000000  # 부모/문자열/가상부착/실제 스택 주소다.
INPUTS=('name','control','source','substituted','first','second','links1','links2','suppress','allocationFail','width','height','serial','geometry','screen','parent','wall','metadata','afterMeta','fallback','mutation','stop');OUTPUTS=('completed','createdSerial','lastCloseBits','nodeSerial','linksFnv','firstOut','secondOut','trace','result','stackDelta','instructions')  # 실제 출력은 입력과 구별한다.
def bits(value):
    """명시float 입력의32비트만 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]
def pack(values):
    """현재DWORD 관측을 순서대로 보존한다."""
    return ':'.join(map(str,values))
def cases():
    """기대 생성/부착 결과 없이 원문 바이트와 하위 장치 입력을 교차한다."""
    rows=[]
    def add(first='Body\n',**values):
        """파서/메타/할당 장치는 명시 입력이며 상위 분기를 계산하지 않는다."""
        byte=lambda value:value if isinstance(value,bytes) else value.encode('ascii')  # 원문 좁은 바이트를 보존한다.
        data=dict(name=str(len(rows)),source=byte(values.pop('source','<p>Source</p>')).hex(),substituted=byte(values.pop('substituted','substituted text')).hex(),first=byte(first).hex(),second=byte(values.pop('second','Rendered\n')).hex(),links1=byte(values.pop('links1',b'first\0target\0')).hex(),links2=byte(values.pop('links2',b'')).hex(),suppress=values.pop('suppress',0),allocationFail=values.pop('allocationFail',0),width=values.pop('width',400),height=values.pop('height',30),serial=values.pop('serial',707),geometry=pack(map(bits,values.pop('geometry',(91.25,53.75)))),screen=pack(values.pop('screen',(640,480))),parent=values.pop('parent',0x87654321),wall=values.pop('wall',123.25),metadata=values.pop('metadata',1),afterMeta=values.pop('afterMeta','-'),fallback=byte(values.pop('fallback','$Button=Okay,Config,0')).hex(),mutation=values.pop('mutation','-'),stop=values.pop('stop','-'));assert not values;rows.append(data)
    for first in ('','\n','!? 	\n','---','A','1','~[T123]text\n','Body\nMore\n','\xff'):
        add(first if first!='\xff' else b'\xff')  # signed CRT 입력도 명시 C로케일 장치를 통과한다.
    for suppress in (1,0xffffffff):add(suppress=suppress)
    add(allocationFail=1);add(metadata=0);add(metadata=0,afterMeta=b'Changed\n'.hex());add(afterMeta=b'Only\n\nlines\n'.hex());add(second='');add(second='No final LF');add(second='Already LF\n');add(second='A'*1004)
    for geometry in ((0,0),(-1.5,2.75),(800.125,601.5),(float('inf'),float('-inf'))):add(geometry=geometry)
    for screen in ((0,0),(320,240),(1024,768),(0xffffffff,0x80000000)):add(screen=screen)
    add(width=0xffffffff,height=0x80000000,serial=0xfedcba98);add(wall=-4.5);add(wall=0.0);add(links2=b'new\0');add(links1=b'',links2=b'after\0');add(links1=b'A\0B\0',links2=b'C\0')
    for mutation in ('clock.created.55','meta.created.66','body.serial.901','attach.serial.902','modal.serial.903','body.screenW.1280','body.parent.4275878552','body.width.1120403456','body.height.1112014848'):add(mutation=mutation)
    for stop in ('allocate','construct','substitute','parse1','clock','meta','body','attach','modal'):add(stop=stop)  # 명시 중단은 실제 EH unwind 검증과 구별한다.
    add(metadata=0,stop='fallback');add(first='!?',stop='kill');return rows
class ExplicitStop(Exception):
    """명시 하위 장치 중단이다."""
class Oracle:
    """실제 상위/파서설정/alnum관문과 명시 하위 장치를 한 CPU에 설치한다."""
    def __init__(self):
        """같은 보호 PE의 읽기 전용 이미지를 준비한다."""
        self.pe=pefile.PE(str(PE));self.image=self.pe.get_memory_mapped_image()
    def run(self,case,control):
        """정상/선택 중단의 현재 전역/문자열/실제 저장소를 다시 읽는다."""
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x400000,(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(0x400000,self.image);cpu.mem_map(0,0x1000);cpu.mem_map(HEAP,0x40000);cpu.mem_map(STACK-0x20000,0x40000);cpu.mem_write(STACK-0x20000,b'\xa5'*0x40000);cpu.reg_write(UC_X86_REG_FPCW,control)
        trace=[];instructions=0;parses=0;metas=0;constructed=False;strings={};first_output=None;second_output=None
        def read(address):
            """현재DWORD다."""
            return struct.unpack('<I',cpu.mem_read(address,4))[0]
        def write(address,value):
            """원시DWORD를 쓴다."""
            cpu.mem_write(address,struct.pack('<I',value&0xffffffff))
        def text(address):
            """실제로 설치된 첫NUL 앞 바이트만 관측한다."""
            result=bytearray()
            while (value:=cpu.mem_read(address,1)[0])!=0:result.append(value);address+=1  # 설치된 현재 바이트를 읽는다.
            return bytes(result)
        def alias(address):
            """실제 부모/파서 링크 포인터만 관측 별칭으로 바꾼다."""
            return 1 if address==NODE else 2 if address==NODE+0xc4 else 0 if not address else address
        def event(name,*values):
            """효과 인자를 기록한 뒤 명시 변경/중단만 적용한다."""
            trace.append(name+(':'+':'.join(map(str,values)) if values else ''))
            if case['mutation']!='-':
                phase,field,value=case['mutation'].split('.')
                if phase==name:write(dict(created=0x5cab38,serial=NODE+0x2c,screenW=0x5c78f0,parent=0x563e38,width=NODE+0x3c,height=NODE+0x40)[field],int(value))
            if case['stop']==name:raise ExplicitStop()
        def hook(current,address,size,unused):
            """명시 경계 외에는 실제 PE 명령을 그대로 실행한다."""
            nonlocal instructions,parses,metas,constructed,first_output,second_output
            instructions+=1;sp=cpu.reg_read(UC_X86_REG_ESP);this=cpu.reg_read(UC_X86_REG_ECX)
            def arg(index):return read(sp+4+4*index)  # 현재 호출자의 실제 스택 인자다.
            def returned(pop=0,eax=0):cpu.reg_write(UC_X86_REG_EAX,eax&0xffffffff);cpu.reg_write(UC_X86_REG_ESP,sp+4+pop);cpu.reg_write(UC_X86_REG_EIP,read(sp))  # 호출 규약 그대로 복귀한다.
            if address==STOP:cpu.emu_stop();return
            if address==0x4e4391:event('allocate',arg(0));returned(eax=0 if case['allocationFail'] else NODE);return
            if address==0x4cf120:
                event('construct',arg(0),arg(1));write(NODE,VTABLE);write(NODE+0x2c,case['serial']);write(NODE+0x3c,int(case['geometry'].split(':')[0]));write(NODE+0x40,int(case['geometry'].split(':')[1]));cpu.mem_write(NODE+0xc4,b'\0');constructed=True;returned(8,NODE);return
            if address==0x441660:
                event('substitute',text(arg(1)).hex(),arg(2),arg(3),arg(4),arg(5));cpu.mem_write(arg(0),bytes.fromhex(case['substituted'])+b'\0');returned();return
            if address==0x46bed0:event('bind',arg(1),alias(arg(2)),arg(3))  # 실제 본체의 현재 필드 쓰기는 대체하지 않는다.
            elif address==0x46c810:
                parses+=1;source=text(read(this+4));output=read(this+8);links=read(this+0x10);event('parse'+str(parses),source.hex(),alias(links),read(this+0x1c),read(this+0x20),read(this+0x2c));data=bytes.fromhex(case['first' if parses==1 else 'second']);cpu.mem_write(output,data+b'\0')
                if links:cpu.mem_write(links,bytes.fromhex(case['links1' if parses==1 else 'links2']))
                if parses==1:first_output=output
                else:second_output=output
                returned();return
            elif address==0x4e7054:
                byte=arg(0);returned(eax=int(48<=byte<=57 or 65<=byte<=90 or 97<=byte<=122));return
            elif address==0x460d70:event('clock');cpu.mem_write(arg(0),struct.pack('<d',case['wall']));returned(eax=arg(0));return
            elif address==0x4c83b0:
                metas+=1;event('meta',text(arg(0)).hex());
                if metas==1 and case['afterMeta']!='-':cpu.mem_write(arg(0),bytes.fromhex(case['afterMeta'])+b'\0')
                returned(4,case['metadata']);return
            elif address==0x4410f0:event('fallback',text(arg(0)).hex(),arg(1),arg(2),arg(3));returned(eax=FALLBACK);return
            elif address==0x4e3180:strings[this]=text(arg(0));returned(4,this);return
            elif address==0x4e35b0:strings[this]=strings[this].replace(text(arg(0)),text(arg(1)));returned(12,this);return
            elif address==0x46ee50:cpu.mem_write(STRING_DATA,strings[this]+b'\0');returned(eax=STRING_DATA);return
            elif address==0x4e31a0:returned();return
            elif address==0x4c7aa0:event('body',text(arg(0)).hex());returned(4);return
            elif address==ATTACH:event('attach',arg(0),arg(1),arg(2),arg(3));returned(16);return
            elif address==0x466600:event('modal',arg(0));returned(4);return
            elif address==0x466480:event('kill');returned();return
            elif not any(low<=address<high for low,high in ((0x4cf270,0x4cf54b),(0x4e4a40,0x4e4a7d),(0x46bea0,0x46bf10),(0x4c8d50,0x4c8d83))):raise RuntimeError('미선언 Native 공장 경계 '+hex(address))
        cpu.mem_write(NODE,b'\xa5'*0x21e8);cpu.mem_write(SOURCE_BUFFER,bytes.fromhex(case['source'])+b'\0');cpu.mem_write(FALLBACK,bytes.fromhex(case['fallback'])+b'\0');write(VTABLE+0x20,ATTACH);write(0x5cab38,777);cpu.mem_write(0x5cad30,struct.pack('<d',-88.5));write(0x5c78f0,int(case['screen'].split(':')[0]));write(0x5c78f4,int(case['screen'].split(':')[1]));write(0x563e38,case['parent'])
        for index,value in enumerate((STOP,SOURCE_BUFFER,case['suppress'],case['width'],case['height'])):write(STACK+4*index,value)  # 실제cdecl 네 인자와 복귀 주소다.
        cpu.reg_write(UC_X86_REG_ESP,STACK);cpu.hook_add(UC_HOOK_CODE,hook);completed=1
        try:cpu.emu_start(0x4cf270,STOP+1,count=1000000);assert cpu.reg_read(UC_X86_REG_EIP)==STOP
        except ExplicitStop:completed=0
        fingerprint=14695981039346656037 if constructed else 0
        if constructed:
            for byte in cpu.mem_read(NODE+0xc4,8192):fingerprint=((fingerprint^byte)*1099511628211)&0xffffffffffffffff  # 현재 부모8192BYTE를 실제로 다시 읽는다.
        return (completed,read(0x5cab38),struct.unpack('<Q',cpu.mem_read(0x5cad30,8))[0],read(NODE+0x2c) if constructed else 0,fingerprint,text(first_output).hex() if first_output else '',text(second_output).hex() if second_output else '', ';'.join(trace),cpu.reg_read(UC_X86_REG_EAX) if completed else 0,cpu.reg_read(UC_X86_REG_ESP)-STACK,instructions)
def main():
    """같은 PC/보호SHA에서 최초 관측 또는 읽기 전용 재생을 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon';peSha=hashlib.sha256(PE.read_bytes()).hexdigest();sourceSha=hashlib.sha256(SOURCE.read_bytes()).hexdigest();assert peSha=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236' and sourceSha=='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'
    oracle=Oracle();lines=['\t'.join(INPUTS+OUTPUTS)];different=[];instructions=0
    for case in cases():  # 같은 입력을 두 x87 정밀도로 실제 실행한다.
        observations=[]
        for control in (0x27f,0x37f):
            output=oracle.run(case,control);observations.append(output);instructions+=output[-1];row={**case,'control':control};lines.append('\t'.join(map(str,[row[key] for key in INPUTS]+list(output))))
        if observations[0][:-1]!=observations[1][:-1]:different.append(case['name'])
    content='\n'.join(lines)+'\n'
    if '--verify' in sys.argv:assert FIXTURE.read_text('utf-8')==content
    else:
        FIXTURE.open('x',encoding='utf-8',newline='\n').write(content)
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',peSha256=peSha,sourceSha256=sourceSha,rows=len(lines)-1,instructions=instructions,precisionDifferences=different,fixtureSha256=hashlib.sha256(content.encode()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gameExecuted=False,limits=['실제4cf270전체/스택probe/파서기본생성·출력/입력설정/alnum관문·x87중앙/RET','할당·HTML부모생성/Config치환·조회/파서실행/메타/String·replace/벽시각/Body/Attach/Modal/Kill/CRT alnum은 명시 하위 장치','파서 실행과 메타/전체 게임·Win32/네트워크를 함께 실행한 증거와 구별','현재 부모8192BYTE·전역·C문자열/순서를 실제 재조회; 미정스택 꼬리/전체EH unwind 검증 아님']),stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(rows=len(lines)-1,instructions=instructions,precisionDifferences=different,verified='--verify' in sys.argv)),flush=True)
if __name__=='__main__':main()
