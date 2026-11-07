# 1103 — NSInfo 메타 루트와 참가자 목록

제품1103 / 표시·파일·제품버전11.01이다.1102(`2e5e1c67 / 20261108-20 CP`)의 실제 CS/Gump 연결 위에0047e630 참가자 목록 전체와0047e9d0 일반 앞단/관문 뒤 메타 루트 본체 전체를 연결했다. 일반 앞단은 기존 HudNsInfo가 그대로 실행한다.

참가자 목록은005930b0의 실제40DWORD 중0번을 보존하고1..39만 초기화한다. 원문의39×39 교환 순서와 교환 직후 현재 후보 재조회, 레벨 동률 때 상대→현재 기술 합계 조회를 보존했다. 같은 Pool SID+22 소유자·현재 metadataMode·팔레트색으로 문맥을 만들며 행 metadata는 참가자 번호가 아닌 정렬 배열의 행 번호다. 120BYTE 행과256BYTE 서식의 물리 범위를 벗어난 호스트 입력은 예외로 드러내며 원문 메모리 오염을 재현하지 않는다.

루트는 현재 소유자의 전투 유무에 따라 제목/목록/구분선/8개 PlayerInfo/BattleInfo의 Push·크기·표시·Attach와 도움말을 실행한다. 첫 master와 비master 순서, master가 없을 때 구분선/전투 패널을 숨기는 조건도 보존한다. 숨은 남은 패널도 실제 Attach를 호출한다. 공통 StyleText/Markup/Wrap·공통 트리·현재 CS/Pool/TYPE·서버명 번역과 같은 도움말 문자열 수명을 사용한다. 005bec3c는 실제 회색 basic8로 바로잡았으며 메타 목록의 노랑/명명색도 같은 현재 팔레트를 읽는다.

전체 함수 범위 기계어를 대조했다. root 함수 중간의 첫 ret에서 범위를 자르지 않았고, PE의3/2/.5 상수·개행·행 버퍼 크기와 master/nonmaster Y의 ftol 위치를 확인했다. 높이는 FST32로 기본 높이를 저장한 뒤 남은 계산값에 배율을 곱하므로 저장된 float 높이를 재조회하지 않는다. x87 모든 입력/픽셀의 비트 동등 검증은 아니다.

**PlayerInfo/BattleInfo 자식의 전체 메타 배치, NSInfo 메타 입력과 버튼 Draw는 아직 명시 미연결이다.** 루트가 이 자식을 실제 호출하므로 이번 단위를 전체 NSInfo/State19 화면 성공으로 보고하지 않는다. CS 수신/생성·카메라 Rig·HTML과1dh/28~33 등록/1095 송신 상태 호출도 후속이다.

첫/최종 NetstormCpp·NetstormRoot Release/x64 빌드 모두 종료0·경고/오류0이다. 보호2800파일 불변·새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 사용자 제외에 따라 새 MP 검사는 실행하지 않았다. Netstorm.exe 12,499,456바이트/SHA-256 `aa8d600750c887037b4d7505e2eb88de2684359e86062fae538858c1378e5f66`, R.exe 669,696바이트/SHA-256 `7c7126698e1fcc4eecb74a70666b24acb973870e61e372803033d88a1e8030c4`다.

다음은 두 자식 전체 배치와 메타 입력/Draw를 복원하고 CS/카메라·상태 접합을 이어간다. 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다. 보고서는`cpppj/nsinfo-meta-root-20261108.json`, 원문/기계어·빌드·정적 감사는`extracted/cpp-resume-1103/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
