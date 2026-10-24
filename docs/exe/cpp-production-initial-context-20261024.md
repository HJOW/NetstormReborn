# 실제 생산 생성 경계와 Client 현재 전역 — 2026-10-24 단위398

시작은 `a89fbc61 / 20261024-20 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일·원본 실행·에뮬레이션은0이다. 10.78 생산 생성자 `0043e640`, 예산 생성자 `0043e490`, 같은 공통 Gump 및 현재 전역의 연결을 수정했다. 전체 목표는 미완료다.

## 생성과 실제 게임 효과의 수명

[ClientGameplayGlobals](../../cpppj/src/client/ClientGameplayGlobals.h)는 현재 Player/두 owner/known, 커서, 내용물 검색, editor/post와 실제 두 Subscriber 참조를 Client 수명에 유지한다. Services→Session→[ProductionControlContext](../../cpppj/src/client/ProductionControlContext.h)가 같은 주소를 빌린다. 두 Subscriber는 prvalue 직접 구성으로 연결하며 참조를 값 복사하는 이동 생성에 맡기지 않는다.

단일 Use는 Session 소비자보다 먼저 획득하고 마지막에 반납한다. 완성된 실제 Session만 Publish하고 모듈 소멸 전 Withdraw한다. 중복 대여와 다른 Subscriber 소유자를 거부한다. 선택한 실제 게임 효과에서만 RequireSession을 사용한다. 초기 생성/배치/출력에 가짜 Session이나 알려진 Player 기록을 만들지 않는다. Acquire의 초기화와 반납은 기존 C++ Session 설치/소멸 의미이며 전체 원본 State 순서의 복원 완료가 아니다.

물리의 `BridgeConnectState`도 [RawSurfaceStorage](../../cpppj/src/client/RawSurfaceWorld.h)에 두고 실제 월드가 검증한 자기 판본의 종류를 같은 주소에 쓴다. 초기 생산은 같은1078 물리의 bridgeType을 읽는다. 기존 두 판본의 월드 검사는 유지한다.

## 원문 생성 경계

[RawProductionControls](../../cpppj/src/client/RawProductionControls.cpp)의 Construct는 실제 부모와 예산·배경3·격자2·구분선을 합친8노드를 만든다. 부모40폭/현재 전체 높이, 예산0시계·금액·점멸, command/flags/idle, 실제 root/budget serial, 정책0, 벽시각과 옵션BYTE1의 슬롯 수를 적용한다. 부모의 자식 포인터와 두 시계/금액/신전/슬롯은 같은 논리 파생 본체에 보관한다. C++ 객체의 실제 메모리 ABI를 원본0xe4 BYTE와 같다고 판정한 것은 아니다.

예산의 `Storm Power Available`/`moneyGump`, 격자와 배경의 `Production Window`/`teleportView`를 공통 소유 문자열에 연결한다. 같은 scalar 반납이 문자열을 해제한다. 격자의 기존 기본 도움말0반환 훅도 공통 Help로 연결했다. 문자열 교체와 실제 격자 우클릭→Help 클릭→주제 토큰 전달을 검사했다. 검토 중 기존 전용 HTML 링크 범위와 새 help 토큰 범위가 겹칠 수 있음을 확인하고 최종 `61100001..61ffffff`로 분리했다. 정적 주제61000001..5/기본 HTML61010000 및 전용 HTML62000000..6fffffff와 구별한다.

footer는 기본 배경 대용물을 없애고 실제0x11c `HudDividerNode`를 사용한다. [HudDividerGump](../../cpppj/src/client/HudDock.h)로 기존 Native 검증 생성/슬레이브/부착 몸체를 공유하고 같은 공통 노드·Overlay·가상 입력·Draw에 연결했다. 이 생성자의 축은0이므로 드래그 기능 완료를 뜻하지 않는다.

Construct에는 현재 호스트 Open의80폭/하단80예약·루트 도킹·즉시 생산 Sync를 넣지 않는다. 기존 실제 플레이 경로는 Construct 뒤 그 후처리를 계속 실행한다. maintenance는 실제 현재 기술/모드에 도달했을 때 연결한다. UI/Form/maintenance는 Session보다 먼저 Close해야 하며 한 생산 UI를 다른 Session에 걸쳐 유지하는 수명은 이번 완료 범위가 아니다. 부분 생성의 미부착 자식과 페이지 전체 교체도 같은 공통 등록표/Kill/Cleanup으로 정리한다.

## 실제 검증

- build1(81114)은 잘못된 `HudDividerInputHooks.node`와 파생 포인터 const_cast의2오류 및 후속 C4390 경고로 종료1이었다. 실제 필드 divider/base와 같은 파생 주소를 사용하도록 수정했다. build2(63995)/3(9536)/4(59027)/5(87841)는 모두 종료0·관측 컴파일 경고오류0이다. 빌드 중 보강한 검사·기본 Help 연결·주석·토큰 영역 교정 뒤 최종 build5만 최종 판정에 사용한다.
- 최종 집중2(신규7+기존59) **66검사/실패0**이다. 초기 생성/Player미할당/벽시각1회/추가 config0회/배치/실제 픽셀/행 padding/반복 생성/부분 번역 실패 정리/실제 전역 alias·중복 거부, 실제 두 Session·부분 생성과 외부 Kernel, 격자 Help 클릭과 문자열 해제를 확인했다. 보존한 Native9자료의 HEAD 원시 BYTE와 기존 HUD/구분선/전체 Uber/격자/항목/유지/단축키도 대조했다. 새 생산 전체 생성자의 독립 x86 실행 fixture는 생성하지 않았다.
- 최종 GUI2(실제 도구 프로세스 기준 **194.12초**)는 종료0이다. Early2 A~I, 사제의 신전1/워크샾1, 발사기4→3, 회수1/환불75/SP10000→3075, 자연 완료→Tutorial3 안내·실제SP2000이 통과했다. 상태 주입0/기존 완료1→1이다. `shared_gameplay_globals` **78행 모두 다섯 값1**, 물리행 **78개 모두 네 정체성1/대여활성**, 물리 대여1→2다. 다음 예산의 단계 갱신 표시와 실제 SP를 구별한다. 두 목표 BMP를 직접 확인한다. 전체 픽셀/영구 진행0→1저장/Early3 이후 완주 증거로 확대하지 않는다.
- 최초 집중1도66/0, GUI1도194.23초/자연 완료·다음 미션/동일 전역·물리로 통과했다. 검토 뒤 토큰 영역을 교정해 최종 집중2/GUI2를 다시 수행했으며 이전 바이너리 관측을 최종 SHA로 합산하지 않는다.
- 전체 CTest1은 **2702검사/실패0·종료0·300.62초(wrapper)**다. 최종 집중/GUI/전체의 제품·검사 SHA는 같고 원본1394개는 기존 기준과 동일하다. AGENTS/originals/CD/Native 추적 변경0이다.

최종 제품 SHA256: `8ccdcfadd7b84978df46223807d65ba776d75c75107e9da0f78d366b7751150e`
최종 검사 SHA256: `16ba8391e9891c68373993b8d15ed81b6f9a9144cac0ce09f7cd54ee049aa2e8`

[제품 보고서](../../cpppj/production-initial-context-runtime-20261024.json)에 코드/검사26파일, 보존 Native9파일, 원시200파일의 SHA와 최초 실패/정적 PE2블록/이전·최종 관측을 기록한다. 원시는 `extracted/cpp-production-context-398-*`다. 정적 PE는 moneyGump 주소5069b0/예산0 double500460/4c6c50 호출을 읽었으며 실행 관측과 구별한다.

첫 완료 작성 wrapper는 PowerShell 리디렉션이 git의 개행 경고를 NativeCommandError로 기록해 종료1이었다. 제품 보고서·문서·인계는 모두 작성됐다. 작성 중이던 final1.log 한 개의 중간 SHA 차이를 확인하고 닫힌 로그의 최종 SHA만 갱신했다. 최초 로그를 보존하고 전체 SHA/UTF-8/링크/stage 감사로 검증한다.

## 이어서 할 일

실제 Client의 새 Uber 교체와 전체 NSInfo/Chat/HTML 공장을 같은 공통 저장소에 연결해야 한다. 기존 Open 배치와 새 원문 Construct를 혼용해 전체 Uber가 연결됐다고 세지 않는다. 전체 State/Core/Pool 초기화·정리 순서, 나머지 Runtime/Form/Graph/장부/Player/SP 소유자, Edit/시나리오 전체와 모든 Early/캠페인/Demo 사용자 완주·완전 동일성은 남아 있다.393의 Wide SaveOptions errno22/doserrno1224도 이번에 재현/수정하지 않았다. Linux/멀티플레이·다른 판본의 추가 목표와 구별하며 /goal은 활성 상태로 계속한다.
