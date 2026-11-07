# 1104 — PlayerInfo 전체 메타 배치

제품1104 / 표시·파일·제품버전11.01이다.1103(`78ad430e / 20261108-24 CP`)의 NSInfo 루트/목록에 이어00479fb0의 일반 Attach/메타/known 관문 뒤 전체 PlayerInfo 본체를 실제 공통 UI에 연결했다. 일반 앞단과 두 관문은 기존 HudNsInfoCreate가 그대로 실행한다.

실제 네 자식 Push와 현재 소유자 master 플래그, 빈 슬롯의 본문/버튼 값, master와 Start Battle 표시, 같은 전투8슬롯의 준비 관문을 원문 순서로 옮겼다. 특수 버튼+c0 현재 색/+c4 체크 그림과 +40 값은 구별한다. PE의 두 버튼 VT를 확인하여 +3c는 실제 SetEnabled, +40은 기본/돌 버튼 SetValue에 연결했다. 같은 ButtonGump의 Redraw와 돌 배경 전환을 사용한다.

같은 CS participant의 전투 옵션 포인터를 먼저 확보하고 각 원문 위치에서 현재 slot을 다시 읽는다. 원문 디컴파일 지역값이 섞인 옵션 포인터와 마스터/시작 버튼 여백을 기계어로 분리했다. 0041c5b0의24DWORD 랭크 표와 기존0042f480 ChatPrefixIcons·현재 icon B/C TYPE 개수를 사용한다. 랭크 진단은 같은 Chat 경로처럼 로그 뒤 복귀한다. 원문 서식/4종 상태 아이콘·명시 문맥 보관/교체/정상 복원·태그100/200/300/400·master 이름의 명시 metadata도 연결했다. 문자열 파싱이 실패하면 이미 쓴 문맥/상태를 되돌리지 않는다.

이름 영역은 실제 RequestScroll(0)·FitContent(0,0) 후 현재 버튼 폭과 여백2를 빼서 놓는다. 최종 패널 높이는 **캡처한 master/Start 여백 + 현재 이름 영역 높이 +2**다. 디컴파일의 중복 이름 높이 해석을 사용하지 않았다. FST32 뒤 잔류 계산값에 배율을 곱하는 폭/높이 저장 지점도 보존한다. 원문 물리 배열·256BYTE 서식 밖 입력은 호스트 예외로 드러내며 원문 메모리 오염을 재현하지 않는다. x87 모든 입력/픽셀의 동등 검증은 아니다.

**BattleInfo 전체 메타 배치·NSInfo 메타 입력과 특수 버튼 Draw는 아직 명시 미연결이다.** 이번 단위를 전체 NSInfo/State19 화면 성공으로 보고하지 않는다. CS 수신/참가자·전투섬 생성·카메라 Rig/HTML→1dh/28~33 등록/1095 송신 상태 호출, 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다.

첫/slot 재조회 교정/최종 NetstormCpp·NetstormRoot Release/x64 빌드 모두 종료0·경고/오류0이다. 전체 함수 기계어·보호 PE의 VT/그림 문자열·현재 실제 장치 배선을 정적으로 대조했다. 보호2800파일 불변·새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 사용자 제외에 따라 새 MP 검사는 실행하지 않았다. Netstorm.exe 12,519,936바이트/SHA-256 `f1cbdf58abb8d78b5fd71e97e5071689cd2d0c8a97f4418c43d3412e298ce5bd`, R.exe 669,696바이트/SHA-256 `26793b505437b1744ddc47f5ff283b7d05a0dda8988a345f308d99351e15d40a`다.

보고서는`cpppj/nsinfo-player-layout-20261108.json`, 원문/기계어·빌드·정적 감사는`extracted/cpp-resume-1104/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
