# 1109 — PlayerFinder의 실제 현재 메타 참가자 표

제품1109 / 표시·파일·제품버전11.01이다.1108(`6683d4fa / 20261108-34 CP`) 뒤 NSInfo 참가자 메뉴의 선행 조회를 준비하며 실제 Chat PlayerFinder의 미연결 CS 표를 연결했다. 기존 RawPlayerIdentity의40기록 검색 본체는 이미 있었으나 ClientChatRuntime이 빈 span만 제공해 메타 분기 도달 시 명시 예외였다.

PlayerIdentityReads에 선택적인 현재 metaRecords getter를 더했다. 실제 메타 분기 또는 명시 MetaRecords 조회에 도달했을 때만 provider를 호출하고, 현재 CS+b70의40×1cc 기록을 직접 빌린다. 로컬 일치의 조기 반환과 일반 Player 완료 경로에서 아직 없는 CS를 선행 요구하지 않는다. 기존 활성 플래그/이름/subscriber 첫 일치·부분완료·Chat100 후속 검색은 그대로다. 참가자 사본이나 가짜 이름/0기록을 만들지 않는다.

provider 미공급의 기존 고정 span 호출과 두 판본의 크기 검증은 유지한다. 새 실제 Client CS 장치는 현재 PATCH1078용이다. 이번 변경을 CD판296BYTE 메타 저장소 연결 완료로 쓰지 않는다. 현재 메타 메뉴가 명시 MetaRecords를 호출할 때도 같은 지연 getter를 재사용할 수 있지만 **NSInfo 세 Handle·실제 참가자 메뉴/카메라와1107 슬롯·1108 송신 UI 호출은 후속**이다.

원문004841c0의 로컬/Player 뒤 현재메타+b70/1cc/40와 Chat100 순서, getter 도달 위치/표 크기/실제 Client 배선을 정적으로 대조했다. NetstormCpp·NetstormRoot Release/x64 빌드 종료0·경고/오류0이다. 보호2800파일 불변·새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 사용자 제외에 따라 새 MP 검사는 실행하지 않았다. 전체 입력/메타 화면/State19 실행 성공이 아니다.

다음은 실제 메뉴/입력/카메라→CS 수신/생성·카메라 Rig/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다. Netstorm.exe 12,548,608바이트/SHA-256 `378d1733bdb27457685c07d1a659bddd131d5756434a2ed660e1c8ddff95d5f8`, R.exe 669,696바이트/SHA-256 `30cc12aea3d65535a0e9ca3b722b18394ab7264dd3629d954fb5829b7ae5a98f`다.

보고서는`cpppj/meta-player-identity-20261108.json`, 원문·빌드·정적 감사는`extracted/cpp-resume-1109/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
