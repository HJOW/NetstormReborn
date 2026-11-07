# 1136 — 메타 좌석 참가·항해 입력과 실제 전송/Rig 연결

제품1136 / 표시·파일·제품버전11.01이다. 이전 단위는 `a949d135 / 20261108-104 CP`다.

`0042bd10`의 fort 폭/높이·현재 칸 크기·PE의 두8쌍 표·전투 TYPE의 C 프레임/기존 좌석 순서·포구 signed WORD·기본 칸 크기·현재 전투 원점 계산을 복원했다. 각 float 저장을 다음 단계가 다시 읽으며 실제 SHP frameCheck/복귀형 진단을 재사용한다. 좌석 범위 밖은 호스트의 실제8쌍 물리 접근 오류로 구별하고 임의 clamp하지 않는다. MSVC의 long double은 double이므로 x87 80비트 중간 정밀도의 전체 입력 일치는 이번 정적 이식으로 입증하지 않았다.

`0042be00`은 BYTE 전투·하위4비트 좌석→현재 subscriber의 첫 참가자→좌석 계산→같은 참가자9c/a0 쓰기→고정14BYTE/1eh TCP11 전송→실제419520 Rig 순서다. `0042beb0`은 위치/0/0→전송→현재 CS/subscriber 재조회→같은9c/a0 쓰기→Rig다. 송신 실패의 bool로 후속 효과를 취소하지 않는다. 첫 일치0번도 허용하며 active/master/known 필터를 추가하지 않는다. 미발견의 Native NULL 쓰기는 명시 호스트 물리 오류이며 성공을 합성하지 않는다.

일반 입력 `6b/6c/e3/fe`는 공통 뗌을 먼저 소비한다. 6b의 passedDiags→현재 noShowWarnFailedDiags→curServer 조건을 short circuit 순서로 조회한 뒤 현재 전투의 빈8자리 여부와 경고 Tell을 판정한다. e3는 직접 참가, fe는 실제 설정1/동기 재해석 뒤 참가한다. 6c는 packed 상위WORD를 signed X·하위WORD를 unsigned Y로 float32 저장한다. 누름 효과 뒤 원문 일반 입력 꼬리를 유지한다.

연결 전체를 추적하다 기존 메뉴→입력 큐 허용 목록에1135의 f3와 항해 명령이 빠져 있음을 확인해 함께 보완했다. `ChalSailJoinBattle/ChalSailPos/ChalForceJoinBattle/ChalJoinBattleNoWarning` 네 이름은 PE 실제 이름표에서 확인해 기존 HTML/확인창 이름 조회에 연결했다. 원문 `BattleDesc`의 HTML 문자열 고리와 CS 논리 주소 구별은 후속이며 이번 f3는 내부 객체 메뉴 경로다. 메타 월드 마우스 picking/모드6 위치 선택 caller는 아직 잔여다.

1eh의 고정14BYTE/version1·빈 관찰기만 등록하고 수신 부모나 접속 성공 ACK를 추가하지 않았다. 실제 Rig는 현재 참가자+b8의 기존 Host/Kernel 공장을 빌려 이동 목표·지연0·기존 kill 관문을 사용한다. 별도 World/시간/난수나 움직이는 가짜 객체를 만들지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일/정적 대조를 MP 동작 검증으로 세지 않는다.

이 근거는10.78이며10.72 대응 함수 이름만으로 전체 동등을 확정하지 않는다. 마지막 전체 정밀 디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon, 생성 원문 변경0이다.

다음은 메타 월드 마우스 객체 입력/선택 caller·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI다. 전체 목표는 진행 중이며 미완료다.

Netstorm.exe 12,855,296바이트/SHA-256 `1d3e4b0de38b6acfcbb7cf73a20efd9c3d40ed1cd4188ed3ec810960863ae2fb` / R.exe 669,696바이트/SHA-256 `f55a053b3fcb8804a3264aa2c3f2f952b8f9e2ed8133f13193c71ec409f96c26`다. 보고서는 `cpppj/meta-sail-input-20261108.json`, 원문/기계어·빌드·보호 기준은 `extracted/cpp-resume-1136/`(Git 제외)다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
