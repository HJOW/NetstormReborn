# webpj 다이스 내용물 알림(RawDaisContentNotice) 이식 — WB04-E219

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "자원·생산·건설 명령" 묶음의 일부다. 다이스(제단 받침)에 포획 내용물이 부착될 때 Dais 가상 +24 의 내용물 부모 알림이 실제 제단의 O 프레임·원래 owner WORD 를 갱신하고 로컬 cue 를 중지하는 몸체를 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawDaisContentNotice.cpp/.h`)·시험(`DaisContentNoticeTests`)·fixture(`daiscontentnotice-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인). 선행 `RawRunePostPop.altarAt`(E61)도 같은 기준선 판이다.
- 근거 커밋: 소스 `efcd13e4`, fixture `db9b626c`.
- 원본 fixture: `daiscontentnotice-x86.tsv` 의 10.78(originals) 722행(originalCD 722행은 웹 범위 밖). 입력 JSON(제단 존재 여부·첫 'A' 프레임 수·내용물 kind·로컬 owner·다이스 owner·reason·원래 owner WORD)에서 가상 공간/cue 요청 순서와 Dais(5)·제단(7)·내용물(10) 세 슬롯의 raw 전체를 대조한다(주입한 가상 표 DWORD 제외).
- 공유 장치(모두 기존 모듈 재사용): E61 `raw-rune-post-pop.js`(`altarAt` typed finder), E2 `raw-squid-word.js`(`setRawSquidWord`), WB03-X `FrameCodeTable.run`.

## 1. 옮긴 것

`www/core/raw-dais-content-notice.js`(DOM 없음, 10.78 전용)

- `RawDaisContentNotice(pool, frames, rune, state, hooks)`: 같은 풀의 `RawRunePostPop`·188칸 프레임 표를 확인한다. `state` 는 현재 전역 getter `{ priestType(), localOwner(), cue(), bridgeType() }`(005412d0·00540c70·00542a50·현재 다리 타입), `hooks` 는 `{ unpop, repop, stopVoice, notifySurface }` 이며 도달한 위치에서만 요구한다.
- `handles(dais)`: 실제 다이스 생성자 VT(0x00507950)만 처리한다(타입 필터를 추가하지 않음).
- `notice(dais, form, reason)`: VT 가 다르면 거부한다. reason 비트 1(부착)이 아니거나 내용물 kind(+18 WORD)가 현재 사제 kind 가 아니면 자연 반환(reason 2 제거도 같음). 부착이면
  1. `rune.altarAt(dais)` 로 곁의 제단을 찾고 있으면 제단 가상 Unpop(flags 0) → 제단 현재 TYPE 의 첫 `'O'` 프레임을 프레임 DWORD(+36)에 직접 쓰고(setter/표시 효과 없음, 첫 프레임이 없으면 −1) → 내용물의 원래 owner WORD(+12)의 하위 7비트를 공통 WORD setter 로 쓰며(현재 다리 TYPE 이면 표면 통지) → 제단 가상 Repop(flags 0).
  2. 다이스 owner(+34)가 현재 로컬 owner 와 같으면 포획 사제 알림 cue 를 `stopVoice(cue, 0, 0)` 으로 중지한다(희생 완료의 voiceCue 와 다른 cue).

## 2. 검증

`tests/dais-content-notice.test.mjs`(3건)

- 원본 관측 재생: 10.78 722행 전부. 행마다 풀/해시/프레임/전역을 새로 만들어(Dais 165·사제 158·제단 164 입력 체인, 내용물 SID 10) 가상 공간/cue 요청 순서와 세 슬롯 raw 를 대조한다. 미수행 Rune 생성/예약은 모두 오류로 둔다.
- 웹 계약 2건: 생성자 VT 가 아닌 부모·다른 풀의 rune·프레임 표 크기의 거부와 reason 2/다른 kind 의 자연 반환, 선택된 장치 누락(앞선 프레임/WORD 쓰기는 repop 앞이라 보존되고 unpop 부재는 쓰기 전 거부).

최초 실패와 교정: 모듈은 처음부터 722행이 모두 통과했다. 비교가 실제로 작동하는지 모듈 사본을 바꿔 돌려 확인했다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: WORD 마스크 127→255(1건), 프레임 글자 'O'→'A'(2건), owner 비교 반전(1건), reason 비트 1→2(3건 전부), kind 비교 반전(3건 전부).
- 첫 작성한 장치 누락 시험에는 무의미한 단언(`3 + 0 === 3 ? …`)이 섞여 있어 고쳤다. 고친 시험은 프레임 3('O' 첫 칸)과 WORD 5 가 repop 앞에서 이미 쓰였음을 확인한다.

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 호출 지점(`ContentFormHost` 의 reason 1/2 → Dais 가상 +24 분배)과 제단 가상 Unpop/Repop·cue 중지·표면 통지 하위는 순서4~5 몫이다(PORT_MAP session 0). cpppj `DaisCheckingParentsTests`·`DaisRitualConsumerTests` 는 Dais 의식 소비자(`RawDaisRegular`·`RawAltarSacrifice`)를 옮길 때 대조한다.
- CD 10.72 는 웹 범위 밖이다. 이 판의 BYTE 프레임(+34)·owner(+32)·Dais VT 0x00504090 분기는 옮기지 않았다.

## 4. 다음 시작점

WB04-E220 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 `o/` 단위(`RawIslandQueries`·`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`·`RawManaBoltRegular` 등)를 권한다.
