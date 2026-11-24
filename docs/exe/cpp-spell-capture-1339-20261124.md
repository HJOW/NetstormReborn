# 주문 생성 제한과 사제 포획 mask 교정 — 제품1339

2026-11-24, HJOW-Athlon. 사용자가 webpj의 주문 보유 골렘 포획 문제를 원본 디컴파일 코드로 확인하고, 필요하면 cpppj에도 반영하도록 지시했다. 현재 호스트와 마지막 정밀 디컴파일 호스트는 HJOW-Athlon으로 같다. 원본/정밀 산출물 변경·게임 실행은 없다. 원본 10.78 SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다.

다음 원문을 확인했다.

- `004277f0`(정밀 `extracted/refined/originals/Netstorm.c`, 신뢰도A): `004ac9f0(this,0x20,0,0)` 결과가 운반물 유무이며, 주문1 검색 결과는 포획 조건에 사용하지 않는다. 지상/비행 모두 운반물이 없으면 적 사제 후보를 허용한다.
- `004ac9f0`(`extracted/cpp-resume-1201/originals-004ac9f0.c`): raw+20 WORD에 mask를 AND한다. 주문은1, 실제 운반물은0x20이다.
- `00426520`(`extracted/cpp-m6-delayed-prep/l1-bridge/native/00426520.c`): 원문 수신에는 NB18 방어가 없고 대상 genus0x100이면1, 그 밖이면0x20으로 내용물을 만든다.
- `00444bc0`(같은 경로의 `.c`·`.asm`): flags2 bit0x100으로 주문만 구분하고, `00444dd9~00444ddc`에서 TYPE를 일반 공장004af530에 그대로 전달한다. 여섯 주문 whitelist는 없다. 기존 제한은 `cpp-carrier-drop-session-20261020.md`에 기록된 당시 통합 이월이었다.

REA Ghidra12.1.4 기본 분석도 같은 불변 원본을 열었으나00426520 함수가 기본 분석에 없어 주소 조회가 실패했다. 독립 load-image 검증은 PE 미지원이라고 명시됐다. 이 결과를 함수 동작 근거로 삼지 않고 기존 동일 호스트 정밀 복구 산출물과 asm을 사용했다. REA 세션은 `extracted/webpj-e1318-rea-session.json`에 저장하고 닫았다. 마지막 디컴파일 PC는 변경하지 않았다.

NB18은 너겟/포로 중복을 막는 제품 방어로 유지하되 `RawCarrierPickup::RefusesHeldPriest`의 mask1을0x20으로 바꿨다. 주문만 든 운반체의 원래 포획 기능을 복구하고 실제 운반물은 요청/수신 모두 막는다. `KnownBugTests`의 find 훅도 mask를 구별하게 고쳐 주문1·운반물0x20·둘0x21·빈손·스위치 꺼짐을 구분한다. 이전1304의 ‘기도 보유 시 거부’ 기대는 당시 잘못된 mask를 고정했으므로 이번 원문 확인으로 대체한다.

`RawCampaignCarrierDrop.inc`의 여섯 주문 whitelist도 제거했다. 실제 World 생성자와 TYPE 범위 진단은 유지한다. 새 `CampaignCarrierDrop_ActualAllSpellKindsUseWorldFactory`는 실제 Session의 내용물·9바이트 수신으로 주문19종 전부가 생성·소유자/시전자 지정·Pop·활성화되는지 확인한다. `CampaignDevastation_ActualEnemyCaptureAndDropFromCast`는 다른 빈 골렘으로 대체하던 경로를 없애고 실제 기도를 시전한 같은 골렘이 실제 경로 도착→로컬 수신→포획→Drop을 수행하게 고쳤다. 기도·SP 보존도 확인한다. 자연 획득/전투/승리를 검증한 것은 아니다.

Release 집중 대상 `netstorm_world_generation_tests`·`netstorm_known_bug_tests`·`NetstormCpp` 빌드 성공. Cast/Drop3/3, 실제 시전 골렘 포획1/1, KnownBug8/8 통과다. 처음 전체 netstorm_tests 빌드는 공용 헤더로 무관한 수백 소스를 재컴파일해 중단하고 기존 집중 대상으로 전환했다. 전체 CTest/GUI/배포 ZIP은 이번에 실행하지 않았다. 로그는 `extracted/cpp-1339-{focused-build,carrier-drop,capture,known-bugs}.log`이며 Netstorm.exe SHA-256은 `116d44b3e08ca27fbc69a3b43aedf83f03e2b9ebaf1b64dcb87672cb02917564`다.

웹은 [E1318 주문](../webpj-spell-cast-20261124.md)·[E1319 포획](../webpj-spell-priest-pickup-20261124.md)에 대응한다. 일본어 번역의 다음 단위는1340(사제 훈련bc4~6)으로 옮긴다.
