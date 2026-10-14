# -*- coding: utf-8 -*-
"""최종 검사 성공을 확인한 뒤 단위25 인수인계와 결과 요약을 마감한다."""
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 동일 작업공간이다.
data=(ROOT/'extracted/run25-all.log').read_bytes()  # 로그를 수정하지 않는다.
all_log=data.decode('utf-16' if data.startswith(b'\xff\xfe') else 'utf-8-sig')  # PowerShell 인코딩이다.
assert '1135 test(s), 0 failed check(s)' in all_log and '100% tests passed' in all_log
seconds=re.search(r'Total Test time \(real\) =\s*([\d.]+) sec',all_log).group(1)  # 실제 CTest 완료 시간이다.
path=ROOT/'LEFT_JOBS.md';text=path.read_text(encoding='utf-8')  # 기존 인수인계 본문을 보존한다.
start=text.index('## 2026-10-15 진행: 단위25');end=text.index('## 2026-10-15 완료: 단위24')
section=f'''## 2026-10-15 완료: 단위25 실제 캠페인 세션·GameWorld 상태 연결 (`HJOW-Athlon`)

- [x] AGENTS/양 LEFT를 다시 읽고 기준 `a24db9d`에서 시작했다. 현재 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 직접 영향의 이전 전체 원문13개를 `run25-base`에 보존했다.
- [x] production `RawCampaignSession`이 동일 Core/Pool/Kernel의 실제 설치·지형·소유권·경로·전투·HP·감시·소리 모듈 수명을 소유한다. 검사 헤더에 의존하지 않으며 실제 TYPE 원문(검사 전용74 변경 없음), 실제 재시드/Unix 초/공유 CRT, 단일 SoundList를 사용한다. 누락된 선택 소비자는 성공으로 대신하지 않는다.
- [x] GameWorld의 캠페인 생성자와 같은 raw SID/위치/프레임/HP 화면 뷰를 연결했다. 캠페인에서 별도 객체 보행 계산은 실행하지 않는다. 내 사제의 선택/좌클릭이 같은 Graph/BFS/Host54를 만든다. 1-1/1-2 설치, paused 프레임, 실제 이동 뷰 동기화 검사와 production 교전 검사가 통과했다.
- [x] production 1-1 명시 재배치 입력의 첫 피해 frame173/HP1 frame178/종료 frame898에서 실제 충돌 자산·보호막 소리·총 감시 해제·잔여 투사체 삭제를 확인했다. UI/장치/송신/no-fencemark는 검사 외부 입력이며 실제 GUI 서비스 완료를 뜻하지 않는다.
- [x] PATCH `00460730` 정밀 export1개를 읽어 이 경계가 사양 재조회가 아니라 **창 제목 갱신**임을 확인하고 이름/주석을 교정했다. CD 대응 주소는 이전 근거이며 새 export/직접 PE 실행 입력은 각각1/0이다. 기존 Native 관찰은 재합산하지 않았다.
- [x] 최종 Release 경고/오류0, 새 집중4개 성공, 전체CTest **1/1·내부1,135개·실패 체크0**({seconds}초). [상세 단위25](docs/exe/cpp-run25-20261015.md), `run25.json`, `tools/check_campaign25.py finalize/verify`로 전체 소스/fixture/실행/이력/로그 SHA를 고정한다. 경로 감사는 기존 ignored 이력 파일명41자1건 외 신규 위반0(최장178자)이며 기존 이력은 변경하지 않았다.
- [x] 인수인계 후 로컬 **`20261015-10 실제 캠페인 세션과 GameWorld 상태 연결 검증`**으로 커밋한다. dotnetpj/AGENTS/보호 원본 변경 및 push는 없다.
- [ ] **다음 우선 작업:** UberGump Battle 거부를 제거하기 위한 실제 창 제목/카메라/로컬 단독 송신/UI·음향·owner 통지 서비스 연결. 상위 Ready/login·9 Player/미션 돈·기술·동맹·경제/생산, fencemark/다른 파생 삭제/운반/의식/AI/Guide/승패와 화면 깊이·선택 동일성은 미완료다. Early Missions/1-1~1-2 실제 플레이·완주 목표는active 유지하며 DirectDraw 전체화면 오류 수정은 제외한다.

'''
path.write_text(text[:start]+section+text[end:],encoding='utf-8',newline='\r\n')
doc=ROOT/'docs/exe/cpp-run25-20261015.md';text=doc.read_text(encoding='utf-8')
text=text.replace('최종 Release와 집중/전체 검사 수치는 마감 후 `cpppj/run25.json` 및 `extracted/run25-*.log`에 고정한다.',
    f'최종 Release 경고/오류0, 집중4개 성공, 전체CTest 1/1·내부1,135개·실패 체크0({seconds}초)다. 결과는 `cpppj/run25.json` 및 `extracted/run25-*.log`에 고정했다.')
doc.write_text(text,encoding='utf-8',newline='\n')
print('단위25 인수인계 마감:',seconds,'초, 새4/전체1135 성공')
