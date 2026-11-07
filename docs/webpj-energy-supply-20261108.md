# webpj 에너지 공급(RawEnergySupply) 이식 — WB04-E29

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물아홉 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawEnergySupply`(10.78)를 옮겼다. 마나 공급원(Mana.cpp)이 건설·생산·AI 건조 지점 판정에 쓰는 수집·요구 문자 대응·정렬·소비 몸체이며, 선택 목록(`selected`)은 커서/생산/Builder가 같은 저장소를 빌려 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-energy-supply.js` — `RawEnergySupply`와 `energyProviderEntry`.

- 문자표 `wrtsWRTS.?>`(바로 뒤 NUL이 11번 칸). 공급원 TYPE의 `theme`(0..7)이 원소이고, 수집 문자열은 0..5 원소만 최대 254자이다(6/7 공급원은 선택 목록에는 남고 문구에는 없음).
- `collect(x, y, type, owner, theme)` [4730c0]: 대상 사각형(40e880/40e8f0: 들어온 모서리만 [1,255]로 클램프, 양 끝점이 유효하면 min/max)에서 공급 목록을 순회해 소유자(편집기이거나 같은 소유자이거나 동맹 표 `[공급 소유자×9+요청 소유자]`)와 공급 가상(+a0)이 허용하는 공급원만 세고 선택 목록에 용량까지 다시 쓴다. `theme`이 -1이 아니면 그 원소를 먼저, 나머지는 범용 `s`로 쓴다. 문자열을 돌려준다.
- `match(output, required, theme)` [473330]: 요구 문자(`s/w/r/t` 외는 진단 `TypeError`)마다 같은 문자 → `s`는 s → 점도 대체 원소도 아닌 아무 문자 → 대체 원소 순서로 소비해 점으로 바꾼다. 성공은 대문자, 실패는 소문자로 결과에 쓰고 하나라도 실패하면 0(Override면 1). `{value, output, remainder}`.
- `evaluate(output, x, y, type, owner)` [4734d0]: Override(특수 또는 미션+사용자 치트, 그리고 ignore)면 출력/목록을 건드리지 않고 1, 요구가 비면 출력을 비우고 1, 아니면 대응 성공 시 수집 문자열의 **원래 길이**(bool로 축약하지 않음), 실패 시 0.
- `covers(...)` [41c0f0/41c170]: 기본 공급은 false, 공급형은 발자국 절반을 뺀 raw 좌표와의 거리 제곱이 현재 제곱 반경 + 대상 크기 항보다 작으면 true. 83개 가상 표 밖은 거부.
- `takeSource()` [473570]: 첫 비영 DWORD만 0으로 바꿔 돌려주고 count는 줄이지 않음. `sortSources(demand, x, y)` [474970]: 요구 길이만큼 가까운 공급원을 최대 10개 골라 같은 목록 앞에 다시 쓰고(미선택 꼬리는 지우지 않음), 진단 두 위치(321 잘못된 공급원 / 336 잘못된 best 색인)는 소비자가 정상 복귀하면 원문처럼 이후 검사를 계속.
- 계산은 x87 중간 float 저장 위치를 따라 `Math.fround`를 쓴다(관측 1058·567행이 두 정밀도 열과 같거나 `-`).
- 대응 PORT_MAP 행: `RawEnergySupply.cpp`. 가상 표 83개는 `decomp_energy_supply`가 현재 원본과 다시 확인한 값을 `PROVIDER_ENTRIES`로 옮겼다.

## 2. 검증 (`tests/energy-supply.test.mjs`, 5건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음):

- `energy-supply-x86.tsv` 1058행: 다섯 연산(대응·수집·평가·덮개·소비)의 반환·공급 문구·잔여 문자·전체 30DWORD 선택 목록·소비 값. 두 번째 정밀도 열은 `-`이거나 같은 값.
- `energy-sort-x86.tsv` 567행: 정렬 직후와 연속 소비 뒤의 전체 목록(원본이 지운 꼬리 포함)·진단 반복·소비 DWORD 반환·정렬 스냅숏.
- `energy-provider-vtables.tsv` 83행: 가상 표 → +a0, 공급형 5개, 표 밖(0) 거부.

계약 검사: 알 수 없는 가상 표(`0xdeadbeef`)·미복원 요구 문자 거부, Override 우회와 현재 전역(`reads.special`) 재조회, 판본/TYPE 수/동맹 표/조회 계약 거부, 진단 소비자가 없으면 손상 목록 정렬을 성공으로 바꾸지 않음.

최초 실패와 교정: 5건이 처음부터 통과했다.

## 3. 경계

- 실제 AI 건조 지점(`RawAiDrySpots`)·Builder·커서 배치·생산 갱신이 이 객체를 빌려 쓰는 연결 검사(`AiDrySpots_ActualEnergySupplyChangesRemoveCoordinate`, `ConstructionBuilderTests`, `CampaignGameMenuTests`)는 해당 소비자 단위를 옮길 때 대조한다.
- 요구 문자열은 `hooks.requirement(type)`로 주입한다. TYPE 정의 AST에서 만드는 실제 값은 [E26 `typeRequiredEnergy`](webpj-region-sid-lists-20261108.md)/`bindAiTypeRequiredEnergy`가 낸다(보호 자산 연결은 자산 적재 단위).
- 선택 목록의 실제 Player 배열 연결(`SquidPostPopList.bindCurrent`)이 아니라 독립 목록(`entries`/`count`)을 대상으로 검증했다. CD 10.72는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등). 작은 후보는 `RawMemStream`·`RawProjectileGeometry`(`RawGunMode` 의존)·대본 부모 `RawAddRandom`/`RawDestroyAt`/`RawMakeAi`/`RawCreateAt`이다.
