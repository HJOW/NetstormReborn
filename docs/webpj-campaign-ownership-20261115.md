# 캠페인 소유권 연결 이식 (WB04-E518)

4-D 구획 C의 `client/RawCampaignOwnership`을 `webpj/www/core/raw-campaign-ownership.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 `git diff --numstat 225553df HEAD`로 확인했다.

## 옮긴 범위

- 독립 조립 생성자: 월드 Core의 풀·해시·TYPE·프레임·공통 Owner를 그대로 빌려
  지면 getter(`RawCanonPlacementTerrain`, 지역 조회만 사용) → 도장(`RawRegionPaint`)
  → 투표(`RawRegionOwnership`) → 신전 Owner(`RawVortexOwner`)를 한 번 구성한다.
  섬 런타임에는 같은 9 Player 기록을 연결한다.
- `setOwner`: 신전 네 가상 표만 신전 Owner로 보내고 나머지는 상위 분배기에 넘긴다.
- `islandNumber`: float 좌표를 CRT 방식으로 먼저 절삭하고(비유한·int32 밖은 거부)
  살아 있는 hash0 표(`hash.view(0)`)에서 지역을 조회한다. 사본(`entries`)을 쓰면
  이후 표면 변경을 읽지 못하므로 쓰지 않았다.
- `updateRegion`·`updateRegionNative`·`vortex()`.
- Native 두 번째 생성 경로(`RawCampaignOwnership.native(owner, otherOwner)`): cpppj는
  `ClientConstructionGlobals`가 보유한 장기 본체를 빌린다. 웹에는 그 소유자가 아직 없어서
  `{ vortexOwner(), islandNumber(x, y), regions().updateNative(x, y) }` 형태의 덕타입을 받는다.
  `ClientConstructionGlobals`(대응표 todo) 이식 때 같은 접근자를 구현한다.
- 생성자는 다른 풀·표 크기 불일치·필수 효과 네 개 누락을 공유 상태를 쓰기 전에 거부한다.

## 시험

`webpj/tests/raw-campaign-ownership.test.mjs` 5건:

1. `VortexOwner_CampaignSameCoreAndFirstRegion` 대응: 실제 TYPE 123(가상 표 `0x519860`)의
   소유자 변경이 Player 신전 두 DWORD·공통 Owner·생산 갱신 1회를 만들고, 섬 소유자
   변경은 Player 첫 지역만 별도로 갱신한다.
2. 비신전 가상 표(`0x503360`)는 상위 분배기로만 가고 신전 Owner를 건드리지 않는다.
3. 다른 풀·표 크기·필수 효과 누락·잘못된 Player 9칸은 거부하며 Player/섬 기록이 변하지 않는다.
4. plain getter의 절삭과 범위 거부(웹 전용 경계 시험).
5. `ClientRegions_WrapperSharesBodyAndCurrentDevicesPinSelectedCall` 대응: Native 경로가 같은 장기
   신전 본체를 빌리고 비신전 분배만 현재 Session에 남긴다.

최초 시험 실패 세 건과 교정:

- 신전 가상 표 기대값을 `0x5196f0`으로 적었으나 TYPE 123 생성자는 `0x519860`을 쓴다(시험 입력 오류).
- 비신전 표 설정에 `pool.slot()`(읽기 사본)을 써서 쓰기가 사라졌다. 살아 있는 `pool.allocated()`로 바꿨다.
- JS 클래스는 `Object.create`로 만든 객체에 `#private`을 쓸 수 없다. 내부 표식(`NATIVE_MODE`)으로 같은 생성자가 Native 경로를 받도록 바꿨다.

## 남은 경계

- 실제 `ClientConstructionGlobals`는 아직 없다. 덕타입 Native 시험은 그 접근자 계약만 확인한다.
- Outpost 지역 소유 분기(`MakeOutpostRegionOwnershipHooks`)는 12차 스커미시 단위 몫이다(3.7절).
- 이 단위는 부품 대조이며 앱 수명에 연결하지 않았다(PORT_MAP `port`, session 아님).
- 다음 시작점: `RawCampaignWorldRuntime`(이 소유권 연결을 `RawCampaignOwnership&`로 요구한다).
