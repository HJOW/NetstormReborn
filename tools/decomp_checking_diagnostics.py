"""게임 없이 원문4ad730과46ba98..46bb42 순수 진단 접두만 독립 CPU로 관측한다."""
from pathlib import Path
import argparse,hashlib,itertools,json,socket,struct,sys
ROOT=Path(__file__).resolve().parent.parent # 같은 보호 PE/정밀 함수표는 읽기만 한다.
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle as BaseOracle,SCRATCH,STACK,STOP
from unicorn.x86_const import *
POOL=SCRATCH+0x4000 # 두 진단에만 사용하는 격리 raw50BYTE 슬롯이다.
RECT=SCRATCH+0x7000 # 발자국 getter의 명시 반환 버퍼다.
class Oracle(BaseOracle):
    """전체 게임 Frame의 앞/뒤를 실행하지 않고 순수 진단 범위만 선택한다."""
    def __init__(self):
        """허용된 원문 진단/변환 범위와 관측용 현재 상태만 초기화한다."""
        super().__init__('originals',[dict(name='Forms',originals='004ad730'),dict(name='GunDiagnosticSlice',originals='0046b830')],helpers=[0x41def0,0x41d770,0x41d7a0,0x4e49c0])
        # 진단 줄·검색 범위·finder 순번·재진입 선택과 명령 수/최근 주소는 관측용 멤버다.
        self.events=[];self.bounds=None;self.current=0;self.mutation=0;self.instructions=0;self.recent=[]
    def word(self,address,value=None):
        """격리 DWORD의 전체 비트를 읽거나 명시 입력으로 쓴다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]
    def ret(self,value=0,purge=0):
        """현재 명시 경계의 반환과 실제 callee 인자 정리만 수행한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_EIP,self.word(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge)
    def on_instruction(self,mu,address,size,data):
        """발자국/일반 finder 제공자는 명시 경계이고 진단/float 변환 몸체는 원문 그대로다."""
        self.instructions+=1
        self.recent=(self.recent+[hex(address)])[-24:]
        if address in (0x46bce7,0x46bb42):mu.reg_write(UC_X86_REG_EIP,STOP);return # 두 정상 진단 출구의 Frame epilogue 전 멈추며 뒤의 동작은 실행하지 않는다.
        if address==0x4ade70:
            esp=mu.reg_read(UC_X86_REG_ESP);target=self.word(esp+4);assert self.word(esp+8)==0
            mu.mem_write(target,struct.pack('<4f',*self.footprint));self.ret(target,8);return
        if address==0x4b16d0:
            esp=mu.reg_read(UC_X86_REG_ESP);finder=mu.reg_read(UC_X86_REG_ECX)
            # 원문의 네 검색 경계 인자를 스택 순서대로 읽는다.
            self.bounds=tuple(self.word(esp+4+4*i) for i in range(4));assert self.word(finder+0x38)==self.word(finder+0x3c)==0
            self.word(finder+0x34,5);self.current=0;self.ret(0,16);return
        if address==0x4b1810:
            self.current+=1;self.word(mu.reg_read(UC_X86_REG_ECX)+0x34,(5+self.current) if self.current<3 else 0);self.ret();return
        if address==0x4e0620:
            esp=mu.reg_read(UC_X86_REG_ESP);line=self.word(esp+12);assert line==0xad;self.events.append(line)
            if len(self.events)==1:
                if self.mutation==1:self.word(0x5e4794,0)
                if self.mutation==2:mu.mem_write(POOL+50+34,b'\x02')
                if self.mutation==3:self.word(0x5412cc,98)
            self.ret();return
        super().on_instruction(mu,address,size,data)
    def reset(self,precision):
        """진단 이외의 상태/시각/패킷은 만들지 않고 같은 격리 입력을 복구한다."""
        self.mu.mem_write(POOL,bytes(50*12));self.mu.mem_write(STACK-0x400,bytes(0x500));self.word(STACK,STOP);self.word(0x5c8464,POOL)
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:self.mu.reg_write(reg,0) # 이전 진단의 레지스터를 다음 관측에 섞지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.events=[];self.bounds=None;self.current=0
    def forms(self,values,precision):
        """부모/두 child의 type/flags/링크를 직접 주고 원문 반환과 불변을 읽는다."""
        self.reset(precision);ptype,pflags,head,ctype,cflags,dtype,dflags=values
        for sid,kind,flags in [(1,ptype,pflags),(5,ctype,cflags),(6,dtype,dflags)]:self.mu.mem_write(POOL+sid*50+10,bytes([kind,flags])) # 진단 raw 슬롯만 만들고 가상 생성/Pop은 하지 않는다.
        self.mu.mem_write(POOL+50+6,struct.pack('<H',5 if head else 0));self.mu.mem_write(POOL+250+4,struct.pack('<H',6 if head==2 else 0))
        before=bytes(self.mu.mem_read(POOL,600));result=self.call('Forms',[],POOL+50)
        assert before==bytes(self.mu.mem_read(POOL,600));return result
    def island(self,values,precision):
        """실제 case1 진단 접두의 반복 gate/type/owner와41def0 변환만 관측한다."""
        self.reset(precision);checking,parent,kind,mutation,index=values;self.mutation=mutation
        self.footprint=[(20.25,20.25,21.0,21.0),(20.25,20.25,21.000002,21.000002),(20.25,20.25,21.00002,21.00002)][index] # 현재 발자국 멤버는 이번 명시 getter 입력만 보관한다.
        self.word(0x5e4794,checking);self.word(0x5412cc,kind);self.mu.mem_write(POOL+50+34,bytes([parent]))
        for sid,ct,owner in [(5,99,2),(6,99,2),(7,98,3)]:self.mu.mem_write(POOL+sid*50+10,bytes([ct]));self.mu.mem_write(POOL+sid*50+34,bytes([owner])) # 일반 finder의 명시 반환 목록만 제공한다.
        self.mu.reg_write(UC_X86_REG_EBP,POOL+50)
        try:self.mu.emu_start(0x46ba98,STOP,count=10000)
        except Exception as error:raise RuntimeError((values,self.recent,hex(self.mu.reg_read(UC_X86_REG_EIP)),hex(self.mu.reg_read(UC_X86_REG_ESP)))) from error
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP
        return len(self.events),self.word(0x5e4794),self.word(0x5412cc),self.mu.mem_read(POOL+50+34,1)[0],'-' if self.bounds is None else ','.join(map(str,self.bounds))
def main():
    """최초 fixture를 새 디렉터리에만 쓰며 재관측 결과로 기존 기대를 고치지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    oracle=Oracle();forms=[];islands=[]
    for ptype,pflags,head,ctype,cflags in itertools.product((69,70),(0,1,2,4),(0,1,2),(6,69,70),(0,1,2,4)): # 부모/자식 타입·상태와 링크 수를 독립적으로 조합한다.
        values=(ptype,pflags,head,ctype,cflags,6,2);first=oracle.forms(values,0x27f);assert first==oracle.forms(values,0x37f);forms.append(','.join(map(str,values))+'\t'+str(first))
    for checking,parent,kind,mutation,index in itertools.product((0,1,29,0xffffffff),(1,2),(98,99),(0,1,2,3),(0,1,2)): # 현재 checking·owner/type·진단 후 변경·발자국 정밀도를 순회한다.
        values=(checking,parent,kind,mutation,index);first=oracle.island(values,0x27f);assert first==oracle.island(values,0x37f);islands.append(','.join(map(str,values))+'\t'+'\t'.join(map(str,first)))
    texts={'checking-forms-x86.tsv':'# parentType,parentFlags,headCount,childType,childFlags,lastType,lastFlags result\n'+'\n'.join(forms)+'\n','checking-island-owner-x86.tsv':'# checking,parentOwner,noIslandType,mutation,rect assertions checkingAfter typeAfter ownerAfter bounds\n'+'\n'.join(islands)+'\n'}
    for name,value in texts.items():(args.output/name).write_text(value,'utf-8',newline='\n') # 최초 원문 관측의 바이트만 새 fixture로 고정한다.
    # 저장한 각 fixture를 순회해 실제 바이트 해시를 보고하며 기존 기대를 정규화하지 않는다.
    report=dict(unit=779,host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',formsInputs=len(forms),islandInputs=len(islands),observations=2*(len(forms)+len(islands)),gameExecutions=0,multiplayerTests=0,airUnitTests=0,instructions=oracle.instructions,binarySha256=oracle.sha256,toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtures={name:hashlib.sha256((args.output/name).read_bytes()).hexdigest() for name in texts},limitations=['noIsland은46ba98..46bb42진단접두만선택했고전체GunFrame/실제공격/Kernel은실행하지않았다.','발자국getter와일반finder반환목록은명시경계이며4단계finder몸체의동일성재검사로합산하지않는다.','41def0/41d770/41d7a0/ftol은실제원문변환이다.'])
    (args.output/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8');print(json.dumps({k:report[k] for k in ['formsInputs','islandInputs','observations','gameExecutions','multiplayerTests']}))
if __name__=='__main__':main()
