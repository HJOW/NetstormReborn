# 1095 — 메타 master 통지와 게임 서버 로그인 전송 장치

제품1095 / 표시·파일·제품버전11.01이다. 새 `/goal`로1094의 사용자 요청 일시 중지를 해제하고 필수 전송 장치 두 개를 복원했다. 같은 MetaConnectionRuntime의 실제 CS와 TCP를 이용한다. **두 메서드는 컴파일됐으나 State33의 호출·1dh 수신 등록·28~33 분배는 아직 미적용이다.** 메타 화면/CS 수신·오브젝트/카메라/HTML 장치 완성 뒤에 접합한다.

004b2c00은 자기 참가자 기록을 먼저 캡처하고1fh 헤더/12BYTE→현재 subscriber→현재 flags DWORD에2000h OR/같은 기록 쓰기→패킷 쓰기→TCP11 송신 순서다. 실제 송신 관문이 실패해도 앞선 flags 쓰기를 되돌리지 않는다. active·PlayerKnown·phase 관문이나 가짜 성공/ACK를 추가하지 않았다.

004b62b0은7ch/28BYTE→+4의0→실제 master 조회→음수이면 원문 State.cpp:1783 진단→현재CS 재조회→master+4→현재 subscriber/owner→자기+a4/+a8→TCP9 송신이다. 진단 뒤에는 같은 당시 CS 바이트를 캡처한다. 원문 진단 복귀 후 master=-1의 물리 밖 읽기는 명시 예외로 거부하며 다른 master를 합성하지 않는다. 현재 owner와 subscriber는 같은 Client 장기 저장소다.

보호PE의0052ed8a/0052ee96 생성자 표에서 각각1fh/버전1/12BYTE와7ch/버전1/28BYTE를 확인했다. **게임 서버 로그인 요청은10h가 아니라7ch다.** 같은 실제 패킷 등록표에 고정 길이/버전/빈 원문 관찰기를 설치했으며 수신기를 새로 만들지는 않았다. 0040b380은 실제 `ret 4`다. 두 패킷은 모든 바이트를 원문 작성하므로 미작성 scratch를 임의 채우는 변경은 없다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0이다. 다섯 원문 함수/관련 전체 PE 범위와 저장·필드·목적지 순서를 정적으로 대조했다. 보호2800파일 불변·새fixture0이며 신규 C++/MP/게임/통신/새R·원본R/전체검사/34미션/ZIP 실행0이다. 실행 미검증을 사전 구현/컴파일과 구별한다. Netstorm.exe 12,397,568바이트/SHA-256 `71094c7ae157c7cd4da71857748f93c379158faef6c4f125c9c0d2d15ebad3a5`, 새R.exe 669,696바이트/SHA-256 `8424e075b6bf94371772a5ca3a03ebdfff03a5b6733e11bfa42bd55eb156e757`다.

다음은0047dd60/479760/47cab0 표시/Poll·CS 실제 수신/가상표·같은 오브젝트/카메라/HTMLserial 장치를 복원하고, 이 두 전송 메서드와 함께1dh/State28~33에 연결하는 작업이다. 전체 목표는 미완료이며 세.server 콘텐츠/배포·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI 순서를 유지한다.

보고서는 `cpppj/meta-peer-requests-20261107.json`, 원문/PE·빌드/정적 감사는 Git 제외 `extracted/cpp-resume-1095/`다. 마지막 전체 정밀 디컴파일은 **2026-11-03/HJOW-Athlon**, R 전용은 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. 원본/CD/AGENTS.md·루트README.md·DN/WB는 수정하지 않았다. 사용자 지정 포획/희생/특정 공격/승리·수동 확인·공중3종·멀티플레이 검사는 제외한다.
