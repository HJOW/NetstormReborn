# 현재 Guide의 State1→7 실제 전환 — 단위714

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 정밀 자료 기준일은 **2026-10-23**이다. 713의 요청/조기 반환 접합을 실제 실행하고 정상 플레이 전에 필요한 두 누락을 수정했다. **714 / 표시11.01**, 최종 EXE SHA-256은 `6590d506dcd8a2a7c017280b04330d78551aac9ff68d37fd2de8e7b4fd84f281`다.

## 실제 수정

캠페인에서 처음 `AskEditFort`→같은 Fort의 `ReadFort`를 입력하면 State1 전에 `편집 복귀의 실제 현재 Fort 파일 장치 없음`으로 종료했다. 파일 장치는 기존 Editor 설치에서만 생성됐기 때문이다. `EnsureEditorFortFiles`가 최초 실제 파일 요청 시 같은 작업본/이관 장부/현재 FileSpec의 장치를 만들고, `PrepareCurrent`로 보호 원본과 분리한 현재 작업본을 준비한다. 이후 State1은 장치를 재생성하지 않는다. World/Player/State 설치 성공을 이 함수에서 합성하지 않는다.

그 뒤에는 State1의 현재 Guide 요청이 State7까지 갔지만 `설치 전 실제 커서 요청`으로 종료했다. 전체 cleanup의 `Clear446e70`이 Session 대여만 보고 활성 플레이용 `Cursor()`를 호출했기 때문이다. `CancelClientStateCursor`가 **실제 C++ 커서 실행기 존재**를 조회해 기존 State 전용 `CancelCursorForState`를 호출하거나, 실행기가 없으면 같은 장기 전역의 `CancelClientCursorForState`/원문 Cancel·Prepare를 실행하게 연결했다. 공개 입력의 loaded 관문은 그대로 유지한다.

실제 HTML의 명령 콜백을 `QueueHtmlScriptCommand`로 모았고 검사 실행기의 `script-command`도 같은 이름표/80칸 문자열 고리/40칸 입력 큐를 사용한다. 수신 성공/전환 결과나 임의 Player·월드 상태를 쓰는 검사 연산이 아니다. 기존 문자열/알림 문법을 유지하며 [실행 도구](../../tools/cpp_guide_fort_state_smoke.py)가 이 경로를 사용한다. 이 경계만으로 후순위4차 로컬MCP가 완료된 것은 아니다.

## 확인한 결과

- **새1/기존2 집중3개, 실패0.** 실제 미설치 Editor Session이 Client를 대여한 상태에서 같은 전역의 Cancel/Prepare가 선택되는 새 회귀, 기존 실제 Session의 Champion/typed Subscriber/공유 My/Zone 요청, 기존 원문 요청144행 replay다. 새 회귀 전후 모두 공개 `Cursor()`의 거부가 유지된다.
- **build-v4 Release 경고/오류0.** v1/v2도 빌드는 통과했다. v3은 새 helper의 `ClientGameplayGlobals` 전방 선언 누락으로 컴파일 실패했으며 선언을 추가해 v4에서 교정했다.
- 최종 **Early1 / Tutorial1, Fort BridgeTheGap, 1024×768 창 모드**에서 실제 `AskEditFort`/목록 닫기/`ReadFort`→로그 순서 **State1→7→8→14**, 같은 Guide/PlayerKnown/일반 미션 모드·HUD를 확인했다. 같은 표면의 세대는1→3, 확정 phase14/현재0/next0이며 편집 모드를 공개하지 않는다.
- 재설치 뒤 비정지 게임 시각은 **10.522→11.927초**다. ESC 메뉴 serial 1143 열림1→닫힘0, 그 뒤 H는 기존 Early1 단계65→66을 진행시켰다. 자연 완주나 다리 배치·자원 목표 완료를 재현하지 않았다.
- 같은 프레임의 표시 논리 버퍼 BMP는 **1024×768, RGB202색**이며 덱/미니맵·지형/사제/신전 표시를 육안으로 확인했다. 물리 화면/원본 픽셀 전체 비교·실제 PCM 증거는 아니다.
- AGENTS·원본1394·CD425·기존 fixture811의 **2631개 전체 변경0**이다. fixture 추가/수정0, AGENTS는5867f35e 그대로다. 각 실제 실행도 전후 전체 보호 장부가 일치하고 허용 설정 파일은 복원했다.

## 실패와 검사 전제 교정

실제 게임은 **총5회** 실행했다. v1/v2는 위 두 제품 누락으로 종료1, v3/v4/v5는 게임 자체 종료0이며 모두 비타임아웃이다. 최종 성공을 다섯 번의 성공으로 합산하지 않는다.

v3은 캠페인1-1의 재접속/후반 설치까지 성공했지만 같은 Guide를 강제로 재사용한 뒤 `BadTeamDead` 안내가 떠 검사기의 즉시 phase4 기대가 실패했다. 보호 TARC의 `[Succeeded][BadTeamDead]` 다중 별칭을 확인했고, 이는 **계획하지 않은 승리 안내 관측1회**다. 해당 승리 경로의 추가 확인/Next Mission 입력은 하지 않았다. 허용 설정은 복원했으며 자연 승리나 승리 검증의 증거로 세지 않는다. 초기 보조 정규식이 줄의 첫 섹션만 읽은 잘못된 기록은 `scenario-sections.json`에 남기고, 다중 별칭을 읽는 `scenario-sections-v2.json`으로 교정했다. 최종 대상은 해당 승리 섹션이 없는 Early1로 바꿨다. 게임에 Start 재실행/AI 복구 같은 임의 우회를 넣지 않았다.

v4는 Early1 재설치와 시각 진행까지 통과했지만 H가 정상적으로 다음 B 안내를 열어 이후 ESC가 모달에 막혔다. 게임 코드를 바꾸지 않고 v5의 순서를 ESC 왕복→H로 고쳤다. v5의 최초 판정은 이미 소비·반납된 Zone의 임시 `Champion` 이름을 최종 Player 보고에서 찾으려 해서 실패했다. 임시 이름/Subscriber 이미지 자체는 기존 원문/실제 Session 검사로 대조하고, 실제 게임에서는 분배기의 순서와 Player/HUD/시각/입력을 확인하도록 검사기를 교정했다. **같은 v5 원시 기록을 읽기 재분석해 최종 통과했으며 재실행하지 않았다.** CPP 제품은 v3/v4/v5 모두 같은 최종 v4 빌드다.

v3 실행 전에는 보호 파일 읽기/프로세스 생성 관측이 지연됐다. 살아 있는 동일 Python/PID와 증가한 실제 읽기량을 확인하며 기다렸고 종료나 중복 실행으로 간주하지 않았다. 게임 PID가 기록된 뒤 실제 실행은 24.754초/종료0이었다.

실기로 새로 생성된 `originals-editor`의 Fort435개와 `originals-editor-state`의 장부2개는 모두 이번 실행 시각에 생성된 작업본이다. 종료 뒤 보호 원본/재분석 지점/목적 경로를 확인하고 `extracted/cpp-guide-fort-state-714/runtime-work/`로 이동 보관했다. 원본·기존 사용자 맵을 이동하거나 삭제하지 않았다. 저장소에 검사 작업본437개를 커밋하지 않는다.

## 남은 범위

평상시 State3는 제목 뒤 메인 메뉴로 이어진다. 제목 callback 뒤 current가 바뀌는 특이 분기는 새 도달 근거를 입증하지 못했으며 여전히 미연결이다. 이번에는 실제 재현한 State1/7 두 누락을 먼저 고쳤다. 나머지 Edit·지형/객체·시나리오 writer와 파일 실패/연속 Retry·Next·Abort, State3 특이 분기, 나머지 커서/메뉴/호환성은 계속 남는다.

실기는 앱 내 입력과 한 화면비의 짧은 확인이다. 물리 키·전체 포커스/픽셀·PCM·CD/Windows11 실기·전체34/전체 C++/CTest/ZIP·장기 누수·자연 완주를 새로 검증하지 않았다. 포획/희생/특정 적 공격·공중3종 전용·신규 MP/통신 검사는0이다. 우발적인 v3 승리 안내는 위와 같이 별도로 남겼다. 전체34/ZIP은645의644제품 그대로이며 전체 목표는 미완료다.

[완료 보고서](../../cpppj/guide-fort-state-20261102.json)의 입력/제품/근거 SHA와 `extracted/cpp-guide-fort-state-714/` 원시 기록을 따른다.
