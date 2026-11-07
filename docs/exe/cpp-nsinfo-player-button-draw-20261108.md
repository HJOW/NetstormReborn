# 1106 — PlayerInfo 특수 버튼 전체 Draw

제품1106 / 표시·파일·제품버전11.01이다.1105(`88b1cd81 / 20261108-29 CP`)의 NSInfo 전체 메타 배치에 이어 특수 버튼004252b0 전체 Draw를 실제 공통 WithDrawing 분배에 연결했다. 기존 명시 미연결 예외를 제거하고 같은 Gump 등록표/수집/정렬/클립에서 실행한다.

현재 Bounds→value 또는 pressed의 비영 상태 캡처→현재 배경 frame/TYPE 조회→고정3e40000 배경→안쪽2픽셀 축소/현재+c0 단색→현재 fortGump TYPE·+c4와 L 첫 프레임 조회→활성일 때만 L+6/7 원색 SHP 순서다. 배경의 pressed 비트는 고정 원문 플래그이며 캡처한 활성 여부로 바꾸지 않는다. 비활성이어도 TYPE/L 조회는 수행한다. 004daa70의 null SHP 조기 반환은 현재 실제 shapes.loaded로 보존한다.

좌표 축소·프레임 합계는 하위 DWORD 감김을 쓰고 중앙점의 signed 차이/2는 음수도0방향으로 자른다. 배경 뒤 바뀐 현재 색/TYPE는 해당 원문 위치에서 읽되 활성 여부는 재조회하지 않는다. 같은 공통 GumpBackgroundDraw·GumpPixelTarget.Fill(checker=false)·현재 TYPE/frames·원색 SHP 기준점 출력을 사용한다. 임시 callback은 같은 WithDrawing 동기 호출 안에서만 유효하며 독립 표면/버튼 상태를 만들지 않는다.

첫/최종 NetstormCpp·NetstormRoot Release/x64 빌드 종료0·경고/오류0이다. 원문 전체 기계어의 상수/호출/레지스터 캡처/중앙 나눗셈, 실제 공통 장치와 분배를 정적으로 대조했다. 보호2800파일 불변·새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 사용자 제외에 따라 새 MP 검사는 실행하지 않았다. 전체 픽셀/비정상 원본 메모리 입력의 동등 검증은 아니다. 1105 문서의 첫BYTE 주소 오타+b164만+164로 정정했다.

**NSInfo 메타 입력, CS 수신/참가자·전투섬 생성·카메라 Rig/HTML→1dh/28~33 등록/1095 송신 상태 호출은 후속이다.** 전체 메타 화면/State19 실행 성공으로 보고하지 않는다. 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다.

Netstorm.exe 12,545,024바이트/SHA-256 `0c356d9db09e42eff0204118fa59fca535ed87bf8aeb09f8a232730b67fa9e29`, R.exe 669,696바이트/SHA-256 `9052d969f98ad35c588cf357e1d71e6f40bbd647d1d0be3e1da13e285959b794`다. 보고서는`cpppj/nsinfo-player-button-draw-20261108.json`, 원문/기계어·빌드·정적 감사는`extracted/cpp-resume-1106/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
