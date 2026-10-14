# -*- coding: utf-8 -*-
"""정밀 창 제목 경계의 이름을 바로잡고 생산 세션 교전 검사를 추가한다."""
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 현재 동일 작업공간이다.
HISTORY=ROOT/'cpppj/hist/run25-base'  # 직접 영향 편집 전 원문 저장소다.
paths=(
    'cpppj/src/client/RawCampaignWorldRuntime.cpp','cpppj/src/client/RawCampaignWorldRuntime.h',
    'cpppj/src/o/RawCampaignWorldLoad.cpp','cpppj/src/o/RawCampaignWorldLoad.h',
    'cpppj/tests/CampaignWorldLoadTests.cpp','cpppj/tests/CampaignInstallSupport.h',
)  # 이번 정밀 결과로 경계 이름을 교정할 기존 파일이다.
saved=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))  # 최초 보존본은 덮어쓰지 않는다.
# 추가 기존 파일은 편집 직전 전체 바이트를 한 번만 보존한다.
for name in paths:
    if name not in saved['files']:
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes());saved['files'][name]=hashlib.sha256(target.read_bytes()).hexdigest()
(HISTORY/'capture.json').write_text(json.dumps(saved,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
# 실제 00460730은 창 제목 번역/설정/OS 쓰기만 수행하며 게임 사양을 재조회하지 않는다.
for name in paths+('cpppj/src/client/RawCampaignSession.cpp','cpppj/tests/CampaignSessionTests.cpp'):
    path=ROOT/name;text=path.read_text(encoding='utf-8').replace('refreshFortSpecification','refreshWindowTitle')
    text=text.replace('PATCH460730/CD4ca4e0의 현재 사양 재조회 경계다.','PATCH460730/CD4ca4e0의 현재 창 제목 갱신 경계다.')
    text=text.replace('원본 config 재조회와 카메라 재설정의 실제 소비자다.','원본 창 제목 갱신과 카메라 재설정의 실제 소비자다.')
    path.write_text(text,encoding='utf-8',newline='\n')
