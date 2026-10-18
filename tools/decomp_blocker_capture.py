#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 스톤/아이스 타워의 ShapeToBuffer/+78/빈+a4/보호SHP 전체 픽셀을 제품 전에 동결한다."""
import argparse,itertools,json,socket,struct
from decomp_temple_capture import Oracle as CaptureOracle
from decomp_factory_capture import BUFFER,OUT,ROOT,TYPES,POOL,digest
from shp import load_type_texts,read_clusters
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_FPSW

SPECS={'sunBlocker':78,'rainBlocker':147} # 보호 TYPE 로딩 순서의 스톤/아이스 타워다.
TABLE=0x50e940 # 실제 두 타입가 공유하는 VT이며 +78은4acfe0, +a4는빈41c100이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/blocker-capture-x86.tsv' # C++ 연결 전에 고정할 메타/전체 원본 픽셀이다.
POST_FIXTURE=ROOT/'cpppj/tests/fixtures/blocker-post-prefix-x86.tsv' # 전용 후처리 접두의 공통 위임/대조 입력이다.
REPORT=ROOT/'cpppj/blocker-capture.json' # 새 실제 실행 범위와 기존 근거를 구별한다.

class Oracle(CaptureOracle):
    """기존 격리 픽셀힙 외에 캡처/가상함수/보호RLE를 대체하지 않는다."""
    def __init__(self):
        """동일 보호SHP/원본 코드에 스톤/아이스 타워의 TYPE 프레임 입력을 더한다."""
        super().__init__();texts=load_type_texts(str(ROOT/'originals'))
        assert self.read(TABLE+0x78)==0x4acfe0 and self.read(TABLE+0xa4)==0x41c100
        for name in SPECS: # 실제 TYPE에 baseFrame 지정이 없는 두 프레임 자산이다.
            self.clusters[name]=read_clusters(texts[name.lower()]);assert len(self.clusters[name])==2
            assert not any('baseframe' in item[1].lower() for item in self.clusters[name])
        self.actual={};self.totalPixels=0;self.postActive=False;self.postTrace=[];self.postInstructions=0
        data=self.shapeFile.read_bytes();offset=0;self.blocks=[]
        for index in range(max(SPECS.values())-70+1): # 실제147번까지 보호SHP 헤더를 읽는다.
            self.blocks.append(offset);offset+=8+struct.unpack_from('<I',data,offset+4)[0]*8
        self.allowed.append((0x48edf0,0x48eef1))

    def on_instruction(self,mu,address,size,data):
        """후처리 접두의 공통 몸체/표면 조회만 명시 경계로 추적한다."""
        if getattr(self,'postActive',False):
            if 0x48edf0<=address<0x48eef1:self.postInstructions+=1
            if address in (0x44b9e0,0x40eaf0):
                esp=mu.reg_read(UC_X86_REG_ESP);count=1 if address==0x44b9e0 else 2;args=struct.unpack('<'+str(count)+'I',mu.mem_read(esp+4,count*4));self.postTrace.append([hex(address),*args])
                if address==0x44b9e0:assert mu.reg_read(UC_X86_REG_ECX)==POOL+50*50
                mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+(8 if address==0x44b9e0 else 4));mu.reg_write(UC_X86_REG_EIP,self.read(esp));return
        super().on_instruction(mu,address,size,data)

    def post_prefix(self,number,extra,flags,editor,authority):
        """스톤의 공통 위임과147의 억제/표면 조회 대조를 실제48edf0으로 실행한다."""
        self.captureActive=False;self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0));self.flat_segments();raw=bytearray(50)
        struct.pack_into('<I',raw,0,TABLE);raw[10]=number;raw[34]=1;raw[40]=extra;struct.pack_into('<ff',raw,14,95.25,105.5);self.mu.mem_write(POOL+50*50,bytes(raw))
        for address,value in ((0x5412a4,147),(0x5412b0,150),(0x5412cc,157),(0x5c89b8,editor),(0x540bc0,authority)): # 원본 전역의 명시 입력이며 전체 초기화 관측은 아니다.
            self.mu.mem_write(address,struct.pack('<I',value))
        self.postTrace=[];self.postActive=True;self.invoke(0x48edf0,[flags],POOL+50*50,4);self.postActive=False
        assert bytes(self.mu.mem_read(POOL+50*50,50))==raw
        return json.dumps(self.postTrace,separators=(',',':'))

    def capture(self,name,frame,word,extra,owner):
        """명시 TYPE 기준점(-2,3)/base=-1/단위배율과 현재 raw에서 전체 버퍼를 읽는다."""
        self.captureActive=False;self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0));self.flat_segments()
        number=SPECS[name];self.input_shape(name,number);raw=bytearray(50)
        struct.pack_into('<I',raw,0,TABLE);raw[10]=number;struct.pack_into('<H',raw,12,word);raw[34]=owner;raw[40]=extra;struct.pack_into('<i',raw,36,frame)
        self.mu.mem_write(POOL+50*50,bytes(raw));self.mu.mem_write(self.c['scale'],struct.pack('<I',65536));self.mu.mem_write(self.c['extra'],struct.pack('<I',1))
        self.mu.mem_write(OUT,bytes(16));self.mu.mem_write(BUFFER,b'\x5a'*0x100000);self.heap=[];self.soundCalls=0
        context=bytes(self.mu.mem_read(self.c['context'],20));self.write_ranges.extend([(self.c['context'],self.c['context']+20),(BUFFER,BUFFER+0x100000),(OUT,OUT+16)])
        self.mu.reg_write(UC_X86_REG_FPCW,0x27f);self.mu.reg_write(UC_X86_REG_FPSW,0);self.captureActive=True
        self.invoke(self.c['entry'],[50,OUT,OUT+4,OUT+8,OUT+12]);self.captureActive=False
        metadata=struct.unpack('<4i',self.mu.mem_read(OUT,16));width,height=metadata[:2];pixels=bytes(self.mu.mem_read(BUFFER,width*height));self.totalPixels+=len(pixels)
        assert self.mu.reg_read(UC_X86_REG_EAX)==BUFFER and self.heap==[width*height] and bytes(self.mu.mem_read(self.c['context'],20))==context
        assert bytes(self.mu.mem_read(POOL+50*50,50))==raw and self.soundCalls==0
        return [','.join(map(str,metadata)),pixels.hex(),str(len(pixels)),'1']

def main():
    """새 실제 결과를 동결하거나 기존 원문 해시/픽셀 길이만 읽기 전용 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path # 동결 원문을 재관측으로 덮지 않는다.
        rows=[row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows)==report['rows']==120
        for row in rows:assert len(bytes.fromhex(row[6]))==int(row[7]) and row[8]=='1' # 전체 버퍼 길이와 문맥 복원을 대조한다.
        print('타워 전체 캡처120 원문/해시 일치');return
    oracle=Oracle();rows=[]
    for name in SPECS: # 두 실제 프레임·출력WORD·추가/추상 플래그·소유자를 교차한다.
        for frame in (0,1):
            for word in (0,1):
                for extra in (0,2,3,16,18):
                    for owner in (0,1,8):
                        values=(name,frame,word,extra,owner)
                        try:result=oracle.capture(*values)
                        except Exception as error:raise RuntimeError(str(values)) from error
                        rows.append([*values,*result])
    FIXTURE.write_text('# 실제10.78 타워 ShapeToBuffer/+78/빈+a4/보호RLE. TYPE 기준점(-2,3), base=-1/단위배율/일반색. 픽셀힙만 선언 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n','utf-8',newline='\n')
    posts=[]
    for values in itertools.product((78,),(0,1,2,8,16),(0,0x100000,0x200000,0x8000000),(0,1),(0,1)): # 스톤의 조건 조합80개는 항상 같은 공통 위임이다.
        posts.append([*values,oracle.post_prefix(*values)])
    for values in ((147,1,0,0,0),(147,0,0x200000,0,0),(147,0,0,1,0),(147,0,0,0,0)): # 아이스의 억제3개와 실제 표면 조회1개를 구별한다.
        posts.append([*values,oracle.post_prefix(*values)])
    POST_FIXTURE.write_text('# 실제48edf0 접두. 공통44b9e0/표면40eaf0(반환0)만 선언 경계. 원본 전체 공통 후처리/아이스 재생성 완료로 세지 않는다.\n'+'\n'.join('\t'.join(map(str,row)) for row in posts)+'\n','utf-8',newline='\n')
    paths={'tools/decomp_blocker_capture.py','tools/decomp_temple_capture.py','tools/decomp_factory_capture.py','tools/decomp_shape_to_buffer_oracle.py','tools/shp.py','originals/Netstorm.exe','originals/netstorm.tarc','originals/d/_shapes.shp','extracted/refined/originals/functions.tsv',FIXTURE.relative_to(ROOT).as_posix(),POST_FIXTURE.relative_to(ROOT).as_posix()}
    paths.update(path.relative_to(ROOT).as_posix() for path in oracle.exports)
    REPORT.write_text(json.dumps(dict(unit=193,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),postRows=len(posts),postBodyInstructions=oracle.postInstructions,postBoundary='실제48edf0 접두. 공통44b9e0과 표면40eaf0(반환0)만 경계이며 전체 아이스 재생성은 미검증.',pixels=oracle.totalPixels,assertions=oracle.assertions,actualCalls=oracle.actual,captureReplaced=0,virtualDrawReplaced=0,rleReplaced=0,osCalls=0,boundaries='실제 캡처/+78/빈+a4/보호RLE, 픽셀힙만 선언 경계. TYPE 기준점(-2,3)/base=-1/단위배율·일반색의 명시 입력. 실제 미션 초기화/전체 완주와 별도.',sha256={path:digest(ROOT/path) for path in sorted(paths)}),ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n')
    print(len(rows),oracle.totalPixels,oracle.assertions,oracle.actual,'post',len(posts),'postInstructions',oracle.postInstructions)

if __name__=='__main__':main()
