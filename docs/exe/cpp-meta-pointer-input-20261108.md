# 1137 — 메타 마우스 선택·영역과 항해 메뉴 호출 연결

제품1137 / 표시·파일·제품버전11.01이다. 이전 단위는 `44626991 / 20261108-105 CP`다.

GameWorld가 없는 준비된 MetaMap에 기존 RawUserPick·RawUserSelection·RawUserHighlight·RawCarrierInput을 연결했다. 같은 Pool/TYPE/Hash/Camera·표시기의 투명 픽셀 판정·Bridge 추가 그리기를 빌리며 선택·다중 선택·강조 상태 사본을 만들지 않는다. 준비된 MetaMap만 두 공장 관문에서 허용하고 loaded/playerKnown/Native State는 쓰지 않는다. Guide demo도 실제 현재 Runtime getter만 공급한다.

원문40ea40은 SID의 물리 범위만 검사한다. 새 active/free/dead 필터를 더하지 않았다. 우클릭 편집은 Select(sid,0,0) 뒤 현재 editor를 재조회하고 최초 SID를 Follow에 전달한다. 객체+ac가 비영을 반환하면 메뉴 생성 여부와 무관하게 opengump.wav를 재생한다. 이동 모드·Shift 다중 선택·원문 일반 primary 선택/진단/편집 꼬리는 같은 기존 몸체에 연결했다. 강제 활성 건설 커서·조기/원격 희생·marker 진단의 기존 명시 경계는 성공으로 대체하지 않았다.

메타 좌클릭의 첫 picker는 Ctrl&&(special||editor), 일반 선택의 둘째 picker는 Ctrl&&special이다. 전투섬은 실제 객체 메뉴를 열고 영역 glyph는 현재 QA의144BYTE 기록 번호로 e8을 예약한 뒤 반환한다. 빈 지도 항해는 앞선 폴링 좌표로 viewport를 검사하되 사건 좌표를 역투영한다. 자기 참가자의 battle0이면6c를 예약하고 아니면 QueryLeaveChalBattle을 요청한 뒤 일반 선택 꼬리를 유지한다. 모드6도 같은 좌표 구별을 보존한다.

우클릭 빈 공간은 known/meta/현재 battle0 조건에서 실제 fancy 메뉴에 Battle Island 제목과 Sail to here 명령6c를 추가한다. 현재 메뉴의 실제 크기로 사건 좌표를 중심 정렬하고 FTOL 하위DWORD를 사용해 object0에 부착한다. 객체 메뉴와 같은 공통 Gump 입력/모달/캡처 관문을 거치며 별도 클릭 성공을 합성하지 않는다.

479390의 영역 메뉴는 실제 배율 관문→Begin→최초 QA 기록 주소→현재 signed 번호의 번역 제목→같은 기록 번호 재조회/하강 항목→helpWord 진단/현재 TYPE 도움말→현재 객체/사건 좌표 부착 순서다. PE의 오자 Decend와 QueryDecendToChalLevel을 그대로 보존했다. e8은 뗌에서 효과 없이 소비하고 누름에서 확인 Tell만 요청한 뒤 일반 입력 꼬리를 유지한다. ViewMeta/상태 전환을 앞당기지 않는다. 현재 helpWord 조회는 Config의 빈 문자열/누락을 구별하지만 원문 전역 캐시 포인터의 전체 수명 복원으로 확대하지 않는다.

실제 두 메타 VT+ac와 이름/명령 값을 보호 PE에서 확인했다. 1136제품의 --dump-types 읽기 전용 자료 조회1회로 playerIsland108/battleIsland109/challengeIsland161과 flags2=0을 확인했다. 이 조회는 게임·GUI·MP·통신 검사가 아니다. 메타 폴링 좌표는 Overlay 앞에 공유하고 현재 이동 강조는 Overlay 뒤/카메라 pan 앞에 갱신한다. 정리 시 입력→표시기→Session 순으로 참조를 반납한다.

호스트 지도 화면을 Mission 합성 phase로 바꿔 이전 MainMenu 버튼을 재생성하지 않도록 했다. 이는 Native 미션 적재나 접속 성공을 합성하는 처리가 아니다. 메타 ESC/H/F4·공통 메뉴/키보드 호출은 아직 잔여다.

최종 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새 C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이며1136제품의 읽기 전용 TYPE자료 조회1회는 별도 기록한다. 최초 불완전 형식10진단→헤더 교정 빌드0, 후속 설정API4진단→실제 Config 경로 교정→최종 빌드0의 원시 로그를 모두 보존했다. 마지막 실제 C++ 실행검사는1052 기존7개이며 새 MP 검사는0이다.

마지막 전체 정밀 디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon, 생성 원문 변경0이다. 이 근거는10.78이며10.72 전체 동등으로 확대하지 않는다.

다음은 메타 ESC/공통 키보드·메뉴 caller·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다. 986 후보는 미적용이고 전체 목표는 진행 중이다.

Netstorm.exe 12,918,272바이트/SHA-256 `8bf36e281459d8c08e9f47eb84c359f5c8fd0cd8e0774e647f337ebcf641f1e9` / R.exe 669,696바이트/SHA-256 `3ca5a77471859aa7fa1061352a0b5e5589879051ba867a7a50c640ef41981d99`다. 보고서는 `cpppj/meta-pointer-input-20261108.json`, 원문/기계어·빌드·보호 기준은 `extracted/cpp-resume-1137/`(Git 제외)다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
