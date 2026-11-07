# 1110 — 메타 참가자 메뉴 본체와 실제 신뢰도

제품1110 / 표시·파일·제품버전11.01이다.1109(`9f7c0604 / 20261108-37 CP`) 뒤004785f0 참가자 메뉴와00478310 신뢰도 전체 본체를 작성했다. 자기 subscriber 일치 시 같은 controlled fancy 메뉴의 조건부 제목/Leave this Battle/View your Player Info를 작성하고 현재 객체 메뉴를 부착·스택 반납한다. 다른 참가자는 기존 전체 PlayerMenu가 꺼낸 토큰을 현재 객체/루트에 직접 가상 부착한다. 임의로 두 경로를 같은 자동 부착으로 합치지 않는다.

신뢰도는 meta0일 때 CS 조회 없이100이다. 현재 참가자+54/+56의 두WORD로 정수 비율을 구하고 분모0/100초과이면100이다. 양의WORD×100은 signed32 범위 안이므로 원문 idiv의 정상 입력 결과와 같다. 기존 CampaignObjectMenuClient의 명시 미연결 신뢰도 훅을 같은 실제 Client CS getter에 연결했다. 별도 플레이어/비율 캐시를 만들지 않는다.

**World 없는 메타 참가자 메뉴의 실제 controlled/전체 PlayerMenu·주소/identity 공장과 NSInfo 세 Handle·카메라·1107슬롯/1108송신 UI 호출은 후속이다.** 이번 메뉴 본체 컴파일을 실제 메타 메뉴 화면/입력 성공으로 쓰지 않는다. 현재 연결한 소비자는 기존 CampaignObjectMenuClient의 신뢰도 훅이다. 256BYTE 제목/실제 이름 NUL·물리 범위 밖은 호스트 예외로 드러내며 원문 메모리 오염을 재현하지 않는다.

NetstormCpp·NetstormRoot Release/x64 빌드 종료0·경고/오류0이다. 전체 원문/기계어의 비교·분기·메뉴 호출/가상 부착과 두WORD/idiv/상한, 실제 신뢰도 소비자를 정적으로 대조했다. 보호2800파일 불변·새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 사용자 제외에 따라 새 MP 검사는 실행하지 않았다.

다음은 실제 메타 메뉴 공장/입력→CS 수신/생성·카메라 Rig/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다. Netstorm.exe 12,549,632바이트/SHA-256 `704741f2693b04ef95dd4381c2a0463532f7106a0916858ff95fcfe63bfd7035`, R.exe 669,696바이트/SHA-256 `c8f9f395f72468b88cef676ed1af1cc0f4ee83c48034cd07a63ee89e9a0ecb22`다.

보고서는`cpppj/meta-player-menu-20261108.json`, 원문/기계어·빌드·정적 감사는`extracted/cpp-resume-1110/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
