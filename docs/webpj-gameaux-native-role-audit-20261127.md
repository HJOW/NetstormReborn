# Native GameAux 역할·이벤트·진단14칸 분류 — WB04-E1457

2026-11-27 HJOW-Athlon. E1451 뒤14칸을 **typed 표현13·개발자 전용 제외1**로 분류했다. E1451의헤더10개와 합하면24개는 typed 표현21·제외3이다. 제품을 바꾸지 않았으며 **Native129개 공용 주소의 동일성을 웹이 제공한다는 판정이 아니다.**

이름 부재가 아닌 실제 선택 분기로 판정했다. C++ `RawCampaignSession.cpp:586-587`은 Native 장치가 없을 때 `localPriestState{&GameAux.PriestType()}`를 선택한다. 이때 사제 TYPE는 장기 owner를 빌리지만 이동 역할 다섯 칸은 지역 기본값이다(`RawPriestWorldBindings.cpp:11-12`). `localPiece`(623-624)·`localSunFlyerType`(909)·`typedAcidTypes`(775)·`localDamageablePostState`(838)도 같은 Native/typed 선택을 명시한다. 실제 웹 Storage는 Native 송신 장치를 설치하지 않으며 이 typed 상태가 대응 경계다.

| Native 칸·초기값 | 웹의 현재 표현과 근거 |
| --- | --- |
| `dudePathType` 24 | `RawMovementProcessTypes`의 지역 `dudeProcess`; 실제 `raw-campaign-session-damageable.js:73`의 `pathType`도24다. Native의 장기 Current 역할 공유와 구분한다. |
| `bulfType` 135 | 이동 지역 역할과 `DamageablePostDestroyState.specialDropType`의135다. C++의 공용 포인터 연결은 `RawSurfaceDamageable.cpp:26` Native 장치 관문 뒤다. |
| `rainWalkerType` 156 | 이동 지역 역할과 typed 삭제 드롭 제외 배열156이다. C++ typed도 지역 post 상태를 사용한다. |
| `priestForcefieldType` 167 | `PriestForcefieldState.type`167을 같은 조회/생성 몸체에 전달한다. C++의 장기 공용 owner 조회는 `RawSurfacePriestDeletion.cpp:9` Native 관문 뒤다. |
| `puzzlePieceType` 107 | Session `piece`와 실제 건설 `patternTypesRead`의 첫 역할이다. C++도 typed에서는 `localPiece`107이다. |
| `windBlockerType` 142 | 실제 건설의 현재 `confirm.patternTypes[7]`을 읽는다. 각 typed 건설 상태의 초기 배열 마지막 값은142다. Native의 `ClientConstructionGlobals` 공용 역할 배열과 구분한다. |
| `sunFlyerType` 73 | Session 지역 `sunFlyerType`과 typed 애시드의 `fenceTypes.sunFlyer`73이다. C++에도 같은 지역 선택이 있다. |
| `rainBalloonType` 146 | typed `fenceTypes.rainBalloon`146이다. |
| `windBalloonType` 141 | typed `fenceTypes.windBalloon`141이다. |
| `sunBalloonType` 111 | typed `fenceTypes.sunBalloon`111이다. C++ 세 벌룬의 공용 포인터는 `RawSurfaceAttack.cpp:55`의 Native 장치 관문 뒤다. |
| `damageRegularEvent` 6699 | `AssetRegularState.damageEvent`·typed 삭제 후처리 `damageEvent`6699, Dude `pollEvent=0x1a2b`다. C++ typed `RawAssetRegular.h:20`·`RawDamageablePostDestroy.h:17`도 같은 지역 이벤트를 쓴다. |
| `constructionRegularEvent` 6700 | `AssetRegularState.constructionEvent`6700과 해당 Regular 분배다. C++ typed `RawAssetRegular::DamageableHandle`도 지역 상태 번호를 비교한다(282줄). |
| `residenceMaximumScan` 0 | 웹 `AssetRegularState.maxDecorationScan`의 지역 진단 집계다. `raw-asset-regular.js:389`는 signed 현재값 비교 뒤 최대 수만 기록한다. C++ Native는 Client 장기 GameAux에 같은 진단을 기록한다. 이 카운터는 게임 선택/난수/생산을 바꾸는 입력으로 소비되지 않는다. 장기 주소/Session 간 통계 보존은 같다고 하지 않는다. |
| `buildReplayProcessType` 49 | 개발자 입력의 건설 기록 재생 Process 부착용이다. [E1242 기존 웹 제외 판정](webpj-build-replay-scope-20261123.md)을 유지한다. 일반 미션 Replay/Restart와 혼동하지 않는다. |

초기에 선택 가능한 역할 owner 생성자만 보고 실제 Session의 연결 누락으로 의심했다. Native 공용 역할을 기대하도록 만든 독립 실험3개는 실패했지만, 위 C++ 선택 분기가 **그 기대를 현재 typed 계약으로 요구하지 않음**을 확인해 제품 결함 판정을 철회했다. 실험 원문은 무시된 `webpj/extracted/e1459-before.log`에 보존하며 기존 검사의 기대값을 바꾸거나 제품을 수정해 통과시키지 않았다. 지원되는 생성자 인자 `roles`를 명시하면 공용 owner를 읽고 쓰는 모듈 기능 자체는 유지된다.

실제 Client와 같은 장기 Storage/Gameplay와 짧은 Session 생성자를 관찰했다(Load/플레이 없음). Native=false, 이동133/187/156/135/24·보호막167·조각107·패턴 마지막142·이벤트6699/6700이었다. 지역 이동을155/25로 바꾼 뒤 다음 Session은156/24로 되돌아오고, 장기 사제 TYPE157은 유지됐다. 이 결과는 위 C++ typed 소유 경계와 맞는다. 초기값만으로 Native 공용 주소 계약까지 같다고 추정하지 않았다.

판정 대상 웹10파일의 원시 해시는 E1453 Node22전체6,923/6,923 때와 모두 같다. 이번 단위는 새 플레이·오라클·전용 회귀 시험을 추가하지 않았다. 현재 E1458 Node26전체는 실행 중이므로 이 문서에서 통과로 세지 않는다. 전체 입력 고정을 유지하며 문서만 커밋하고, 종료 후v26에1개 이월·v22=5/8로 센다.

[소스/관찰 식별자·분류·철회한 실험](webpj-e1457-evidence/provenance.json). 3.6절의 Native GameAux 잔여 분류는 이 단위로 마쳤다. 향후 Native 장치를 실제 웹 경계에 도입하거나 공용 역할/헤더 임의 변경을 공개 계약으로 제공하기로 정하면 해당 공유 owner 연결과 수명/실제 소비자 검증은 새 범위로 설계한다.
