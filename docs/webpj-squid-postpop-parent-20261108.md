# webpj 공통 postPop 부모(004b0d30) 이식 — WB04-E24

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물네 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawSquidPostPopParent`(10.78)를 옮겼다. `SquidPostPop` 처리기 본체가 이 부모를 장부·그래프·AI 통지·선택 재검사 훅과 연결하고, 파생 종류별 PostPop이 이것을 공통 꼬리로 호출한다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/squid-postpop-parent.js` — `runSquidPostPopParent(pool, sid, flags, state, hooks)`. 깊이(`state.depth`)는 호출자가 이미 증가시켰고 이 몸체가 끝에서 한 번 줄인다.

1. `suppressed`(메타맵)이 참이면 슬롯·TYPE·판본을 보기 전에 깊이만 줄이고 끝(0에서 줄이면 32비트로 감김).
2. 슬롯의 첫 TYPE genus(`flags2`)를 읽어 영역 군(`0x50444208`)이고 그래프가 비활성이면 그래프 단계를 통째로 건너뛴다(그래프 활성을 다시 조회하지 않음).
3. 영역 군이고 `flags&3`이 있고 noGraph(`0x2000000`)가 아니면 `region(sid)`. 그래프가 활성이면 `(flags&0x203)==0`이거나 noGraph일 때 현재 TYPE이 표면(flags1 `0x800`)이고 `flags&1`이며 noGraph이면 `resetGraph`, 그 밖에는 `flags&0x800`이면 원문 진단(`!(popFlags&pfDESTROY_GRAPH)`, `Squid.cpp` 0x80b; checking 무관, 정상 복귀 가능) 후 현재 TYPE이 표면이면 `add`.
4. 저장 genus(`flags2`)를 따로 저장한 뒤 `flags&1`이고 현재 extra(+40)에 비트 1/8이 없으면 장부: `cost` → 현재 TYPE이 공급원(flags1 `0x10000000`)이면 `provider` → 저장 genus가 공장 군(`0x4200`)이면 `ownerFactory(현재 owner, sid)`·`factory` → `aiNotice(현재 owner, sid)` → 저장 owner를 **먼저** 읽은 뒤 현재 로컬 owner를 읽어 같으면 `localCount(현재 TYPE)` → `globalCount(현재 TYPE)` → `recheckSelection`.
5. 각 하위 호출 뒤에 슬롯의 TYPE/owner/extra와 전역을 다시 읽는다. 하위 실패 때 이미 일어난 효과와 증가된 깊이는 되돌리지 않는다.

## 2. 검증 (`tests/squid-postpop-parent.test.mjs`, 11건)

원본 관측 fixture 없이 cpppj `PostPopCurrentTests`의 PE 정적 확인 계약 열 개와 JS 판본/누락 장치 경계 하나를 옮겼다: suppressed 조기 종료와 깊이 감김, 영역 군+그래프 비활성 직행, 영역 하위가 그래프를 끈 뒤 재조회, 파괴 진단 복귀 뒤 현재 TYPE 분기, 진단 중단 시 깊이/영역 효과 보존, noGraph 리셋 우선, 현재 TYPE과 저장 genus의 독립 역할(`region→add→cost→provider→owner:3→factory→ai:5→local:255→global:254→selection`), 추상 extra 1/8/9의 장부 생략, 선택 재검사 실패 시 통계/깊이 보존, 저장 owner 읽기 선행.

최초 실패와 교정: 11건이 처음부터 통과했다.

## 3. 경계

- cpppj `PostPopCurrent_ActualManaDiagnosticRetainsSavedFactoryGenusAndCurrentOwner`(실제 `SquidPostPop` 본체와 마나 중복 진단·장부 연결)는 `SquidPostPop` 본체 단위(`PostPopBase`/`Activate`/`DispatchPostPop`와 `RawGraph` 연결)를 옮길 때 대조한다. 훅이 요구하는 `region`/`add`/`resetGraph`는 `RawGraph`, `provider`는 이미 옮긴 `registerManaProvider`이다.
- CD 10.72와 파생 종류별 PostPop(Asset·Fence·Nugget·Buried·Carrier·Bomb·Factory·Priest·Rune)은 이 부모를 공통 꼬리로 호출하는 별도 단위이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체(분배·장부·훅 연결), `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
