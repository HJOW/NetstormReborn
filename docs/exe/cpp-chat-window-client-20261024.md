# 실제 ChatWindow 공장과 Client 공유 로그 — 2026-10-24

현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 소스 SHA256은 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`다. 새 전체 디컴파일·원본 게임 직접 실행·GUI 실행은0이다.

## 실제 공장과 수명

[ClientChatWindowGlobals](../../cpppj/src/client/ClientChatWindowGlobals.h)는 같은 Client의 current/registered/completionReady·색256/배경256/실제 TYPE 프레임 코드 수와 SHP 높이8..16의 아이콘 목록·공유128줄 로그·파일 토큰을 유지한다. 표시 부모를 교체해도 같은 로그/명령 등록/목록 주소를 사용한다. TYPE+114는 프레임 코드/클러스터 수이며 추가 SHP 레이어 수와 구별한다.

[RawMenuChat.inc](../../cpppj/src/client/RawMenuChat.inc)는 원문 ChatWindow::Create/Construct/Handle/Attach를 기존 실제 공통 Gump/돌 버튼/Console/문자 물리/ScrollingText/StyleText/Slider/controlled 메뉴에 연결한다. 실제 네 자식과 하위 **24노드/세 Slider**를 생성하고 스크롤 노드 자신의 버퍼를 반납한 뒤 Client 공유 버퍼를 owns0으로 빌린다. Console은80문자 제한·단일 줄·초기 숨김·실제 자동완성 callback 토큰을 원문 순서대로 쓴다. 가짜 기본 자식/문자 버퍼로 대신하지 않는다.

최초 clear/quit은 기존 ClientDotCommandGlobals의 같은 실제 인접 표에 등록해 say1→clear/quit 포함3이 되며 다음 창 생성에서 반복하지 않는다. 원문 소멸자는 현재 창이 다른 창이어도 current를0으로 쓴다. 동시에 두 창을 만든 뒤 기존 것만 반납해 이를 검증했다. 로그는 창/페이지 교체 중 유지하며 공통 UI 대여를 끝낼 때 실제 StyleTextBuffer 조각을 반납한다. 이는 아직 Native CRT/startup/State 순서의 완료가 아니다.

helper 메뉴의 정적/벡터 C문자열은 Client 수명의 안정된 호스트 토큰으로 빌린다. 80칸 임시 입력 고리로 수백 항목을 덮어쓰지 않는다. 실제 전체 helper 메뉴에서500개 넘는 서로 다른 인자/NULL 구별·색상 삽입과 임시 입력100회 뒤 보존을 검사했다. host 문자열 별칭은 게임 계산 결과 캐시가 아니다.

## callback 원문 대조

새 [독립 원문 도구](../../tools/decomp_chat_callbacks.py)는 PE42f680(clear)/42e520(quit)/42f590(append)의 실제 본문·분기·strlen·RET를 실행해 **29행**을 최초 작성했다. 이전 자료를 갱신하지 않았다. 이후 --verify로 같은29행을 읽기 전용 재생했다. 원문 하위 ReplaceText/RequestBottom/파일 valid·write/사건 예약은 명시 장치이며 원문 callback 중 current/자식/파일이 바뀌는 입력을 분리했다.

실제 C++ 공장/공유 로그/자식/예약에 같은 입력29행을 실행했다. clear가 저장한 기존 root의 현재 text를 두 번째 호출에서 다시 읽는지, current0이면 아무것도 하지 않는지, append가 유효성 검사 후 현재 file을 다시 읽는지, quit이 현재 전체DWORD 사건과0/0을 예약하는지 대조했다. clear의 첫 대상은 실제 LowerBody의 dirty 효과로 관측한다. 원문 clear는 완료 줄만 지우고 미완료 끝 줄을 보존한다. 일반 파일0 callback은 장치를 요구하지 않으며 선택된 미연결 장치는 명시 거부한다. 416명령 위치 수에는 장치/STOP 훅도 포함된다.

## 검증

- build1(45966)은 const scalar 경계의 비const Release 호출 C2662로 종료1이었다. 같은 실제 Chat 저장소의 scalar 반납으로 고쳤다. build2/3/4 및 최종 빌드는 종료0·관측 컴파일 경고오류0이다. 최초 집중1은52/0이었다. 추가 Native29행에 맞춰 파일 재조회/현재 사건 예약을 보강하고 최종 집중은 **53검사/실패0**(실제 Chat 통합7 포함)이다.
- 보존된 Chat/편집/기하 Native **7002행**은 현재 PC/PE/정밀 소스 SHA를 고정한 재생에서 결과/명령 수까지 전부 같고 변경0이다. 새 callback29행과 합쳐 원문 관측은7031행이다. 기존 VM main/쓰기 경로는 호출하지 않았다.
- 전체 CTest는 **2726검사/실패0·328.57초(wrapper)**다. 최종 집중/전체의 제품과 검사 바이너리는 같다. 최초 전체도2726/0이었으며 표현 정리 뒤 최종 산출물 확인 경과는 terminal/원시 로그에 남긴다. 실제 최종 ChatWindow BMP를 직접 확인했다. 이는 기존 게임 화면의 full Uber 도킹/전체 게임 픽셀/실제 파일·전송 완주 증거가 아니다.
- originals1394개가 같은 보호 기준과 일치하고 originals/CD/기존 Native 추적 변경0이다. 작업 중 보존한 AGENTS 네임스페이스 주석 규칙 두 행의 외부 변경은 이번 stage/커밋에서 제외한다.

최종 제품 SHA256: `48eb8e14d27865428fecff426568dc286dd850a81bf8de54cdc03ab9b55a6594`

최종 검사 SHA256: `13f53384e429e8b318efba7809e6e2d4c44d070dbcef98ef4445c7aa6317814e`

[제품 보고서](../../cpppj/chat-window-client-runtime-20261024.json)에 변경 소스14/참조18/보존 Native5/새 Native 도구·자료·보고서/닫힌 원시116파일 SHA를 기록한다. 원시는 `extracted/cpp-chat-window-401-*`다.

## 다음 접합

**실제 기존 게임 화면에는 아직 이 Chat 공장을 설치하지 않았다.** 전체 Native Uber의 생성/도킹/State 경계에 연결해야 하며 임의 레거시 위치/숨김 기본 노드를 만들지 않았다. HTML 전체 공장, Production/Minimap의 기존/새 제공자 동시 소유와 full Uber/State/Core/Pool 순서가 이어진다. Win32 clipboard·recipient/42e540 Tab·42da00 Chatmanager/identity와 실제 파일 장치는 선택 경계로 남는다. 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오 전체·완전 동일성 및393 Wide SaveOptions 원인은 미완료이며 /goal은 활성 상태다.
