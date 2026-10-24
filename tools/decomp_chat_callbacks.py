"""보호 PE의 Chat clear/quit/파일 append callback을 독립 CPU 실행으로 검증한다."""
from pathlib import Path
import hashlib,itertools,json,socket,struct,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX

ROOT=Path(__file__).resolve().parents[1]  # 보호 자료와 새 출력의 기준 경로다.
PE=ROOT/'originals/Netstorm.exe';SOURCE=ROOT/'extracted/refined/originals/Netstorm.c'  # 읽기 전용 원문이다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236';SOURCE_SHA='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'  # 마지막 HJOW-Athlon 정밀 자료다.
FIXTURE=ROOT/'cpppj/tests/fixtures/chat-callbacks-x86.tsv';REPORT=ROOT/'cpppj/chat-callbacks-native-20261024.json'  # 최초 관측은 덮어쓰지 않는다.
HEAP=0x02000000;STACK=0x03000000;STOP=0x01f00000;CURRENT=0x548740;FILE=0x54874c;EVENT=0x542944  # 독립 CPU 메모리와 실제 원문 전역 주소다.
ROOT_A=HEAP+0x1000;ROOT_B=HEAP+0x2000;TEXT_A=HEAP+0x3000;TEXT_B=HEAP+0x4000;FILE_A=HEAP+0x5000;FILE_B=HEAP+0x6000;STRING=HEAP+0x7000  # 정규화 별칭1/2의 유효 물리다.
SCHEMA=('operation','current','file','valid','mutation','text','event','trace','finalCurrent','finalFile','result','instructions')  # 입력/실제 관측을 분리한다.
ENTRIES={'clear':0x42f680,'log':0x42f590,'quit':0x42e520}  # 실제 함수 진입 주소다.

def sha(path):
    """파일의 현재 원시 바이트 전체를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def cases():
    """원문 콜백 도중 바뀌는 전역/자식도 독립 입력으로 제공한다."""
    rows=[]
    for current,mutation in itertools.product((0,1),('none','currentZero','currentNext','textNext','textNextCurrentZero')):  # clear 첫 포인터와 두 번째 자식 조회를 구별한다.
        rows.append(dict(operation='clear',current=current,file=0,valid=0,mutation=mutation,text='',event=0x2000094))
    for file,valid,mutation in itertools.product((0,1),(0,1),('none','fileZero','fileNext')):  # 첫 file 관문/유효성/두 번째 현재 file 조회다.
        rows.append(dict(operation='log',current=0,file=file,valid=valid,mutation=mutation,text=b'A\n'.hex(),event=0x2000094))
    for text in (b'',b'X\0tail',b'ascii'):  # NUL 앞 길이와0BYTE 쓰기 호출도 원문 그대로 관측한다.
        rows.append(dict(operation='log',current=0,file=1,valid=1,mutation='none',text=text.hex(),event=0x2000094))
    for event in (0,0x2000094,0x81234567,0xffffffff):  # quit은 현재 전체DWORD 사건을 매번 읽는다.
        rows.append(dict(operation='quit',current=0,file=0,valid=0,mutation='none',text='',event=event))
    return rows

class Oracle:
    """세 작은 원문 본체만 실행하고 텍스트/파일/예약 장치를 호출 인자 관측기로 제공한다."""
    def __init__(self):
        """보호 PE를 읽어 독립 CPU가 사용할 원시 이미지로만 준비한다."""
        self.pe=pefile.PE(str(PE));self.image=self.pe.get_memory_mapped_image();self.base=self.pe.OPTIONAL_HEADER.ImageBase

    def run(self,case):
        """현재 입력 하나를 새 CPU/스택에서 정상 RET까지 실행한다."""
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(self.base,(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);cpu.mem_write(self.base,self.image)
        cpu.mem_map(HEAP,0x10000);cpu.mem_map(STACK,0x10000);cpu.mem_map(STOP,0x1000)
        trace=[];instructions=0  # 실제 실행/장치 관측만 누적한다.
        def read(address):
            """같은 CPU의 정렬되지 않은 네BYTE를 읽는다."""
            return struct.unpack('<I',cpu.mem_read(address,4))[0]
        def write(address,value):
            """원문 전역/스택의 실제 DWORD를 쓴다."""
            cpu.mem_write(address,struct.pack('<I',value))
        def alias(value,first,second):
            """유효 독립 물리 주소만0/1/2 별칭으로 표현한다."""
            if value==0:return 0
            if value==first:return 1
            if value==second:return 2
            raise RuntimeError('예상 밖 실제 포인터 '+hex(value))
        def returned(pop=0,eax=None):
            """명시 장치 함수의 실제 호출 규약으로 현재 호출자에 복귀한다."""
            esp=cpu.reg_read(UC_X86_REG_ESP);pc=read(esp);cpu.reg_write(UC_X86_REG_ESP,esp+4+pop);cpu.reg_write(UC_X86_REG_EIP,pc)
            if eax is not None:cpu.reg_write(UC_X86_REG_EAX,eax)
        def hook(cpu,address,size,user):
            """원문 분기는 실행하고 다섯 하위 장치만 현재 인자/명시 돌연변이로 관측한다."""
            nonlocal instructions
            instructions+=1
            if address==STOP:cpu.emu_stop();return
            esp=cpu.reg_read(UC_X86_REG_ESP);this=cpu.reg_read(UC_X86_REG_ECX)
            if address==0x4bc250:
                assert read(esp+8)==0 and cpu.mem_read(read(esp+4),1)==b'\0'
                trace.append('replace:'+str(alias(this,TEXT_A,TEXT_B)))
                if case['mutation'] in ('textNext','textNextCurrentZero'):write(ROOT_A+0xa4,TEXT_B)
                if case['mutation'] in ('currentZero','textNextCurrentZero'):write(CURRENT,0)
                if case['mutation']=='currentNext':write(CURRENT,ROOT_B)
                returned(8);return
            if address==0x4ba3b0:trace.append('bottom:'+str(alias(this,TEXT_A,TEXT_B)));returned();return
            if address==0x41aad0:
                trace.append('valid:'+str(alias(this,FILE_A,FILE_B)))
                if case['mutation']=='fileZero':write(FILE,0)
                if case['mutation']=='fileNext':write(FILE,FILE_B)
                returned(eax=case['valid']);return
            if address==0x41a910:
                assert read(esp+12)==0xffffffff
                trace.append('write:'+str(alias(this,FILE_A,FILE_B))+':'+bytes(cpu.mem_read(read(esp+4),read(esp+8))).hex());returned(12,eax=read(esp+8));return
            if address==0x452fe0:trace.append('queue:'+':'.join(str(read(esp+offset)) for offset in (4,8,12)));returned();return
            if not any(low<=address<high for low,high in ((0x42f680,0x42f6ad),(0x42f590,0x42f5ca),(0x42e520,0x42e535))):raise RuntimeError('미선언 원문 진입 '+hex(address))
        write(CURRENT,ROOT_A if case['current'] else 0);write(FILE,FILE_A if case['file'] else 0);write(EVENT,case['event']);write(ROOT_A+0xa4,TEXT_A);write(ROOT_B+0xa4,TEXT_B);cpu.mem_write(STRING,bytes.fromhex(case['text'])+b'\0')
        esp=STACK+0x8000;write(esp,STOP);write(esp+4,STRING);cpu.reg_write(UC_X86_REG_ESP,esp);cpu.hook_add(UC_HOOK_CODE,hook);cpu.emu_start(ENTRIES[case['operation']],STOP+1,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==STOP
        return dict(trace=';'.join(trace),finalCurrent=alias(read(CURRENT),ROOT_A,ROOT_B),finalFile=alias(read(FILE),FILE_A,FILE_B),result=0 if case['operation']=='log' else cpu.reg_read(UC_X86_REG_EAX),instructions=instructions)

def main():
    """마지막 정밀 PC/원문을 고정하고 최초 작성 또는 기존 자료의 읽기 전용 재생을 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon' and sha(PE)==PE_SHA and sha(SOURCE)==SOURCE_SHA
    oracle=Oracle();lines=['\t'.join(SCHEMA)];total=0
    for case in cases():  # 기대 자료를 읽지 않고 원문 CPU 결과를 먼저 얻는다.
        output=oracle.run(case);total+=output['instructions'];row={**case,**output};lines.append('\t'.join(str(row[key]) for key in SCHEMA))
    content='\n'.join(lines)+'\n'
    if len(sys.argv)==2 and sys.argv[1]=='--verify':assert FIXTURE.read_text('utf-8')==content
    else:
        assert len(sys.argv)==1
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write(content)
        result=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',peSha256=PE_SHA,sourceSha256=SOURCE_SHA,rows=len(lines)-1,instructions=total,instructionCountIncludesDeviceHooks=True,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),gameExecuted=False,limits=['세 callback의 원문 분기/두 번째 현재 포인터 읽기/strlen/RET를 실제 실행','하위 ReplaceText/RequestBottom/파일 valid·write/사건 예약은 명시 입력/인자 관측 장치','파일/텍스트 하위 본문과 게임/GUI/전송 완주의 증거가 아님','log void EAX는 제품 반환값이 아니므로0정규화'])
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(result,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(rows=len(lines)-1,instructions=total,verified=len(sys.argv)==2,fixtureSha256=sha(FIXTURE))))

if __name__=='__main__':main()  # 모듈 import는 보호 파일/기대 자료를 쓰지 않는다.
