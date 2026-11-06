# webpj Unpop 처리기·미니맵 dirty 큐·공통 종속 해제·공통 삭제기 — WB04-D2

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3(WB04-D)의 두 번째 하위 단위다. D1의 공통 공간 해제 부모 위에 **`SquidUnpop` 처리기**(표시 갱신 vtable 지원표·미니맵 dirty 큐·건물 옆면·contained 종단 연결), **공통 contained 해제 종단**, **공통 삭제기 `RawSquidDestroy`**와 원본 대조에 필요한 **다리 삭제 전후 훅**을 옮겼다. 화면 변경과 앱 수명 연결은 없다.

## 1. 근거

- 시작 트리 `df2f5fa7`. 작업 중 다른 세션의 `9c4533d9`(운반체 커서·화물 메뉴 검증, cpppj 일시 정지 인계)가 커밋됐으나 아래 근거 소스는 바뀌지 않았다. 근거는 모두 커밋본이다.
- 근거 소스: cpppj `o/SquidUnpop.cpp`(`ea848459`)·`.h`(`6ff267a1`), `o/UnpopVtables.inc`(`fbc27346`), `o/ContentNoticeVtables.inc`(`68ca4e3a`), `o/RawMetadisplayDirty.cpp/.h`(`bae8b702`), `o/RawSquidContainedUnpop.cpp/.h`(`d678fcad`), `o/RawSquidDestroy.cpp`(`f2215dc5`)·`.h`(`886f2d23`), `o/RawBridgeLifecycle.cpp`(`5879152b`)·`.h`(`225f2322`), 합성 Take 근거 `o/RawSquidTake.cpp`(`e1a617ac`).
- 원본 관측: `cpppj/tests/fixtures/unpop-vtables.tsv`(`a86150db`, 10.78 83행), `metadisplaydirty-x86.tsv`(`028bd402`, 10.78 520행), `destroy-x86.tsv`(`5edb2b6d`, 10.78 Destroy 600행·Bridge 64행, 두 x87 정밀도 639/895 포함). 장면 구성은 cpppj `tests/UnpopTests.cpp`·`MetadisplayDirtyTests.cpp`·`RawDestroyTests.cpp`·`SquidContainedUnpopTests.cpp`와 같다. 10.37·CD 10.72 행은 범위 밖이다.

## 2. 파일과 계약

| 파일 | 내용 |
| --- | --- |
| `core/squid-vtables.js` (신규) | `SQUID_ASSET_VTABLES`(지원 자산 생성자 최종 VT 83칸: [VT, Unpop 표시 비트, 내용물 부모 통지 +0x24]), `SQUID_FORM_VTABLES`(Depend/Process/Gump/Content), `SQUID_BASE_VTABLE`, `squidAssetVtable`, `isSquidFormVtable`. cpppj 두 `.inc`에서 생성했고 두 표의 83칸 VT가 같은 순서임을 확인했다 |
| `core/squid-unpop.js` | `supportsUnpopDisplay(vtable, flags)`와 `SquidUnpop` 클래스 추가(D1 부모는 그대로) |
| `core/metadisplay-dirty.js` (신규) | `createMetadisplayDirtyState({ fullRefreshOwner })`, `RawMetadisplayDirty`(enqueue·enqueueSid·refresh) |
| `core/squid-contained-unpop.js` (신규) | `RawSquidContainedUnpop`(detach·notifyUi·unpop), `commonContainedParent/Previous/SetParent/SetPrevious` |
| `core/squid-destroy.js` (신규) | `RawSquidDestroy`(destroy·completePreDestroy·completePostDestroy·preDepth·postDepth), `SQUID_DESTROY_EFFECT` |
| `core/bridge-lifecycle.js` (신규) | `RawBridgeLifecycle`(linkNeedsDestroy·preDestroy·postDestroy·delayedFallPayload), `makeBridgeLifecycleHooks`, `BRIDGE_LIFECYCLE_EFFECT`, `BRIDGE_FALL_EVENT` |
| `tests/raw-support.mjs` (신규) | 검사 공용: 행 읽기·float 비트·리틀엔디언 put/get·Adler-32·FNV-32·DWORD 직렬화 |

- **SquidUnpop:** `new SquidUnpop(pool, hash, spots, display = null)`. `bindDiagnostics({ checking, assertion, types })`가 없으면 진단 DWORD는 0이고 TYPE 조회는 Unpop 인자 레코드를 그대로 쓴다(cpppj 독립 입력과 같음). 전체 188칸 표가 연결되면 현재 TYPE BYTE로 재조회한다. 미니맵 통지는 연결된 `RawMetadisplayDirty`가 있을 때만(원문 helper의 자연 반환), 건물 옆면은 `setBuildingSpace`(같은 풀/해시 확인), contained 종단은 `bindContainedUnpop`. 표시 갱신은 체인 해제 뒤 현재 VT를 읽어 `supportsUnpopDisplay`가 거짓이면 오류(미복원 파생 표시)이고, `display`가 null이면 억제한다. 선택된 장치가 없을 때의 오류는 앞선 void·spot·체인 변경을 되돌리지 않는다.
- **supportsUnpopDisplay:** 마나(103)·볼트(105) VT(`0x50c3b0`·`0x509810`)는 항상 참, 그 밖은 지원표 비트(플래그 `0x2000`이면 비트 2=update8c, 아니면 비트 1=update88)다. 10.78 표의 비트는 0 또는 3뿐이다.
- **RawMetadisplayDirty:** 셀 번호는 `trunc(float)`의 signed32를 다시 0 방향 `/4`(-3.9→0, -4.0→-1). 활성 표시(존재·플래그 비트1)일 때만 등록하고, 같은 셀은 포화 검사보다 먼저 걸러지며, 100칸이 차면 전체 갱신 1을 쓴다. `refresh`는 비활성이어도 비트맵을 최초 확보하며 전체 갱신 1을 쓰고, 활성일 때 view가 바뀌었거나 큐/전체 갱신이 있으면 view를 저장한 뒤 재생성 장치를 부른다. 그 뒤 프레임을 올리고 전체(y 바깥 64×64) 또는 부분(그림 장치가 새로 등록한 셀까지 현재 count로) 셀을 그린 뒤 count·전체 갱신을 0으로 만든다. `fullRefreshOwner`(`{ value }`)가 있으면 Client의 005949d4와 같은 소유자를 직접 읽고 쓴다.
- **RawSquidContainedUnpop:** void는 쓰지 않는다. 접두 contained 진단(0x3a3)과 부모 getter·`parentSid`(0x3a5)·free 부모(0x3a8, 진단 뒤 계속) 다음, dead 부모면 링크를 그대로 두고 끝난다. 그 밖은 contained 재진단(0x3b1)을 거쳐 next를 이전 getter보다 먼저 저장하고, prev의 next 쓰기(같으면 `n != getSid()` 진단 뒤에도 씀)·next의 setPrevious·부모 머리 갱신(prev가 있으면 `prev == 0` 진단)·contained 비트 해제를 한다. 마지막으로 자산 또는 현재 내용물 Form TYPE이면 부모에 이유 2를 통지한다. `notifyUi`는 (10.78) 진단 비0·메타맵 아님·자신 dead·boss 아님이면 Squidfinder 진단을 부른다. 그 뒤 `+6` 목록에서 Form(TYPE ≤ 0x45) 중 현재 GumpForm TYPE(DWORD 전체 비교)을 처음 만나면 현재 사건을 한 번 큐에 넣는다. 전역(`contentFormType` 00541088·`gumpFormType` 0054107c·`uiEvent` 005427dc·진단·메타맵·boss)은 호출 시점마다 읽는 getter로 받는다. GameAuxState에는 아직 이 세 전역이 없으므로 실제 연결은 Form 소비자(D3)와 함께 한다.
- **공통 링크 getter/setter:** 타입·상태로 추정하지 않고 현재 VT로 고른다. 네 Form은 부모 +16/이전 +14, 지원 자산 83종은 부모 +14/이전 +16이며, 모르는 VT는 오류다.
- **RawSquidDestroy:** 범위(5 미만·예측 머리·풀 밖) → 이미 dead면 끝 → 미지원 root(free·TYPE 범위 밖·Form인데 `unpopForm` 없음·contained 자산)·장치·권한(비서버의 서버 SID는 flags 8 필요)을 dead 전에 거부한다. 서버의 client SID는 flags에 0x10을 더한다. 그 뒤 dead → pre 깊이+1·preDestroy(완료 호출로 깊이가 돌아와야 함)를 거쳐 현재 `+6` 머리부터 각 next를 먼저 저장하며 ReleaseDependent·(free가 아니면) DestroyDependent(flags|0x40)를 낸다. 이어 서버 전파(0x10/0x40 없고 서버이며 서버 SID, flags|8), 선택 해제를 거친 뒤, 현재 void가 아니면 공간 해제(자산은 `SquidUnpop.unpop(sid, types[TYPE], 0)`, Form은 `unpopForm`)를 한다. 마지막은 post 깊이+1·postDestroy → `pool.release(sid)`이다. 종속 체인 손상(free·비contained 자산·순환)과 깊이 불균형은 그 경계에서 멈추며 이미 한 변경은 되돌리지 않는다.
- **RawBridgeLifecycle:** preDestroy는 일반 다리(extra 비트 9 없음)면 `trunc(x + 0.99999)`(float 비트 0x3f7fff58, x87 double 덧셈)의 ±1 사각형을 실제 `RawSquidFinder`로 돌며, 현재 링크 TYPE이고 첫 참조(+12)가 일반이며 두 참조(+12/+8)의 dead OR가 참인 링크를 DestroyLink로 낸다. 그 뒤 공통 preDestroy로 간다. postDestroy는 파편→소리(그 시점 좌표)→다시 읽은 좌표 한 칸의 walker(flags2 0x10000) 낙하→공통 postDestroy다.
- **다리 가상 삭제 분배(004220f0, `RawSquidDestroyDispatch`)는 옮기지 않았다.** Graph 표면 수·현재 프레임 코드·권한/편집기 단락이 필요해 다리 수명 단계(WB07)에서 옮긴다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| vtable 지원표 | 원본 **83행**: 플래그 0/0x2000 각각의 지원 여부가 PE mask와 같다(마나/볼트·소용돌이 8c·간헐천 부분 8c는 cpppj와 같이 추가 허용). 웹 83칸 VT 집합이 원본 행과 정확히 같다 |
| 미니맵 dirty | 원본 **520행** 전체: count·전체 갱신·비트맵 폭·초기화 비트·프레임·저장 view와 큐 100칸(count 밖 포함)·비트맵·장치 호출 기록의 FNV-32 |
| 공통 삭제 | 원본 **Destroy 600행**: 사건 순서(P/R/C/T/S/O와 효과 직전 상태), 대상·주변 일곱 슬롯 전체 바이트, 풀 카운터·free 머리·꼬리, 풀 전체(1.6MB)·네 단계 해시·spot·삭제 기록 Adler-32, 선택, pre/post 깊이 |
| 다리 삭제 | 원본 **Bridge 64행**: 같은 탐색기를 유지한 채 링크를 실제 공통 destroy로 재귀 삭제·반납(A/B/N/W/F 사건 포함), 같은 비교 항목 |
| 계약 | Unpop 부분 변경 경계 6종(found 진단·미복원 표시·접두 진단·self-next 머리·순환·NaN 좌표), 표시 장치/현재 TYPE 표/연결 확인, 미니맵 통지 시점(void 직후 기존 좌표)·매몰/비표면 제외, contained 종단 연결(진단 복귀 뒤 종속 해제·UI 통지), 공통 종속 해제 10종(cpppj 계약 검사의 10.78 부분), 삭제 dead 재진입·깊이 불균형·권한/root·종속 손상, 다리 낙하 값·탐색 사각형 보정 |

- **역할 1/2 root의 합성 준비:** cpppj는 서버/예측 SID root를 실제 `Factory.Take(74)`로 준비한다. Take는 D4 범위이므로 `syntheticTake`가 원본 Take(004af610, `RawSquidTake.cpp`)의 free 슬롯 효과만 같은 순서로 쓴다(상태 &0xfc → base VT → TYPE → 상태|4 → zOrder). free 슬롯은 VT+48을 부르지 않고 free 목록/카운터도 바꾸지 않으며, 장면이 곧바로 그 슬롯 50바이트를 덮어쓴다. D4에서 실제 Take로 바꾼다.
- **Unpop 경계 검사의 합성 준비:** cpppj Guards 검사의 `Factory.Create(82)` 두 번은 같은 서버 할당(`allocate(0)`)과 base VT로 대신했다. 검사는 그 뒤의 초기 바이트 대비 변화만 비교한다.
- **음성 대조:** 임시로 ① 선택 해제 제거 ② 전파 조건에서 0x40 제외 ③ walker 낙하 제거 ④ 미니맵 셀을 floor로 계산 ⑤ 큐 포화 경계 변경 ⑥ contained UI 검색의 Form 필터 제거를 하면 각각 원본 대조 또는 계약 검사가 실패함을 확인하고 원래대로 되돌렸다. ⑦ 다리 preDestroy의 0.99999 보정 제거는 원본 Bridge 64행이 모든 객체를 같은 칸에 두어 드러나지 않으므로 탐색 사각형 계약 검사를 추가했고, 그 검사로 검출됨을 확인했다. ⑧ update8c 비트 무시는 10.78 표의 비트가 0/3뿐이라 관측상 구별되지 않는다(동작 차이 없음).
- 신규 검사 4파일(`squid-destroy` 8, `metadisplay-dirty` 4, `squid-unpop-handler` 5, `squid-contained-unpop` 10). 이식 추적 목록에 `unpop-vtables`·`metadisplay-dirty`·`squid-destroy`를 등록했다(44항목, 해시 불일치 0). 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 남은 경계

- **`unpop-x86.tsv`(Create/Take/FirstPop/Release 포함 3618단계)는 D4로 옮긴다.** 각 단계가 실제 Factory Create/Take(생성자 recipe·VT+48/+2c)를 거쳐 풀 전체 Adler를 비교하므로 Factory 없이 일부만 재생할 수 없다.
- 공통 pre/postDestroy 내용(`SquidDestroyLifecycle`, `destroylifecycle-x86.tsv`)과 Graph 삭제(`destroygraph-x86.tsv`)·파생 Pre/PostDestroy(사제·운반체·폭탄·비행체 등)는 소유자/장부·Graph·각 파생 단계에서 옮긴다. 이번 삭제기는 그 훅들을 `emit` 장치로 받는다.
- 앱 수명 연결(세션이 풀·해시·spot·Unpop·미니맵 큐·삭제기를 함께 소유)은 첫 실제 소비자(Fort Load→Start, WB05)와 함께 한다. 미니맵 셀 그림/표시 재생성 몸체는 WB03-M2 미니맵 표시에서 연결한다.

## 5. WB04-D 하위 단위 계획 (2026-11-05 갱신)

| 하위 | 범위 | 원본 관측 후보 |
| --- | --- | --- |
| [x] D1 | 공통 공간 해제 부모·spot genus·건물 옆면 해제 | `squid-unpop-parent-hardware-x86.tsv`, `hash-x86.tsv` Genus |
| [x] D2 | `SquidUnpop` 처리기·vtable 지원표·미니맵 dirty 큐·공통 contained 종단·공통 삭제기·다리 삭제 훅 | `unpop-vtables.tsv`, `metadisplaydirty-x86.tsv`, `destroy-x86.tsv` |
| [ ] D3 | Form 부착·생성자(`RawFormAttach`·`SquidForms`·Depend/Process/Gump/Content Form 호스트)와 Form Unpop, 종속 전역(00541088·0054107c·005427dc) | `form-*-current-x86.tsv`, `form-unpop-current-x86.tsv` |
| [ ] D4 | `SquidFactory` Create/Take/FirstPop과 `SquidProcessHost`·Regular(`AddRegular`/`FindEvent`), `unpop-x86.tsv` 재생과 합성 Take 교체 | `unpop-x86.tsv`, `regular-current-*-x86.tsv` 등 |

D3/D4의 cpppj 소스는 착수 시점의 커밋본을 기준으로 하고 이후 변경은 3.5절에서 반영한다.

**2026-11-06 갱신:** D3는 공통 Form 층과 Form 종속 전역으로 완료했고, Form 호스트는 Factory 뒤로 옮겨 D4(Factory)·D5(Form 호스트·Process/Regular·분배)로 다시 나눴다. 현행 계획은 [공통 Form 층 문서 5절](webpj-squid-forms-20261106.md)을 따른다.
