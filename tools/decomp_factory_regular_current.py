"""Factory4549c0 분배·현재 레벨/TYPE·실제 게임 난수를 격리 PE에서 대조한다."""
from pathlib import Path
import argparse,itertools,json,struct,sys,hashlib
ROOT=Path(__file__).resolve().parents[1]  #보호 원본과 기존 fixture는 수정하지 않는다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW
POOL,STACK,STOP,VT,TYPES=0x2000000,0x3000000,0xf00000,0x11010000,0x2100000  #실제 OS/프로세스와 분리한 현재 입력 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/factory-regular-current-x86.tsv'  #새 부모 분배 관찰이며 기존 기대값과 구별한다.
def cases():
    """순수 분배·현재 두 조회·상태0 seed·0범위 예외 선행 쓰기를 교차한다."""
    rows=[(2,0,0x3e800000,l,f,n,s,m) for l,f,n,s,m in itertools.product([0,1,2,7],[0,5,0x7fffffff,0xffffffff],[0,1,4,0xffffffff],[0,1,0x12345678,0xffffffff],[0,1])]
    rows += [(e,c,p,l,0,4,1,0) for e,c,p,l in itertools.product([0,1,3,4,5,0xffffffff],[0,1,0xffffffff],[0x3e800000,0x7fc01234],[0,2,3,7])]
    return list(dict.fromkeys(rows))
def sample(pe,row):
    """분배·가상80·4558c0은 실제 명령이다. 큰 Begin/Finish는 기존 전체 복원 모듈의 명시 경계다."""
    image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase;mu=Uc(UC_ARCH_X86,UC_MODE_32);mu.mem_map(base,(len(image)+4095)&~4095);mu.mem_write(base,image)
    for address,size in [(POOL,0x10000),(STACK,0x10000),(STOP,0x1000),(VT,0x1000),(TYPES,0x20000)]:  #겹치지 않는 raw/TYPE/가상 효과만 확보한다.
        mu.mem_map(address,size)
    parent=POOL+50*50;event,count,payload,level,first,size,seed,mutation=row;trace=[];type_reads=0;instructions=0
    def put(at,value,width=4):
        """원문 폭의 낮은 비트만 쓴다."""
        mu.mem_write(at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
    def get(at,width=4):
        """현재 little endian 필드를 읽는다."""
        return int.from_bytes(mu.mem_read(at,width),'little')
    def ret(value=None,pop=0):
        """각 경계의 원래 스택 규약으로 복귀한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);mu.reg_write(UC_X86_REG_EIP,get(esp));mu.reg_write(UC_X86_REG_ESP,esp+4+pop)
        if value is not None:mu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
    def hook(emu,at,length,unused):
        """조건/난수 결과를 대신 계산하지 않고 현재 원문이 고른 장치 인자만 기록한다."""
        nonlocal type_reads,instructions
        instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);args=[get(esp+4+4*i) for i in range(3)]
        if at==STOP:emu.emu_stop();return
        if at==0x4ad0f0:trace.append('L:'+str(get(parent+11,1)>>5))
        if at==0x49a840:ret(TYPES+args[0]*500);return
        if at==0x4ac550:
            kind=get(parent+10,1);trace.append('T:'+str(kind));type_reads+=1
            if mutation and type_reads==1:put(parent+10,101,1);put(parent+11,(((level+1)&7)<<5)|4,1)
            ret(TYPES+kind*500);return
        if at==0x4558c0:trace.append('R:'+str(args[0]))
        if at==0x4549ee:
            trace.append('U:'+str(payload));mu.reg_write(UC_X86_REG_EIP,0x454ac8);return
        if at==0x454ad8:
            trace.append('F');mu.reg_write(UC_X86_REG_EIP,0x454b21);return
        if at==0x4acee0:trace.append('S:'+str(args[0])+':'+str(args[1]));ret(None,8);return
        if at==0x4afc90:trace.append('A:'+str(args[0])+':'+str(args[1]));ret(0,8);return
        if at==0x45a6c0:trace.append('P:'+':'.join(map(str,args)));ret();return
        if at==0x44b900:trace.append('B:'+':'.join(map(str,args)));mu.reg_write(UC_X86_REG_EIP,STOP+0x200);return
        if not (0x4549c0<=at<=0x454be7 or 0x4558c0<=at<=0x4558e7 or 0x4ad0a0<=at<=0x4ad0ef or 0x4ad0f0<=at<=0x4ad110 or STOP+0x200<=at<STOP+0x209 or STOP+0x300<=at<STOP+0x330):raise RuntimeError(f'원문 실행 범위 밖 {at:08x}')
    mu.mem_write(parent,b'\xa5'*50);put(parent,VT);put(parent+10,100,1);put(parent+11,(level<<5)|4,1);put(0x5c8464,POOL);put(VT+0x80,0x4ad0f0);put(0x532710,seed)
    for kind in [100,101]:  #현재 TYPE마다 별도의 첫 번호/개수 표를 명시 입력한다.
        for index in range(16):put(TYPES+kind*500+0x130+index*4,first+(100 if kind==101 else 0)+index*10);put(TYPES+kind*500+0x170+index*4,size+(kind-100))
    put(STOP+0x250,0x3ec00000);mu.mem_write(STOP+0x200,b'\xd9\x05'+struct.pack('<I',STOP+0x250)+b'\xc2\x0c\x00')
    caller=STOP+0x300;code=b'\xb9'+struct.pack('<I',parent)
    for value in [payload,count,event]:code+=b'\x68'+struct.pack('<I',value)  #세 원문 DWORD를 실제 caller의 역순으로 쌓는다.
    code+=b'\xe8'+struct.pack('<i',0x4549c0-(caller+len(code)+5));code+=b'\xd9\x1d'+struct.pack('<I',STOP+0x260);code+=b'\xe9'+struct.pack('<i',STOP-(caller+len(code)+5));mu.mem_write(caller,code)
    mu.reg_write(UC_X86_REG_ESP,STACK+0x8000);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0);mu.hook_add(UC_HOOK_CODE,hook)
    try:mu.emu_start(caller,STOP+1,count=3000);result=str(get(STOP+0x260));assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+0x8000
    except UcError:
        assert mu.reg_read(UC_X86_REG_EIP)==0x4558e1;result='fault'
    return result,';'.join(trace) or '-',str(get(0x532710)),bytes(mu.mem_read(parent,50)).hex(),instructions
def main():
    """신규 독립 기대값을 저장하거나 기존 파일을 변경하지 않고 다시 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));rows=[];total=0
    for row in cases():  #제품 C++/기대 분기 계산 없이 원본 명령 결과만 읽는다.
        result=sample(pe,row);rows.append((','.join(map(str,row)),*result[:4]));total+=result[4]
    text='# input return events rng raw50\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:assert FIXTURE.read_text(encoding='utf-8')==text
    else:assert not FIXTURE.exists();FIXTURE.write_text(text,encoding='utf-8',newline='\n')
    report=dict(passed=True,cases=len(rows),faults=sum(r[1]=='fault' for r in rows),instructions=total,originalGameExecutions=0,boundaries='큰 업그레이드 Begin/Finish는 기존 전체 RawFactoryUpgrade 모듈 경계다. 현재 가상80/4558c0와 나머지 분배는 실제 PE이며 Frame.Set/Advance/플레어/기본Handle/TYPE getter는 명시 장치 경계다.',sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in ['originals/Netstorm.exe','tools/decomp_factory_regular_current.py','cpppj/tests/fixtures/factory-regular-current-x86.tsv']})
    (ROOT/'extracted/cpp-resume-1236/oracle.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(dict(cases=len(rows),faults=report['faults'],passed=True)))
if __name__=='__main__':main()
