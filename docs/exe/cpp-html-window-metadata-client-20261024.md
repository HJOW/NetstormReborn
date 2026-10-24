# 실제 HTML 메타·메뉴·문자열·시간 접합 — 2026-10-24

현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호 PE SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 소스 SHA256 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`가 같다. 새 전체 디컴파일/원본 게임 직접 실행/게임 GUI 실행0이며 보호1394파일/AGENTS/기존 Native 추적을 보존했다.

## 원문 메타 본체

[HtmlWindowMetadata](../../cpppj/src/client/HtmlWindowMetadata.cpp)는 **4c83b0**의 ClearButtons→9regex 고정 우선순위→원시 캡처/명령 조회·첫ASCII숫자·32비트atol/같은80칸 고리→효과→원시 줄 제거·부분치환→처리 수 반환을 복원한다. Button/ButtonInstant/Menu/Checked/Timeout/OnExit/Key/Input/Zsend 순서이며 문서 내 앞 위치보다 종류 우선순위가 먼저다. 매 제거 뒤 Button부터 다시 검색한다. 캡처는 Trim/따옴표 제거를 하지 않으며 FFFFFFFF 명령도 인자 처리 후 보존하고 조회0만 진단한다. Key는 원문처럼 해석/고리 소비 뒤 별도 컨트롤을 만들지 않는다.

원문 버튼 처리 수는 실제20개 한도와 별개다. 메뉴 최초 열은 BeginVertical 효과 뒤 현재 열 수를 증가시킨다. Checked의 enabled는 원시값/현재 두 전역을 단락 평가한다. SOLO/FORT는 실제 원문 분할/캐시 무효화/키 전환·SOLO 정상복구를 사용한다. TECH/THEMETECH/@/#/%는 해당 원문 인자와 시점의 명시 하위 경계다. Timeout은 signed초+벽시각과 명령·인자를 쓰고 정상 idle에 등록한다. 없는 지시문은 이전 Timeout/OnExit/열 수를 지우지 않는다. Input은 창 호출 뒤 현재 MenuJustClosed 원시 사건을 예약한다.

줄 삭제는 regex 끝이 아닌 **일치 시작부터 다음 LF/$/NUL**이다. %의 보호 PE4c8b77..4c8c49를 직접 확인했다. 원문은 줄 제거 후 병합 문자열을 만들지만 **반환 replacement 길이만 원래 버퍼 앞에 REP 복사하고 NUL/전체 길이는 복사하지 않는다.** 따라서 통상적인 문자열 insert로 대체하지 않았다.8192BYTE 전체의 NUL 뒤 바이트까지 독립 지문으로 대조했다. 캡처80/256BYTE 뒤 NUL 오버런에 해당하는 범위 밖 입력은 호스트에서 명시 거부한다.

## 실제 공통 UI 연결

[RawMenuHtml](../../cpppj/src/client/RawMenuHtml.inc)의 ProcessHtmlMetadata는 실제 전체 HTML 부모/돌 버튼/controlled MenuGump·Checked/같은 Client/80칸 ScriptStringRing/정상 GumpLifetime.SetIdle/현재 벽시각/원시 InputQueue를 사용한다. [RawHtmlWindowContext](../../cpppj/src/client/RawHtmlWindowContext.h)는 같은 실제 명령·맵 캐시/파일 키/현재 선택 이름과 선택된 하위 서비스를 빌린다. 기술·목록·입력·전송·링크를 임의 빈 성공으로 채우지 않는다. 부분 생성된 미부착 메뉴도 HTML 소유 목록에 넣고 Close 때 실제 상단 controlled 칸을 반납한 뒤 같은 자식 우선 수명으로 정리한다.

[실제 결합7검사](../../cpppj/tests/HtmlWindowMetadataClientTests.cpp)는 우선순위/실제20버튼/원시 공백 명령, 일반·비활성 Checked 메뉴/전체 Fit·스택 반납, 실제 idle 기한→raw 예약→Kill→OnExit, 실제 보호 SOLO/FORT 캐시 모든 행·같은 고리 인자·현재 선택·키 복구, 미연결7분기 실패 뒤 메뉴/스택 정리·재사용, Key82개 소비와80칸 이전 토큰 해제, % 명시 하위 반환의 물리 부분복사를 확인한다. 이는 전체 파일 목록 생성/기술·입력 창·패킷 구현을 완료했다는 뜻이 아니다.

## 독립 관측과 검사

[도구](../../tools/decomp_html_window_metadata.py)는 실제4c83b0 전체·Clear/Add·캡처/시작·String길이/줄제거·부분REP복사·RET를 **64입력×두 정밀도=128행**으로 관측했다. 실제 regexp compile/search는 Pythonre 명시 경계다. 명령/CRT/고리/문자열·컨트롤/메뉴·맵·기술/시각/Idle/Input/Packet/메모리이동도 명시 장치다. 실제 원본 게임 전체 실행이나 실제 regexp엔진 전체를 검증했다고 주장하지 않는다. 첫 도구 실행의 미선언 _stricmp를 명시 ASCII 비교 경계로 보완했고 두 번째 실행을 최초 근거로 사용했다. C++ 전체 물리 지문/현재9필드/20행/장치 trace128행이 일치한다. 두 x87 정밀도 차이0·170132명령 위치에는 장치/STOP 포함이다.

새128행과 보존 HTML498행, 합계 **626행**을 읽기 전용 재생해 같음을 확인했다. 최종 집중 **120검사/실패0**, 전체 CTest **2755검사/실패0·322.93초(wrapper)**다. 최종 빌드 종료0·관측 경고오류0이며 집중/전체에서 제품·검사 바이너리가 같고 originals1394개가 같다. 초기 결합 검사의 수정·빌드별 상태는 원시/제품 보고서에 남겼다.

제품 SHA256 `4fd7d140266915ff9a10fc7ab933850d3633f30d591da893394fa836664f676b`

검사 SHA256 `f3a4f2d4353fc1777f77ffcbd354a62aea15719dc201f0714b9341f2f1086963`

[제품 보고서](../../cpppj/html-window-metadata-client-runtime-20261024.json)에 소스10/참조12/보존Native7/새Native3와 종료 원시86파일 SHA·검사 결과·경계를 남겼다. 원시는 `extracted/cpp-html-meta-405-*`다.

## 다음 접합

**factory4cf270 전체·현재 HTML 링크·Config 치환/같은 파서 두 번 처리·기술/@/#/%/입력 장치와 기존 LoadingFields 부분 미션 안내 교체**가 남는다. 기존 게임의 full Uber에는 새 전체 HTML 공장을 아직 설치하지 않았다. Production/Minimap 동시 제공자·새 부모/자식 먼저 생성→이전 반납·State/Core/Pool/Win32 경계 및 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오 전체·완전 동일성/393 Wide SaveOptions 원인이 남으며 /goal은 활성이다.
