"""장식462420 전체의 원본 signed 곱/나눗셈·현재 가상 효과를 격리 PE에서 관찰한다."""
from pathlib import Path
import argparse,itertools,json,struct,sys,hashlib
ROOT=Path(__file__).resolve().parents[1]  #원본 및 이미 확정한 fixture는 읽기만 한다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW
POOL,STACK,STOP,VT=0x2000000,0x3000000,0xf00000,0x11010000  #실제 게임/OS와 분리한 raw/가상 표 입력 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/decoration-regular-x86.tsv'  #다른 원문 기대값과 구별한 신규 자료다.
def cases():
    """분기·signed 경계·DWORD 곱 감김과 큰 하위 후 현재 raw 재조회를 교차한다."""
    rows=[(0,c,0x3e800000,b,m,h,0) for c,m,h,b in itertools.product([0,1,2,3,0xffffffff,0x80000000,0x7fffffff],[0,1,2,3,100,0x7fffffff,0x80000000,0xffffffff],[0,1,100,0x7fffffff,0x80000000],[0,1])]
    rows += [(e,c,p,0,100,100,0) for e,c,p in itertools.product([1,0xffffffff],[0,3,0xffffffff,0x80000000],[0x80000000,0x7fc01234])]
    rows += [(0,c,0x3e800000,b,m,h,1) for c,b,m,h in itertools.product([0,1,2],[0,1],[0,100,0x7fffffff],[1,100,0x80000000])]
    return list(dict.fromkeys(rows))
def sample(pe,row):
    """127바이트 부모 전체를 실행하며 최대HP/가상98·삭제/공통Handle만 명시 경계로 둔다."""
    image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase;mu=Uc(UC_ARCH_X86,UC_MODE_32);mu.mem_map(base,(len(image)+4095)&~4095);mu.mem_write(base,image)
    for address,size in [(POOL,0x10000),(STACK,0x10000),(STOP,0x1000),(VT,0x20000)]:  #서로 겹치지 않는 실제 입력 범위를 확보한다.
        mu.mem_map(address,size)
    parent=POOL+50*50;event,count,payload,boss,maximum,health,mutation=row;trace=[];instructions=0
    def put(at,value,width=4):
        """원문 폭의 낮은 비트를 little endian으로 쓴다."""
        mu.mem_write(at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
    def get(at):
        """현재 DWORD를 unsigned 비트로 읽는다."""
        return int.from_bytes(mu.mem_read(at,4),'little')
    def ret(value=None,pop=0):
        """현재 하위의 호출 규약에 맞춰 원래 caller로 복귀한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);address=get(esp);mu.reg_write(UC_X86_REG_ESP,esp+4+pop);mu.reg_write(UC_X86_REG_EIP,address)
        if value is not None:mu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
    def hook(emu,at,size,unused):
        """기대 분기는 계산하지 않고 원문이 선택한 실제 효과 인자만 기록한다."""
        nonlocal instructions
        instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);args=[get(esp+4+4*i) for i in range(3)]
        if at==STOP:emu.emu_stop();return
        if at==0x4adcb0:
            trace.append('M:50')
            if mutation:put(parent,VT+0x10000);put(parent+26,77)
            ret(maximum);return
        if at in [STOP+0x100,STOP+0x120,STOP+0x140,STOP+0x160]:
            changed=at in [STOP+0x140,STOP+0x160];hit=at in [STOP+0x120,STOP+0x160]
            trace.append(('h' if changed else 'H')+':'+':'.join(map(str,args)) if hit else ('d' if changed else 'D')+':'+str(args[0]));ret(1,12 if hit else 4);return
        if at==0x44b900:
            trace.append('B:'+':'.join(map(str,args)));mu.reg_write(UC_X86_REG_EIP,STOP+0x200);return
        if not (0x462420<=at<=0x46249e or STOP+0x200<=at<STOP+0x209 or STOP+0x300<=at<STOP+0x330):raise RuntimeError(f'원문 범위 밖 {at:08x}')
    mu.mem_write(parent,b'\xa5'*50);put(parent,VT);put(parent+10,100,1);put(parent+26,health);put(0x540bc4,boss)
    for table,off in [(VT,0),(VT+0x10000,0x40)]:  #현재 가상 표 교체도 별도 실제 호출 주소로 관찰한다.
        put(table+0x10,STOP+0x100+off);put(table+0x98,STOP+0x120+off)
    put(STOP+0x250,0x3ec00000);mu.mem_write(STOP+0x200,b'\xd9\x05'+struct.pack('<I',STOP+0x250)+b'\xc2\x0c\x00')
    caller=STOP+0x300;code=b'\xb9'+struct.pack('<I',parent)
    for arg in [payload,count,event]:code+=b'\x68'+struct.pack('<I',arg)  #실제 caller는 세 원문 DWORD를 역순으로 전달한다.
    code+=b'\xe8'+struct.pack('<i',0x462420-(caller+len(code)+5));code+=b'\xd9\x1d'+struct.pack('<I',STOP+0x260);code+=b'\xe9'+struct.pack('<i',STOP-(caller+len(code)+5));mu.mem_write(caller,code)
    mu.reg_write(UC_X86_REG_ESP,STACK+0x8000);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0);mu.hook_add(UC_HOOK_CODE,hook);mu.emu_start(caller,STOP+1,count=2000)
    assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+0x8000
    return str(get(STOP+0x260)),';'.join(trace) or '-',bytes(mu.mem_read(parent,50)).hex(),instructions
def main():
    """신규 기대값을 저장하거나 기존 파일을 변경하지 않고 실제 PE를 다시 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));rows=[];total=0
    for row in cases():  #제품 코드/기대 분기 계산 없이 원본의 현재 결과를 독립 관측한다.
        result=sample(pe,row);rows.append((','.join(map(str,row)),*result[:3]));total+=result[3]
    text='# input return events raw50\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:assert FIXTURE.read_text(encoding='utf-8')==text
    else:
        assert not FIXTURE.exists();FIXTURE.write_text(text,encoding='utf-8',newline='\n')
    report=dict(cases=len(rows),normalReturns=len(rows),instructions=total,passed=True,originalExecutions=0,entry='00462420',boundaries='최대HP4adcb0/현재가상98·삭제/공통44b900은 명시 경계. 부모127BYTE/최종float32는 실제 PE 전체 실행.',sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in ['originals/Netstorm.exe','tools/decomp_decoration_regular.py','cpppj/tests/fixtures/decoration-regular-x86.tsv']})
    out=ROOT/'extracted/cpp-resume-1235/oracle.json';out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(dict(cases=len(rows),instructions=total,passed=True)))
if __name__=='__main__':main()
