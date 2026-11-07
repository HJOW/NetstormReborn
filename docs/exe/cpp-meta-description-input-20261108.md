# 1135 — 메타 설명 변경 모달과 실제 권한·전송 연결

제품1135 / 표시·파일·제품버전11.01이다. 이전 단위는 `be0ad21c / 20261108-103 CP`다. 사용자2026-11-08 `/goal` 재개 지시에 따라 진행했다.

`004d3550/004d37a0`의 설명 창을 기존 `SaveFortNameDialog` 형제 공장에 연결했다. `517c40` 가상표·65번 command·41BYTE 단일행·제목/테두리·두 돌 버튼·가운데 배치·signed 깊이 -76·공통 모달 수명을 사용한다. NULL 초기 문자열에서만 기본 문구를 번역하며 빈 문자열은 그대로 유지한다. f3의 현재 CS 논리 주소는 같은 실제 할당에서 해석하고 호스트 포인터를 자르지 않는다.

확정은 `004c92c0`의 실제256BYTE CopyText·255번째 NUL·첫 LF 절단 순서다. 원문 후행 isspace 루프가 마지막 문자 대신 NUL부터 검사하여 공백이 남는 동작도 보존한다. 취소는 원문 기본 성공 반환으로 지연 Kill하고, 확정도 송신 결과와 관계없이 원문처럼1을 반환한다.

`0042c040(-1,text,0)`은 현재 자기 참가자/전투·master 비트를 다시 읽는다. 권한이 없으면 실제 `MayNotBattleUpdate` Tell만 열며, 유효 권한에서만40BYTE로 제한하고 NUL을 포함한71h 요청을 현재 TCP11/공유 가변 길이 관찰기로 전송한다. 수신 전 CS 설명을 미리 변경하거나 성공 ACK를 합성하지 않는다. UI의41BYTE 한계와 송신의40BYTE 제한을 구별한다.

기존 창/편집/클립보드/공통 자식 반납을 재사용했다. SaveFortNameDialogContext의 새 확정 장치는 기존 aggregate 순서를 보존하도록 끝에 추가했다. 원문 ABI/할당실패의 NULL 접근 자체 일치는 기존 호스트 경계이며 원문에 없는 게임 상태 관문을 추가하지 않았다.

이 근거는10.78이며10.72 대응 표의 함수 이름만으로 CD판 UI/71h 동일성을 확정하지 않는다. 생성 디컴파일 소스 변경0, 마지막 전체 정밀 디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon이다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일·정적 대조를 MP 실제 동작 검증으로 세지 않는다.

다음은 메타 월드 객체 입력/선택 caller·6b/6c 항해 소비자·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI다. 전체 목표는 진행 중이며 미완료다.

Netstorm.exe 12,830,720바이트/SHA-256 `afffbcd7a383867040b432eacdd14692d9282a318e8c4988863a361901569ed2` / R.exe 669,696바이트/SHA-256 `b25cc264c9566b56f9496ebf0db63571c94ecc7a947aeff6e94dea8449417b65`다. 보고서는 `cpppj/meta-description-input-20261108.json`, 원문/기계어·빌드·보호 기준은 `extracted/cpp-resume-1135/`(Git 제외)다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
