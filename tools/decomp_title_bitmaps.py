#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제비MP4ab3e0/4ab370/4ab2d0/4b3500의전역·출력요청·반납순서를관측한다."""
import argparse,itertools,json,socket,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 채택한tools에서보호저장소를읽기만한다.
if not (ROOT/'originals/Netstorm.exe').is_file():ROOT=Path('H:/T5Workspace/NetstormReborn')  # 격리준비본은같은호스트의확인된저장소를빌린다.
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
from decomp_priestmenu import Oracle,POOL,TEXT,STACK,STOP,digest
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

TABLE,CURRENT,DEPTH,GAME=0x5c7c24,0x5c7c20,0x54dc54,0x55b4d0  # 원문현재8칸/선택/paint깊이/FrameTime게임시각이다.
WIDTH,HEIGHT=0x531860,0x531864  # 실제SCREENW/H를bitmap치수로바꾸지않는다.

class TitleOracle(Oracle):
    """큰부모/대기/다시그리기/초기Statewriter를직접실행하며OS장치만기록한다."""
    def __init__(self):
        """같은32비트메모리장치와실제함수범위만준비한다."""
        super().__init__();self.mu.mem_map(0,0x1000);add_ranges(self,(0x4ab3e0,0x4ab370,0x4ab2d0,0x4b3500))

    def instruction(self,mu,address,size,data):
        """새게임/대기sleep없이명시wall/input으로실제분기를진행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,8)
        if address==0x435b30:self.events.append('P');self.ret();return
        if address==0x452f00:
            pointer,skip=args[:2];assert skip==1;self.pops+=1;value=65 if self.skip and self.pops==self.skip else 0;mu.mem_write(pointer,struct.pack('<3I',value,0,0));self.events.append('I:'+str(value));self.ret(pointer);return
        if address==0x459c60:
            assert args[0]==0xfffffffe and args[2]==0xfffffffe and args[4]==0xfffffffd and args[6]==0
            self.literal=self.string(args[5]).decode('ascii');mu.mem_write(TEXT,self.literal.encode()+b'\0');self.events.append('N:'+self.literal);self.ret(TEXT);return
        if address==0x4dae60:
            name,width,height=args[:3];literal=self.string(name).decode('ascii');index={'title0.gif':0,'title6.gif':6,'title7.gif':7}[literal];self.events.append('G:'+str(index))
            if self.fail==index+1:raise BoundaryFailure('load'+str(index))
            dimensions=(100,80) if index==0 else (120,90) if index==6 else (111,81);self.put(width,dimensions[0]);self.put(height,dimensions[1]);self.ret(POOL+0x3000+index*4);return
        if address==0x4a4850:
            literal=self.string(args[0]).decode('ascii');assert args[1]==1;self.events.append('A:'+literal)
            if self.fail==20:raise BoundaryFailure('palette')
            self.ret();return
        if address in [0x460cd0,0x460d70]:
            value=123.5 if address==0x460cd0 else self.clock;self.events.append(('Gtime:' if address==0x460cd0 else 'W:')+str(value));self.clock+=1.25 if address==0x460d70 else 0;mu.mem_write(args[0],struct.pack('<d',value));self.ret(args[0]);return
        labels={0x4a1580:('C',4),0x4a14e0:('L',0),0x4a1550:('U',0),0x4a46d0:('V',0),0x4a16d0:('F',0),0x4a1fe0:('B',8),0x4a2060:('Z',5),0x4a17c0:('FL',0),0x4a17e0:('FU',0),0x4a19d0:('FB',8),0x43fa90:('R',1)}
        if address in labels:
            label,count=labels[address];self.events.append(label+(':'+','.join(map(str,args[:count])) if count else ''));self.ret();return
        if not any(a<=address<b for a,b in self.allowed):raise RuntimeError('범위밖 '+hex(address))

    def sample(self,case,precision):
        """원문전역과전체장치효과만직접읽고제품기대값을계산하지않는다."""
        mode,width,height,self.skip,self.fail,current,seed=case;self.clock=0.;self.events=[];self.pops=0;self.put(WIDTH,width);self.put(HEIGHT,height);self.put(CURRENT,current);self.put(DEPTH,3);self.mu.mem_write(GAME,struct.pack('<d',7.25));self.mu.mem_write(STACK-0x4000,b'\x65'*0x4100);self.put(STACK,STOP);self.put(0,0)
        slots=[POOL+0x3000+index*4 if seed or mode==1 else 0 for index in range(8)];self.mu.mem_write(TABLE,struct.pack('<8I',*slots));self.mu.mem_write(0x5c8878,b'\xa5'*144);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok';entry=[0x4ab3e0,0x4ab2d0,0x4b3500][mode]
        try:self.mu.emu_start(entry,STOP,count=50000)
        except BoundaryFailure as error:outcome=str(error)
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return '\t'.join([','.join(map(str,case)),outcome,str(self.words(CURRENT,1)[0]),str(self.words(DEPTH,1)[0]),str(struct.unpack('<d',self.mu.mem_read(GAME,8))[0]),','.join(map(str,self.words(TABLE,8))),';'.join(self.events) or '-',bytes(self.mu.mem_read(0x5c8878,144)).hex()])

def cases():
    """입력생략/초기·후반입력/파일중단/다시그리기/42writer를독립입력으로준다."""
    result=[(0,w,h,skip,fail,0,seed) for (w,h),skip,fail,seed in itertools.product(((640,480),(651,491)),(0,1,2,5,8),(0,),range(2))]
    result.extend((0,640,480,0,fail,0,0) for fail in (1,7,8,20))
    result.extend((1,w,h,0,0,current,0) for (w,h),current in itertools.product(((640,480),(651,491)),(-1,0,6,7)))
    result.append((2,640,480,0,0,0,0));return result

def main():
    """새자료최초생성과읽기verify만가능하며기존파일을덮지않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists());oracle=TitleOracle();rows=['# case outcome current paintDepth game slots events next144Hex']
    for case in cases():  #두정밀도에서전역/장치열/현재표와실제함수정상ABI를대조한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text('utf-8')==content
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(content,encoding='utf-8',newline='\n')
    report={'unit':782,'rows':len(rows)-1,'observations':2*(len(rows)-1),'host':socket.gethostname(),'lastFullDecompilation':'HJOW-Athlon / 2026-10-23','bodies':['004ab3e0','004ab370','004ab2d0','004b3500'],'boundary':'실제부모/시간대기분기/표/반납순서/초기42writer. GIF·palette·OS pump/input·시계·Lock/blit/Present/free는명시경계. 실제픽셀/게임/WM_PAINT OS/실제대기/MP검사0.','sha256':{str(path):digest(path) for path in [ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture]}};args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'rows':len(rows)-1,'observations':2*(len(rows)-1)}))

if __name__=='__main__':main()
