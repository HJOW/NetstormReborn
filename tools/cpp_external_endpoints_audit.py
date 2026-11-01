#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 두 판본과 현재 cpppj의 외부 접속 출처를 실행/접속 없이 목록화한다(684)."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import socket
import pefile
from nscfg import decode_file
from taff import TaffArchive

ROOT = Path(__file__).resolve().parents[1]  # 원본 읽기와 현재 소스 식별의 저장소 기준이다.
URL = re.compile(r'\b(?:https?|ftp|mailto|telnet|nntp|gopher|rlogin|wais|mid|news|prospero):[^\s<>"\x00`]+',re.I)  # 고정 URL뿐 아니라 원문 치환 템플릿도 기록한다.
CONFIG = re.compile(r'^(?:Ping\d+|PingName\d+|diag\w*|masterServer|PatchUrl|.*Home URL|Master URL|autoOpen|techHelpExe|techHelpFile|rootServerExe|rootServerBasePort|rootServerBroadcastPort|rootServerBucketsServer|officialBasePort|gameServerPort|httpServer|httpPort)$',re.I)  # 외부/로컬 통신과 외부 실행의 현재 설정 키다.
CPP_PATHS = ('cpppj/src/client/NativeMainMenuState.cpp','cpppj/src/client/NativeMainMenuState.h','cpppj/src/client/UberEditor.inc','cpppj/src/client/ClientLocalStateGlobals.h','cpppj/src/client/HelpLinkPolicy.cpp','cpppj/src/client/ExternalHelpClient.cpp','cpppj/src/client/TechnicalHelp.cpp','cpppj/src/client/RawCampaignSessionLogin.inc','cpppj/src/zacket/LocalMissionTransport.cpp','cpppj/src/CMakeLists.txt')  # 제품 상태를 판정할 때 읽은 실제 소비자 경계다.


def sha(path):
    """원본과 현재 출처를 파일의 실제 바이트로 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def urls(source,text):
    """각 실제 줄의 URL과 동적 치환 문자열을 원문 그대로 기록한다."""
    rows=[]
    for number,line in enumerate(text.splitlines(),1):  # 본문에 적힌 주소와 클릭 가능한 href를 구분할 문맥도 보존한다.
        for found in URL.finditer(line):  # 같은 줄의 각 주소 후보를 앞에서부터 기록한다.
            value=found[0].rstrip('.,;)')
            rows.append(dict(source=source,line=number,url=value,template=('{' in value),context=line.strip()[:280]))
    return rows


def main():
    """보호 자료를 읽기만 하고 원격 DNS/HTTP/소켓·프로그램을 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    output.parent.mkdir(parents=True,exist_ok=True)
    sources=set(CPP_PATHS)|{'AGENTS.md','tools/cpp_external_endpoints_audit.py','tools/nscfg.py','tools/taff.py'}
    editions=[]
    for root,archive,setup,exe in (('originals','netstorm.tarc','d/setup.cfg','Netstorm.exe'),('originalCD','NETSTORM.TARC','D/setup.cfg','NETSTORM.EXE')):  # 두 판본의 주소를 섞지 않는다.
        prefix=ROOT/root
        config_path=prefix/setup
        config=[]
        for number,line in enumerate(decode_file(str(config_path)).splitlines(),1):  # 암호 설정의 실제 키/값을 정적으로 읽는다.
            match=re.match(r'\s*([^=]+?)\s*=\s*(.*?)\s*$',line)
            if match and not match[1].lstrip().startswith('//') and (CONFIG.fullmatch(match[1].strip()) or re.search(r'(?:https?://|mailto:|telnet:|[a-z0-9-]+\.(?:com|net|org|edu)\b|(?:\d{1,3}\.){3}\d{1,3})',match[2],re.I)):
                config.append(dict(line=number,key=match[1].strip(),value=match[2]))
        sources.add(f'{root}/{setup}')
        tarc_path=prefix/archive
        tarc=TaffArchive(str(tarc_path))
        links=[]
        for entry in tarc.entries:  # 원본 아카이브에서 직접 읽어 과거 추출 사본의 상태에 의존하지 않는다.
            if entry.name.lower().endswith(('.english','.cfg','.txt','.html','.htm')):
                links.extend(urls(f'{root}/{archive}!{entry.name}',tarc.read(entry).decode('latin1')))
        sources.add(f'{root}/{archive}')
        for path in sorted(prefix.rglob('*')):  # 사용자 제공 느슨한 시나리오의 명시 외부 링크도 목록에 넣는다.
            if path.is_file() and path.suffix.lower() in ('.english','.html','.htm'):
                relative=path.relative_to(ROOT).as_posix()
                found=urls(relative,path.read_bytes().decode('latin1'))
                if found:
                    sources.add(relative)
                    links.extend(found)
        image_path=prefix/exe
        image=pefile.PE(str(image_path))
        literals=[]
        for match in re.finditer(rb'[\x20-\x7e]{5,}',image.__data__):  # 코드의 URL/보안 파일/리슨·루프백 문자열은 데이터 출처로만 기록한다.
            text=match[0].decode('ascii')
            if URL.search(text) or text in ('security.dat','127.0.0.1','0.0.0.0'):
                literals.append(dict(va=f'{image.OPTIONAL_HEADER.ImageBase+image.get_rva_from_offset(match.start()):08x}',text=text))
        imports=[]
        for library in getattr(image,'DIRECTORY_ENTRY_IMPORT',()):  # 실제 PE의 네트워크/외부실행 API를 정적으로 기록한다.
            functions=[item.name.decode('ascii') if item.name else f'ordinal:{item.ordinal}' for item in library.imports]
            if re.search(r'wsock|ws2_|wininet|winhttp|shell32',library.dll.decode('ascii'),re.I):
                imports.append(dict(dll=library.dll.decode('ascii'),functions=functions))
        sources.add(f'{root}/{exe}')
        editions.append(dict(root=root,configuration=config,links=links,peLiterals=literals,peImports=imports,archiveEntries=len(tarc.entries)))
    support=[]
    for relative in ('originals/R.exe','originalCD/R.EXE'):  # 원본 루트 서버의 외부 주소/진단 문자열도 실행 없이 따로 기록한다.
        path=ROOT/relative
        if not path.is_file():
            continue
        image=pefile.PE(str(path))
        literals=[]
        for match in re.finditer(rb'[\x20-\x7e]{5,}',image.__data__):  # 단순 메시지/템플릿을 현재 접속 기능으로 단정하지 않는다.
            text=match[0].decode('ascii')
            if URL.search(text) or re.search(r'(?:master|diag)\.[a-z0-9.-]+|security\.dat',text,re.I):
                literals.append(dict(va=f'{image.OPTIONAL_HEADER.ImageBase+image.get_rva_from_offset(match.start()):08x}',text=text))
        support.append(dict(source=relative,literals=literals))
        sources.add(relative)
    rootlist=ROOT/'originals/d/!rootservers.dat'
    sources.add('originals/d/!rootservers.dat')
    services=[dict(kind='retired-online-validation',original='WinMain00438dc0/438f32 security.dat → UserUpdated0054da8c; State3 Not Validated',product='검증 writer/온라인 접속 미구현. 제품은 상태 조회와 Not Validated 안내도 비활성화한다. 성공1을 합성하지 않는다.'),
              dict(kind='internet-master-root-directory',original='masterServer/!rootservers.dat/officialBasePort/구 nsLaunchC.exe',product='외부 서버 디렉터리/DNS/TCP 접속 미구현.3차는 로컬 서버만 사전 구현하며 옛 공식 서버를 복구하지 않는다.'),
              dict(kind='internet-ping-diagnostic',original='Ping*/PingName*/diagServer/diag*Port',product='외부 ping/진단 접속 미구현.목록만 보존한다.'),
              dict(kind='user-selected-external-help-update-mail',original='Help HTML/Config URLs/PatchUrl/autoOpen/프로토콜 명령; 오류 보고 URL',product='사용자 선택 Help 링크는 ShellExecuteW/프로토콜 실행에 연결돼 있다.이 함수는 임의 설정/시나리오 주소도 받을 수 있다.클릭 없이 자동 접속하지 않으며 업데이트 다운로드/오류 자동 전송은 미구현이다.'),
              dict(kind='local-game-root-server',original='TCP6799/루트6800/탐색UDP8998/내장HTTP80 기본off/0.0.0.0/127.0.0.1',product='현재 싱글/Edit는 프로세스 내부 전송이다.로컬 TCP/IP는3차 후순위이며 검증 성공을 요구하지 않는다.')]
    product=None
    product_path=ROOT/'cpppj/build/bin/Release/Netstorm.exe'
    if product_path.is_file():
        image=pefile.PE(str(product_path))
        dlls=[library.dll.decode('ascii') for library in getattr(image,'DIRECTORY_ENTRY_IMPORT',())]
        product=dict(sha256=sha(product_path),importedDlls=dlls,directNetworkDllImports=[dll for dll in dlls if re.search(r'wsock|ws2_|wininet|winhttp',dll,re.I)])
    report=dict(unit=684,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),kind='read-only static inventory',product=product,networkProbes=0,nativeCodeExecutions=0,gameExecutions=0,services=services,supportBinaries=support,rootServerList=rootlist.read_text('latin1').splitlines(),editions=editions,cppSources=list(CPP_PATHS),sha256={path:sha(ROOT/path) for path in sorted(sources)},limitations='제공된 두 판본/아카이브/느슨한 HTML·시나리오/현재소스의 정적 출처다. 현재 DNS·서비스 상태·실제 접속·간접 alias 전체 부재·모든 동적 URL의 고정값을 검증하지 않았다. 사용자 지정 URL/프로토콜 실행은 별도 명시 기능이며 로컬 서버 기능과 구분한다.')
    output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('외부 기능5분류, 원본 URL',*[len(edition['links']) for edition in editions],'접속/실행0')


if __name__=='__main__':
    main()
