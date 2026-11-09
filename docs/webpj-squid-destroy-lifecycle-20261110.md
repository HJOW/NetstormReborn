# webpj 공통 삭제 장부(SquidDestroyLifecycle) 이식과 보호막 조합 시험 — WB04-E213

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "총·피해·삭제 수명" 묶음의 일부다. 공통 pre/postDestroy(004b0950·004b0840)의 비전투 선택·공급/작업장 목록·타입 통계·비용 장부·Graph 분할/Free/주변 Add 를 옮겼고, E212(보호막 수명)가 미룬 한 장면 조합 시험을 같이 닫았다.

- cpppj 기준선 `1f00a25a`. 이식 원본은 기준선 판이다. **기준선 뒤 cpppj 변경이 있다**(처음 "변경 없음"으로 적은 E212 문서의 다음 작업 문구는 이 단위에서 정정): `SquidDestroyLifecycle.cpp/.h` 가 `541624a9`·`417f22d2`·`7ba0130b` 세 커밋으로 +79/−28줄 바뀌었다. 판정은 아래 3절이다.
- 근거 커밋(기준선 판): 소스·시험 `1e8e43d9`. fixture 마지막 변경: `destroylifecycle-x86.tsv` `e0169cba`, `destroygraph-x86.tsv` `67070e23`, `island-graph-frame-x86.tsv` `a726b49f`.
- 원본 fixture(모두 10.78 `originals` 행, 두 x87 정밀도 639/895 를 모두 재생)
  - `destroylifecycle-x86.tsv`: Pair(직접 Pre/Post 한 쌍) 384행 + Destroy(공통 destroy 전체) 192행
  - `destroygraph-x86.tsv`: Pre 192·Post 192·Destroy 192행. 입력 25칸·객체 7개 노드·spot·Graph 기록 255행·flood 스택 시드를 읽고, Graph 기록과 스택 Adler 까지 대조한다.
  - `island-graph-frame-x86.tsv`: 실제 157(H=8, 9프레임)→162(단일 프레임) 치환의 10.78 행
  - CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
- cpppj 시험: 같은 PC 의 통합 시험 실행 파일(`netstorm_tests.exe`, 2026-11-08 빌드, 게임 실행 아님)에서 `DestroyLifecycle` 6건·`DestroyGraph` 8건·`GameAux_DeletionReadsCurrentLossAndCostAfterSoundEffect` 1건 통과를 확인했다. 이 빌드는 기준선 뒤 변경을 포함하지 않을 수 있어 기준선 판의 확인으로만 본다.
- 공유 장치(모두 기존 모듈 재사용): D2 `squid-destroy.js`(`RawSquidDestroy`)·`squid-unpop.js`, E2 `squid-postpop-state.js`(목록·통계 표·장기 비용 대여), E40 `raw-graph.js`(분할/Free/PostDestroy), D5b `squid-reward.js`(`plan`/`refund`).

## 1. 옮긴 것

`www/core/squid-destroy-lifecycle.js`(DOM 없음, 10.78 전용)

- `SQUID_DELETION_EFFECT`(`clearSelection`·`refund`·`unitLostSound`)·`SquidDeletionState`
  - `selected`·배치 기억(`placementType/X/Y`)·손실 좌표(`lostX/Y`, 장기 저장소가 `{ value }` 상자로 있으면 `sharedLostX/Y` 를 직접 읽고 단정도로 쓴다)·두 소리 억제 전역·`silentType`·`regionMask`(0x50444200)·`rebuildGraph`·특수 H/대체/대형 표면 TYPE(157/162/162).
- `SquidDestroyLifecycle(pool, types, bookkeeping, state, destroy, hooks, graph, reward)`: 타입 188칸 사본, 풀·Graph(`graph.pool`·`validateTypes`·삭제기의 `validateGraph`)·보상(풀과 장부) 연결을 구성 때 확인한다.
  - `preDestroy(sid, flags)`(004b0950): `validatePre`(무효과 사전 검사) → 선택 복구(abstract/buried 이고 flags2 0x30000 이면 배치 기억 기록 뒤 선택 해제 효과) → Graph 분할/Free → 장부 억제가 아니면 소유자 작업장 제거 → 보상 → 현재 타입 수·전역 수 감소 → 소리/손실 좌표 → 공급원 제거 → 현재 AI 통지 → 전체 작업장 제거 → pre 깊이 감소.
  - `postDestroy(sid, flags)`(004b0840): `validatePost` → 주변 표면 Add(Graph) → ordinary 이면 전역 비용 차감(패치 비용 인코딩: `cost + TYPE×23` 절삭 뒤 바이어스 제거, DWORD 감김) → post 깊이 감소.
  - `hooks()`: 공통 `RawSquidDestroy.destroy` 의 `{ emit, selected }`. preDestroy/postDestroy/clearSelection 은 이 몸체로, 그 밖(종속 release/destroy·전파)은 `forward` 로 보낸다(미연결이면 그 경계에서 오류).
  - `bindPatchAiNotice(notice)`: 공급 목록 제거 뒤·전체 작업장 제거 앞에서 현재 소유자/SID 로 부르며, AI 부착 소유자는 통지가 없으면 사전 검사에서 거부한다. 함수가 아니면 연결을 해제한다.
  - 사전 검사는 장부 쓰기 전에 손상 목록(count 초과)·미연결 Graph/AI/보상/선택 해제/소리 콜백·비유한 비용·소유자 범위 밖을 거부한다. `#object` 는 매 효과 사이에 현재 슬롯을 다시 읽으며 free/contained/예약 SID/Form TYPE/TYPE 표 밖을 거부한다.
- 보조 변경(웹 쪽 기존 모듈에 작은 추가): `squid-unpop.js` 에 `validateGraph(graph)`(cpppj `SquidUnpop::ValidateGraph`), `squid-destroy.js` 에 같은 이름의 위임을 추가했다. 기존 동작은 바뀌지 않는다.

## 2. 검증

`tests/squid-destroy-lifecycle.test.mjs`(17건) · `tests/forcefield-lifecycle-integration.test.mjs`(1건)

- 원본 관측 재생: lifecycle Pair 384·Destroy 192, graph Pre/Post/Destroy 각 192, island 행. 효과 순서(P/O/S/Q/L/T), 대상·주변 일곱 슬롯 바이트, 풀 카운터·두 free 머리/꼬리·풀 전체/네 단계 해시/spot/삭제 기록 Adler, 선택, pre/post 깊이, 장부 전체(통계 세 표·공급/작업장/소유자별 목록 inactive 꼬리 포함), 배치/손실 좌표, Graph 기록 255행과 flood 스택 Adler 를 모두 대조한다.
- cpppj 계약 이식 10건: 생성/삭제 균형과 누적 생산 수, 미지원 효과의 쓰기 전 거부, 억제와 비용 경계(비유한 비용), 다른 풀/Form/contained 거부, GameAux(소리 뒤 현재 손실 좌표·장기 비용), Free 안전, H 치환 타입 검증, 후보 프레임 오류의 쓰기 전 거부, 다른 해시/spot/타입 Graph 연결 거부. 웹 계약 2건(AI 통지 순서·해제, 손실 좌표 단정도).
- 조합 시험(E212 가 미룬 것): 사제 곁 보호막 확보→두 Regular 예약→Kernel 프레임 진행→사제 소멸 뒤 사건 2 시각에 자기 삭제→종속 Form 반납과 장부 복구→재확보→외부 삭제. 실제 `SquidFactory`·`SquidOwner`·`SquidPop/Unpop`·`SquidDisplay`·`SquidFrame`·`RawFrameAdvance`·`SquidProcessHost`·`Kernel`·`SquidDestroyLifecycle` 로 조립했다. 합성 입력은 TYPE 표(실제 보호 자료에서 보호막 167·사제 158 의 필요한 필드만 바꾼 사본)·프레임/SHP 표·위치뿐이다.

최초 실패와 교정: 모듈은 처음부터 17건 + 1건이 모두 통과했다. 비교가 실제로 작동하는지 모듈 사본을 바꿔 돌려 확인했다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: 전역 수 감소 −1→−2(7건), 9칸 감소를 1칸으로(3건), 보상 없음 마스크 0x200000→0x100000(6건), 배치 기억 마스크 0x30000→0x10000(2건), 비용 바이어스 제거 생략(2건), `regionMask|8`→`regionMask`(3건). 조합 시험에서도 비용 차감 제거·사건 2 삭제 제거·두 번째 예약 제거가 각각 실패했다.

## 3. 기준선 뒤 cpppj 변경 판정 (3.6절)

기준선 뒤 세 커밋은 실제 PATCH(Native) 연결용 분기를 `destroy_.HasPatchDiagnostics()` 아래에 더한 것이다. 독립 typed 경로(이 모듈이 옮긴 판)는 같은 입력에서 결과가 같고 호출 경계만 늘었다.

| 변경 | 내용 | 웹 처리 |
| --- | --- | --- |
| 현재 전역 getter | `selectedRaw`(선택 DWORD 전체)·`localOwner`·loading 깊이·fort 모드·`silentType`·특수/대체/대형 표면 TYPE 를 비교 위치마다 다시 읽음 | ③ WB05 World 연결 때 |
| 실제 PATCH 진단 | 자산 pre 가 `Object(sid)[kType]<70` 이면 `!isForm()` 0x771 진단 | ③ |
| 배치 기억 | Native 는 UI 기억점(`rememberPlacement`)에 원시 좌표 비트를 직접 기록하고 flags2 관문을 UI 안에서 판정 | ③ |
| 목록 제거 | 0040ea00 의 DWORD 감김 순회 압축(`Remove native`) | ③ |
| 비용/TYPE 표 | 최초 로딩 비용 표 소비, 현재 TYPE 표 빌림 | ③ |
| 검증 생략 | PATCH 는 선행 `ValidatePre/Post` 를 생략하고 도달한 접근만 수행 | ③ |

모두 ③(실제 Session/World 연결 몫)으로 판정하고 LEFT_JOBS.webpj.md 3.6절 ③ 표에 새 행으로 올렸다. 연결 때 같은 시기에 바뀐 `RawSquidDestroy`(+143/−2줄)·`SquidPostPop`(+35/−1)·`SquidReward`(+6/−3)의 변경도 함께 판정한다. base 는 `1f00a25a` 로 유지한다.

## 4. 경계와 남은 것

- 웹은 기준선 typed 경로만 옮겼다(3절). Native 경로는 실제 World 와 UI 전역(선택·배치 기억·로딩 깊이·fort 모드)이 연결되는 순서4~5 에서 추가한다. 그 전에는 이 모듈을 독립 장면에서만 쓴다(PORT_MAP session 0).
- `SquidDestroyLifecycle` 의 보상 연결(`reward`)은 D5b 의 `SquidReward.plan/refund` 를 호출하는 경로를 구성만 확인했다. 실제 SP/지갑 계산 대조는 D5b 단위의 fixture 몫이며 이 단위에서는 보상 이벤트(`refund`) 경로를 fixture 로 대조했다.
- 보호막 조합 시험은 보호막 소리(`priestForceField.wav`) 확보를 실패로 두었다. 소리 연결은 순서8 몫이다.

## 5. 다음 시작점

WB04-E214 는 3.1절에서 고른다. 같은 묶음에서 기준선 뒤 변경이 없는 `RawSquidDestroyDispatch` 를 먼저 권한다(`RawFrameChoose`·`RawGeyserMined`·`RawRitualChoice` 도 변경 없음). `RawDamageablePreDestroy`·`RawDamageablePostDestroy`·`RawDamageableRelease`·`RawGunImpact`·`RawDiscRegular` 는 기준선 뒤 변경이 있어 이 단위의 3절과 같은 Native 분기 판정을 함께 한다.
