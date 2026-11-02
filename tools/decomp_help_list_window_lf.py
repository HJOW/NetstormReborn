"""직접HTML fixture와 실제 Help ctor의 LF 경계를 원문에서 따로 확인한다."""
from pathlib import Path
import argparse
import hashlib
import json
import socket
import sys

ROOT=Path(__file__).resolve().parents[1]  # 같은 보호 원문을 읽기만 한다.
sys.path.insert(0,str(ROOT/'tools'))
from decomp_quick_help_window import Oracle as WindowOracle,SOURCE,bits
from decomp_quick_help_html import Oracle as HtmlOracle

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output",type=Path,required=True,help="기존파일이없는새원시JSON")
arguments=parser.parse_args()
assert not arguments.output.exists()
window=WindowOracle()  # 실제4c5380의 파일 child는 기존 명시 입력으로만 대신한다.
window.sample('C',(0,0,0,bits(1)))
cached=bytes(window.mu.mem_read(SOURCE,len(window.helpText)+2))
assert cached==window.helpText+b'\n\0'
html=HtmlOracle();rows=[]  # 제품 출력 없이 원문 parser가 추가 LF를 기록하는지 관측한다.
# 고정된세입력을그대로읽고원문parser에만LF가추가된본문을공급한다.
for line in (ROOT/'cpppj/tests/fixtures/quick-help-list-html-x86.tsv').read_text('utf-8').splitlines():
    if line.startswith('#'): continue
    source,expected,hp=line.split('\t');raw=bytes.fromhex(source).decode('utf-8')+'\n'
    case=(raw,'AlphaType',1,1,1,8);first=html.sample('D',case);assert first==html.sample('D',case)
    actual=first.split('|')[3];assert actual==expected+'0a'
    rows.append(dict(sourceHex=source,nativeCachedInputHex=raw.encode().hex(),nativeStreamHex=actual))
record=dict(unit=766,host=socket.gethostname(),readerInputs=1,readerObservations=1,htmlInputs=3,htmlObservations=6,
    constructor='004c5380',directFixture='cpppj/tests/fixtures/quick-help-list-html-x86.tsv',cachedBytesHex=cached.hex(),
    originalReaderSourceHex=window.helpText.hex(),rows=rows,gameExecutions=0,multiplayerTests=0,
    boundary='실제ctor LF/NULstore와 실제parser만관측한다. 기본GUI/파일child/TYPE/Config/번역/stream callback은기존도구의명시입력이다. 제품출력은읽지않고기존fixture는수정하지않는다.',
    sourceSha256={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('originals/Netstorm.exe','tools/decomp_quick_help_window.py','tools/decomp_quick_help_html.py')})
target=arguments.output;target.parent.mkdir(parents=True,exist_ok=True);assert not target.exists();target.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8')
print('Native ctor LF/NUL + three appended HTML streams match; 7 observations')
