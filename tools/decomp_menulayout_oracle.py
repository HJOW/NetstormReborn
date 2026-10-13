#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 실제 PE의 메뉴 항목 추가/텍스트 설정/측정/전체 레이아웃을 격리 x86에서 관찰한다."""
import argparse
import collections
import csv
import json
import socket
import struct
import zlib
from pathlib import Path
from decomp_menugump_oracle import (Oracle as FrameOracle, ROOT, TOKENS, DATA, VTABLE,
    RESULT, STACK, STOP, STUBS, VIRTUAL, SIZE, raw_nodes, change, bits, digest,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, REGISTERS)

# 측정/소유/제목 경계 주소다. CRT 문자열 복사/절삭과 메뉴 글자 위임 몸체는 실제 실행한다.
SPECS={
    'originals':dict(Append=0x477450,Layout=0x476f60,Set=0x475dd0,Measure=0x475d70,
        plain=0x4a3df0,styled=0x475760,clear=0x4756b0,parse=0x4bb430,
        fit=0x4ba360,title_layout=0x4b9fb0,marker=0x50c6c8),
    'originalCD':dict(Append=0x451730,Layout=0x451f00,Set=0x451500,Measure=0x451430,
        plain=0x425f90,styled=0x450640,clear=0x450590,parse=0x4b7770,
        fit=0x4b6330,title_layout=0x4b5fa0,marker=0x51ffe8),
}
# 입력 문자열/문맥은 원본 실행 중 읽기 전용이며 StyleText 토큰은 명시 경계의 응답 값이다.
TEXT, CONTEXT, STYLE_BASE = DATA+0x8000,DATA+0x8200,0x19000000
NODE_SIZES=(SIZE,160,160)
OBSERVATIONS=ROOT/'cpppj/recovery-menulayout-observations.tsv'
REPORT=ROOT/'cpppj/menulayout.json'


def snapshot(nodes):
    """메뉴 전체와 두 기본 자식의 실제 바이트를 그대로 직렬화한다."""
    return ';'.join(raw[:size].hex() for raw,size in zip(nodes,NODE_SIZES))


def words(values):
    """입력 DWORD 비트를 little endian으로 기록한다."""
    return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values)).hex()


def metrics(profile):
    """plain/styled 측정의 명시 외부 응답을 생성한다. 메뉴의 예상 폭/높이는 계산하지 않는다."""
    result=[]
    # 각 항목의 두 측정 경계에 서로 다른 signed DWORD를 공급한다.
    for i in range(40):
        value=[11+i*7,12+i*3,20+i*5,15+i*2] if profile==0 else [-12-i,-5+i,-20+i,-3-i] if profile==1 else [2147483643,2147483646,2147483641,2147483645]
        result.extend(value)
    return result+[3,25]


def initial(count,flags):
    """전체 미작성 패턴을 유지하면서 레이아웃/항목의 안전한 입력만 준비한다."""
    result=raw_nodes(bool(flags&0x800000));result[0]=change(change(result[0],544,flags),548,count)
    raw=bytearray(result[0]);struct.pack_into('<8I',raw,128,2,3,29,1,99,7,1,2)
    # 모든 40칸을 초기 항목으로 준비해 콜백의 정상 항목 수 증가도 관찰할 수 있게 한다.
    for i in range(40):
        label=f'item-{i}'.encode();offset=0x228+i*60;raw[offset:offset+40]=label+b'\0'*(40-len(label))
        item_flags=0x10000|(0x400 if i%3==2 else 0)|(0x20 if i%4==1 else 0)|(0x800 if i%4==3 else 0)
        struct.pack_into('<5I',raw,offset+40,0,0x701+i,0x901+i,STYLE_BASE+i*4,item_flags)
        struct.pack_into('<f',raw,0xd0+i*4,3.5+i);struct.pack_into('<f',raw,0x17c+i*4,-2.25-i)
    result[0]=bytes(raw)
    # 자식 등록 ID와 제목의 소수 크기/위치를 실제 기본 필드로 공급한다.
    for i in (1,2):
        result[i]=change(change(change(result[i],28,77+i),60,bits(75.5)),76,bits(2.25*i))
        result[i]=change(result[i],80,bits(5.75*i))
    return result


def plan():
    """방향/한도/구분선/정수 감김/일부 쓰기/콜백 이후 현재 조회 입력을 만든다."""
    cases=[]
    def add(actions,count=3,flags=0x80000,profile=0,text=b'Go',patches=()):
        """예상 레이아웃 없이 완성 원본 입력과 명시 콜백 응답만 저장한다."""
        case=dict(actions=actions,nodes=initial(count,flags),metrics=metrics(profile),text=text,
            context=[2,5,27,1,98,7,1,2],patches=list(patches));cases.append(case);return case
    # 세 측정 응답과 세 방향·장식·작은 여백을 모든 항목 수 경계에서 교차한다.
    for count in (0,1,3,40):
        for flags in (0,0x40000,0x80000,0x1080000,0x880000,0x800000):
            for profile in (0,1,2): add([('Layout',)],count,flags,profile)
    # 일부 필드만 쓰는 항목 추가와 자식 소유 방식 및 40개 full 조기 반환이다.
    for count in (0,1,39,40):
        for flags in (0,1,0x80,0x81,0x400,0x600): add([('Append',flags,123,456,0)],count)
        for flags in (0,0x800): add([('Append',flags,123,456,TOKENS[1])],count)
        add([('Append',0x10,0,0,0)],count,text=b'')
    # 선두/연속 구분선과 null 문자열의 실제 도달 여부를 구별한다.
    c=add([('Append',0x10,0,0,0)],1,text=b'');c['nodes'][0]=change(c['nodes'][0],0x260,0x10010)
    add([('Append',0x10,0,0,0)],0,text=None);add([('Append',0,0,0,0)],40,text=None)
    # plain strncpy(40)는 짧은 문자열의 padding과 NUL을 안 붙이는 정확한 경계를 관찰한다.
    for text in (b'',b'A',b'Q'*39,b'Q'*40,b'Q'*50):
        c=add([('Set',1),('Measure',1)],text=text);c['nodes'][0]=change(c['nodes'][0],0x260+60,0x10000)
    for flags in (0x400,0x600):
        c=add([('Set',2),('Measure',2)],text=b'~3Styled');c['nodes'][0]=change(c['nodes'][0],0x260+120,flags)
    # 측정 콜백이 변경한 상태는 다음 항목, 표시 문자와 크기 계산에 반영된다.
    add([('Layout',)],1,patches=(('plain',TOKENS[0],0,TOKENS[0],548,3),))
    add([('Layout',)],3,patches=(('plain',TOKENS[0],0,TOKENS[0],548,1),))
    add([('Layout',)],3,patches=(('plain',TOKENS[0],0,TOKENS[0],0x260,0x10020),('plain',TOKENS[0],0,TOKENS[0],132,6)))
    add([('Layout',)],3,patches=(('marker',TOKENS[0],0xffffffff,TOKENS[0],0xbc,bits(7.25)),('marker',TOKENS[0],0xffffffff,TOKENS[0],0xc0,bits(3.5))))
    add([('Layout',)],3,patches=(('styled',TOKENS[0],2,TOKENS[0],544,0),))
    add([('Layout',)],3,0x880000,patches=(('fit',TOKENS[2],0xffffffff,TOKENS[2],60,bits(203.5)),('title_layout',TOKENS[2],0xffffffff,TOKENS[2],64,bits(12.25))))
    add([('Layout',)],3,0x880000,patches=(('title_layout',TOKENS[2],0xffffffff,TOKENS[0],544,0x40000),))
    add([('Layout',)],3,0x880000,patches=(('fit',TOKENS[2],0xffffffff,TOKENS[0],2956,TOKENS[1]),))
    add([('Layout',)],3,0x880000,patches=(('push',TOKENS[1],0xffffffff,TOKENS[0],0xd0,bits(9.75)),))
    # 자식 Attach와 StyleText Clear/Parse 이후에는 현재 항목 수, 플래그와 문맥을 다시 읽는다.
    add([('Append',0x81,123,456,TOKENS[1])],0,patches=(('attach',TOKENS[1],0xffffffff,TOKENS[0],548,1),))
    add([('Append',0x600,0,0,0)],0,patches=(('clear',TOKENS[0],0,TOKENS[0],0x260,0x10400),('clear',TOKENS[0],0,TOKENS[0],128,7)))
    add([('Append',0x400,0,0,0)],0,patches=(('parse',TOKENS[0],0,TOKENS[0],548,2),))
    return cases


class Oracle(FrameOracle):
    """실제 메뉴/CRT 몸체와 입력 응답 경계만 실행하며 원본 게임 진입점/OS/assert는 거부한다."""
    def __init__(self,edition):
        """기존 격리 매핑을 공유하고 이번 내보내기 범위/함수 진입 목록으로 제한한다."""
        super().__init__(edition);self.d=dict(self.d);self.d.update(SPECS[edition]);self.allowed=[];self.function_entries=set()
        # 선택한 함수의 실제 몸체 범위만 허용한다.
        for row in csv.DictReader((ROOT/f'extracted/menulayout/{edition}/functions.tsv').open(encoding='utf-8'),delimiter='\t'):
            self.function_entries.add(int(row['entry'],16))
            # 분리된 코드 범위도 끝 주소를 포함한 원본 TSV 그대로 읽는다.
            for part in row['ranges'].split(';'):
                low,high=part.split('-');self.allowed.append((int(low,16),int(high,16)+1))
    def nodes(self):
        """현재 전체 메뉴 바이트와 기본 자식 둘을 읽는다."""
        return snapshot([bytes(self.mu.mem_read(token,size)) for token,size in zip(TOKENS,NODE_SIZES)])
    def crc(self):
        """각 콜백 직전 전체 저장소의 CRC32를 관찰한다. 최종 출력은 전체 바이트로 비교한다."""
        return str(zlib.crc32(b''.join(self.mu.mem_read(token,size) for token,size in zip(TOKENS,NODE_SIZES))))
    def string(self,address):
        """명시 입력/원본 문자열의 NUL 종료까지 실제 바이트를 읽는다."""
        result=bytearray()
        # NUL이 나오면 멈추고 준비된 입력 범위를 넘는 문자열은 거부한다.
        for i in range(4096):
            value=self.mu.mem_read(address+i,1)[0]
            if not value: return bytes(result)
            result.append(value)
        raise RuntimeError('메뉴 관찰 문자열 범위 초과')
    def item(self,address,offset=0):
        """실제 항목/StyleText 필드 주소를 메뉴 토큰과 항목 index로 정규화한다."""
        value=address-TOKENS[0]-0x228-offset
        if value<0 or value%60 or value//60>=40: raise RuntimeError(f'메뉴 항목 주소 오류 {address:x}')
        return value//60
    def respond(self,kind,token,index,details):
        """경계 인자와 현재 전체 CRC를 기록한 뒤 명시 raw 응답만 적용한다."""
        self.calls.append(','.join(map(str,(kind,token,index,*details)))+'@'+self.crc());self.boundaries[kind]+=1
        # 현재 경계의 종류/토큰/index가 같은 응답만 호출 이후에 적용한다.
        for k,t,i,target,offset,value in self.patches:
            if (k,t,i)==(kind,token,index): self.put(target+offset,value)
    def code(self,mu,address,size,data):
        """텍스트/가상/제목 경계를 관찰하고 나머지는 허용한 실제 명령만 실행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);cleanup=None
        if address==self.d['plain']:
            source,width,height,context,length=[self.u32(esp+4+i*4) for i in range(5)]
            kind='marker' if source==self.d['marker'] else 'plain';index=0xffffffff if kind=='marker' else self.item(source)
            ctx=bytes(mu.mem_read(context,32)).hex();text=self.string(source).hex()
            # 측정 출력은 사례 입력의 명시 응답이다. 관찰한 메뉴 측정값으로 다시 계산하지 않는다.
            pair=self.metrics[-2:] if kind=='marker' else self.metrics[index*4:index*4+2]
            self.respond(kind,TOKENS[0],index,(length,ctx,text));self.put(width,pair[0]);self.put(height,pair[1]);cleanup=0
        elif address==self.d['styled']:
            index=self.item(this,52);width,height=[self.u32(esp+4+i*4) for i in range(2)]
            self.respond('styled',TOKENS[0],index,());pair=self.metrics[index*4+2:index*4+4];self.put(width,pair[0]);self.put(height,pair[1]);cleanup=2
        elif address==self.d['clear']:
            index=self.item(this,52)
            self.calls.append(f'clear,{TOKENS[0]},{index}@'+self.crc());self.boundaries['clear']+=1;self.put(this,0)
            # 소유 필드를 비운 뒤 호출자가 지정한 원본 상태 변경을 반영한다.
            for k,t,i,target,offset,value in self.patches:
                if (k,t,i)==('clear',TOKENS[0],index): self.put(target+offset,value)
            cleanup=0
        elif address==self.d['parse']:
            source=self.u32(esp+4);context=bytes(mu.mem_read(esp+8,32)).hex();destination=self.u32(esp+40);index=self.item(destination,52)
            self.calls.append(f'parse,{TOKENS[0]},{index},{self.string(source).hex()},{context}@'+self.crc());self.boundaries['parse']+=1
            self.put(destination,STYLE_BASE+index*4)
            # 파싱 결과 토큰을 쓴 뒤 명시 응답을 반영한다.
            for k,t,i,target,offset,value in self.patches:
                if (k,t,i)==('parse',TOKENS[0],index): self.put(target+offset,value)
            cleanup=0
        else:
            boundary=VIRTUAL.get(address)
            if address==self.d['fit']: boundary=('fit',1)
            if address==self.d['title_layout']: boundary=('title_layout',2)
            if boundary:
                kind,count=boundary;args=[self.u32(esp+4+i*4) for i in range(count)]
                self.respond(kind,this,0xffffffff,args);cleanup=count
        if cleanup is not None:
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup*4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed): raise RuntimeError(f'허용 밖 메뉴 레이아웃 실행 {self.edition} {address:x}')
        if address in self.function_entries: self.entries[f'{address:08x}']+=1
    def run(self,case,control):
        """연속 action의 정상 반환·ABI·x87·입력/객체 보호와 전체 출력 바이트를 확인한다."""
        mu=self.mu;self.calls=[];self.patches=case['patches'];self.metrics=case['metrics'];results=[]
        mu.mem_write(DATA,b'\xa5'*0x10000)
        # 세 노드와 실제 가상 슬롯 경계의 입력 바이트를 초기화한다.
        for token,raw in zip(TOKENS,case['nodes']): mu.mem_write(token,raw)
        # 원본의 Push/Pop/Attach/Redraw만 별도 명시 응답 주소로 연결한다.
        for slot,address in ((8,STUBS+0x140),(0x18,STUBS+0x100),(0x1c,STUBS+0x120),(0x20,STUBS+0x160),(0x3c,STUBS+0x180)):
            self.put(VTABLE+slot,address)
        readonly=bytes(mu.mem_read(VTABLE,64));context=bytes.fromhex(words(case['context']));mu.mem_write(CONTEXT,context)
        source=(case['text'] or b'')+b'\0';mu.mem_write(TEXT,source)
        self.writes=[(STACK-0xf000,STACK+0x1000),(RESULT,RESULT+8),(TOKENS[0]+60,TOKENS[0]+76),
            (TOKENS[0]+0xbc,TOKENS[0]+0xcc),(TOKENS[0]+0xd0,TOKENS[0]+0x21c),(TOKENS[0]+548,TOKENS[0]+552),
            (TOKENS[0]+0x228,TOKENS[0]+0xb88)]
        self.writes += [(token+offset,token+offset+4) for token in TOKENS[1:] for offset in (4,60,64,68,72)]
        # 공개 함수의 원래 thiscall/인자 정리를 유지하며 중간 상태를 다음 action에 전달한다.
        for action in case['actions']:
            op,*args=action;this=TOKENS[0]
            if op in ('Set','Measure'):
                this+=0x228+args[0]*60;args=[TEXT if case['text'] is not None else 0,CONTEXT] if op=='Set' else [RESULT,RESULT+4,CONTEXT]
            if op=='Append': args=[TEXT if case['text'] is not None else 0,*args]
            # 모든 callee-save 레지스터를 구별되는 입력 값으로 설정한다.
            for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
            mu.reg_write(UC_X86_REG_ECX,this);mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0)
            mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(self.d[op],STOP,count=2000000)
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+len(args)*4 or any(mu.reg_read(r)!=v for r,v in REGISTERS.items()): raise RuntimeError('메뉴 레이아웃 ABI/정상 반환 오류')
            if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800: raise RuntimeError('메뉴 레이아웃 x87 오류')
            self.returns+=1;results.append(words(struct.unpack('<2I',mu.mem_read(RESULT,8))) if op=='Measure' else '-')
        if bytes(mu.mem_read(VTABLE,64))!=readonly or bytes(mu.mem_read(CONTEXT,32))!=context or bytes(mu.mem_read(TEXT,len(source)))!=source: raise RuntimeError('메뉴 레이아웃 입력/가상 표 변경')
        return [self.nodes(),'|'.join(self.calls) or '-', ';'.join(results)]


def main():
    """실제 두 판본의 두 x87 관찰이 같은 경우를 저장하거나 저장 자료를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 근거 파일이 저장 당시 해시와 같은지 확인한다.
        for name,sha in report['files'].items():
            if digest(ROOT/name)!=sha: raise RuntimeError('메뉴 레이아웃 해시 변경: '+name)
        rows=list(csv.DictReader(OBSERVATIONS.open(encoding='utf-8'),delimiter='\t'))
        if len(rows)!=report['rows']: raise RuntimeError('메뉴 레이아웃 행 수 변경')
        print(f'메뉴 레이아웃 감사 통과: {len(rows)}행');return
    rows=[];stats={};files=[Path(__file__),ROOT/'tools/ghidra/menulayout-functions.json',ROOT/'tools/decomp_menugump_oracle.py',ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py']
    # 두 판본은 서로 다른 PE와 내보내기를 사용하는 독립 관찰이다.
    for edition in SPECS:
        oracle=Oracle(edition);cases=plan()
        # 같은 입력을 두 x87 정밀도에서 실제 실행해 동일한 경우만 한 행으로 저장한다.
        for i,case in enumerate(cases):
            out=oracle.run(case,0x027f);other=oracle.run(case,0x037f)
            if out!=other: raise RuntimeError(f'메뉴 레이아웃 x87 모드 불일치 {edition}/{i}')
            rows.append([edition,str(i),';'.join(','.join(map(str,a)) for a in case['actions']),snapshot(case['nodes']),words(case['metrics']),
                case['text'].hex() if case['text'] is not None else '-',words(case['context']),';'.join(','.join(map(str,p)) for p in case['patches']) or '-',*out])
        stats[edition]=dict(cases=len(cases),normal_returns=oracle.returns,entries=dict(oracle.entries),boundaries=dict(oracle.boundaries))
        files.extend([ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'),ROOT/f'extracted/menulayout/{edition}/creation.c',ROOT/f'extracted/menulayout/{edition}/functions.tsv'])
    with OBSERVATIONS.open('w',encoding='utf-8',newline='') as f:
        writer=csv.writer(f,delimiter='\t',lineterminator='\n');writer.writerow(['edition','case','actions','nodes','metrics','text','context','patches','out_nodes','calls','results']);writer.writerows(rows)
    files.append(OBSERVATIONS)
    REPORT.write_text(json.dumps(dict(last_decompile_host=socket.gethostname(),rows=len(rows),editions=stats,
        method='실제 메뉴/CRT/strncpy/측정 위임 몸체; 텍스트/StyleText/제목/가상 부착은 명시 외부 경계',
        cpp_comparison='미수행: 원본 분석 자료이며 C++ 재생 검사는 후속',
        files={str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in files}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'메뉴 레이아웃 관찰 완료: {len(rows)}행, 정상 반환 {sum(s["normal_returns"] for s in stats.values())}회')


if __name__=='__main__': main()
