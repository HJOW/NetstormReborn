"""보호10.78 BackgroundGif 창의 생성/교체/입력/출력/소멸 전체 본체를 독립 실행한다."""
from pathlib import Path
import hashlib,itertools,json,math,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1];PE=ROOT/'originals/Netstorm.exe';SOURCE=ROOT/'extracted/refined/originals/Netstorm.c'  # 현재 보호 자료다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236';SOURCE_SHA='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'  # 현재 HJOW-Athlon 정밀 자료다.
FIXTURE=ROOT/'cpppj/tests/fixtures/background-gif-gump-x86.tsv';REPORT=ROOT/'cpppj/background-gif-gump-native-20261024.json'  # 최초 자료를 덮어쓰지 않는다.
HEAP=0x02000000;NODE=HEAP+0x1000;NAME=HEAP+0x2000;PATH=HEAP+0x3000;BITMAP=HEAP+0x4000;STOP=HEAP+0xf000;STACK=0x03000000  # 독립 실제 메모리다.
OFFSETS=(0,4,0x20,0x38,0x3c,0x40,0x44,0x48,0x4c,0x50,0xa0,0xa4,0xa8,0xac)  # 입력/관측의 같은14DWORD다.
FIELDS=('vtable','flags','kind','scale','bw','bh','width','height','x','y','event','first','second','bitmap')  # 파생/기본 필드 이름이다.
ENTRIES={'construct':0x41a030,'refresh':0x419ec0,'setEvent':0x419e10,'handle':0x419e40,'draw':0x419f50,'destroy':0x419d90,'reattach':0x419df0}  # 가상표의 실제 진입점이다.
INPUTS=('name','operation','control','state','kind','present','loaded','dimensions','arguments','position','baseResult','mutation','stop')  # 관측을 입력으로 쓰지 않는다.
OUTPUTS=('completed','stateout','trace','result','stackdelta','instructions')  # 실제 CPU/장치 관측이다.

def sha(path):
    """원시 파일 전체의 현재 식별자를 얻는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()
def uint(value):
    """모든 입력/관측을 같은DWORD 폭으로 만든다."""
    return value&0xffffffff
def bits(value):
    """단정도 입력을 원시DWORD로 보관한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]
def packed(values):
    """같은 원시 필드를 십진수 목록으로 표현한다."""
    return ':'.join(str(uint(x)) for x in values)

def cases():
    """모든 본체와 선택 관문/현재 필드/명시 중단을 서로 다른 입력으로 만든다."""
    rows=[]
    def add(operation,**changes):
        """원문 입력만 구성하고 기대 결과는 넣지 않는다."""
        state=[0x501b60,0x1200,77,bits(1),bits(12.75),bits(9.5),bits(20),bits(30),bits(7.25),bits(-3.5),0x2000094,0x12345678,0x87654321,1]
        row=dict(name='gif%04d'%len(rows),operation=operation,state=packed(state),kind=3,present=1,loaded=3,dimensions='640:480',arguments='0:0:0',position='11:22',baseResult=0x87654321,mutation='-',stop='-');row.update(changes);rows.append(row)
    configurations=((0,0,'0:0'),(1,3,'640:480'),(1,0,'640:480'),(1,3,'65535:1'))  # 적재 생략/성공/NULL/16비트 치수다.
    for kind,scale,config in itertools.product((3,127,128,255),(.5,1,1.25),configurations):  # signed 종류BYTE와 실제 FILD/FST 분기다.
        add('construct',kind=kind,present=config[0],loaded=config[1],dimensions=config[2]);state=rows[-1]['state'].split(':');state[3]=str(bits(scale));rows[-1]['state']=':'.join(state)
    for old,scale,config in itertools.product((0,1),(.5,1,1.25),configurations):  # 같은 창의 이전 free와 새 치수를 구별한다.
        add('refresh',present=config[0],loaded=config[1],dimensions=config[2]);state=rows[-1]['state'].split(':');state[3]=str(bits(scale));state[-1]=str(old);rows[-1]['state']=':'.join(state)
    for flags,args in itertools.product((0,1,0xa5a5a5a5),('0:0:0','33554580:4294967295:2147483648')):  # 세 필드 쓰기/flags OR다.
        add('setEvent',arguments=args);state=rows[-1]['state'].split(':');state[1]=str(flags);rows[-1]['state']=':'.join(state)
    for enabled,event,result in itertools.product((0,1),(0x10000,0x20000,27,13,0x80010000,0x2000001b,0x1000001b,0x0200001b,0x8000001b,0xffffffff,0x20010000,0x10010000),(0,0x87654321)):  # release/수정자/command 비트를 독립 보존한다.
        add('handle',arguments=packed((event,123,456)),baseResult=result);state=rows[-1]['state'].split(':');state[10]=str(enabled);rows[-1]['state']=':'.join(state)
    for bitmap,size in itertools.product((0,1),((640.75,480.5),(-1,0),(float('nan'),float('inf')),(16777216,1))):  # base 치수 절삭과 bitmap0 조기 복귀다.
        add('draw');state=rows[-1]['state'].split(':');state[4]=str(bits(size[0]));state[5]=str(bits(size[1]));state[-1]=str(bitmap);rows[-1]['state']=':'.join(state)
    for bitmap in (0,1):  # 원문은 소멸 뒤 pointer0을 쓰지 않는다.
        add('destroy');state=rows[-1]['state'].split(':');state[-1]=str(bitmap);rows[-1]['state']=':'.join(state)
    add('reattach');add('reattach',mutation='attach.flags.7')  # 같은 현재 x/y/parent0/mode0이다.
    for operation,mutation in (('construct','base.kind.255'),('construct','load.event.123'),('refresh','free.bitmap.2'),('refresh','load.scale.1067450368'),('refresh','exists.bitmap.2'),('draw','position.bitmap.0'),('draw','position.bw.1101004800'),('handle','dispatch.poison.0')):  # 효과 뒤 재조회/기존 레지스터 보존이다.
        add(operation,mutation=mutation,arguments='65536:123:456')
    for operation,stop in (('construct','base'),('construct','spec'),('construct','load'),('refresh','free'),('refresh','exists'),('draw','position'),('handle','dispatch'),('destroy','free')):  # 명시 중단은 일반 EH 복원의 증거가 아니다.
        add(operation,stop=stop,arguments='65536:123:456')
    return rows

class ExplicitStop(Exception):
    """선택 장치의 명시 중단을 정상 RET와 구별한다."""

class Oracle:
    """현재 PC의 보호 PE와 독립 기본/파일/픽셀/입력 장치를 사용한다."""
    def __init__(self):
        """게임 프로세스 없이 원시 PE 이미지만 준비한다."""
        self.pe=pefile.PE(str(PE));self.image=self.pe.get_memory_mapped_image();self.base=self.pe.OPTIONAL_HEADER.ImageBase
    def run(self,case,control):
        """모든 실제 본체와 _ftol을 새 CPU에서 실행한다."""
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(self.base,(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(self.base,self.image);cpu.mem_map(0,0x1000);cpu.mem_map(HEAP,0x10000);cpu.mem_map(STACK-0x10000,0x20000)
        cpu.reg_write(UC_X86_REG_FPCW,control);cpu.reg_write(UC_X86_REG_FPSW,0);trace=[];instructions=0  # 실제 명령 위치/장치 순서다.
        def read(address):
            """같은 실제 네BYTE를 읽는다."""
            return struct.unpack('<I',cpu.mem_read(address,4))[0]
        def write(address,value):
            """CPU의 현재DWORD를 원시 비트로 쓴다."""
            cpu.mem_write(address,struct.pack('<I',uint(value)))
        def pointer(value):
            """호스트 bitmap 별칭0..3을 유효 독립 주소로 만든다."""
            return BITMAP+value*0x100 if value else 0
        def alias(value):
            """실제 독립 bitmap 주소만 별칭으로 관측한다."""
            if not value:return 0
            if value in (pointer(1),pointer(2),pointer(3)):return (value-BITMAP)//0x100
            raise RuntimeError('미선언 bitmap 포인터 '+hex(value))
        def event(name,values=()):
            """실제 호출 인자 뒤에만 명시 입력의 부작용/중단을 적용한다."""
            trace.append(name+(':'+packed(values) if values else ''))
            if case['mutation']!='-':
                phase,field,value=case['mutation'].split('.')
                if phase==name:
                    if field=='kind':cpu.mem_write(0x54041e,bytes([int(value)&255]))
                    elif field=='poison':
                        for offset in OFFSETS:write(NODE+offset,0x5c5c5c5c)  # dispatch 뒤 읽지 않는지 검사한다.
                    else:write(NODE+OFFSETS[FIELDS.index(field)],pointer(int(value)) if field=='bitmap' else int(value))
            if case['stop']==name:raise ExplicitStop()
        def returned(cleanup=0,eax=None):
            """실제 CALL 규약대로 원문 호출자에 복귀한다."""
            esp=cpu.reg_read(UC_X86_REG_ESP);target=read(esp);cpu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);cpu.reg_write(UC_X86_REG_EIP,target)
            if eax is not None:cpu.reg_write(UC_X86_REG_EAX,uint(eax))
        def hook(cpu,address,size,user):
            """본문/CRT는 실행하고 명시 하위 장치만 동일 규약으로 공급한다."""
            nonlocal instructions
            instructions+=1
            if address==STOP:cpu.emu_stop();return
            esp=cpu.reg_read(UC_X86_REG_ESP);arg=lambda index:read(esp+4+index*4)  # 원문 진입 스택이다.
            if address==0x4642c0:
                assert [arg(i) for i in range(4)]==[0,0,0,1]
                write(NODE,0x501b60);write(NODE+4,1)
                for offset in (0x3c,0x40,0x44,0x48):write(NODE+offset,0)  # 기본 생성 장치의 명시 필드다.
                event('base');returned(16,NODE);return
            if address==0x4e4354:event('free',(alias(arg(0)),));returned();return
            if address==0x4410f0:assert arg(1)==NAME and arg(2)==arg(3)==0;event('spec');returned(eax=PATH);return
            if address==0x41ab10:assert arg(0)==PATH;event('exists');returned(eax=int(case['present']));return
            if address==0x4dae60:
                assert arg(0)==PATH
                width,height=map(int,case['dimensions'].split(':'));write(arg(1),width);write(arg(2),height);event('load');returned(eax=pointer(int(case['loaded'])));return
            if address==0x464740:
                x,y=map(int,case['position'].split(':'));write(arg(0),x);write(arg(0)+4,y);event('position');returned(4);return
            if address==0x4a1fe0:event('copy',(arg(0),arg(1),alias(arg(2)),*[arg(i) for i in range(3,8)]));returned();return
            if address==0x466750:event('attach',tuple(arg(i) for i in range(4)));returned(16);return
            if address==0x452f90:event('dispatch',tuple(arg(i) for i in range(3)));returned();return
            if address==0x464990:event('baseHandle',tuple(arg(i) for i in range(3)));returned(12,int(case['baseResult']));return
            if address==0x464420:event('destructBase');returned();return
            if not any(low<=address<high for low,high in ((0x419d90,0x419fc0),(0x41a030,0x41a0b0),(0x4e49c0,0x4e4a35))):raise RuntimeError('미선언 원문 진입 '+hex(address))
        state=list(map(int,case['state'].split(':')))
        for offset,value in zip(OFFSETS,state):write(NODE+offset,pointer(value) if offset==0xac else value)  # 같은 실제 입력 필드다.
        cpu.mem_write(0x54041e,bytes([int(case['kind'])&255]));cpu.mem_write(NAME,b'title\0');cpu.mem_write(PATH,b'spec\0')
        operation=case['operation'];args=[NAME] if operation in ('construct','refresh') else list(map(int,case['arguments'].split(':'))) if operation in ('setEvent','handle') else []
        esp=STACK;write(esp,STOP)
        for index,value in enumerate(args):write(esp+4+index*4,value)  # 호출자 인자만 순서대로 넣는다.
        cpu.reg_write(UC_X86_REG_ESP,esp);cpu.reg_write(UC_X86_REG_ECX,NODE);cpu.hook_add(UC_HOOK_CODE,hook);completed=1
        try:cpu.emu_start(ENTRIES[operation],STOP+1,count=100000);assert cpu.reg_read(UC_X86_REG_EIP)==STOP
        except ExplicitStop:completed=0
        output=[read(NODE+offset) for offset in OFFSETS]
        if output[-1] in (0,pointer(1),pointer(2),pointer(3)):output[-1]=alias(output[-1])
        result=cpu.reg_read(UC_X86_REG_EAX) if completed and operation=='handle' else 1 if completed and operation=='construct' else 0
        return (completed,packed(output),';'.join(trace),result,cpu.reg_read(UC_X86_REG_ESP)-esp,instructions)

def main():
    """현재/정밀 PC·보호 SHA를 고정하고 최초 자료 또는 읽기 전용 재생만 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon' and sha(PE)==PE_SHA and sha(SOURCE)==SOURCE_SHA
    oracle=Oracle();lines=['\t'.join(INPUTS+OUTPUTS)];different=[];instructions=0
    for case in cases():  # 같은 독립 입력을 두 x87 정밀도로 실행한다.
        observations=[]
        for control in (0x27f,0x37f):
            output=oracle.run(case,control);observations.append(output);instructions+=output[-1];row={**case,'control':control};lines.append('\t'.join(map(str,[row[key] for key in INPUTS]+list(output))))
        if observations[0][:-1]!=observations[1][:-1]:different.append(case['name'])
    content='\n'.join(lines)+'\n'
    if len(sys.argv)==2 and sys.argv[1]=='--verify':assert FIXTURE.read_text('utf-8')==content
    else:
        assert len(sys.argv)==1
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write(content)
        result=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',peSha256=PE_SHA,sourceSha256=SOURCE_SHA,rows=len(lines)-1,cases=len(cases()),instructions=instructions,instructionCountIncludesDeviceHooks=True,precisionDifferences=different,entryPoints=ENTRIES,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),gameExecuted=False,limits=['실제 생성/교체/설정/가상 입력/Draw/소멸/재부착과 _ftol 본문 실행','419e40 진입은 원문 vtable/PE로 확인하며 정밀 텍스트의419e9e 꼬리 분리와 구별','기본 생성/파일/원시 복사/입력/수명 하위는 명시 장치; 전체 게임/GUI/디코더 증거 아님','명시 중단은 실제 EH unwind 검증 아님; 미완료/void EAX0·constructor this1 정규화'])
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(result,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(rows=len(lines)-1,instructions=instructions,precisionDifferences=different,verified=len(sys.argv)==2,fixtureSha256=sha(FIXTURE))))

if __name__=='__main__':main()  # import는 보호 자료/기대값을 수정하지 않는다.
