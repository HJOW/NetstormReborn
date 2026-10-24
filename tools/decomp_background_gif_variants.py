"""보호 GIF의 헤더만 메모리에서 바꾼 독립 입력을 실제 원본 해독기로 관측한다."""
import hashlib,json,socket,struct,sys
from decomp_background_gif_assets import ROOT,PE,decode

FIXTURE=ROOT/'cpppj/tests/fixtures/background-gif-variants-x86.tsv';REPORT=ROOT/'cpppj/background-gif-variants-native-20261024.json'  # 보호/최초 자료를 덮어쓰지 않는다.
def variants():
    """논리 화면/오프셋/인터레이스/로컬 팔레트의 독립 파일 입력만 만든다."""
    path=ROOT/'originals/d/titleMenu.gif';original=path.read_bytes();descriptor=13+(3<<((original[10]&7)+1));result=[]
    for name,width,height,left,top,interlace in [('logicalOffsets',683,521,17,19,False),('smallCanvas',1,1,40000,40000,False),('interlaced',683,521,17,19,True)]:  # 압축 바이트/실제 이미지 치수는 바꾸지 않는다.
        data=bytearray(original);struct.pack_into('<HH',data,6,width,height);struct.pack_into('<HH',data,descriptor+1,left,top)
        if interlace:data[descriptor+9]|=0x40
        result.append((name,path,bytes(data)))
    header=bytearray(original[:13]);header[10]&=0x7f;image=bytearray(original[descriptor:descriptor+10]);image[9]=(image[9]&0x78)|0x80|(original[10]&7)
    result.append(('localPalette',path,bytes(header+image+original[13:descriptor]+original[descriptor+10:])))
    return result
def main():
    """기존34행과 구별되는 최초 변형4행 또는 읽기 전용 재생을 수행한다."""
    assert socket.gethostname().lower()=='hjow-athlon' and hashlib.sha256(PE.read_bytes()).hexdigest()=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    rows=[]
    for name,path,data in variants():  # 실제 원문 LZW만 기대 픽셀을 만든다.
        row=decode(path,data);row['path']=name;rows.append(row);print(json.dumps(row),flush=True)
    keys=('path','width','height','fnv1a64','pixelSha256','sourceSha256','rows','basicBlockEntries');content='\t'.join(keys)+'\n'+''.join('\t'.join(str(row[key]) for key in keys)+'\n' for row in rows)
    if '--verify' in sys.argv:assert FIXTURE.read_text('utf-8')==content
    else:
        FIXTURE.open('x',encoding='utf-8',newline='\n').write(content)
        with REPORT.open('x',encoding='utf-8',newline='\n') as output:json.dump(dict(host=socket.gethostname(),gameExecuted=False,protectedFilesModified=False,fixtureSha256=hashlib.sha256(content.encode()).hexdigest(),cases=rows),output,ensure_ascii=False,indent=2);output.write('\n')
    print(json.dumps(dict(rows=len(rows),verified='--verify' in sys.argv)),flush=True)
if __name__=='__main__':main()
