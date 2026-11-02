"""원문0041db70의현재checking·Graph효과뒤진단/부분쓰기만게임없이격리관측한다."""
from pathlib import Path
import argparse,hashlib,itertools,json,socket,struct,sys
ROOT=Path(__file__).resolve().parent.parent # 보호PE/전체정밀함수표와기존순수관측도구는읽기만한다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'));sys.path.insert(0,str(ROOT/'tools'))
from decomp_board_approach import Oracle as BaseOracle,RECT,OUT,STACK,STOP,bits
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

class Oracle(BaseOracle):
    """Graph응답은명시입력이고실제접근점/루프/현재전역읽기/재개순서는원문이다."""
    def instruction(self,mu,address,size,data):
        """첫후보Graph/선택진단뒤현재owner·caller사각형이바뀌는경계를그대로기록한다."""
        if address==0x462b80:
            self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);xb,yb=struct.unpack('<II',mu.mem_read(sp+4,8))
            value=1 if self.count==0 or (self.count==1 and self.match) else 2
            if self.count==1 and self.mutation:
                mu.mem_write(RECT,struct.pack('<4f',10,10,30,30))
                if self.mutation>=2:self.put(0x5e4794,(0,1,99,0xffffffff)[self.mutation-2])
                self.events.append('M:'+str(self.mutation)+':'+str(self.word()))
            self.events.append(f'G:{xb}:{yb}:{value}');self.count+=1;mu.reg_write(UC_X86_REG_EAX,value);self.return_stub();return
        if address==0x4e0620:
            self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);line=int.from_bytes(mu.mem_read(sp+12,4),'little');expression=int.from_bytes(mu.mem_read(sp+4,4),'little')
            assert line==0x235 and bytes(mu.mem_read(expression,2))==b'0\0';self.events.append('A:0:565');self.assertions+=1
            if self.after==1:self.put(0x5e4794,0)
            if self.after==2:mu.mem_write(RECT,struct.pack('<4f',11,11,29,29))
            self.return_stub();return
        return BaseOracle.instruction(self,mu,address,size,data)
    def word(self):
        """같은원문005e4794의전체비트를관측한다."""
        return int.from_bytes(self.mu.mem_read(0x5e4794,4),'little')
    def run_case(self,values,precision):
        """정상유한caller자료와명시Graph효과만주고기대결과는원문에서읽는다."""
        checking,self.mutation,self.after,self.match,flags=values;self.events=[];self.count=0;self.assertions=0;mu=self.mu
        mu.mem_write(RECT,struct.pack('<4f',20,20,24,24));mu.mem_write(OUT,b'\xa5'*8);self.put(0x5e4794,checking)
        mu.reg_write(UC_X86_REG_ECX,RECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,precision);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<5I',STOP,OUT,bits(30),bits(10),flags));mu.emu_start(0x41db70,STOP,count=400000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+20 and mu.reg_read(UC_X86_REG_EAX)==OUT
        assert not(mu.reg_read(UC_X86_REG_FPSW)&0x3800)
        return ','.join(map(str,struct.unpack('<4I',mu.mem_read(RECT,16)))),','.join(map(str,struct.unpack('<2I',mu.mem_read(OUT,8)))),str(self.word()),str(self.assertions),';'.join(self.events)
def main():
    """기존696fixture를읽거나덮지않고새입력의최초관측만새폴더에고정한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    oracle=Oracle();rows=[]
    for values in itertools.product((0,1,29,0xffffffff),range(6),range(3),(0,1),(0,4)): # 현재 checking·Graph/진단 후 변경·일치 여부·flags를 독립 입력으로 조합한다.
        first=oracle.run_case(values,0x27f);assert first==oracle.run_case(values,0x37f)
        rows.append(','.join(map(str,values))+'\t'+'\t'.join(first))
    fixture=args.output/'board-checking-parent-x86.tsv';fixture.write_text('# checking,graphMutation,assertMutation,match,flags rectangle4 point2 checkingAfter assertions events\n'+'\n'.join(rows)+'\n','utf-8',newline='\n')
    report=dict(unit=783,dependsOn=779,host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',inputs=len(rows),observations=2*len(rows),instructions=oracle.instructions,gameExecutions=0,multiplayerTests=0,airUnitTests=0,actualConstructionOrMovement=0,fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),binarySha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),limitations=['GraphAt은명시응답/변형경계이며실제Graph/UI의동일성을검증하지않는다.제품후보는기존실제anchor.GraphAt을유지한다.','caller사각형변형은선택진단/부분효과의순서를분리하기위한격리입력이다.정상게임에서자연발생했다고주장하지않는다.','원문assertioncallee는0식/Boardcoord.cpp0x235를관측하고정상복귀시켰다.실제assertUI/CRT파일은호출하지않는다.'])
    (args.output/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps({k:report[k] for k in ['inputs','observations','gameExecutions','multiplayerTests','actualConstructionOrMovement']}))
if __name__=='__main__':main()
