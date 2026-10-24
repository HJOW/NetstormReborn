"""보호10.78 HTML 부모의 생성/소멸/제어 행/본문/입력 전체 본체를 독립 관측한다."""
from pathlib import Path
import hashlib,json,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW
ROOT=Path(__file__).resolve().parents[1];PE=ROOT/'originals/Netstorm.exe';SOURCE=ROOT/'extracted/refined/originals/Netstorm.c'  # 현재 보호 원문이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/html-window-gump-x86.tsv';REPORT=ROOT/'cpppj/html-window-gump-native-20261024.json'  # 최초 관측은 덮어쓰지 않는다.
HEAP=0x2000000;NODE=HEAP+0x1000;TEXT=HEAP+0x5000;CHILD=HEAP+0x10000;VTABLE=HEAP+0x20000;ATTACH=HEAP+0x21000;STOP=HEAP+0x22000;STACK=0x3000000  # 실제 독립 주소와 가상 장치다.
OFFSETS=(0,4,0x20,0x34,0x38,0x3c,0x40,0x44,0x48,0x4c,0x50,*range(0x21b4,0x21e8,4));NAMES=('vtable','flags','kind','depth','scale','bw','bh','width','height','x','y','count','body','update','rw','rh','columns','aux','timeLo','timeHi','timeout','timeoutArg','exit','exitArg')  # 같은24현재DWORD다.
ENTRIES=dict(construct=0x4cf120,destroy=0x4cf1c0,clear=0x4c7c10,add=0x4c7c60,body=0x4c7aa0,handle=0x4c79b0)  # 실제 원문 진입이다.
INPUTS=('name','operation','control','fields','slots','fill','instances','kind','args','text','reference','wall','mutation','stop');OUTPUTS=('completed','outFields','outSlots','htmlFnv','children','outInstances','outWall','trace','result','stackDelta','instructions')  # 입력과 최초 관측을 구별한다.
def pack(values):
    """원시DWORD/정수 관측을 십진수 목록으로 보존한다."""
    return ':'.join(map(str,values))
def bits(value):
    """명시 float 입력의 원시 비트만 얻는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]
def cases():
    """기대 결과 없이 독립 입력과 장치 변경/중단만 만든다."""
    rows=[];fields=[0x515e7c,9,27,0x12345678,bits(1),bits(80),bits(50),bits(80),bits(50),bits(3.5),bits(-7.25),2,2,7,400,30,0,17,0,0,0,0x89abcdef,0,0x76543210];slots=[0]*60
    for index in (0,1):slots[index*3:index*3+3]=[index+2,0x02000095+index,0x87654321+index]  # 실제 두 자식의 입력만 배치한다.
    def add(operation,changes=None,**kw):
        """기본 현재 필드를 복사한 뒤 명시 입력만 바꾼다."""
        current=fields.copy()
        for key,value in (changes or {}).items():current[NAMES.index(key)]=value  # 원시 필드 입력이며 게임 계산이 아니다.
        rows.append(dict(name=str(len(rows))+'-'+operation,operation=operation,fields=pack(current),slots=pack(kw.pop('slots',slots)),fill=165,instances=kw.pop('instances',5),kind=kw.pop('kind',27),args=kw.pop('args','400:30:0:0'),text=kw.pop('text','Hello'.encode().hex()),reference=kw.pop('reference',5),wall=kw.pop('wall',struct.unpack('<Q',struct.pack('<d',123.25))[0]),mutation=kw.pop('mutation','-'),stop=kw.pop('stop','-')));assert not kw
    for kind in (27,255):  # ctor의 signed 종류와 인스턴스 감김이다.
        for count in (0,0xffffffff):
            for args in ('0:0:0:0','400:30:0:0','4294967295:2147483648:0:0'):add('construct',kind=kind,instances=count,args=args)
    for command in (0,0x02000095,0xffffffff):  # 원문은 종료 인자를 두 번 그대로 예약한다.
        for reference in (0,5):add('destroy',{'exit':command},reference=reference)
    add('destroy',{'exit':0xffffffff},instances=0,mutation='queue.aux.99',reference=0)
    for count in (0,1,2,20,0xffffffff):add('clear',{'count':count})  # 음수 count는 루프를 돌지 않는다.
    add('clear',{'count':20},slots=[0]*60);add('clear',{'count':2},mutation='kill.count.1')
    for count in (0,1,19,20):  # 정상 마지막 행과 한도 거부다.
        for instant in (0,1,4):add('add',{'count':count},args=f'0:4275878552:2309737967:{instant}')
    add('add',{'count':0},mutation='button.count.3');add('add',{'count':0},mutation='attach.count.3')
    for text in (b'',b'A',b'hello\n',b'a\0tail',b'~rStyled\n',b'B'*1006):  # 끝LF/첫NUL과 원문 지역 버퍼의 유효 끝이다.
        for flags in (9,1):
            for body in (0,2):add('body',{'flags':flags,'body':body},text=text.hex())
    add('body',mutation='kill.body.3');add('body',mutation='parse.body.3');add('body',{'flags':1},mutation='push.x.1101004800')
    for slot in range(20):  # count와 무관한 모든 실제 행의 serial 선택이다.
        current=[0]*60;current[slot*3:slot*3+3]=[slot+2,0xfedcba98,0x87654321]
        for modifier in (0,0x80000000):add('handle',{'count':0},slots=current,args=f'{0x0200002c|modifier}:{1000+slot}:0:0')
    for event in (0x02000038,0x82000038,0x22000038,0x020000be,0x820000be,0x123456,0xffffffff):add('handle',args=f'{event}:10:20:0')  # release를 별도 거부하지 않는 원문이다.
    for second in (0,123):add('handle',args=f'{0x02000025}:10:{second}:0')
    for operation,phase in (('construct','base'),('destroy','queue'),('destroy','wall'),('clear','kill'),('add','alloc'),('add','attach'),('body','parse'),('body','reference'),('handle','queue')):add(operation,{'exit':0x02000095},args='33554488:1000:0:1',stop=phase)  # 선택 중단이며 Native EH 전체 검증과 구별한다.
    return rows
class ExplicitStop(Exception):
    """선택 장치의 명시 중단을 정상 RET와 구별한다."""
class Oracle:
    """실제 보호 본체와 명시 기본/자식/큐/시각 장치를 사용한다."""
    def __init__(self):
        """읽기 전용 PE 이미지를 준비한다."""
        self.pe=pefile.PE(str(PE));self.image=self.pe.get_memory_mapped_image()
    def run(self,case,control):
        """새 독립 CPU에 같은 원시 노드와 현재 전역을 설치한다."""
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x400000,(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(0x400000,self.image);cpu.mem_map(0,0x1000);cpu.mem_map(HEAP,0x30000);cpu.mem_map(STACK-0x20000,0x40000);cpu.reg_write(UC_X86_REG_FPCW,control);trace=[];instructions=0
        def read(address):
            """현재 원시DWORD다."""
            return struct.unpack('<I',cpu.mem_read(address,4))[0]
        def write(address,value):
            """현재 원시DWORD를 그대로 쓴다."""
            cpu.mem_write(address,struct.pack('<I',value&0xffffffff))
        def pointer(token):
            """실제 부모/24개 자식의 독립 주소다."""
            return NODE if token==1 else CHILD+(token-2)*0x200 if token else 0
        def alias(address):
            """관측에서만 같은 주소를 안정된 정수로 바꾼다."""
            if address==NODE:return 1
            if address==0:return 0
            assert CHILD<=address<CHILD+24*0x200 and (address-CHILD)%0x200==0,hex(address)
            return (address-CHILD)//0x200+2
        def event(name,args=()):
            """실제 인자를 기록한 뒤에만 외부 변경/중단을 적용한다."""
            trace.append(name+(':'+pack(args) if args else ''))
            if case['mutation']!='-':
                phase,field,value=case['mutation'].split('.')
                if name==phase:write(NODE+OFFSETS[NAMES.index(field)],pointer(int(value)) if field=='body' else int(value))
            if case['stop']==name:raise ExplicitStop()
        def string(address):
            """현재C문자열을 첫NUL까지 관측한다."""
            result=bytearray()
            while (value:=cpu.mem_read(address,1)[0])!=0:result.append(value);address+=1  # 원문 입력 바이트만 읽는다.
            return result.hex()
        def hook(current,address,size,unused):
            """원문 본체 밖의 실제 장치 경계만 처리한다."""
            nonlocal instructions
            instructions+=1;sp=current.reg_read(UC_X86_REG_ESP);this=current.reg_read(UC_X86_REG_ECX)
            def arg(index):
                """현재 스택 인자를 읽는다."""
                return read(sp+4+index*4)
            def returned(pop=0,eax=0):
                """cdecl/thiscall 정리를 실제 반환에 적용한다."""
                current.reg_write(UC_X86_REG_EAX,eax);current.reg_write(UC_X86_REG_ESP,sp+4+pop);current.reg_write(UC_X86_REG_EIP,read(sp))
            if address==STOP:current.emu_stop();return
            if address==0x465d10:
                assert this==NODE and arg(0)==arg(1)==0
                for offset,value in ((0,0x50ac70),(4,9),(0x20,57),(0x34,0),(0x38,bits(1)),(0x3c,0),(0x40,0),(0x44,0),(0x48,0),(0x4c,0),(0x50,0)):write(NODE+offset,value)  # 원문 하위 기본 장치의 명시 효과다.
                event('base');returned(8,NODE);return
            if address==0x4e4391:event('alloc',(arg(0),));assert arg(0) in (0x170,0xd8);returned(eax=pointer(24 if arg(0)==0x170 else 25));return
            if address in (0x4254b0,0x4bc060):
                assert arg(0)==arg(1)==0
                if address==0x4254b0:assert arg(3)==0x11223344 and arg(4)==0;write(this+0xac,0);event('button',(alias(this),string(arg(2))));returned(20,this)
                else:assert arg(2)==0 and arg(3)==0x55667788;write(this+0x34,0);event('body',(alias(this),));returned(16,this)
                return
            if address in (ATTACH,0x4c7d40):event('attach',(alias(this),arg(0),arg(1),alias(arg(2)),arg(3)));returned(16);return
            if address==0x4ba230:event('scroll',(alias(this),arg(0)));returned(4);return
            if address==0x4ba8f0:event('parse',(alias(this),string(arg(0))));returned(4);return
            if address==0x464020:event('reference',(arg(0),));returned(eax=pointer(case['reference']));return
            if address==0x466480:event('kill',(alias(this),));returned();return
            if address==0x464560:event('push',(alias(this),));returned();return
            if address==0x452f90:event('queue',tuple(arg(i) for i in range(3)));returned();return
            if address==0x4a00c0:event('link',(arg(0),));returned();return
            if address==0x460d70:cpu.mem_write(arg(0),struct.pack('<Q',case['wall']));event('wall');returned(eax=arg(0));return
            if address==0x464420:event('destruct');returned();return
            if address==0x464990:event('baseHandle',tuple(arg(i) for i in range(3)));returned(12,0xfedcba98);return
            if not (0x4cf120<=address<0x4cf270 or 0x4c79b0<=address<0x4c7d40):raise RuntimeError('미선언 Native 경계 '+hex(address))
        cpu.mem_write(NODE,bytes([case['fill']])*0x21e8)
        for offset,value in zip(OFFSETS,map(int,case['fields'].split(':'))):write(NODE+offset,pointer(value) if offset==0x21b8 else value)  # 실제 현재 부모 필드다.
        for index,value in enumerate(map(int,case['slots'].split(':'))):write(NODE+0x20c4+4*index,pointer(value) if index%3==0 else value)  # 실제20행이다.
        for token in range(2,26):  # 현재 실제 자식 메모리와 호출 가능한 가상 표다.
            address=pointer(token);write(address,VTABLE);write(address+0x2c,token+998);write(address+0x34,0x12345678);write(address+0xac,0x2468)
        write(VTABLE+0x20,ATTACH);write(0x5cab2c,case['instances']);cpu.mem_write(0x540436,bytes([case['kind']]));write(0x5acd24,0x11223344);write(0x5bec2c,0x55667788);cpu.mem_write(0x5cad30,struct.pack('<d',-7.5));cpu.mem_write(TEXT,bytes.fromhex(case['text'])+b'\0')
        args=list(map(int,case['args'].split(':')));operation=case['operation']
        args=args[:2] if operation=='construct' else [TEXT,*args[1:]] if operation=='add' else [TEXT] if operation=='body' else args[:3] if operation=='handle' else []
        write(STACK,STOP)
        for index,value in enumerate(args):write(STACK+4+index*4,value)  # 호출자의 실제 인자다.
        cpu.reg_write(UC_X86_REG_ESP,STACK);cpu.reg_write(UC_X86_REG_ECX,NODE);cpu.hook_add(UC_HOOK_CODE,hook);completed=1
        try:cpu.emu_start(ENTRIES[operation],STOP+1,count=1000000);assert cpu.reg_read(UC_X86_REG_EIP)==STOP
        except ExplicitStop:completed=0
        out=[read(NODE+offset) for offset in OFFSETS];out[NAMES.index('body')]=alias(out[NAMES.index('body')]);slots=[read(NODE+0x20c4+4*i) for i in range(60)]
        for index in range(0,60,3):slots[index]=alias(slots[index])  # 주소 별칭은 관측에만 사용한다.
        fingerprint=14695981039346656037
        for value in cpu.mem_read(NODE+0xc4,8192):fingerprint=((fingerprint^value)*1099511628211)&0xffffffffffffffff  # 실제8192BYTE 전체 관측이다.
        children=[read(pointer(token)+offset) for token in range(2,26) for offset in (0x2c,0x34,0xac)]
        result=cpu.reg_read(UC_X86_REG_EAX) if completed and operation=='handle' else int(completed and operation=='construct')
        return (completed,pack(out),pack(slots),fingerprint,pack(children),read(0x5cab2c),struct.unpack('<Q',cpu.mem_read(0x5cad30,8))[0],';'.join(trace),result,cpu.reg_read(UC_X86_REG_ESP)-STACK,instructions)
def main():
    """현재 정밀PC/원문SHA를 고정하고 최초 자료 또는 읽기 재생만 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon';peSha=hashlib.sha256(PE.read_bytes()).hexdigest();sourceSha=hashlib.sha256(SOURCE.read_bytes()).hexdigest();assert peSha=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236' and sourceSha=='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'
    oracle=Oracle();lines=['\t'.join(INPUTS+OUTPUTS)];differences=[];instructions=0
    for case in cases():  # 입력마다 두 실제 x87 정밀도로 실행한다.
        observations=[]
        for control in (0x27f,0x37f):
            output=oracle.run(case,control);observations.append(output);instructions+=output[-1];row={**case,'control':control};lines.append('\t'.join(map(str,[row[key] for key in INPUTS]+list(output))))
        if observations[0][:-1]!=observations[1][:-1]:differences.append(case['name'])
    content='\n'.join(lines)+'\n'
    if '--verify' in sys.argv:assert FIXTURE.read_text('utf-8')==content
    else:
        FIXTURE.open('x',encoding='utf-8',newline='\n').write(content)
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',peSha256=peSha,sourceSha256=sourceSha,rows=len(lines)-1,cases=len(cases()),instructions=instructions,precisionDifferences=differences,fixtureSha256=hashlib.sha256(content.encode()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gameExecuted=False,limits=['실제6본문/24현재DWORD/20행/8192BYTE/RET와 장치 변경·중단','기본 장식/할당/자식/가상 부착/큐/링크/시각/수명 하위는 명시 장치','전체 geometry4c7d40/HTML factory/메타/Idle/EH unwind 증거 아님','명령 위치 수에는 장치/STOP 포함; 미완료/void EAX0·constructor this1 정규화']),stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(rows=len(lines)-1,instructions=instructions,precisionDifferences=differences,verified='--verify' in sys.argv)),flush=True)
if __name__=='__main__':main()
