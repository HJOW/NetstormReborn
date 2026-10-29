# dotnetpj R3k-5 — 도움말 열기/요청/이력/입력

2026-10-29, HJOW-Athlon. AGENTS.md·현행/공통 인계를 다시 읽었다. 직전 목표 턴은 전용 본문/내장 Tell2단위의 구현·검증·커밋을 완료한 진행이며 시작 HEAD는 `2aef224e`다. 현재/마지막 전체 정밀 디컴파일 PC도 HJOW-Athlon(2026-10-23)이다. cpppj QuickHelpControl/QuickHelpRequest.h/.cpp·대응 검사와 기존 공통 등록/트리/입력/수명을 읽었다. 게임/창·PE 에뮬레이터·금지된 플레이·멀티플레이 검사는 실행하지 않았다.

## 1. 구현

열기는 sticky 팝업 닫기→현재 두 등록 ID 조회→현재 모달 관문 순서다. 모달 조회는 필요한 경우 두 번 독립 호출한다. Help가 없을 때만 전용 생성→현재 화면/폭의 가운데 x/40y 부착→부착 뒤 현재 depth-76을 자식까지 더한다. 같은 모달/가상 입력을 거쳐 Help 또는 Request 명령을 전달한다. 다른 모달에 막혀도 이미 있던 Help 토큰을 반환하며 도달하지 않는 생성/부착 장치는 요구하지 않는다.

요청은 type0일 때만 원문80BYTE 한도·두 번의 type/help 접미 비교·현재 signed 타입 조회를 수행한다. 접미를 지운 복사본에 %s→sun%s→wind%s를 순서대로 적용하고 음수일 때만 다음 조회를 한다.0은 유효한 조회 결과다. 이력/Show에는 원래 주제를 보내고 Show 뒤 flags, Push 뒤 현재 x/y를 다시 읽는다. 원본 버퍼 앞을 읽는 짧은 입력은 cpppj처럼 명시 거부한다.

이력은 원문 첫 NUL/ASCII 비교로 머리의 연속 중복만 생략한다. 중간 중복과 빈 주제는 유지하며 Back은 현재 한 항목을 제거한 다음 머리를 반환한다. 공용 팝업은 같은80BYTE를 사용하고 null/비null 빈 문자열/짧은 덮어쓰기를 구별한다. 범위 거부 전 명령/바이트를 바꾸지 않는다.

입력은 실제 좌표 변환을 가장 먼저 수행한 뒤 바깥 좌누름·일반 키·요청/링크/현재 버튼 순서다. 현재 버튼의 ID 대신 serial을 비교하며 PageUp/Down·방향키·수정자/해제·signed 인자 칸의 TYPE 판단을 원문대로 유지한다. 마지막 Back에서는 이전 명령을 보관→Kill→현재 팝업 인자를 읽는다. 큐의 두 인자는 같은 실제 BYTE 영역을 빌리는 ReadOnlyMemory이므로 콜백 중 변경도 두 인자에 동시에 보인다. 삭제 뒤 창 필드를 다시 읽지 않는다. 학습은 실제 소비자를 연결해야 하는 요청 콜백이며 게임 행동을 재현하지 않았다.

현재 Help 노드의 부모 사건/직접 입력 가상 분배와 같은 실제 좌표/지연 수명 바인딩도 작성했다. 다른 파생의 이전 분배는 보존한다. 전용 생성/표시·명령 소비자를 빈 성공으로 대체하지 않는다.

## 2. 독립 원본 대조

읽기 전용 `quick-help-control-x86.tsv`150행과 `quick-help-request-x86.tsv`985행을 Link했다. 총1,135행이 최초부터 일치했다. 앞2열만 실행 입력으로 쓰고 마지막 열의 원본 관찰은 비교에만 쓴다. 열기 반환/깊이·이력·공용80BYTE·현재 버튼/페이지/팝업과 외부 효과 순서, 요청 원문/두 접미/세 조회·Show 뒤 flags·Push 뒤 signed0/NaN payload 좌표를 전수 대조한다. 원본 기대값은 수정/재생성하지 않았다.

fixture118개, 새 SHA 접두부 `461a479171cf`/`8126552d1d05`, reflected-commit은 `f95a8db2...` 유지다. 포트253행(L1 133/port120)이며 새4행 기준은 `2aef224e`다.

## 3. 실제 연결 검사

집중8개가 최초부터 통과했다. 원본 전수2개 외6개는 실제 공통 등록/트리·부모/직접 입력·dialog 전달/요청/이력/Back·현재 popup/원시 InputQueue·자식 우선 지연 수명, 두 현재 모달 조회/부착 뒤 depth, 큐 중 같은 가변 BYTE alias와 이전 명령, 팝업 null/빈 문자열/원문/물리 한도와 ASCII 이력, 잘못된 접미 버퍼/직접 TYPE 원문, signed TYPE/선택 장치·NaN 바깥 판정을 확인한다.

실제 조립의 창 생성/Show/기본 Help와 타입 조회·문자열 토큰 공급은 명시 외부 경계다. 전체 전용 창 생성/본문·등록 조회 소유자와 RawMenuControls.SetModal의 모든 깊이/선택 계산을 복원했다는 뜻이 아니다. 공통 등록 표/트리/실제 AddDialog/Route/RemoveDialog/Kill/Cleanup과 원시 큐는 기존 제품 코드를 사용한다.

전체 **1,963개(Assets363+Core1030+Client570) 통과**, 실패/건너뜀0이다. 전체 Release 빌드도 경고/오류0·종료0이다. Git 제외 로그는 `extracted/dotnet-quick-help-control-native-initial-20261029.log`·`dotnet-quick-help-control-focused-initial-20261029.log`·`dotnet-quick-help-control-tests-20261029.log`·`dotnet-quick-help-control-build-20261029.log`다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 D1 전체 화면이나 미션 플레이 완료 판정은 아니다.

## 4. 다음

QuickHelpWindow의 실제 자식 생성/배치·본문/그림 갱신과 HtmlWindowGump/Factory/Metadata/Fit를 잇는다. 현재 전체 자산/ConfigInterface/RawMenuControls 상위 소유·모달/링크 소비자/음향·호스트와 R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 전체 목표는 계속 진행 중이다. cpppj·원본/CD·AGENTS.md·공통 인계·외부 TODO.md는 수정하지 않았다.
