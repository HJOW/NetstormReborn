# webpj 너겟 보충·자산 소유자 분배·총 공격 모드(RawNuggetReplenish·RawAssetOwner·RawGunMode) 이식 — WB04-E71

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔한 번째 부분이다. 서로 다른 기능이지만 모두 호출 경계가 작고 이미 옮긴 부품(E57 Player 간헐천 범위, E42 다리 소유자 분배, E62 경로 애니메이션 코드표)만 쓰는 세 파일을 cpppj 기준선 `1f00a25a` 의 `o/RawNuggetReplenish`·`o/RawAssetOwner`·`o/RawGunMode` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE 레벨/프레임, 다른 가상 표 값)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-nugget-replenish.js` — `RawNuggetReplenish(bounds, state, hooks)`

- `run()`(00485e40): 현재 권한이 꺼져 있으면 원본 진단(Nugget.cpp 246)을 요구한 뒤 권한을 다시 읽어 여전히 꺼져 있으면 끝. 단독 미션·요새·편집 모드이면 자연 반환. 아니면 현재 지역 0 의 첫 record 와 원점 배치(E57 `RawPlayerGeyserBounds.originPlacement`)를 구하고, 지역 확장 단위(signed)를 `<< 4` 한 뒤 사각형 네 변 = (칸 `<< 4` 를 float 로 저장한 값) ∓ (`확장*16 - 16` / `확장*16`)을 두 번째 반올림 없이 계산해 실제 무작위 배치 하위에 수량 1 로 요청한다. 진단 효과가 바꾸는 전역은 다음 조회에서 직접 읽고(`state` 의 getter), 쓰지 않는 큰 하위는 생성 때 요구하지 않는다.

`www/core/raw-asset-owner.js` — `makeAssetOwnerDispatch(pool, state, productionDirty, base)`

- 자산의 가상 소유자 지정(+74) 재정의 분배(패치 00454cb0/00462580): 호출 때 읽은 현재 가상 표 기록값으로 고정 중립 자산(0x50a8e8)이면 소유자 BYTE 를 0 으로 쓰고 끝, 생산 자산 두 종류(0x509148, 0x509210)이면 extra 의 비트 9 가 꺼져 있을 때 생산 목록 변경 카운터(`{ value }` holder)를 먼저 올린 뒤, 기존 섬 받침/종유석 소유자 분배(E42 `makeOwnerDispatch`)와 공통 부모를 같은 순서로 부른다. 카운터를 먼저 올리므로 공통 부모가 중단돼도 그 변경은 남는다.

`www/core/raw-gun-mode.js` — `RawGunMode(pool, frames, windArcherType)`

- `get(sid)`(004d4d00): 현재 자산 TYPE 이 70 이상 표 안이어야 하고, 비astral 크로스보우(현재 windArcher 타입이고 extra 비트 8 이 없음)이면 현재 graph BYTE × 2, 그 밖에는 현재 프레임 DWORD 로 코드표의 side 글자를 signed 로 읽어 `글자 × 2 − 152` 가 0..7 이면 그 값, 아니면 0 을 반환한다. 현재 graph/frame 을 항상 실제 슬롯에서 읽는다.

## 2. 검증

- `tests/nugget-replenish.test.mjs`(2건): `nugget-replenish-x86.tsv` 372행(권한·모드·확장 단위·record·발자국·프레임 유무·진단 효과 변이 → 호출 순서 A/P 와 효과 뒤 현재 저장소 8칸; 사각형 네 float 비트와 수량까지 비교), 모드 관문의 미선택 하위 비요구와 선택된 미연결 효과 거부.
- `tests/asset-owner.test.mjs`(2건): `asset-owner-current-local-x86.tsv` 10.78 64행(생산 접두/중립 몸체의 종료 원인, 공통 부모 호출 기록, 효과 뒤 카운터, 슬롯 전체 raw), 현재 가상 표/extra 를 호출마다 읽고 부모 실패 뒤에도 앞선 증가를 유지하는 계약.
- `tests/gun-mode.test.mjs`(3건): cpppj 에 독립 fixture 가 없는 파일이라 소스 계약을 직접 입력으로 확인(signed 문자 계산과 0..7 제한, 크로스보우 graph × 2 와 astral 제외, mutable 타입 재조회, 표 밖 타입/프레임·연결 오류 거부).

최초 실패와 교정: 새 모듈은 위 fixture 436행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 World 생성자와 생산 장부 합성(`AssetOwnerCurrent_ActualWorldConstructorsAndProductionStorage`)은 `RawSurfaceWorld`(client) 이식 때, 총 공격 모드를 소비하는 캠페인 프레임·얼음/천둥 미사일 시험은 해당 파일 이식 때 대조한다. 너겟 보충의 실제 무작위 배치(`0040e050`)와 진단 본체도 해당 파일 단위의 몫이다.

## 4. 다음 시작점

`RawAssetPostPop`, 사제 배치 계열(`RawPriestPlacement*`, 선행으로 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, 그 밖의 작은 몸체(`RawNuggetRegular`·`RawCarrierFallStart`·`RawForeignRitualReward`·`RawAiCommandState` 등)이다.
