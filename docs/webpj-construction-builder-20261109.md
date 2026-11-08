# webpj 건설 사제 이동(RawConstructionBuilder) 이식 — WB04-E142

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔두 번째 부분이다. 건설이 사제를 필요로 하는 타입에서 건설용 사제와 자원 운반 객체를 작업장/공급원에서 생성해 목표로 보내는 전체 이동(Construction.cpp `00443000`)을 옮긴 `o/RawConstructionBuilder`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E134 통지·E140 로컬 배치의 `moveBuilder` 경계가 이 이동에 이어진다. 선행은 E57 기준점(`RawPlayerPlacementAnchor`)·E29 에너지 공급(`RawEnergySupply`)·기존 `SquidPostPopList`·`SquidFactory`/`SquidOwner`/`SquidProcessHost.addRegular` 이다.

## 1. 옮긴 것

`www/core/raw-construction-builder.js`

- `ConstructionBuilderState` — `builderType`(00541270)·`carrierType`(005411fc)·`bridgeType`(005411a0, 82)·`approachMask`(005424b8)·`timerPayload`(00532750, 0.05f)·`now`(0055b4d0)·공유 공급원 목록 `sources`(`SquidPostPopList`).
- `RawConstructionBuilder(pool, types, metadata, state, hooks, supply, reads)` — 10.78 `SidPool`·타입 표 188칸·메타 188칸(`speed`·`sourceFrameIndex`·`firstFrames[16]`)·필수 경계 10개(`anchor`·`create`·`setOwner`·`approach`·`pop`·`move`·`speed`·`notifySurface`·`reserveTimer`·`addTimer`)를 생성 때 확인한다. `chooseSources`/`sortSources` 는 실제 에너지 공급(`supply`)이 없을 때만 필수이며, `supply` 는 같은 풀이어야 한다. `reads` 는 실제 Session 이 같은 전역을 읽게 하는 getter 묶음이다.
  - `move(player, target, timeLow, timeHigh)` (00443000): 목표의 (타입, 좌표)로 기준점을 찾아 없으면 `5.0`. 있으면 건설용 객체를 클라이언트 플래그 2 로 생성(풀 안의 새 객체인지 확인)하고 소유자를 정한 뒤 목표 발자국 위쪽 가운데를 `floor(좌표+0.5)` 로 반올림한 도착점으로 삼아, 기준점 genus 가 접근 mask 와 겹치면 `mask|1`, 아니면 `1` 로 접근점을 구해 첫 객체를 Pop(0x10)하고 경로를 시작한다(경로 실패는 `5.0`, 앞선 효과는 유지). 요청 도착 시각이 0 이 아니면 `(요청−now)×속도/(도착−now)` 를 경로 속도로 넘긴다. 이어 현재 목표 좌표·타입과 기준점 소유자로 자원 선택(또는 에너지 공급 평가)과 정렬 뒤, 공유 공급원 목록에서 첫 0 이 아닌 항목을 0 으로 소비하며 운반 객체마다 [단어 index → 현재 다리 타입이면 표면 알림 → 기준점 소유자 → 공급원 타입의 첫 프레임 → 공급원 중심에서 Pop(0) → 타이머 확보에 성공하면 Regular 부착 → 목표로 이동(경로 실패 무시)]을 한다. 마지막에 첫 경로의 현재 도착 시각을 반환한다.
  - `takeSource()` (00473570): 첫 0 이 아닌 공급원 DWORD 를 0 으로 바꾸어 소비한다(signed 개수 0 이하이면 0, 개수보다 짧은 저장소는 거부). `supply` 가 연결되면 그 마지막 목록을 쓴다.
- `makeConstructionBuilderHooks(pool, anchor, factory, owner, hooks, process)` — 기준점 조회·`factory.create`·`owner.set` 과 선택적 Regular 부착(`process.addRegular`)을 같은 풀의 실제 모듈로 잇는다. `makeConstructionBuilderLocalHooks`/`makeConstructionBuilderNoticeHooks` — 로컬 배치·통지의 `moveBuilder` 를 이 이동에 잇는다(double 결과는 버림). 풀이 다르면 모두 거부한다.

## 2. 검증

`tests/construction-builder.test.mjs`(4건):

1. `constructionbuilder-x86.tsv` 의 `originals` 370행(`originalCD`·`original1037` 은 다른 판본이라 제외) — 이동/공급원 소비의 입력 21개(플레이어·기준점·소유자·접근 mask·genus·목표 좌표·발자국·도착 시각·now·경로·속도·공급원 윤곽·할당 결과·프레임·변동 0~8·다리·개수)에서 사건 순서(A/C/O/P/B/M/V/Q/S/N/L/T), 반환 double 비트, 생성 raw 전체(주변 바이트 보존 포함), 최종 공급원 목록(개수 밖 꼬리 포함)·개수가 모두 같다.
2. signed 개수(`0xffffffff`)의 조기 반환, 개수가 저장소보다 크면 거부, 프레임 표 인덱스 16·경로 단계 범위 밖·풀 밖 기준점 거부, 필수 경계 12종 각각·다른 판본 거부.
3. 기준점 실패는 생성 전에 `5.0`, 첫 경로 실패는 앞선 생성/소유자/Pop 효과를 유지하고 `5.0`, 타이머 확보 실패에도 모든 운반 객체의 이동은 계속된다(Regular 부착 없음, 도착 시각 반환 10).
4. 어댑터: 기준점·factory·owner·Regular 연결과 로컬/통지 `moveBuilder` 연결, 풀이 다르면 거부.

최초 실패와 교정: 새 모듈은 370행 fixture 를 시험 쪽 상태 객체 갱신 방식(객체 주소를 유지한 채 내용 교체)만 고쳐 통과했다.

## 3. 경계와 남은 것

- 실제 `RawEnergySupply` 를 `supply` 로 연결한 이동(자원 선택→정렬→소비)은 이름 계약(`evaluate`/`sortSources`/`takeSource`/`pool`)만 확인했고 fixture 로는 시험하지 않았다. cpppj 의 에너지 공급 통합 시험은 통합 단계에서 옮긴다.
- 건설 계열의 남은 것은 `RawConstructionCommand`(25바이트 요청 작성·`444af0` 수신·분배 등록; `RawPacketDispatch` 선행)뿐이다.
- 10.72(CD) 경로(소유자 +32·프레임 BYTE·나눈 뒤 곱함)는 범위 밖이다.

## 4. 다음 시작점

`RawConstructionCommand`(선행 `RawPacketDispatch`), `RawPriestPlacementGeometry`, `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
