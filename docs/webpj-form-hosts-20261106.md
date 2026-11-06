# webpj Form 호스트(Depend·Gump·Content)와 부모들 — WB04-D5a

2026-11-06, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3(WB04-D)의 다섯 번째 하위 단위(D5)의 첫 부분이다. 원본 10.78의 내장 Form 세 종 호스트와 그 부모들을 D3 공통 Form 층·D4 SquidFactory 위에 옮겼다.

- **세 호스트:** 감시 쌍 DependForm(TYPE 1), UI 부착 GumpForm(TYPE 3), 생산/유닛 목록 ContentForm(TYPE 6).
- **부모들:** 감시 쌍 생성, 정렬 Content 부착, Form 연결 검사, 내용물 부모 통지 분배, contained 생성.

화면 변경과 앱 수명 연결은 없다.

## 1. 근거

- 시작 트리 `7dd335e0`(작업 트리에 다른 세션의 cpppj 미커밋 변경이 있었으나 보존했고 근거로 쓰지 않았다). 근거는 모두 커밋본이다.
- 근거 소스(cpppj):
  - `o/RawDependCreate.cpp/.h`, `o/DependFormHost.cpp/.h`
  - `o/GumpFormHost.cpp/.h`
  - `o/RawContentAttach.cpp/.h`(`f83db66c`), `o/ContentFormHost.cpp/.h`
  - `o/RawFormLinked.cpp/.h`(`72012e83`)
  - `o/RawContentParentNotice.cpp/.h`
  - `o/RawContainedCreate.cpp/.h`
  - `o/RawSquidContainedUnpop.cpp/.h`(`f83db66c`, `…AtTable`)
  - 커밋은 이식 추적 목록의 `cpppjBasis`가 기록한다.
- 원본 관측(10.78):
  - `cpppj/tests/fixtures/depend-create-parent-current-x86.tsv`(810행)
  - `content-attach-current-x86.tsv`(41행, 판본 열 없는 10.78 전용 표)
  - `form-linked-parent-x86.tsv`(120행, 판본 무관 물리 입력 표)
  - `formlifecycle-x86.tsv`(456행)

  행마다의 합성 입력과 명시 장치는 cpppj `tests/DependCreateCurrentTests.cpp`·`ContentAttachCurrentTests.cpp`·`RawFormLinkedTests.cpp`·`FormLifecycleTests.cpp`와 같다. 기대값은 fixture에서 읽는다. CD 10.72 행은 범위 밖이다.

## 2. 파일과 계약

| 파일 | 내용 |
| --- | --- |
| `www/core/depend-form-host.js` (신규) | `createDependForms`(004b1510), `DependFormHost`(create·has·remove·watcher·notify·destroy·hooks·isDependForm·bindCreation·bindUnpop) |
| `www/core/gump-form-host.js` (신규) | `GumpFormHost`(create·take·isGumpForm·owner·parent·setOwner·attach·unpop·notify·destroy·hooks) |
| `www/core/content-form-host.js` (신규) | `attachSortedContent`(004aec10), `ContentFormHost`(attach·unpop·robust·linked·damage·bindCreate·bindPop/Unpop/Reattach/CurrentTypes) |
| `www/core/raw-form-linked.js` (신규) | `checkRawFormChain`, `checkRawFormLinked`(004ae080), `readRawProcessFormParent` |
| `www/core/content-parent-notice.js` (신규) | `RawContentParentNotice`(현재 부모 VT의 +0x24 분배), `EMPTY_CONTENT_NOTICE` |
| `www/core/contained-create.js` (신규) | `RawContainedCreate`(004afa70), `makeContainedCreateHooks`, `INDEPENDENT_CONTAINED_STATE` |
| `www/core/squid-contained-unpop.js` | `commonContainedPreviousAtTable`·`commonContainedSetPreviousAtTable` 추가(3.5절 재반영 `f83db66c`) |
| `tests/form-hosts.test.mjs`·`form-hosts-contract.test.mjs` (신규) | 원본 fixture 4종 재생과 계약 검사 19개 |
| `tests/fixtures/manifest.json` | 이번 fixture 4개 등록(62항목). D4 항목 9개와 `squid-destroy`의 `webCommit`을 `7dd335e0`으로 기록 |

### 감시 쌍(DependForm)

**`createDependForms`**

1. 어느 부모든 client 번호(5..14999)면 flags 2를 쓴다. 아니면 현재 서버 권한을 확인하고, 아닐 때 진단(0xb29)한다. 정상 복귀해도 flags 0을 유지한다.
2. 두 Create는 각각 **직전에** 현재 Depend TYPE DWORD를 읽는다.
3. 링크 WORD를 쓴다(첫째 +20=둘째·+18=0, 둘째 +18=첫째·+20=0).
4. 첫째 VT를 읽고 부착한다.
5. 첫 부착 **뒤에** 둘째 VT를 읽고 부착한다.

**`DependFormHost`**

- `bindCreation(factory, () => aux.dependFormType)`으로 같은 공장과 GameAuxState의 현재 번호를 빌린다. 다른 풀의 공장은 거부한다.
- 공장이 없으면 삭제기의 TYPE 표로 독립 공장을 만든다. 고정 VT 경로는 추가하지 않는다.
- 부착은 Depend/Gump/Content VT의 공통 thunk만 연결한다. Process(2)의 직렬화 접두는 D5c다.
- `has`/`remove`는 부모 +6 체인에서 next를 먼저 저장한 뒤, **현재 Depend TYPE 번호**와 상대 부모(+18 watcherForm의 Form 부모)로 후보를 고른다. TYPE이 맞으면 상대 조회가 감시 VT를 요구한다. `remove`는 watched가 dead면 무동작이다.
- 가상 preDestroy는 watched(+20)와 현재 watcher(+18) 상대를 같은 삭제기로 지운 뒤 깊이를 반납한다.
- release(`notify`)는 상대가 client 번호이거나 서버 권한일 때 watcher 부모에 통지하고, 통지 뒤 watchedFid(+20)가 0인지 확인한다.
- 클래스 판정(`isDependForm`)은 현재 VT로 한다.

### GumpForm

- 생성, Take, 부착, Unpop은 같은 Factory의 현재 TYPE 3 생성자와 공통 부착, Form Unpop으로 간다.
- 소유 ID는 +18 DWORD 전체다.
- 부모 삭제의 release는 소유 ID가 0이 아닐 때만 UI 통지(owner, 현재 부모)를 부른다.
- 클래스 판정은 사용 중이거나 dead인 슬롯은 현재 VT로, 반납된 슬롯은 TYPE 기록(3)으로 한다.

### ContentForm

**`attachSortedContent`**

1. 부모와 자신의 dead/free를 진단한다(0xbe6/0xbe7). 정상 복귀 뒤 다시 판정해 dead/free면 false를 돌려준다.
2. 같은 목록 그룹(+20의 0x300 제외)에서 현재 contentFormType 항목을 찾는다. 순서 BYTE(+23)가 지금 순서보다 큰 첫 항목 앞에 넣고, 없으면 끝에 넣는다.
3. 이웃 getter보다 먼저 자기 VT를 저장해 이전 setter에 쓴다. 이전 getter를 두 번 부르는 것도 원문과 같다.
4. 순서 BYTE를 쓰고, 현재 VT로 부모 setter를 부른 뒤 contained를 켠다.
5. Pop(flags|0x100) 뒤 저장한 부모에 이유 1로 통지한다.

**`ContentFormHost`**

- **부착:** 정렬 부착 뒤 **통지/전송이 바꾼 현재** 값으로 사제 포획 꼬리를 판정한다. 판정 순서는 내용물 TYPE genus 0x200000 → 부모 genus 0x30000 → 상태 → 전투다. 빌린 현재 표(`bindCurrentTypes`)가 있으면 그 표에서 읽는다. checking이 켜져 있으면 isValid/isRobust 진단(0xc2b/0xc2c)을 하고, 연결돼 있으면 `captured(parent, +12 & 0x7f)`를 부른다.
- **`robust`:** WORLD(0x80)면 진단(0xbb1)한다. 정상 복귀 뒤 현재 flags를 다시 읽어 0x34 비트를 본다.
- **`linked`:** 현재 부모 getter를 한 번 읽고 전체 체인을 검사한다.
- **`damage`:**
  - damage가 0이거나 음수면 받은 float를 그대로 돌려준다(-0 포함, this를 읽지 않음).
  - 양수면 현재 부모를 저장하고 Unpop한다. 이어서 +21 bit0(0x100)을 지우고, 빌린 재부착(`bindReattach`)이나 Content VT일 때의 정렬 부착으로 다시 붙인 뒤 0을 돌려준다.
- **`bindCreate`:** contained 생성기의 생성을 같은 공장으로, 마지막 부착을 이 호스트로 연결한다.

### 기타 부모

- **`RawContentParentNotice`:** 부모의 현재 VT로 +0x24 진입점을 고른다. D2의 지원 자산 VT 표 셋째 칸을 쓴다.
  - 네 Form과 대부분의 자산은 빈 ret8(0x41c100)이라 아무것도 읽지 않는다.
  - 파생 진입점(0x429560 등)은 derived 장치로 보낸다.
  - 모르는 VT는 거부한다.
- **`RawContainedCreate`:** 다음 순서로 진행한다.
  1. 부모 범위를 확인하고, flags가 0이면 0x20으로 바꾼다.
  2. 부착 flags로 할당 flags를 정한다(0x20→1, 0x10→2, 그 밖 0).
  3. **현재** contained TYPE으로 생성한다.
  4. +18 TYPE, +12 인자 WORD를 쓴다.
  5. **생성 뒤의** 현재 다리 TYPE과 같으면 표면을 알린다.
  6. +30 품질, +20 flags를 쓴다.
  7. 부착한다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 감시 쌍 생성 | 원본 **810행**: 진단·Create(type,flags,sid)·WORD·부착(VT·부모·flags) 사건 순서, 최종 번호·권한, 두 슬롯 전체 |
| 정렬 부착 | 원본 **41행**: 반환/중단 원인(attached·returned·pop·notice·previous), 사건 순서, 다섯 슬롯 전체 |
| Form 연결 | 원본 **120행**: 결과와 입력 바이트 불변 |
| Form 수명 | 원본 **456행**(Pair 11·Destroy 12·Notify 8·Has 16·Remove 9·Attach 136·Robust 256·Damage 8): 결과, 통지/전송 당시 두 슬롯 전체, 관측 슬롯, 풀 요약·전체 Adler·삭제 깊이 |
| 계약 | 19개: 감시 쌍 조회/제거/재진입, 현재 Depend TYPE 후보, 부분 쓰기·같은 부모 쌍, contained 생성·사제 포획·Damage, GameAuxState 번호 두 번 읽기, 다른 풀/Process 부착 부재, 현재 생성자 VT/Content thunk, Gump 통합·통지·판정, 통지 뒤 현재 TYPE, 늦은 Pop, 별칭/체인, WORLD 진단 뒤 재확보 풀, damage 조기 반환·재부착 4종, 공유 깊이, 부모 통지 분배, 연결 검사의 현재 풀/순환/Process getter, contained 생성 순서 |

- **음성 대조(18종, 임시 변경 후 원복):** 아래 18가지 변경은 모두 원본 대조 또는 계약 검사에서 실패했다(실패 1~5개). 변경은 저장소 밖 임시 도구로 했고, 여섯 모듈이 원래 내용으로 돌아왔는지 바이트 비교로 확인했다.
  - 감시 쌍 생성: ① client 상한 포함 ② 둘째 TYPE 재조회 생략 ③ 둘째 VT를 첫 부착 전에 읽음
  - Depend 호스트: ④ preDestroy의 watcher 상대 삭제 생략 ⑤ 통지 조건 무시 ⑥ has의 TYPE 비교를 VT 비교로
  - 정렬 부착: ⑦ 순서 비교 <= ⑧ 둘째 previous getter 생략 ⑨ 저장 VT 대신 현재 VT ⑩ 통지 생략
  - Content 호스트: ⑪ 포획 이전 소유자 마스크 ⑫ Robust 마스크 ⑬ Damage 0x100 해제 생략 ⑭ 포획 꼬리 판정 순서
  - 기타 부모: ⑮ 연결 검사 조기 성공 ⑯ Gump 소유 ID 0 통지 ⑰ 파생 통지를 빈 몸체로 ⑱ contained 다리 TYPE 사전 읽기
  - 처음 돌렸을 때 ⑥만 검출되지 않았다. 계약 검사 `DependFormHost.has/remove: 현재 Depend TYPE 번호로 후보를 고른다`를 추가했고, 그 검사가 ⑥을 검출함을 다시 확인했다.
- 전체 검사: Node v24.21.0, 이 단위 신규 23개 포함 **429/429 통과**(5절). 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 남은 경계

- **분배기·Content 수명(D5b):** 여러 Form 클래스가 섞인 종속 체인의 중첩 삭제를 현재 VT로 분배하는 `SquidFormDispatch`와 `ContentFormLifetime`이 남아 있다. ContentFormLifetime은 Content의 pre/post 삭제·경제 효과·커서 취소를 맡으며 `SquidReward`(보상 경제)에 의존한다. 관련 원본 관측은 `contentlifetime-x86.tsv`(10.78 198행)이고, 관련 검사는 cpppj `FormClassLifetime_CurrentVtableKeepsGumpAndContentEffects`와 `DependCreateCurrent_UnregisteredClassDoesNotUseDifferentRawTypeRoute`다.
- **ProcessHost(D5c):** 다음이 남아 있다. 관련 원본 관측은 `process-x86.tsv`(120행), `processform-x86.tsv`(612행), `process-attach-current-local-x86.tsv`·`baseprocess-attach-current-local-x86.tsv`(각 24행), `regular-current-*`다.
  - `SquidProcessHost`와 Process/BaseProcess 부착
  - Regular(`AddRegular`/`AddSharedRegular`/`FindEvent`)
  - Depend 생성이 Process VT를 만났을 때의 직렬화 부착 접두
- **CD 전용 정렬 경로:** `SquidForms::ValidatePop`은 cpppj에서 CD 정렬 Content 경로만 소비하므로 옮기지 않는다(웹 10.78 범위 밖).
- **실제 World 검사:** cpppj의 World 검사(`DependCreateCurrent_World…` 등)는 같은 Factory·GameAuxState를 직접 빌려 주는 형태로 옮겼다. 세션이 호스트들을 함께 소유하는 연결은 WB05다.
- **파생 통지:** 파생 내용물 부모 통지(워커/다이스/워크샾/Player)의 실제 몸체는 각 소비 단계에서 옮긴다. 원본 관측은 `carrier-content-notice`, `daiscontentnotice`, `player-content-notice`다.

## 5. 검사 수치와 다음 단계

- form-hosts 4개(약 2.5초), form-hosts-contract 19개.
- 다음은 D5b다(분배기·ContentFormLifetime·보상 경제 의존 확인). D5b를 시작할 때 `node webpj/tools/fixture-manifest.mjs --write`로 이번 4개 항목의 `webCommit`(`worktree`)을 실제 커밋으로 갱신한다.
