"""보호4c83b0 전체와 실제 캡처/시작점·버튼·제거·부분복사를 독립 관측한다."""
from pathlib import Path
import hashlib,json,re,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_GDTR,UC_X86_REG_CS,UC_X86_REG_SS,UC_X86_REG_DS,UC_X86_REG_ES
ROOT=Path(__file__).resolve().parents[1];PE=ROOT/'originals/Netstorm.exe';SOURCE=ROOT/'extracted/refined/originals/Netstorm.c'  # 보호 입력을 읽기만 한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/html-window-metadata-x86.tsv';REPORT=ROOT/'cpppj/html-window-metadata-native-20261024.json'  # 최초 관측은 덮어쓰지 않는다.
HEAP=0x2000000;NODE=HEAP+0x1000;BUFFER=HEAP+0x5000;CHILD=HEAP+0x10000;TABLE=HEAP+0x20000;ATTACH=HEAP+0x21000;RECORDS=HEAP+0x30000;STRINGS=HEAP+0x40000;STOP=HEAP+0xf0000;STACK=0x3000000  # 실제 독립 부모/8192BYTE/자식/캡처/문자열 메모리다.
OFFSETS=(4,0x21b4,0x21c8,0x21d0,0x21d4,0x21d8,0x21dc,0x21e0,0x21e4);NAMES=('flags','count','columns','timeLo','timeHi','timeout','timeoutArg','exit','exitArg')  # 현재 메타 필드다.
INPUTS=('name','control','text','fill','columns','force','developer','wall','replacement','lookup','mutation','stop');OUTPUTS=('completed','fields','slots','bufferFnv','textOut','stores','trace','result','stackDelta','instructions')  # 입력과 원문 관측을 구별한다.
def pack(values):
    """원시 관측을 십진수 목록으로 보존한다."""
    return ':'.join(map(str,values))
def cases():
    """기대 메타 결과 없이 실제 바이트/하위 장치 입력을 교차한다."""
    rows=[]
    def add(text,**values):
        """같은8192BYTE 버퍼와 명시 외부 값만 만든다."""
        if isinstance(text,str):text=text.encode('ascii')
        rows.append(dict(name=str(len(rows)),text=text.hex(),fill=values.pop('fill',0),columns=values.pop('columns',0),force=values.pop('force',0),developer=values.pop('developer',0),wall=values.pop('wall',123.25),replacement=values.pop('replacement',b'ZZ').hex(),lookup=values.pop('lookup',0x02000095),mutation=values.pop('mutation','-'),stop=values.pop('stop','-')));assert not values
    for text in ('Plain body\n','$Button=Okay,Cmd,1\nTAIL','$ButtonInstant=Go,Cmd,Word\nTAIL','$Menu=Entry,Cmd,-1\nTAIL','$Checked=Mark,Cmd,2,1,1\nTAIL','$Timeout=3,Cmd,9\nTAIL','$OnExit=Unused,Cmd,Word\nTAIL','$Key=Unused,Cmd,+2\nTAIL','$Input=Prompt,20,Default\nTAIL','$Zsend=Topic,Cmd,Message\nTAIL'):add(text)
    for argument in ('0','123tail','4294967296','2147483648','999999999999999999999','-1','+2','"Word"','Word\nTail','A'*255):add('$Button=Go,Cmd,'+argument+' $OnExit=X,Cmd,9')
    for command in ('Cmd',' Cmd ','Missing'):
        for code in (0,0xffffffff,0x02000095):add('$OnExit=X,'+command+',Word\n',lookup=code)
    for force,developer in ((0,0),(1,0),(0,1)):
        for checked in ('0','-1','2'):add('$Checked=Mark,Cmd,Word,'+checked+',0\n',force=force,developer=developer)
    for label in ('*SOLOLIST*','*FORTLIST*','*TECHLIST*','*THEMETECHLIST*','@Names','#Files','%Tiny'):
        add('$Menu='+label+',Cmd,4294901761\nTAIL')
    add('P$Menu=%Tiny,Cmd,1\nTAIL');add('P$Menu=%Tiny,Cmd,1\nTAIL',replacement=b'');add('P$Menu=%Tiny,Cmd,1\nTAIL',replacement=b'ABCDEFGHIJKLMNO')
    add('$OnExit=X,Cmd,Word $Timeout=4,Cmd,8 $Menu=Entry,Cmd,9 $ButtonInstant=Go,Cmd,7 $Button=Okay,Cmd,6\nTAIL')
    add(''.join(f'$Button=B{i},Cmd,{i}\n' for i in range(24))+'TAIL');add('$Button=First,Cmd,1\n$Button=Second,Cmd,2\nTAIL')
    add('No metadata',columns=2);add('$Menu=Entry,Cmd,1\n',mutation='begin.columns.3');add('$Input=Prompt,20,Default\n',mutation='input.closed.4275878552');add('$Timeout=-2,Cmd,7\n');add('$Timeout=0,Cmd,8\n',wall=0.0)
    for text,phase in (('$Button=Go,Cmd,1\n','kill'),('$Button=Go,Cmd,1\n','lookup'),('$Button=Go,Cmd,Word\n','store'),('$Menu=Entry,Cmd,1\n','begin'),('$Menu=%Tiny,Cmd,1\n','inline'),('$Timeout=3,Cmd,1\n','clock'),('$Input=Prompt,20,Default\n','input'),('$Zsend=Topic,Cmd,Message\n','send')):add(text,stop=phase)  # 명시 중단은 실제 EH unwind와 구별한다.
    return rows
class ExplicitStop(Exception):
    """선택 하위 장치 중단을 정상 반환과 구별한다."""
class Oracle:
    """원문 메타/캡처/시작점/행 제거/버튼·부분복사와 명시 하위 장치를 사용한다."""
    def __init__(self):
        """읽기 전용 보호 PE를 준비한다."""
        self.pe=pefile.PE(str(PE));self.image=self.pe.get_memory_mapped_image()
    def run(self,case,control):
        """모든 실제 출력 물리와 현재 전역을 새 CPU에 설치한다."""
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x400000,(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(0x400000,self.image);cpu.mem_map(0,0x1000);cpu.mem_map(HEAP,0x100000);cpu.mem_map(STACK-0x20000,0x40000)
        cpu.mem_write(HEAP,struct.pack('<QQQ',0,0x00cf9a000000ffff,0x00cf92000000ffff));cpu.reg_write(UC_X86_REG_GDTR,(0,HEAP,23,0));cpu.reg_write(UC_X86_REG_CS,8)
        for register in (UC_X86_REG_SS,UC_X86_REG_DS,UC_X86_REG_ES):cpu.reg_write(register,16)  # 실제 REP MOVS의32비트 평면 장치다.
        cpu.reg_write(UC_X86_REG_FPCW,control);regex={};strings={};trace=[];stores=[];instructions=0;next_string=0;next_child=4
        def read(address):
            """현재DWORD다."""
            return struct.unpack('<I',cpu.mem_read(address,4))[0]
        def write(address,value):
            """원시DWORD를 보존한다."""
            cpu.mem_write(address,struct.pack('<I',value&0xffffffff))
        def cstring(address):
            """설치한 실제C문자열의 첫NUL까지만 읽는다."""
            result=bytearray()
            while (value:=cpu.mem_read(address,1)[0])!=0:result.append(value);address+=1  # 관측 바이트만 읽는다.
            return bytes(result)
        def pointer(token):
            """현재 부모/자식 주소다."""
            return NODE if token==1 else CHILD+(token-2)*0x200 if token else 0
        def alias(address):
            """실제 포인터만 선언된 별칭으로 바꾼다."""
            if not address:return 0
            if address==NODE:return 1
            assert CHILD<=address<CHILD+64*0x200 and (address-CHILD)%0x200==0
            return (address-CHILD)//0x200+2
        def event(name,*values):
            """실제 장치 인자 뒤에만 명시 변경/중단을 적용한다."""
            trace.append(name+(':'+':'.join(map(str,values)) if values else ''))
            if case['mutation']!='-':
                phase,field,value=case['mutation'].split('.')
                if phase==name:write(0x5429ec if field=='closed' else NODE+OFFSETS[NAMES.index(field)],int(value))
            if case['stop']==name:raise ExplicitStop()
        def make_string(this,data):
            """하위 CString 장치의 실제 바이트를 독립 소유한다."""
            nonlocal next_string
            data=data.split(b'\0',1)[0];address=STRINGS+next_string*0x2000;next_string+=1;assert address+len(data)+1<STOP;cpu.mem_write(address,data+b'\0');write(this,address);strings[this]=data
        def atol(data):
            """명시 CRT 경계는 기존 별도 Native 검증의32비트C 계약이다."""
            match=re.match(rb'[\t\n\v\f\r ]*([+-]?\d+)',data);return int(match[1])&0xffffffff if match else 0
        def hook(current,address,size,unused):
            """실제 상위/캡처/시작점/버튼 밖의 regex검색·문자열·UI 장치만 처리한다."""
            nonlocal instructions,next_child
            instructions+=1;sp=current.reg_read(UC_X86_REG_ESP);this=current.reg_read(UC_X86_REG_ECX)
            def arg(index):
                """현재 스택 인자다."""
                return read(sp+4+4*index)
            def returned(pop=0,eax=0):
                """실제cdecl/thiscall 반환을 수행한다."""
                current.reg_write(UC_X86_REG_EAX,eax&0xffffffff);current.reg_write(UC_X86_REG_ESP,sp+4+pop);current.reg_write(UC_X86_REG_EIP,read(sp))
            if address==STOP:current.emu_stop();return
            if address==0x496c90:
                pattern=cstring(arg(0));index=len(regex);regex[this]=(re.compile(pattern),RECORDS+index*0x100);write(this,RECORDS+index*0x100);write(this+4,0);write(this+8,0);returned(4,this);return
            if address==0x4964e0:
                pattern,records=regex[this];source=arg(0);match=pattern.search(cstring(source));cpu.mem_write(records,bytes(0x50));write(this+4,int(bool(match)));write(this+8,source)
                if match:
                    for group in range(len(match.groups())+1):write(records+4*group,source+match.start(group));write(records+0x28+4*group,source+match.end(group))  # 실제 캡처 함수에 시작/끝 물리만 공급한다.
                returned(4,int(bool(match)));return
            if address==0x495a00:returned();return
            if address==0x4d52c0:event('lookup',cstring(arg(0)).hex());returned(eax=case['lookup']);return
            if address==0x4c2630:event('diagnostic',cstring(arg(2)).hex());returned();return
            if address==0x4e6fa1:returned(eax=int(48<=arg(0)<=57));return
            if address==0x4e66de:returned(eax=atol(cstring(arg(0))));return
            if address==0x4e65ed:left,right=cstring(arg(0)).lower(),cstring(arg(1)).lower();returned(eax=int(left>right)-int(left<right));return
            if address==0x4c7910:
                data=cstring(arg(0));stores.append(data.hex());token=0x60000000+len(stores);event('store',data.hex());returned(eax=token);return
            if address==0x466480:event('kill',alias(this));returned();return
            if address==0x4e4391:
                assert arg(0)==0x170;token=next_child;next_child+=1;write(pointer(token),TABLE);returned(eax=pointer(token));return
            if address==0x4254b0:assert arg(0)==arg(1)==0;write(this+0xac,0);event('button',alias(this),cstring(arg(2)).hex());returned(20,this);return
            if address==ATTACH:event('attach',alias(this),arg(0),arg(1),alias(arg(2)),arg(3));returned(16);return
            if address==0x477bb0:event('begin',arg(0));returned();return
            if address==0x477cb0:event('text',cstring(arg(0)).hex(),*[arg(i) for i in range(1,6)]);returned();return
            if address==0x477dd0:event('checked',arg(0),cstring(arg(1)).hex(),*[arg(i) for i in range(2,6)]);returned();return
            if address==0x4cafd0:event('mapRefresh',arg(0));returned();return
            if address==0x4cb210:event('mapRow',read(arg(0)));returned(eax=0);return
            if address==0x4cb690:event('tech',arg(0));returned();return
            if address==0x4cb4b0:event('theme',arg(0),arg(1));returned();return
            if address in (0x4ce1b0,0x4ce580):event('named' if address==0x4ce1b0 else 'files',cstring(arg(0)).hex(),arg(1),cstring(arg(2)).hex());returned();return
            if address==0x4cdd40:make_string(arg(0),bytes.fromhex(case['replacement']));event('inline',cstring(arg(1)).hex(),cstring(arg(2)).hex(),cstring(arg(3)).hex());returned(eax=arg(0));return
            if address==0x460d70:cpu.mem_write(arg(0),struct.pack('<d',case['wall']));event('clock');returned(eax=arg(0));return
            if address==0x465330:write(this+4,(read(this+4)|0x80) if arg(0) else (read(this+4)&~0x80));event('idle',alias(this),arg(0));returned(4);return
            if address==0x4d3ed0:event('input',arg(0),cstring(arg(1)).hex(),cstring(arg(2)).hex());returned();return
            if address==0x452f90:event('queue',*[arg(i) for i in range(3)]);returned();return
            if address==0x40a150:cpu.mem_write(this,bytes(0x122));returned(eax=this);return
            if address==0x4e2dc0:assert arg(0)==11;packet=arg(1);event('send',cstring(packet+0x22).hex(),cstring(packet+0x72).hex(),cstring(packet+0xc2).hex());returned();return
            if address==0x4e3090:make_string(this,b'');returned(eax=this);return
            if address==0x4e3180:make_string(this,cstring(arg(0)));returned(4,this);return
            if address==0x4e3110:make_string(this,cstring(read(arg(0))));returned(4,this);return
            if address==0x46ee50:returned(eax=read(this));return
            if address==0x4e33e0:old=cstring(read(this));pos=arg(0);make_string(this,old[:pos]+cstring(arg(1))+old[pos:]);returned(8,this);return
            if address==0x4e31a0:returned();return
            if address==0x4e5b40:destination,source,count=arg(0),arg(1),arg(2);cpu.mem_write(destination,bytes(cpu.mem_read(source,count)));returned(eax=destination);return
            if not any(low<=address<high for low,high in ((0x4c83b0,0x4c8d50),(0x4c7c10,0x4c7d40),(0x495a10,0x495a80),(0x495ab0,0x495ae0),(0x4e30a0,0x4e30b0))):raise RuntimeError('미선언 Native 메타 경계 '+hex(address))
        cpu.mem_write(NODE,bytes(0x21e8));write(NODE+4,9);write(NODE+0x21b4,2);write(NODE+0x21c8,case['columns']);write(NODE+0x21dc,0xa5a5a5a5);write(NODE+0x21e4,0x5a5a5a5a)
        for index in range(2):write(NODE+0x20c4+12*index,pointer(index+2));write(NODE+0x20c8+12*index,7);write(NODE+0x20cc+12*index,8)  # ClearButtons가 실제 반납할 입력이다.
        data=bytes.fromhex(case['text']);assert len(data)<8192;cpu.mem_write(BUFFER,bytes([case['fill']])*8192);cpu.mem_write(BUFFER,data+b'\0');write(TABLE+0x20,ATTACH);write(0x5cab30,case['force']);write(0x55a480,case['developer']);write(0x5429ec,0x020000be);write(0x5cad20,0)
        write(STACK,STOP);write(STACK+4,BUFFER);cpu.reg_write(UC_X86_REG_ESP,STACK);cpu.reg_write(UC_X86_REG_ECX,NODE);cpu.hook_add(UC_HOOK_CODE,hook);completed=1
        try:cpu.emu_start(0x4c83b0,STOP+1,count=2000000);assert cpu.reg_read(UC_X86_REG_EIP)==STOP
        except ExplicitStop:completed=0
        fields=[read(NODE+offset) for offset in OFFSETS];slots=[read(NODE+0x20c4+4*i) for i in range(60)]
        for index in range(0,60,3):slots[index]=alias(slots[index])  # 실제 현재 행의 포인터만 관측 별칭으로 바꾼다.
        fingerprint=14695981039346656037
        for byte in cpu.mem_read(BUFFER,8192):fingerprint=((fingerprint^byte)*1099511628211)&0xffffffffffffffff  # NUL 뒤 버퍼 효과도 실제로 읽는다.
        return (completed,pack(fields),pack(slots),fingerprint,cstring(BUFFER).hex(),';'.join(stores),';'.join(trace),cpu.reg_read(UC_X86_REG_EAX) if completed else 0,cpu.reg_read(UC_X86_REG_ESP)-STACK,instructions)
def main():
    """현재 PC/보호 SHA를 고정해 최초 관측 또는 읽기 전용 재생을 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon';peSha=hashlib.sha256(PE.read_bytes()).hexdigest();sourceSha=hashlib.sha256(SOURCE.read_bytes()).hexdigest();assert peSha=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236' and sourceSha=='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'
    oracle=Oracle();lines=['\t'.join(INPUTS+OUTPUTS)];different=[];instructions=0
    for case in cases():  # 같은 명시 입력을 두 x87 정밀도로 실행한다.
        observations=[]
        for control in (0x27f,0x37f):
            output=oracle.run(case,control);observations.append(output);instructions+=output[-1];row={**case,'control':control};lines.append('\t'.join(map(str,[row[key] for key in INPUTS]+list(output))))
        if observations[0][:-1]!=observations[1][:-1]:different.append(case['name'])
    content='\n'.join(lines)+'\n'
    if '--verify' in sys.argv:assert FIXTURE.read_text('utf-8')==content
    else:
        FIXTURE.open('x',encoding='utf-8',newline='\n').write(content)
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',peSha256=peSha,sourceSha256=sourceSha,rows=len(lines)-1,instructions=instructions,precisionDifferences=different,fixtureSha256=hashlib.sha256(content.encode()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gameExecuted=False,limits=['실제4c83b0 전체·ClearButtons/AddButton·캡처95a10/시작95ab0·String길이/줄제거·부분REP복사·RET','regex compile/search는 명시 Pythonre 하위이며 실제 regexp엔진 전체를 실행했다는 뜻 아님','현재명령/CRT/80칸고리/문자열/Ctrl·메뉴/맵·기술/시각/Idle/Input/Packet/메모리이동은 명시 장치','8192BYTE 전체를 실제 설치/재조회해 NUL 뒤 변경 포함; 정상 완료와 선택 중단 구별','명령 위치에는 장치/STOP 포함; 전체 EH/네트워크/게임GUI 증거 아님']),stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(rows=len(lines)-1,instructions=instructions,precisionDifferences=different,verified='--verify' in sys.argv)),flush=True)
if __name__=='__main__':main()
