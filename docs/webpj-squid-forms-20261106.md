# webpj 공통 Form 층(생성자·Create/Take·부착·Pop·Form Unpop)과 Form 종속 전역 — WB04-D3

2026-11-06, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3(WB04-D)의 세 번째 하위 단위다. 원본 10.78 `Squid.cpp`의 내장 Form 네 종(Depend 1·Process 2·Gump 3·Content 6)이 함께 쓰는 **공통 Form 층**을 옮기고, contained 해제·부착 꼬리 진단이 읽는 **Form 종속 전역 세 칸**(0054107c·00541088·005427dc)을 `GameAuxState`에 연결했다. 화면 변경과 앱 수명 연결은 없다.

## 1. 근거

- 시작 트리 `8db4af6f`(작업 트리에 다른 세션의 cpppj 미추적 파일 7개가 있었으나 보존했고 근거로 쓰지 않았다). 근거는 모두 커밋본이다.
- 근거 소스: cpppj `o/SquidForms.cpp/.h`(`09ad72b0`), `o/RawFormAttach.cpp/.h`(`d678fcad`), `o/RawFormPop.cpp/.h`(`09ad72b0`), `o/RawSquidContainedUnpop.cpp/.h`(`f83db66c`, 공통 링크 getter/setter와 세 전역 주소), `client/RawCampaignSession.cpp`(005427dc의 `0x02000039` UpdateContents와 005427bc Player 통지의 구별).
- 원본 관측(10.78 행만): `cpppj/tests/fixtures/form-constructor-x86.tsv`(`7a4ef74b`, 28행), `form-attach-current-x86.tsv`(`d678fcad`, 432행), `form-pop-current-x86.tsv`(`09ad72b0`, 1280행), `form-unpop-current-x86.tsv`(`36777e48`, 864행), `formtake-x86.tsv`(`a466b2a6`, 300행). 행마다의 합성 입력·명시 장치는 cpppj `tests/FormConstructorTests.cpp`·`FormAttachCurrentTests.cpp`·`FormPopCurrentTests.cpp`·`FormUnpopCurrentTests.cpp`·`FormTakeTests.cpp`와 같고 기대값은 fixture에서 읽는다. CD 10.72 행은 범위 밖이다.
- 세 전역의 초기값은 보호 원본 `originals/Netstorm.exe`(SHA-256 `a305414c…`)의 `.data`를 파일 읽기로만 대조했다(실행 없음).

## 2. 파일과 계약

| 파일 | 내용 |
| --- | --- |
| `www/core/squid-forms.js` (신규) | `SQUID_FORM_TYPES`(1·2·3·6), `SQUID_FORM_CONSTRUCTOR_VTABLES`, `formConstructorVtable`, `isFormConstructorVtable`, `attachCommonForm`, `popCommonForm`, `squidFormGlobals`, `SquidForms`(vtable·constructVtableAt·create·take·attach·pop·unpop·parent·previous·bindAttach/bindPop/bindUnpop) |
| `www/core/game-aux-state.js` | 필드 3칸 추가: `gumpFormType`(0054107c, 3), `contentFormType`(00541088, 6), `contentUnpopEvent`(005427dc, `0x02000039`). 47→50필드 |
| `tests/squid-forms.test.mjs` (신규) | 원본 fixture 5종 재생과 계약 검사 10개 |
| `tests/game-aux-state.test.mjs` | 필드 수 50·TYPE 칸 28·내장 Form 이름 3/6 추가(PE `.data` 대조는 기존 검사가 그대로 수행) |
| `tests/fixtures/manifest.json` | 이번 fixture 5개 등록(49항목). PE 항목 범위 문구 갱신. D2 항목 4개의 `webCommit`을 `3ef1d18b`로 기록 |

- **생성자/Create:** `constructVtableAt(type, sid)`는 원문 cdecl 생성자처럼 지정 물리 슬롯(0·예약·free·dead 포함)에 VT DWORD 하나만 쓴다. `create(type, flags)`는 타입 확인 → `pool.allocate(flags)` → VT → TYPE 순서이며 빈 postCreate다(004af530). 네 타입 밖은 할당 전에 거부한다(자산 생성은 Factory 몫).
- **Take(004af610):** 서버 영역(15000 이상) SID만 받는다. free가 아니면 raw TYPE이 같아야 하고 먼저 Form `unpop`을 한다. 통지가 바꾼 현재 상태에서 free/dead만 끄고 VT → TYPE → void를 쓴다(패치판 순서). payload와 풀 목록/카운터는 그대로다.
- **attachCommonForm(004ac800):** 패치판은 parent 0이면 무동작. `genus(현재 TYPE BYTE) & 0x44200`이면 진단 → 현재 VT를 넘긴 `setParent` → **setParent 뒤** 부모 +6 머리를 한 번 읽음(자기 번호면 `squid.h` 0x123 진단) → 자신 +4=머리 → 머리가 있으면 그 이웃의 현재 VT로 `setPrevious(head, form)` → 부모 머리=자신 → `setPrevious(form, 0)` → contained 비트 → `pop(form, flags|0x100)` → Pop 뒤 TYPE BYTE와 현재 `contentType()` DWORD 비교(0x3e0) → 현재 TYPE이 0x45 초과면 `isForm()`(0x3e3, 재조회).
- **popCommonForm(004ac560):** `isValid`(0 또는 capacity 이상) → 슬롯 → `isVoid` 진단 **뒤** 상태를 읽어 void 해제 → 이전 상태에 xmit(0x10)이 있으면 진단 → 현재 `server()`가 아니면 flags&8 없을 때 |0x10, 서버면 client 번호(5..14999)는 |0x10(flags&8이면 진단), 그 밖은 0x10이 없을 때 |8 → flags&0x50이 없으면 하위 3비트가 있을 때만 xmit 비트를 켠 채 `transmit(form, -1, -1, 0, flags)` 뒤 끔 → 깊이+1 → 현재 VT의 `postPop` → 깊이가 돌아오지 않으면 진단(0x392).
- **SquidForms.pop 기본 장치:** 권한은 풀의 현재 `isServer`(같은 `{value}` 소유자), 깊이는 지역 DWORD, 진단은 Unpop 진단, 전송은 생성 때 받은 `transmit(form, flags)`(없으면 그 위치에서 오류), postPop은 네 Form VT일 때만 깊이−1(004ac1d0)이고 그 밖은 오류다. 실제 세션은 `bindPop({ server, depth, setDepth, ... })`으로 공유 깊이를 빌린다.
- **SquidForms.unpop(004ae140):** TYPE>0x45면 `isForm()`(0xa59) → 번호 0이면 `isValid()`(0xa5a) → void면 `!isVoid()`(0xa5b) → void 쓰기 → 현재 VT를 넘긴 부모 getter(기본은 VT 기준 `commonContainedParent`) → **getter 뒤** 부모 free면 끝 → `detach`가 연결되면 그것만, 아니면 독립 입력의 공통 종속 분리(`RawSquidContainedUnpop.detach`, 초기 전역 6/3/0, 생성 때 받은 `notice`)다. UI 검색(`notifyUi`)은 Form 해제가 부르지 않는다.
- **parent/previous:** 예약(5 미만)/free 슬롯과 네 Form VT가 아닌 현재 VT는 거부한다(dead는 공통 삭제 중이라 허용). 바뀔 수 있는 TYPE 번호로 VT를 추정하지 않는다.
- **squidFormGlobals(aux):** `{ contentFormType(), gumpFormType(), uiEvent() }` getter 묶음이며 호출할 때마다 `GameAuxState`의 현재 DWORD를 읽는다(사본 없음). `RawSquidContainedUnpop`의 상태와 `bindAttach({ contentType })`에 그대로 넘긴다. 세 칸이 없는 객체는 거부한다.
- **부분 쓰기:** 선택된 장치가 없거나 미복원 VT를 만나면 그 위치에서 오류이며 앞선 쓰기(링크·void·xmit·깊이)는 되돌리지 않는다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 생성자 | 원본 **28행**: 슬롯 전체 바이트·풀 카운터 불변·TYPE 74 유지·반환 상대 주소 |
| 공통 부착 | 원본 **432행**: genus/setter/Pop/진단 사건 순서(호출 당시 VT·Pop 직전 링크 포함)·현재 content 번호·네 슬롯(0·5·7·9) 전체 바이트 |
| 공통 Pop | 원본 **1280행**: 사건 순서(진단 당시 상태·권한·깊이, 전송/후처리 인자)·최종 권한·깊이·this 슬롯 전체 |
| Form Unpop | 원본 **864행**: 진단·getter(VT)·분리 사건 순서·두 슬롯 전체 |
| Create/Take/Unpop/Reset | 원본 **300행**: 반환값·통지 당시 Form/부모 바이트·관측 슬롯·capacity/free/cursor/두 머리와 꼬리·전체 풀(1.6MB) Adler-32 |
| 계약 | 타입 거부·getter 경계, Take 통지 시점/payload 보존, 풀 재확보 뒤 삭제 기록 보존, Unpop 통지 부재·진단 뒤 현재 VT·부모 free/dead, Pop 전송/후처리 부재와 전송 flags, 부착 늦은 꼬리 진단·이웃 VT 부재·혼합 링크 WORD, GameAuxState 두 연결(부착 비교값·contained 통지/UI 사건) |
| GameAuxState | PE `.data` 50필드+장식4 대조 통과(새 3칸 모두 `.data`), TYPE 칸 28개 이름 대조 |

- **음성 대조(12종, 임시 변경 후 원복):** ① parent 0 조기 반환 제거 ② 부모 머리를 setParent 전에 읽음 ③ 부착 isForm을 저장 TYPE로 판정 ④ Pop 상태를 isVoid 진단 전에 읽음 ⑤ client 끝 6000 ⑥ xmit 비트 해제 생략 ⑦ 깊이 재조회 생략 ⑧ Unpop 부모 free를 getter 전에 판정 ⑨ Unpop void 쓰기 생략 ⑩ Take dead 비트 보존 ⑪ Take의 사용 중 Unpop 생략 ⑫ 생성자의 TYPE 쓰기 추가 — 모두 원본 대조 또는 계약 검사가 실패했다(실패 1~6개). 변경은 저장소 밖 임시 도구로 했고 원래 내용으로 복원됨을 바이트 비교로 확인했다.
- **합성 준비:** 계약 검사에서 cpppj의 `Factory.Create(74)` 부모/이웃은 D4 범위이므로 같은 `allocate`와 base 생성자 VT(`0x501dd0`)·TYPE 74만 쓴다(링크 WORD만 관측). `CreateGumpForm`/`CreateContained`는 Form 생성 경로가 VT→TYPE만 쓰므로 `create(3/6)`와 바이트가 같다(Factory의 현재 생성자 표 선택은 D4).
- 전체 검사: Node v24.21.0, 이 단위 신규 15개 포함 **365/365 통과**(5절 수치). 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 범위 조정과 남은 경계

- **Form 호스트(Depend/Gump/Content/ContentLifetime/Process)와 `SquidFormDispatch`는 이번 범위에서 뺐다.** cpppj에서 이들은 `SquidFactory`(Create/CreateGumpForm/Take/AttachForm/UnpopForm)를 호출하고, Factory는 이 공통 층을 쓰므로 순서가 Form 층 → Factory → 호스트다. 5절 계획에 D5로 옮겼다.
- `SquidForms::ValidatePop`(정렬 Content 부착의 선행 검증)과 `RawFormLinked`(Form::isRobust, `form-linked-parent-x86.tsv`)는 소비자가 ContentFormHost·건물 checking이므로 D5에서 함께 옮긴다.
- Factory의 Form 진입(현재 생성자 표로 고르는 `CreateContained`/`CreateGumpForm`, 파생 +74 접두의 `AttachForm`, 진단을 묶은 `PopForm`/`UnpopForm`, 공유 깊이 `BindFormPopDepth`)과 그 World 검사(`FormAttachCurrent_World…`·`FormPopCurrent_ActualWorld…`·`FormUnpopCurrent_WorldTake…`)는 D4다.
- 세 전역의 **실제 writer**(타입 적재 등)와 앱 수명 연결(세션이 풀·Form 층·contained 종단을 함께 소유)은 첫 실제 소비자(WB05 Fort Load→Start)와 함께 한다. 이번에는 같은 `GameAuxState`를 빌리는 모듈 계약까지다.
- **cpppj 변경(3.5절 기록):** ① `SidPool.cpp/.h`(`7b78cc20`·`435cfa1d`, 976/977) — 패치판 할당의 진단 정상 복귀와 소진 시 boss의 현재 World 다리 회수(최대 50). 정상 할당 결과는 같다(formtake 300행 통과). 웹 `allocate`는 아직 오류로 멈추며 현재 World 삭제 대여가 필요하므로 ③(Factory·World 수명 때 반영). ② `RawSquidContainedUnpop.cpp/.h`(`f83db66c`) — 정렬 Content 부착용 `…AtTable` getter/setter 추가. 기존 getter/setter 의미는 같아 ③(D5 Content 호스트 때 반영).

## 5. WB04-D 하위 단위 계획 (2026-11-06 갱신)

| 하위 | 범위 | 원본 관측 후보 |
| --- | --- | --- |
| [x] D1 | 공통 공간 해제 부모·spot genus·건물 옆면 해제 | `squid-unpop-parent-hardware-x86.tsv`, `hash-x86.tsv` Genus |
| [x] D2 | `SquidUnpop` 처리기·vtable 지원표·미니맵 dirty 큐·공통 contained 종단·공통 삭제기·다리 삭제 훅 | `unpop-vtables.tsv`, `metadisplaydirty-x86.tsv`, `destroy-x86.tsv` |
| [x] D3 | 공통 Form 층(생성자·Create/Take·공통 부착·공통 Pop·Form Unpop)과 Form 종속 전역 3칸의 GameAuxState 연결 | `form-constructor`·`form-attach-current`·`form-pop-current`·`form-unpop-current`·`formtake` |
| [ ] D4 | `SquidFactory` Create/Take/FirstPop·현재 생성자 표·Form 진입(CreateContained/CreateGumpForm/AttachForm/PopForm/UnpopForm), `unpop-x86.tsv`(3618단계) 재생과 D2 합성 Take·D3 합성 부모 교체 | `unpop-x86.tsv`, `factory-initialization-current-x86.tsv` 등 |
| [ ] D5 | Form 호스트(Depend/Gump/Content·ContentLifetime)·`SquidProcessHost`와 Regular(`AddRegular`/`FindEvent`)·`SquidFormDispatch`, `ValidatePop`·`RawFormLinked` | `formlifecycle-x86.tsv`, `depend-create-parent-current-x86.tsv`, `content-attach-current-x86.tsv`, `regular-current-*-x86.tsv`, `form-linked-parent-x86.tsv` 등 |

D4/D5의 cpppj 소스는 착수 시점의 커밋본을 기준으로 하고, 이후 변경은 3.5절 재반영에서 반영한다.
