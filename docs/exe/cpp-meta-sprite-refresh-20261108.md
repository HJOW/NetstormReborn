# 1131 — 메타 파생 표시 갱신과 실제 World·Gump 연결

제품1131 / 표시·파일·제품버전11.01이다. 이전 단위는 `be336598 / 20261108-88 CP`로 커밋됐다.

`004787c0` 전체와 기본 영역498ff0·viewport 등록499310·Form finder의 원문/기계어를 대조했다. 현재 TYPE/frame·위치의 기본 사각형을 읽은 뒤 큰 배율에서만 전투섬 중심 유지/참가자 오른쪽·아래 고정 보정을 수행한다. lowDWORD 합/차/곱·signed 나눗셈·현재 viewport 좌표별 clamp와 면적 판정을 유지한다. 작은 배율/숨김에 원문에 없는 보정을 추가하지 않는다.

dirty 뒤 현재 checking/metamap/dead/boss 진단과 기본 Form finder를 실행한다. 반환 전 next를 저장하고 현재 TYPE70미만·excluded/process 제외 조건을 적용한 뒤, 현재 등록ID가 실제 Gump를 찾으면 기존 가상Redraw(false)를 호출한다. 새 목록/가짜 이름표를 만들거나 free/dead/active 필터를 추가하지 않는다.

보호 VT의88=4787c0,8c=4ad670이며 후자는 현재88로 JMP한다. 같은 SquidDisplaySink와 실제 RawSurfaceWorld callback에 연결하고 메타 Session 생성 직후 첫 메타 객체 생성 전에 현재 Client/Renderer/Gump 장치를 설치했다. 일반 view 교체로 이 장치를 지우지 않고 현재 World가 수명을 반납한다. 일반 frameCheck 부모/선택표식/그림자/표시 억제 호출로 대체하지 않는다.

사용자 지시대로 신규 단위/MP/게임/통신 검사는 실행하지 않았다. 메타 객체 입력·일반 선택 캡처·나머지 CS 수신/참가자 생성은 후속이다.

첫 Release 성공 뒤 실제 비영 Gump 토큰의 호스트 노드가 없을 때 빈 Find 결과로 숨기지 않고 그 선택 위치에서 예외로 거부하도록 보완했다. 첫/최종 Release를 별도로 보존하며 신규 실행 검사는 하지 않는다.

기존 Unpop 지원표 검사의 두 기대값만 새 실제 메타88/8c 분배에 맞게 갱신했다. 새 검사 사례/fixture는 없으며 사용자 지시대로 이 검사 대상은 빌드·실행하지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일을 전체 메타 장면·MP 실행 성공으로 쓰지 않는다.

다음은 메타 객체 입력/일반 선택 캡처 접합/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI이다. 전체 목표는 미완료다.

Netstorm.exe 12,776,960바이트/SHA-256 `644fdd9c190331dfbde57b0d05f5878a05bbd296c3a727632024e5c11b0e6c4f`, R.exe 669,696바이트/SHA-256 `1e267b93322f96ec2a179b3a446d9551229af4b862448029848688e935e1b8a7`다. 보고서는 `cpppj/meta-sprite-refresh-20261108.json`, 원문/기계어·빌드·정적 감사는 `extracted/cpp-resume-1131/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
