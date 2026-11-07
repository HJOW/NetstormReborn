# 1102 — 실제 메타 표시·공통 Renderer 접합

제품1102 / 표시·파일·제품버전11.01이다.1101(`6317535a / 20261108-16 CP`)의 실제 메타 Session에1096~1100의 Poll/전체 표시 본체를 연결했다. RawLocalMissionState의 metaFirst/Second는 원문004b88d0처럼479760/47dd60을 실행하며 metaThird는 기존 실제 NSInfo 갱신을 유지한다. 필수 선택 장치가 빈 callback으로 성공하지 않는다.

장치를 연결하며 기존 훅의 의미를 두 곳 정정했다.004ac220은 +c 종류/QA WORD를 쓰고 현재 다리 TYPE일 때만 표면 진단을 호출한다. 가상+74 소유자 BYTE 쓰기와 구별하여 setKind/paintOwner를 각각 실제 몸체에 연결했다.004ad6c0은 TYPE+114의 현재 SHP 프레임 개수를 반환하며, 그 값을 같은004558c0 게임 난수의 한계로 넘긴다. 이전 defaultFrame/remapFrame이라는 잘못된 이름을 frameCount/selectFrame으로 고쳤다. 기본 프레임을 난수 한계로 사용하지 않는다.1097~1100 본체의 당시 하위 장치가 미연결이던 상태와 실제 이번 연결을 구별한다.

ClientRendererFrame이 같은 Screen flags/폭/높이·Renderer/viewport·팔레트색·CS/존·권한·Pool CapacityValue/공통 등록표0번 RootTokenValue를 참조로 빌린다. named36은005c78d0의 RGB(47,24,17) 현재 색이다. ProcessTypeEnd는 장기 unsigned DWORD를 빌려 비교 위치에서 signed로 해석하므로 임시 참조/주소 별칭 오류가 없다. 실제 비const Pool의50BYTE만 빌리며 free/active 필터를 추가하지 않는다. 선택된 Unpop/QA·다리통지/가상Owner/Pop/Destroy/Create·현재 SHP 개수/공유 난수와 같은 현재 Renderer의 분모/dirty·번역/진단·실제 Gump/TextGump/SHP 장치를 연결했다.

전체 배치 뒤4340d0은 기존 Session.ResetCamera를 사용한다. 현재 참가자TYPE108·화면 크기·Renderer limits와 같은 로딩 깊이/현재 소유자/즉시 Commit을 공급한다. 메타 Commit은 전체 화면 대신 같은 HUD 지도 viewport를 사용한다. 참가자/전투섬 TYPE108/109를 Client 수명 DWORD로 더했고 보호 PE와 대조했다. 원문 카메라 Rig의 이동/확대/대기 애니메이션은 후속 CS 수신 부모에 남았다.

메타 Sprite 출력기도 기존 RawCampaignFactoryDisplay를 같은 Session Pool과 현재 소유자 색표/파일 팔레트에 연결했다. 공통 수집→정렬→SpriteDraw와 기존479340 배율 그림을 사용하고 별도 Sprite 목록을 만들지 않는다. 현재 Session의 입자 수집/출력은 게임/메타가 공유하며 커서가 실제 존재할 때만 미리보기를 수집한다. 팔레트 변경을 같은 출력기에 공급하고 Session보다 먼저 출력기를 반납한다.

**CS 수신/참가자·전투섬 생성·카메라 Rig·HTML/NSInfo 메타 배치와1dh/28~33 등록 및1095 송신의 실제 상태 호출은 후속이다.** 이번 접합을 전체 메타 화면 또는 플레이 성공으로 보고하지 않는다. 사용자가 제외한 새 MP 실행/단위/통신 검사는 수행하지 않았다.

첫 빌드의 개명 누락 세 곳(C2039/C2672)과 지역 변수 중복 경고(C4456)를 교정하고 첫 로그/종료값을 보존했다. 최종 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0이다. 원문/PE의 QA WORD/TYPE+114·현재 TYPE 상수와 실제 참조/장치·공통 Draw/수명 배선을 정적으로 대조했다. 보호2800파일 불변·새fixture/새C++/MP/게임/통신/새R·원본R/전체검사/34미션/ZIP 실행0이다. Netstorm.exe 12,468,224바이트/SHA-256 `859756ed551c87160edf75b9ce79b2cc24f198080c29e91c6c1a65d4239b2ba7`,새R.exe 669,696바이트/SHA-256 `1060d18119ce8e77cd690b31c4fdff31a6284b7277dacfae8fef11835ac520ca`다.

다음은 CS 수신/생성·카메라 Rig와 HTML/NSInfo를 완성해1dh/28~33을 접합한다. 이후 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어후속→Edit시나리오UI 순서다. 전체 목표는 미완료다. 보고서는`cpppj/meta-renderer-integration-20261108.json`, 원문/기계어·빌드/감사는`extracted/cpp-resume-1102/`(Git제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
