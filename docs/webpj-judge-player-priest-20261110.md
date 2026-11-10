# webpj 3-E 판정-Player·사제·신전 — WB04-E260 (2026-11-10)

3.1절 3-E의 두 번째 항목 **판정-Player·사제·신전** 12단위를 판정했다. 방법과 ①/②/③ 기준은 [판정-공통층 문서](webpj-judge-common-20261110.md)와 같다: 이식 원본은 기준선 `1f00a25a` 판이고, `git diff 1f00a25a HEAD -- <단위 파일>`(12단위 `.cpp/.h` 약 470줄 전부)과 새 API 의 소비자(`grep -rn … cpppj/src`)를 읽어 단독 미션의 웹 결과가 바뀌는지만 판정했다. 판정 기준 커밋은 착수 때 `HEAD` `b0d261cf`(20261110-113 WB)이며 행 base 는 단위 파일을 마지막으로 바꾼 커밋이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 판정 표

| 단위 | 판정 base(기준선 뒤 커밋) | 기준선 뒤 변경 | 판정 | 처리·시점 |
| --- | --- | --- | --- | --- |
| RawPlayerContentNotice | `a942a75a` (1) | `battleShown` 필드의 설명 주석만(00595344 PlayerKnown) | ① | 이름·폭·동작 불변 |
| RawPlayerGeyserBounds | `3102fadd` (2) | 현재 Player 기록 조회기를 받는 Native 생성자·`BindCurrentPlayers`·`BoundsRecord` | ③ | **4-D**(Player 배열 장기 소유). 독립 typed `Bounds` 결과 동일 |
| RawPlayerIdentity | `9f7c0604` (1) | `PlayerIdentityReads.metaRecords`(현재 CS+b70 의 40기록) | ① | metamap 분기에서만 읽힘(메타 웹 제외). 고정 span 독립 계약 불변 |
| RawPlayerParticipation | `3f766a97` (1) | `RemoveAlly`·`ReceiveAllianceRequest`/`BrokenAlliance`/`Draw`(77h/79h/6fh)·`HandleAllianceRequestInput`(키 fb/fa/7e/db) | ① + ③ | 일반 전투의 동맹 협상·무승부(웹 제외; 소비자 `RawCampaignSession` 수신·`UberGump` 입력). 기존 `AddAlly`/`SetAlliance` 결과 동일. **6**(입력) 소비자 이식 때 단독 도달 재확인 |
| RawPlayerWorld | `c9140743` (1) | `PreDestroyNative`: 주소 계산마다 현재 Player 기록을 다시 구하는 +9c 해제→공통 Pre→+50 bit0 해제→bit8 설정 | ③ | **4-D**. typed 경로 불변 |
| RawPriestDamage | `a84509c9` (1) | `invulnerable` 이 `RawCheckingFlag`(현재 55a48c DWORD 뷰)로 바뀜 | ① | 웹 `PriestDamageState` 는 이미 전체 DWORD 비영 판정(`sharedInvulnerable` getter)으로 같음 |
| RawPriestForcefield | `f9db0e53` (1) | 현재 TYPE 표·역할 DWORD 참조를 받는 Native 생성자(Find 의 SID 범위/raw 선검사 생략) | ③ | **4-D**. 독립 typed 불변 |
| RawPriestPreDestroy | `f9db0e53` (1) | 목록 제거를 `RemoveStateSidList` 로 공유·`EnableNativeBody` | **②** + ③ | **재반영**: `removeStateSidList`(E259) 사용. Native 본체 선검사 생략은 **4-D** |
| RawPriestRegen | `c54f488c` (1) | 회복 진입이 TYPE BYTE 158 을 따로 검사하지 않고 가상 표만 확인·`sharedSample` 표본 주소 | **②** + ① | **재반영**: TYPE 검사 제거. `sharedSample` 은 웹 `sharedSample` holder 가 이미 같은 역할(①) |
| RawVortexAscend | `11fbb988` (1) | 후보마다 현재 noIsland TYPE 을 읽는 `sharedNoIslandType` | ① | 웹 `raw-vortex-ascend` 는 이미 `sharedNoIslandType` holder 를 후보마다 읽음 |
| RawVortexOwner | `f0d065dc` (1) | 현재 Player base 조회기를 받는 Native 생성자·`HandlesRawVortexOwner` | ③ | **4-D**. typed 결과 동일 |
| RawVortexPreDestroy | `f0d065dc` (1) | outpost 반복의 index/count signed 비교, 현재 Player base 조회기 Native 생성자 | **②** + ③ | **재반영**: signed 비교. Native 생성자는 **4-D** |

합계: ② 재반영 3(RawPriestPreDestroy·RawPriestRegen·RawVortexPreDestroy)·① 단독 5(RawPlayerContentNotice·RawPlayerIdentity·RawPlayerParticipation 일부·RawPriestDamage·RawVortexAscend)·③ 단독 4(RawPlayerGeyserBounds·RawPlayerWorld·RawPriestForcefield·RawVortexOwner) — 12단위가 모두 판정됐다(RawPlayerParticipation 은 ① 우선, 입력 소비자 재확인을 6단계에 남김).

## 2. ② 재반영 3건

1. **RawPriestPreDestroy** (`www/core/raw-priest-pre-destroy.js`): 인라인 압축 루프를 `removeStateSidList(list, sid)`(E259, PATCH 0040ea00 signed count)로 바꿨다. 정상 count 의 결과는 같고(원본 10.78 2184행 fixture 통과), 음수 count 는 반복하지 않으며 물리 범위 밖 칸은 `RangeError`다. 앞의 `list.items()` 선검사와 Carrier 선검증은 독립 typed 계약이라 그대로 둔다.
2. **RawPriestRegen** (`www/core/raw-priest-regen.js`): 회복 분기가 `raw[10] !== 158` 로도 거부하던 것을 가상 표(VT)만 확인하도록 바꿨다(cpppj 주석: "원본 가상 진입은 현재 TYPE BYTE 가 158 인지 별도 검사하지 않는다"). 오류 문구는 `사제 회복 가상 표 오류`. TYPE 이 다른 사제 VT 슬롯은 회복 자신이 거부하지 않고 하위 몸체(사제 HP 변경의 자체 TYPE 검사)에서 멈춘다.
3. **RawVortexPreDestroy** (`www/core/raw-vortex-pre-destroy.js`): outpost 반복 조건을 `(index|0) < (count|0)` 로 바꿨다. count 의 상위 비트가 서 있으면(음수) 한 번도 반복하지 않는다.

## 3. 검사와 변형 확인

- 신규 시험 2건을 기존 파일 끝에 더했다: `tests/priest-regen.test.mjs`(TYPE≠158 사제 VT 는 회복 자신의 오류로 거부되지 않고, 사제 VT 가 아니면 `사제 회복 가상 표 오류`) / `tests/vortex-pre-destroy.test.mjs`(음수 count 는 풀 밖 outpost 에 도달하지 않고 양수 count 는 거부). `removeStateSidList` 자체는 E259 의 `judge-common-layer.test.mjs` 4건이고 priest-pre-destroy fixture 2184행이 소비 쪽을 재확인한다.
- 변형 확인(되돌려 보기): 두 새 시험은 해당 재반영을 옛 코드로 되돌리면 실패하고(vortex 1건·regen 1건 탐지) 다시 적용하면 통과한다. 처음 쓴 regen 단언은 오류 문구 정규식이 옛 문구도 통과시켜(`사제 회복 가상 표 오류` 만 거부) 옛 코드에서도 통과했다 — `사제 회복` 으로 좁혀 교정했다.
- 단위 검사: `node --test tests/priest-pre-destroy.test.mjs tests/priest-regen.test.mjs tests/vortex-pre-destroy.test.mjs tests/vortex-owner.test.mjs tests/vortex-ascend.test.mjs` 통과.

## 4. 대응표·문서 반영

- `PORT_MAP.tsv` 12행의 base 와 메모를 갱신했고 `node webpj/tools/port-map.mjs` 오류 0, 12단위의 `cpppj 변경` 보고가 사라진다.
- LEFT_JOBS.webpj.md 3.6절 표에 ③ 묶음 1행(4-D Player/사제/신전 Native 조회기·본체)을 더했다. [대응표 문서](webpj-port-map-20261106.md) 9절 표에 이 묶음을 더했다.

## 5. 남은 경계와 다음 시작점

- ③ 은 4-D World 수명 때 Player 배열을 장기 소유하는 Native 조회기(`RawPlayerGeyserBounds`·`RawPlayerWorld`·`RawVortexOwner`·`RawVortexPreDestroy`)와 `RawPriestForcefield`/`RawPriestPreDestroy` 의 Native 본체를 한 묶음으로 cpppj 시험과 대조한다.
- 다음은 3-E 세 번째 항목 **판정-섬·다리·자원**(6단위)이다.
