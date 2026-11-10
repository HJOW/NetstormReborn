# 최초 PostPop NoWorld 잔여 부분 감사 인계

2026-11-10 / HJOW-Athlon. 부모의 사용자 일시중지 전달을 받아 조사 범위를 더 넓히지 않고 현재 자료를 보존했다. 제품/LEFT_JOBS/docs 수정·빌드·단위/패킷/통신/게임 실행·커밋은 모두0이다. 마지막 Client 디컴파일2026-11-03/HJOW-Athlon과 현재 호스트가 같고 새 디컴파일은 없다.

## 실제 확보 범위

`inventory.py`는 기존 `cpppj/no-world-regular-audit-20261109.json`에서 **자산83개의 VT 주소만** 가져와 보호 PE의 **+20 DWORD를 새로 읽었다**. 옛 Regular(+5c) 판정은 사용하지 않았다. 결과는 `inventory.json`의83행/원본 해시/검토 소스5개 해시에 있다. 현행1264의 최초4VT(Dude·그라비테이션·힐·미티어)도 포함했다.

- 실제 +20은 **46개 함수**를 가리킨다.
- 현행에서 확인한 진입 경로는 공통 허용표28·장기 전용 분배18·장기 접두3, 합계49행이다. **이는 전체 하위/호출자/실행 검증 완료 수가 아니다.** 공통 AI·선택 재검사 및 장기 전용의 선택된 큰 후단은 별도다.
- 최초 flags1에서 미연결32행과 최초 이외에만 경로가 있는2행(Residence·Teleport), 합계 **최초 진입 잔여34행**을 찾았다. 각 행의 활성 원본 caller까지 전수 대조한 것은 아니다. 실제 현재 NoWorld caller/도달성·수명/하위를 마쳐야 구현 마감할 수 있다.
- 최초 helper의 SurfacePrefix 주소를 잘못 넣었던 것을 실제 `RawIslandPostPop.h`의 `kPatchNoIslandVtable=00506f08`로 교정했다. 최종 JSON/스크립트는 교정된 값이다. 0050c9f0은 SurfacePrefix가 아니다.

## 남은34행의 원본 VT / 실제 +20

| 부류/기존 본문 | VT → +20 |
| --- | --- |
| Generator / RawAssetPostPop |00501ea0 → 0041c150|
| 크리스탈 크랩 / RawCarrierSteal |00503f20·00503ff0 → 0042abb0|
| Teleport / Session teleportPrefix |005070c0 → 004429f0 (flags1만 미연결)|
| Rune / RawRunePostPop |005075d8 → 00448930|
| 제단 / RawDaisLifecycle |005076b8 → 00448ad0|
| 보호막 / RawForcefieldPostPop |00507770 → 00448d20|
| 다이스 / RawDaisLifecycle |00507950 → 00449320|
| Aviary3 / RawSunAviary·RawRainAviary·RawWindAviary |00509930 → 0045ec50,005099e8 → 0045ed10,00509aa0 → 0045edf0|
| Flyer3 / 기존 전용 비행 후처리 |0050a050 → 0045e810,0050a1e8 → 0045ef30,0050a388 → 0045fac0|
| 장식 Gun / RawAssetPostPop |0050b570 → 004694b0|
| Gun Angle/First/Archer/Sun / RawAssetPostPop |0050b6e8 → 0046a520,0050b7c0 → 0046a600,0050b888 → 0046a810,0050b9b8·0050bad0 → 0046aca0|
| 메타 Sprite 공통 진입 허용표 누락 후보 |0050c9f0·0050cc08 → 004b0d30|
| 아크/애시드 / RawAssetPostPop의 기존 순수 접두 |0050e668 → 0048c080,0050e738 → 0048c7a0|
| Blocker3 / RawAssetPostPop |0050e940·0050ea08·0050ead0 → 0048eda0|
| Player / RawPlayerWorld |0050ee08 → 004905d0|
| 사제 / RawPriestPostPopTail |0050f210 → 004950f0|
| 거주 구역 / RawAssetPostPop |0050f540 → 0049a2b0 (flags1만 미연결)|
| 신전4 / RawAssetPostPop |005196f0 → 004dce40,005197a8·00519860 → 004dd020,00519918 → 004dd1f0|

## 현행 미연결 위치와 재개점

`o/SquidPostPop.cpp::DispatchPostPop`은 Native 기본 장치와 `!HandlesDerived`일 때 일부 주소/RawAsset24부류를 `RawSurfaceStorage::DefaultResourcePostPop`에 보낸다. 그 기본 분배는 장기18행과 Residence 비최초만 구현돼 있다. 나머지 RawAsset 종류는 `World없는현재VT의전용자원PostPop몸체미설치`에서 멈춘다. Player/Rune/Dais/Priest/Forcefield/Flyer/Crab은 기본 Native predicate에도 없으며 Session이 등록한 포인터를 반납하면 마지막 `현재가상후처리몸체미복원` 경계가 남는다. Teleport의 실제 flags1은 `teleportPrefix_`가 필요하고 `ReleaseRuntimeBindings`가 이를 지운다.

`RawCampaignSession.cpp`의 종료는 `SetDude/CarrierSteal/Bomb/Player/Buried/Flyer/Rune/Dais/BalloonPostPop(nullptr)`와 `ReleaseRuntimeBindings()`로 현재 대여를 분리한다. `RawCampaignAssetBindings::~`는 Asset/Factory 포인터를, `RawPriestWorldBindings::~`는 Priest/Forcefield 포인터를 분리한다. 이 반납을 삭제하거나 죽은 Session 포인터를 오래 보유하는 방법으로 고치면 안 된다.

재개 때 먼저 작은 순수 접두부터 확인할 수 있다.

1. **보호막00448d20**: 기존 `RawForcefieldPostPop::Prefix`는 flags1에서 실제 new40 두 번을 서로 독립적으로 시도해 event/frame0.08·check0.01을 예약한다. 첫 할당 실패여도 둘째 시도가 필요하다. 기존 장기 Host/예약 장치로 연결할 여지가 크며 새 World/AI 상태가 필요한 본문은 아니다. Native 기본 분배 predicate와 실제 기본 본문 연결을 함께 해야 한다.
2. **Generator·장식 Gun·아크·애시드**: 기존 RawAssetPostPop의 선택된 분기는 현재 TYPE theme의 순수 읽기 또는 Regular 접두와 실제 Damageable/base를 사용한다. 장기 RawAsset 부모를 빌리는 후보가 있으나 전체24종을 성공으로 허용하지 말고 도달한 기존 몸체만 연결한다.
3. **Residence/Blocker/Gun/신전**: `AssetPostPopState`의 값형 `loadingDepth/fortMode/residenceAnimation/templeCount/selected`를 단순 사전 snapshot으로 복사하면 콜백 뒤 현재값과 달라질 수 있다. 동일 장기 `SerializationGlobals`·목록·현재 owner/모드·난수·Host·Gun/Contained/지역/실제 UI 장치의 필요한 소비 위치를 각각 연결해야 한다. `DefaultResourcePostPop`의 Residence flags0 공통 경로를 최초 전체 구현으로 세지 않는다.
4. **Player**: `RawPlayerWorld`는 현재9기록 span을 보관하고 `PlayerWorldState.fortMode`를 읽는다. 장기화 때 Player 재초기화/재할당 이후 span과 현재 Storage의 fortMode가 어긋나지 않게 실제 현재 접근 경계를 확인해야 한다. 현재 +9c Technology SID와 조건부 Regular5.0/special 직접frame1 순서다.
5. **메타 Sprite2**: 실제 +20은 공통004b0d30이고 `SquidUnpop::SupportsDisplay`는 이미0050c9f0/0050cc08을 true로 처리한다. 그러나 `SquidPop::Supports`의 공통 허용표/특별허용에는 둘 다 없어 현재 NoWorld PostPop의 마지막 거부가 남는다. 이 두 VT의 실제 생성/Pop caller를 먼저 확인한 뒤 공통 경로 허용 누락인지 판정한다. 이번 조사에서 활성 caller 전수는 못 했다.

원본 전체 함수/하위 순서 재대조·미분석34행의 실제 caller/수명·수정·컴파일·허용검증은 미완료다. 공중3종/사제 포획·희생/MP 실행은 사용자 제외를 유지한다. Snapshot/AI 큰 후단은 부모의 다른 감사와 합쳐야 하며 이83행 +20 집계로 전체 NoWorld/M6를 닫지 않는다.
