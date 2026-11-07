# 1128 — 삭제 선행 전체 HUD 재생성과67h 연결

제품1128 / 표시·파일·제품버전11.01이다. 이전 단위는 `3f942573 / 20261108-80 CP`로 커밋됐다.

`004c7880` 전체 원문·각 다음 함수까지 기계어와 기존 공통 Kill/Cleanup/ReplaceRoot를 대조했다. 원래 경로는 기존 등록0이 비영이면 Kill하고, 루트0인 경우에도 삭제 큐를 정리한 뒤 전체200BYTE Uber를 새로 생성한다. 임시 등록칸을0으로 쓰고 새 노드ID를0으로 변경한 다음 실제 공통 루트 교체를 호출한다.

기존004c77c0은 새 HUD 생성 뒤 이전 자식을 이관하므로 별도 `Recreate` 부모로 구현했다. 이전 자식 이동이나 원문에 없는 선행 empty 관측은 넣지 않는다. 네 실제 패널·세 divider·같은 공통 등록표/삭제 큐·Renderer/도킹 장치를 사용한다. 호스트 화면 크기 관측은 정상 루트 공개 뒤에만 갱신한다. 실패할 때 이전 HUD를 되살리거나 빈 HUD로 성공시키지 않는다.

실제 RawUberRootControls/RawButtonControls와 CampaignLoginServices.reconnectUi를 통해 일반 Session의67h UI 선택에 연결했다. 호출 관문은1127 공통 부모의 현재battle&&!mission&&saved에 그대로 둔다. **메타67h 등록표의 전체 부모 연결과 공유 반경·파생 전역은 후속**이다. 전체 메타 장면이나 실제 HUD 재생성 실행 성공으로 쓰지 않는다. 사용자 지시에 따라 신규 단위/멀티플레이/게임/통신 검사는 실행하지 않았다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일을 전체 메타 장면·MP 실행 성공으로 쓰지 않는다.

다음은 메타67h 전체 부모 등록/공유 반경·파생 전역·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI이다. 전체 목표는 미완료다.

Netstorm.exe 12,729,344바이트/SHA-256 `488588a600946bb71cb18fa3d5085c16aa8192a8719bc0fb2930135ec53e79e4`, R.exe 669,696바이트/SHA-256 `e87dba0b85776a13ed1c9bb4b5664dc17305227dcc697ed8b7b47d44dfb0976a`다. 보고서는 `cpppj/hud-recreate-20261108.json`, 원문/기계어·빌드·정적 감사는 `extracted/cpp-resume-1128/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
