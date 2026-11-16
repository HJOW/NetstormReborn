# Session의 독립 typed 자산 삭제 분배 (WB04-E614)

기준선 `225553df` 생성자1680..1756의 독립 NoWorld/typed 분기를
현재 World 훅 표에 연결했다. 수명 자격은 Source364의14VT와
1969..2018의 현재 VT/TYPE 조건이다. Session.cpp의 기준선 뒤 변경은0이다.

피해→내용물→건설 순서로 기존 DestroyHooks/Destroy 몸체를 합치고,
파생 Pre/Post 선택 뒤 사제 전처리 래퍼와 최종 가상 삭제를 연결했다.
기존 함수 값/fallback과 송신·선택 같은 다른 효과를 보존한다. 선택된
부모가 실패하면 다른 몸체로 재시도하지 않는다. 실제 Runtime 게시1825는
이번 단위에 포함하지 않았다.

VT 전용인 공통 Damageable·썬 바리케이드·너겟·Rune/신전 등에 임의의
표시 TYPE 자격을 추가하지 않는다. 다리/연결자·사제·독립 보호막·묻힌
보상·표식/포스 필드·표시 더미의 실제 TYPE/VT 쌍은 유지한다. 현재 역할의
전체 DWORD를 BYTE로 줄이지 않는다. Bomb163의 전용 그림 쌍은 일반
Bomb genus/VT 관문과 따로 처리한다. 같은 현재 물리 슬롯을 읽는다.

Pre/Post의 공통 부모, 펜스 고유 Pre, 아이스의 두 Post, 피해 Post 직전
현재 상태 준비·가상 삭제/다리 수명은 이미 옮긴 실제 모듈에 연결했다.
의식 수명 포인터는 현재 슬롯에서 선택하므로 아직 미설치인 몸체를
임시 제단/신전으로 대신하지 않는다.

**이번 범위는 진단 장치를 설치하지 않은 독립 typed 경로다.** C++의
HasPatchDiagnostics/UsesNativeTransmissionDevices가 선택하는 Native
자산 통지·Player/Dude/거주 구역·제단/다이스 수명과 Native Outpost 분기는
실제 Client/장기 Storage 결선에 남아 있다. 현재 웹 공통 삭제기에는
BindPatchDiagnostics 경로가 아직 없으며 Native 동등성을 주장하지 않는다.
이 경계는 이전 생성자 단위의 Native 장기 대여 잔여와 같다.

신규8건은 첫 실행에 통과했다. 정적 슬롯 입력으로 현재 수명 자격과
실제 래퍼 공장 순서·선택 부모/인수·기존 fallback/반환·송신·예외 전파를
확인했다. 실제 유닛 Pop/교전·공중3종 새 전용 생성/행동 검사는 하지 않았다.
기존 공통 삭제 부품을 포함한 집중16건도 통과했다. 기존 관련 부품 검사는
전체 회귀에 포함한다. 새 fixture는0이며 새 조립 검사는 소스 계약이다.

| 기존 부품의 원본 fixture | 범위 | SHA-256 |
| --- | --- | --- |
| squid-destroy | Destroy600·Bridge64행 | `d304cecad36bc85a910c9ea09f9eab5e6aed341909dce055f5f3ce81e34b7e20` |
| destroylifecycle | 576행 | `c3200decd6f3028dcfcefb84429e37f05a6d162fedb969fd331e711fb0846a19` |
| priestdestroy | 2,184행 | `56da3fd0581263e9b9d1ba9e39cd60ac1158d5129cbe67ea8e730c873aa6c19d` |

Node v22.23.3·v26.13.0 전체는 각각5,899건 통과했고 실패·취소·건너뜀은0이다.
두 실행 exit0·대응표 오류0·UTF-8/구문/diff 검사를 확인했다.
웹 JS132,543줄/검사784파일·manifest806항목이며 port846/partial39/todo122·session0을 유지한다.
전체 명령은 `--test-concurrency=4`를 사용하며 Node24는 기존 지시로 생략한다.

실제 World 게시/Load·삭제/수명 통합과 Native 장기 장치는 잔여다.
다음은 생성자1757 BuriedRegular와1758..1814의 전체 Regular 분배,
이어 같은 World Runtime 게시1825와 후속 모듈 생성이다.
보호 원본/cpppj 변경·디컴파일·게임/브라우저 실행·실제 교전/포획/희생은
하지 않았다. 검사 로그 접두는 `extracted/wb-e614-`이다.
