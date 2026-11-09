# webpj 에너지 공급·캠페인 피해 가상 표 자료 이식 — WB04-E250

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-C의 첫째·둘째 항목(`EnergyProviderVtables`·`CampaignDamageVtables`)을 한 단위로 옮겼다. 둘 다 코드가 아니라 **실제 보호 PE 10.78의 가상 표 83개에 대한 진입 주소 표**(`.inc`)이며, cpppj 가 `decomp_energy_supply.py` 등으로 현재 원본과 다시 확인하는 자료다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 두 `.inc` 모두 기준선 뒤 변경이 없다(저장소 루트 `git diff --numstat 1f00a25a HEAD`가 빈 결과).

## 1. 소비자 확인

| 자료 | cpppj 소비자 | 웹 소비자 |
| --- | --- | --- |
| `o/EnergyProviderVtables.inc`(+a0 에너지 공급 진입, `{가상 표, 함수}` 83쌍) | `o/RawEnergySupply.cpp` | E29 `raw-energy-supply.js`의 `energyProviderEntry` — 이미 같은 표를 모듈 안에 손으로 복사해 갖고 있었다 |
| `o/CampaignDamageVtables.inc`(+98 피격 진입 83쌍·+9c 방어 진입 83쌍) | `client/RawCampaignHitBindings.cpp` | E210 `raw-damageable-hit.js`의 `makeDamageableGunHitHooks(damage, entries, hooks)` — `entries`를 호출자가 공급해야 했고 실제 표는 웹에 없었다 |

## 2. 옮긴 것

- **`tools/make-vtable-tables.mjs`**(새 도구): 두 `.inc`의 쌍을 읽어 웹 자료 모듈 둘을 만든다. `--write`로 다시 만들고 `--check`로 재생성 결과가 현재 파일과 같은지 본다(WB04-D4 `squid-constructors.js` 방식). 값은 계산하지 않고 옮기기만 한다.
- **`www/core/energy-provider-vtables.js`**(생성): `ENERGY_PROVIDER_ENTRIES` — `[가상 표, +a0 함수]` 83쌍(얼림). `raw-energy-supply.js`의 손으로 쓴 표를 이 모듈의 `import`로 바꿨다(결과 동일, `energy-supply` 기존 검사 통과).
- **`www/core/campaign-damage-vtables.js`**(생성): `CAMPAIGN_DAMAGE_HIT_ENTRIES`·`CAMPAIGN_DAMAGE_DEFENSE_ENTRIES` — `{ vtable, address }` 83개씩(얼림). 피격 진입은 일반 Base `0x004add10`·Damageable `0x0044c8e0` 외에 사제·신전·특수 피해 넷(`0x00448b70`·`0x0048ef20`·`0x00494380`·`0x004dba40`)이 있고, 방어 진입은 기본 false `0x0043d9b0`와 TYPE 142 특별 방향 `0x0048ef70`이다. `.inc` 의 TYPE 번호 주석(어느 TYPE 이 어느 진입을 쓰는지)은 옮기지 않았다(분배는 raw 슬롯의 현재 가상 표만 본다).

## 3. 검증 (`tests/vtable-tables.test.mjs`, 4건)

- 모듈 값이 `.inc` 의 쌍과 **순서까지** 같고(세 배열 83개), 생성기를 다시 돌린 본문이 현재 모듈 파일과 같다(`--check` 와 같은 비교, 줄바꿈 변환 무시).
- 불변식: 세 표가 **같은 실제 PE 가상 표 83개를 같은 순서로** 담고(중복 없음), 진입 함수 주소는 알려진 집합(에너지 2·피격 6·방어 2) 안이며 모든 종류가 실제로 쓰이고, 공급형 `0x0041c170`은 정확히 다섯 개이고, 표는 얼려 있다.
- 소비자: `energyProviderEntry`가 83쌍을 그대로 돌려주고 표 밖(0, 1 차이)은 거부한다. `makeDamageableGunHitHooks`에 피격 표를 넣어 Base(`0x00501dd0`)→`baseHit`, Damageable(`0x00501ea0`)→`hit`, 사제/신전/특수 진입과 표 밖 가상 표→기존 `damage` 훅으로 분배되는지 확인한다.

최초 실패와 교정: 처음부터 통과했다. 모듈 값 변형(피격/방어 주소·순서·에너지 가상 표/진입)을 직접 넣은 5개는 모두 탐지됐다. 본문은 생성 모듈이라 값의 정당성은 `.inc` 가 보증하고, `.inc` 자체가 원본 PE 와 일치하는지는 cpppj 의 `decomp_energy_supply.py`·피해 표 생성 도구 몫이다(웹은 원본을 다시 읽지 않는다).

## 4. 경계와 다음 시작점

- `RawCampaignHitBindings`(client 영역 D, todo)가 이 표로 캠페인 세션의 피격·방어를 분배하는 연결은 4-D/7 에서 한다. 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0).
- 이 단위로 두 `.inc` 행은 PORT_MAP `port`다. 3-C의 다음 항목은 `RawDamageablePreDestroy`(+36)다.
