# 1150 — M1.2 항해 오류25h와 실제 객체 안내

제품1150/표시·파일·제품버전11.01이다. 직전은1149(`c0f7843a / 20261108-118 CP`)다. M1.2를 끝내 **M1을 완료**했으며 현재3차의 다음 작업은M2다.

4b47e0은 메타 모드0이면 CS/packet을 읽지 않는다. 모드가 있으면 CS0의 State.cpp d33 진단 뒤 전체42c7f0을 호출한다. 원문 빈 observer/버전1/정책0/PE52ec40의 고정8BYTE 등록을 실제 같은 dispatch에 연결했다. sender/known/phase/active 관문이나 NSInfo 갱신을 추가하지 않는다.

PE42c7f0처럼 subscriber를 진입에서 한 번 캡처하고 같은 진입CS의40개 기록(0번/비활성 포함)에서 첫 +4 일치를 고른다. 검색 후 packet WORD+6을 읽는다. 오류1~4는 해당 원문 문구를 한 번 번역한 뒤 현재 선택 기록+b8 객체를 읽어472ad0 안내를 실행한다. 오류5~7은 TooManyPirates/BattleVersionMismatch/BattleFailedDiags의 실제 Tell이다. 알려지지 않은 코드도 포함해 모든 복귀 분기는 같은 선택 기록의 **현재 +0 ID**를 다시 읽고ff 표시를 실행한다. 기록 미발견은 원문의 NULL 접근 위치에서 호스트 오류가 되며 앞선 번역/Tell을 되돌리지 않는다.

472ad0은 현재 실제 Session Pool의50BYTE 객체 x/y와 같은 Snapshot을497220으로 투영한 뒤 이미 번역한 바이트에 `~0~w~E%s`만 붙인다. 첫NUL/240BYTE 물리를 지키며 다시 번역하지 않는다. 실제 b8 ProductionHintNode를 기본 등록 전에 소유 집합에 보관하고 기존 ProductionHintGump::Construct를 재사용한다. TextGump 생성(0,0,6,0)→원문 가상표/종류/현재 색/strdup→StyleText 파싱/6·4 Fit→idle등록→실제 벽시각+4초가 그대로 사용된다.

생산 패널의 별도 생산 Context를 만들지 않는다. 두 안내 공장은 같은 RawMenuControls 수명의 ProductionHintState(종류/white/black/lastSerial)를 공유하고 생산 패널을 다시 생성해도 이 전역을 초기화하지 않는다. 생산 노드/문자열은 기존 RawProductionControls가, 메타 노드/문자열은 같은 메뉴 트리가 각각 소유한다. 기존 생산 소스는 상태 참조 한 줄을 제외하고 같고 ProductionHintGump 본체는 바이트 불변임을 확인했다.

생성 뒤 flags4000→실제 MenuGumpWidgets::Reference/현재 Session GumpForm→깊이-100→당시 투영점/현재 루트/모드0 부착→공유 마지막 serial 순서를 지켰다. 일반 월드 안내의2000 비트나 생산 안내 y+20을 더하지 않는다. 현재 트리의 Node/Text 조회·idle·Draw·문자열 free/Text scalar·Release·메타 이탈 Cleanup을 연결했다. 메타 노드의 표시가 단순 로그/성공값/가짜 팝업으로 대체되지 않는다.

42c7f0/4b47e0/472ad0/4728d0/4729d0/472a00/472a50/4644e0/497220 C/PE, 같은 진입 subscriber/CS/기록과 후행 읽기, 고정8BYTE 표/등록, 현재 객체/Snapshot 투영, 기존 TextGump/StyleText/4초/객체 Form/루트/idle/Draw/소멸·공유 안내 전역을 정적으로 대조했다.

NetstormCpp/NetstormRoot Release/x64 최종 종료0·경고/오류0이다. 첫 컴파일도 종료0이었으며 PE의 subscriber 진입 캡처를 명시한 뒤 다시 빌드했다. 두 빌드 로그는 Git 제외 `extracted/cpp-resume-1150/`에 보존한다. 보호2800파일 불변, 새 C++ 단위/MP/게임/통신/전체/ZIP 실행0이며 마지막 C++ 실행 검사는1052의 기존7개다.

Netstorm.exe 13,103,104바이트/SHA-256 `53cbca240cba47bd6755dd4a759ecf7cc0c7aed66f0e952495977465dce96d6a` / R.exe 669,696바이트/SHA-256 `ce40a189f0d5a432270945d1d03ce97a9d7d31ba1705def1bcd95be28ead8ee1`

마지막 전체 디컴파일2026-11-03/HJOW-Athlon, R전용2026-11-07/동일 호스트이며 생성 원문을 수정하지 않았다. 실제 안내 위치/수명/장면/안정성의 실행 검증 완료를 뜻하지 않는다.

**다음은 M2:** 1ah/4b4df0와1bh/4b8b20의 checking/상태·실제 진단/선택적 packet-record·연결 정리 후속을 현재 저장소에 연결한다.3차의 잔여는M2~M8의7묶음이며 규모는 커밋/일수로 환산하지 않는다.

1151 후속 교정: 첫 컴파일 로그에서 안내 hook의 지역 이름 중복 C4456 1건을 확인했다. 이 문서의1150 최종 증분 빌드 경고0과 첫 컴파일 경고1을 구별한다.1151에서 이름만 바꾸고 최종 재빌드 경고/오류0을 확인했으며 원시 로그는 보존했다.
