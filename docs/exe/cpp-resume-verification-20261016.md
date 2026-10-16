# cpppj 목표 재개 시 이관 검사

2026-10-16, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**이다. 시작 커밋은 `c2b4401`이며 시작 작업 트리는 깨끗했다. 새 `/goal` 요청에 따라 단위75에서 취소·이관했던 감사를 먼저 수행했다. 이 기록의 실행 파일은 새 썬 바리케이드 제품이 포함되기 전 기준선이다.

`AGENTS.md`, 두 LEFT_JOBS, 단위75 중단 인수인계를 읽었다. 오늘 이 PC에서 생성한 정밀 산출물이 있어 전체 재디컴파일은 반복하지 않았다. 훨리베이스 관찰기 `--verify`는 기존 고유1,734입력과 SHA를 통과했다.

요구에너지 관찰기에서는 네 파일의 SHA가 달랐다. 세 제품 파일은 보고서의 과거 CRLF 바이트를 현재 LF로 바꾼 것과 완전히 같았고, 테스트 한 파일은 체크아웃 CRLF를 LF로 바꾸면 기존 Git/보고서와 완전히 같았다. 줄바꿈 이외 변경은 없음을 SHA로 확인했다. `.gitattributes`에서 관련 다섯 소스/보고서 경로를 LF로 고정하고, `type-required-energy.json`에 이전/현재 SHA를 보존했다. 같은2,352입력과 fixture를 유지하며 `--verify`를 통과했다. Python 구문과 `git diff --check`도 통과했다.

`cmake --build cpppj/build --config Release --parallel 4`는 종료0이며 경고/오류가 없다. 로그는 `extracted/goal-resume-20261016-build.log`다. 이 실행 파일로 전체 CTest를 새로 수행했다.

- 내부 검사: **1,321개 중1,320개 통과·실패1**.
- CTest: **0/1 통과·종료8**, 검사132.92초·전체133.21초.
- 유일한 실패: `CampaignMissionStart_ProtectedEightMissions`의 **Tutorial6 Load / SID15006 / TYPE136 / VT00509470** 미복원 Pop 후처리.
- 로그: `extracted/goal-resume-20261016-ctest.log`, `extracted/goal-resume-20261016-tests.log`.

기존 훨리베이스5개와 그 밖의 기준선은 통과했다. 단위75의 취소된 로그를 완료 근거로 사용하지 않았다. 자연 플레이나 원본/클론 GUI를 실행한 검사는 아니다.

이후 진행 중인 썬 바리케이드 복원은 별도 제품 단위다. 그 변경은 이 기준선 실행 파일에 포함되지 않았으며 위 결과를 새 제품의 통과 근거로 사용하지 않는다. Early Missions·캠페인1-1/1-2·데모3개 자연 완주와 동일성 목표는 계속 미달성이다. 보호 원본/AGENTS/dotnetpj 수정·원격 push·전체화면 DirectDraw 오류 수정은 없다.
