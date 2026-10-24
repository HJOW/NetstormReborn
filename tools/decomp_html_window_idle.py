"""정밀 텍스트에서 독립 진입이 누락된 HTML Idle4c8350을 보호 가상 표/PE로 관측한다."""
import copy,hashlib,json,math,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW
from decomp_html_window_gump import ROOT,PE,SOURCE,HEAP,NODE,CHILD,STACK,STOP,OFFSETS,NAMES,INPUTS,OUTPUTS,cases,pack
FIXTURE=ROOT/'cpppj/tests/fixtures/html-window-idle-v2-x86.tsv';REPORT=ROOT/'cpppj/html-window-idle-v2-native-20261024.json'  # 모든 출력 필드를 실제 설치/재조회하는 별도 최초 자료다.
def idle_cases():
    """0/음수/동일/NaN/무한 기한과 현재 재조회·선택 중단 입력만 만든다."""
    rows=[];base=cases()[0]
    def add(timeout,now,mutation='-',stop='-'):
        """실제 두double과 원시 명령을 입력으로 설치한다."""
        row=copy.deepcopy(base);fields=list(map(int,row['fields'].split(':')));lo,hi=struct.unpack('<II',struct.pack('<d',timeout));fields[18:24]=[lo,hi,0xfedcba98,0x87654321,0,0x76543210];row.update(name=f'{len(rows)}-idle',operation='idle',fields=pack(fields),wall=struct.unpack('<Q',struct.pack('<d',now))[0],mutation=mutation,stop=stop);rows.append(row)
    for timeout in (-2.0,-0.0,0.0,5e-324,0.5,1.0,math.nan,math.inf,-math.inf):  # 원문 두 비교의 ordered/unordered를 구별한다.
        for now in (0.25,0.5,2.0,math.nan):add(timeout,now)
    add(1.0,1.0);add(1.0,1.5,'wall.timeHi.1073741824');add(2.0,1.5,'wall.timeHi.1072693248');add(1.0,2.0,'wall.timeout.0');add(1.0,2.0,'wall.timeoutArg.4294967295')
    for stop in ('wall','queue','kill'):add(1.0,2.0,stop=stop)  # 명시 중단은 Native EH 검증과 구별한다.
    return rows
def run(case,control):
    """실제4c8350 비교/분기/현재 재조회를 실행하고 clock/queue/Kill만 장치로 둔다."""
    pe=pefile.PE(str(PE));cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(0x400000,pe.get_memory_mapped_image());cpu.mem_map(0,0x1000);cpu.mem_map(HEAP,0x30000);cpu.mem_map(STACK-0x10000,0x20000);cpu.reg_write(UC_X86_REG_FPCW,control);trace=[];instructions=0;completed=1
    def read(address):
        """현재DWORD다."""
        return struct.unpack('<I',cpu.mem_read(address,4))[0]
    def write(address,value):
        """원시DWORD를 보존한다."""
        cpu.mem_write(address,struct.pack('<I',value&0xffffffff))
    def pointer(token):
        """입력 부모/자식의 실제 주소다."""
        return NODE if token==1 else CHILD+(token-2)*0x200 if token else 0
    def alias(address):
        """실제 관측 주소만 부모/자식 별칭으로 바꾼다."""
        if address==0:return 0
        if address==NODE:return 1
        assert CHILD<=address<CHILD+24*0x200 and (address-CHILD)%0x200==0
        return (address-CHILD)//0x200+2
    class Stop(Exception):
        """선택 장치 중단을 정상 RET와 구별한다."""
    def event(name,args=()):
        """실제 호출 뒤에만 명시 입력의 현재 필드 변경/중단을 수행한다."""
        trace.append(name+(':'+pack(args) if args else ''))
        if case['mutation']!='-':
            phase,field,value=case['mutation'].split('.')
            if phase==name:write(NODE+OFFSETS[NAMES.index(field)],int(value))
        if case['stop']==name:raise Stop()
    def hook(current,address,size,unused):
        """가상 표가 가리키는 실제 Idle와 선택 장치만 허용한다."""
        nonlocal instructions
        instructions+=1;sp=current.reg_read(UC_X86_REG_ESP)
        def returned(eax=0):
            """현재cdecl 반환이며 인자는 호출자가 정리한다."""
            current.reg_write(UC_X86_REG_EAX,eax);current.reg_write(UC_X86_REG_ESP,sp+4);current.reg_write(UC_X86_REG_EIP,read(sp))
        if address==STOP:current.emu_stop();return
        if address==0x460d70:target=read(sp+4);cpu.mem_write(target,struct.pack('<Q',case['wall']));event('wall');returned(target);return
        if address==0x452f90:event('queue',tuple(read(sp+4+4*i) for i in range(3)));returned();return
        if address==0x466480:assert current.reg_read(UC_X86_REG_ECX)==NODE;event('kill',(1,));returned();return
        if not 0x4c8350<=address<0x4c83aa:raise RuntimeError('미선언 Native Idle 경계 '+hex(address))
    assert read(0x515e7c+0x30)==0x4c8350
    cpu.mem_write(NODE,bytes([case['fill']])*0x21e8)
    for offset,value in zip(OFFSETS,map(int,case['fields'].split(':'))):write(NODE+offset,pointer(value) if offset==0x21b8 else value)  # 실제 현재 부모 필드다.
    for index,value in enumerate(map(int,case['slots'].split(':'))):write(NODE+0x20c4+4*index,pointer(value) if index%3==0 else value)  # 실제20행이다.
    for token in range(2,26):  # 관측하는 모든 자식/전역도 실제 독립 메모리에 설치한다.
        write(pointer(token)+0x2c,token+998);write(pointer(token)+0x34,0x12345678);write(pointer(token)+0xac,0x2468)
    write(0x5cab2c,case['instances']);cpu.mem_write(0x5cad30,struct.pack('<d',-7.5))
    write(STACK,STOP);cpu.reg_write(UC_X86_REG_ESP,STACK);cpu.reg_write(UC_X86_REG_ECX,NODE);cpu.hook_add(UC_HOOK_CODE,hook)
    try:cpu.emu_start(0x4c8350,STOP+1,count=1000);assert cpu.reg_read(UC_X86_REG_EIP)==STOP
    except Stop:completed=0
    out=[read(NODE+offset) for offset in OFFSETS];out[12]=alias(out[12]);fingerprint=14695981039346656037
    for value in cpu.mem_read(NODE+0xc4,8192):fingerprint=((fingerprint^value)*1099511628211)&0xffffffffffffffff  # 실제 HTML 전체가 유지되는지도 관측한다.
    children=[read(pointer(token)+offset) for token in range(2,26) for offset in (0x2c,0x34,0xac)];slots=[read(NODE+0x20c4+4*i) for i in range(60)]
    for index in range(0,60,3):slots[index]=alias(slots[index])  # 실제 원시 행에서 읽은 포인터만 변환한다.
    return (completed,pack(out),pack(slots),fingerprint,pack(children),read(0x5cab2c),struct.unpack('<Q',cpu.mem_read(0x5cad30,8))[0],';'.join(trace),0,cpu.reg_read(UC_X86_REG_ESP)-STACK,instructions)
def main():
    """현재 보호 원문을 고정해 최초 관측 또는 읽기 전용 재생을 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon' and hashlib.sha256(PE.read_bytes()).hexdigest()=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236' and hashlib.sha256(SOURCE.read_bytes()).hexdigest()=='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'
    lines=['\t'.join(INPUTS+OUTPUTS)];differences=[];instructions=0
    for case in idle_cases():  # 같은 입력을 실제 두 x87 정밀도로 실행한다.
        observations=[]
        for control in (0x27f,0x37f):
            output=run(case,control);observations.append(output);instructions+=output[-1];row={**case,'control':control};lines.append('\t'.join(map(str,[row[key] for key in INPUTS]+list(output))))
        if observations[0][:-1]!=observations[1][:-1]:differences.append(case['name'])
    content='\n'.join(lines)+'\n'
    if '--verify' in sys.argv:assert FIXTURE.read_text('utf-8')==content
    else:
        FIXTURE.open('x',encoding='utf-8',newline='\n').write(content)
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',rows=len(lines)-1,instructions=instructions,precisionDifferences=differences,fixtureSha256=hashlib.sha256(content.encode()).hexdigest(),toolSha256=hashlib.sha256(__import__('pathlib').Path(__file__).read_bytes()).hexdigest(),gameExecuted=False,limits=['515e7c+30에서4c8350을 확인하고 실제 비교/현재 기한·명령/분기/RET 실행','clock/queue/Kill 하위만 명시 장치; 전체 Idle 프레임 등록/HTML meta/EH 증거 아님','명령 위치에는 장치/STOP 포함; void/중단 EAX0 정규화']),stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(rows=len(lines)-1,instructions=instructions,precisionDifferences=differences,verified='--verify' in sys.argv)),flush=True)
if __name__=='__main__':main()
