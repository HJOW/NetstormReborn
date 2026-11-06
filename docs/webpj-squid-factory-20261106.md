# webpj SquidFactory(생성·Take·초기화·firstPop)와 생성자 표 — WB04-D4

2026-11-06, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3(WB04-D)의 네 번째 하위 단위다. 원본 10.78 `Squid.cpp`의 **객체 생성 공장**을 옮겼다. 같은 SID 풀에서 할당하고, 생성자 쓰기와 가상 초기화를 하고, 지정 SID를 수신(Take)한다. 생성자 주소 표와 자산 파생 생성자 쓰기 목록을 함께 옮겼다. D2/D3 검사에 남아 있던 합성 준비(Take·Create 대체)는 실제 Factory로 바꿨다. 화면 변경과 앱 수명 연결은 없다.

## 1. 근거

- 시작 트리 `410beda4`(작업 트리에 다른 세션의 cpppj 미커밋 변경이 있었으나 보존했고 근거로 쓰지 않았다). 근거는 모두 커밋본이다.
- 근거 소스: cpppj `o/SquidFactory.cpp/.h`(`cf4bdec8`), `o/RawSquidCreation.cpp`(`e6259e97`), `o/RawSquidTake.cpp`(`e1a617ac`), `o/RawRiftHitPoints.cpp`(`804b87a5`), `o/TypeConstructors.inc`(`078ea3e8`), `o/DerivedConstructors.inc`(`f7675661`).
- 원본 관측(10.78 행·블록만): `cpppj/tests/fixtures/constructors-x86.tsv`(188행), `asset-constructor-current-x86.tsv`(567행), `squid-creation-parent-x86.tsv`(294행), `squid-take-parent-current-x86.tsv`(360행), `factory-initialization-current-x86.tsv`(288행), `first-pop-flags-x86.tsv`(256행), `creation-x86.tsv`(2블록 482단계), `derived-x86.tsv`(4블록 3300단계), `unpop-x86.tsv`(9블록 1809단계). 행마다의 합성 입력과 명시 장치는 cpppj `tests/CreationTests.cpp`, `AssetConstructorCurrentTests.cpp`, `SquidCreationParentTests.cpp`, `SquidTakeParentCurrentTests.cpp`, `FactoryInitializationCurrentTests.cpp`, `FactoryFirstPopCurrentTests.cpp`, `UnpopTests.cpp`와 같다. 기대값은 fixture에서 읽는다. CD 10.72 행은 범위 밖이다.

## 2. 파일과 계약

| 파일 | 내용 |
| --- | --- |
| `www/core/squid-constructors.js` (신규) | `PATCH_TYPE_CONSTRUCTORS`(188칸, 비0 122), `PATCH_CONSTRUCTOR_RECIPES`(자산 파생 82개), `typeConstructorAddress`, `constructorRecipe`, `applyConstructorRecipe`, `supportsConstructedVtable`. cpppj 두 `.inc`에서 저장소 밖 도구로 생성했다 |
| `www/core/squid-factory.js` (신규) | `createRawSquid`(004af530), `takeRawSquid`(004af610), `readRiftHitPoints`(0049fce0), `currentRiftTypeNumber`, `SquidFactory` |
| `tests/squid-factory.test.mjs` (신규) | 원본 fixture 9종 재생과 생성자 VT 83칸 교차 확인 |
| `tests/squid-factory-contract.test.mjs` (신규) | cpppj 계약 검사 30개의 10.78 부분과 음성 대조 보강 1개 |
| `tests/squid-destroy.test.mjs`·`squid-unpop-handler.test.mjs`·`squid-forms.test.mjs` | D2/D3 합성 준비(`syntheticTake`, allocate+base VT)를 실제 `take(74)`/`create(82)`/`create(74)`로 교체 |
| `tests/fixtures/manifest.json` | 이번 fixture 9개 등록(58항목). D3 항목 6개의 `webCommit`을 `410beda4`로 기록. 삭제기 항목 범위 문구와 모듈 갱신 |

- **생성자 표:** 타입 번호마다 생성자 주소가 있다. 0이면 base 생성자로, VT `0x501dd0` DWORD 하나만 쓴다. 자산 파생 recipe는 `[offset, width, OR, 값]` 쓰기를 순서대로 적용한다. 중간 VT 쓰기, extra(+40) OR, anim의 WORD8/HP 지우기도 그대로다. 내장 Form 네 생성자 주소는 `squid-forms.js`의 `constructVtableAt`으로 보낸다. base와 recipe의 최종 VT 83개는 D2의 지원 자산 VT 83칸과 정확히 같다.
- **createRawSquid:** 진행 순서는 다음과 같다. 먼저 현재 fakeSurface TYPE과 같은지 진단한다(0x189). 이어서 할당하고 signed SID의 예측 경계를 진단한다(0x191/0x194). 그다음 현재 TYPE의 생성자를 조회해 실행한다. 반환 객체의 VT는 **TYPE BYTE를 쓰기 전에** 저장한다. 마지막으로 TYPE BYTE를 쓰고, 저장해 둔 VT로 +28을 부른다. 반환 객체 토큰은 할당 SID와 다를 수 있다.
- **takeRawSquid:** 먼저 SID 범위를 진단한다(0x1a7). capacity 조회는 SID가 15000 이상일 때만 한다. free가 아니면 TYPE이 다를 때 상세를 쓴 뒤 진단하고(0x1ba), 현재 VT로 +48(Unpop)을 부른다. 이어서 상태를 다시 읽어 &0xfc로 쓰고, 현재 생성자를 실행한다. 반환 객체의 현재 상태와 VT를 읽은 다음 TYPE, void(|4), 저장한 VT의 +2c 순서로 진행한다. free 목록과 카운터는 바꾸지 않는다.
- **SquidFactory:** `new SquidFactory(pool, types, { weakenedMana, unpop })`. 레코드는 생성 때 복사한다. `bindDiagnostics({ types, assertion, detail, checking })`에 준 `types`는 복사하지 않고 빌린다. 매 조회는 현재 표, 진단 장치, 상세 writer를 다시 읽으므로 콜백 중에 다시 연결해도 다음 위치부터 새 장치를 쓴다.
  - `create`는 생성자를 실행한 뒤 Form VT면 빈 +28로, 지원 VT면 공통 `postCreate`로 가고, 그 밖의 VT는 오류다. 할당 뒤에 실패해도 할당은 되돌리지 않는다.
  - `take`는 Form VT면 `unpopForm`을 부른다. 지원 자산이면 `SquidUnpop.unpop(sid, 현재 raw TYPE 레코드, 0)`을 부른다.
  - `postCreate`(004b0c60)는 `!isForm` 진단(0x7d4)과 `!isContained` 진단(0x7d5) 뒤에 TYPE을 고른다. zOrder BYTE를 쓰고, 서버면 owner를 0으로 하고 HP를 쓴다. HP 필요 여부는 다이스 flags2면 extra 비트0으로, 아니면 HAS_HIT_POINTS로 정한다. 마지막에 contained를 다시 진단한다(0x7df).
  - `postTake`(004ad4a0)는 `!isForm` 진단 뒤 현재 TYPE의 zOrder를 쓰고, 그다음 contained를 진단한다.
  - `firstPopFlags`(004ad470)는 extra의 1/8 비트만 읽고 아무것도 쓰지 않는다.
  - `construct`는 독립 생성자다. 예약·free·contained 슬롯과 미복원 주소는 쓰기 전에 거부한다. `constructCurrent`(004af720)는 상태 검사 없이 현재 생성자만 실행한다.
  - `constructByAddress`는 원문 주소 하나의 쓰기만 적용한다. NULL 0x4429e0은 SID를 읽지 않고 true를 돌려준다. 다이스 0x44a420은 진단이 정상 복귀한 뒤에야 범위를 확인하고 VT 0x507950을 쓴다. 미지원 주소와 0은 false다.
  - `retype`(004b1620)는 void와 발자국 진단 뒤 TYPE BYTE를 쓰고 현재 생성자를 실행한다.
- **TYPE HP(웹 적응):** cpppj가 장기 주소로 빌리는 다음 전역은 웹에서 `bindGameAux(aux)`로 같은 `GameAuxState`를 빌려, 호출할 때마다 읽는다. 다른 소유자로 다시 연결하는 것은 거부한다. 빌리지 않은 독립 입력은 원문 초기값(158/165/162)과 생성자 인자 `weakenedMana`를 쓴다. checking은 `bindDiagnostics`의 함수다.

  | 주소 | GameAuxState 필드 |
  | --- | --- |
  | 005ca8f0 | 사제 1/4 HP `priestQuarterHp` |
  | 005412d0 | `priestType` |
  | 005412ec | `daisType` |
  | 005412e0 | `fakeSurfaceType` |

  getNum은 HP를 고른 레코드 객체가 현재 표 배열에서 차지하는 위치다. 같은 레코드를 갖지 않은 다른 표는 거부한다. cpppj의 물리 주소 차이 계산과 결과가 같은 범위는 같은 배열이거나, 그 레코드를 포함하도록 옮겨 본 표다. 레코드가 현재 표 앞에 있는 음수 번호는 웹에서 오류다.
- **Form 진입:** `createContained`는 Create(6), `createGumpForm`은 Create(3)이다(기본 flags 2). 둘 다 현재 TYPE 생성자를 쓰며 고정 VT를 쓰지 않는다. 나머지 진입은 다음과 같다.
  - `attachForm`은 현재 TYPE의 genus(flags2)와 `bindFormAttach`로 빌린 contentFormType을 쓴다. 빌린 것이 없으면 6이다. 예: `squidFormGlobals(aux).contentFormType`.
  - `popForm`은 `bindFormPopDepth({value})`로 빌린 공유 깊이를 쓴다. 다른 소유자로 다시 연결하는 것은 거부한다.
  - `unpopForm`은 `bindFormDetach`로 빌린 종속 분리를 쓰고, 없으면 독립 기본 분리를 쓴다. 진단은 모두 같은 Factory 진단이다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 생성자 주소 | 원본 **188행** 일치(비0 122) |
| 자산 생성자 | 원본 **567행**: 물리 슬롯 전체·다이스 진단 횟수·free 카운터·상태 BYTE 불변 |
| Create 부모 | 원본 **294행**: 사건 순서(진단·할당·생성자 조회·base/cdecl·VT·TYPE·+28)·반환 객체·그 객체 전체 바이트(32비트 토큰 감김 포함) |
| Take 부모 | 원본 **360행**: 사건 순서(Q/R/T/V/F/L/D/U/W/C/A/B/Y/P)·반환 객체·두 객체 전체 바이트 |
| 초기화/HP | 원본 **288행**: 슬롯 전체·진단(당시 슬롯 hex 포함)·최종 서버 권한·1/4 HP·사제 TYPE(GameAuxState) |
| firstPop | 원본 **256행** × 6 물리 입력(예약 0/1/2·free·다른 VT): 반환값과 풀 전체 무쓰기 |
| base 생성 | 원본 **482단계**: 반환 SID·free/cursor·두 머리와 꼬리·풀 전체 Adler·결과 슬롯 전체 |
| 파생 생성 | 원본 **3300단계**: 같은 항목(서버/클라이언트·약화 HP 블록) |
| Unpop/Take/반납 | 원본 **1809단계**: 같은 항목 + 삭제 기록·네 단계 해시·spot Adler |
| 계약 | Create 경계(할당 전/후), 미복원 생성자, anim Take, Construct 거부, Take 진단 6종, PostCreate 3종, Form VT의 빈 +28, fakeSurface·GameAuxState 장기 전역, constructCurrent, Create 부모 장치 부재, 다이스 진단 경계, FactoryTakeChecking 7종(실제 SquidUnpop·contained 종단), 자산 Unpop의 현재 raw TYPE 인자, Form 진입 5종, retype |

- **음성 대조(16종, 임시 변경 후 원복):** 아래 16가지 변경은 원본 대조 또는 계약 검사에서 실패했다(실패 1~5개). 변경은 저장소 밖 임시 도구로 했고, 두 모듈이 원래 내용으로 돌아왔는지 바이트 비교로 확인했다.
  - 처음 돌렸을 때 ⑪만 검출되지 않았다. 기존 검사에서는 Unpop이 void로 조기 반환하거나 전체 TYPE 표를 따로 빌려 인자 레코드를 쓰지 않았기 때문이다. 공간 해제 경계를 명시 장치로 관측하는 계약 검사(`SquidFactory.take: 자산 Unpop 에 현재 raw TYPE 레코드를 넘긴다`)를 추가했고, 그 검사가 ⑪을 검출함을 다시 확인했다.
  - Create: ① VT 저장을 TYPE 쓰기 뒤로 ② fakeSurface 진단을 할당 뒤로
  - Take: ③ 반환 객체의 상태/VT 읽기 순서 교환 ④ 상태 마스크 0xfe ⑪ 자산 Unpop의 TYPE을 요청 TYPE으로 ⑭ Form의 빈 +2c 대신 공통 postTake
  - PostCreate: ⑤ 초기 TYPE을 진단 전에 고름 ⑥ 서버 권한 무시 ⑦ 꼬리 contained 진단 생략
  - PostTake: ⑫ zOrder를 진단 전의 TYPE으로
  - HP·생성자: ⑧ 1/4 HP를 floor로 ⑨ 다이스 범위 확인을 진단 앞으로 ⑬ firstPop의 buried 비트를 2로
  - 생성자 표: ⑮ recipe OR를 대입으로 ⑯ 생성자 주소 한 칸(82) 변경
- 전체 검사: Node v24.21.0, 이 단위 신규 41개 포함 **406/406 통과**, 약 37.0초. 교체한 D2/D3 검사(삭제기 Destroy 600행·Bridge 64행, Unpop 경계, Form 층 계약)도 모두 통과했다. 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 남은 경계

- **웹 타입 로더 연결:** `buildTypeRecords`는 아직 `zorder`(23개 이름 상수와 오프셋)와 `constructorAddress`를 채우지 않는다. cpppj는 `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata`에서 이를 확인한다. 실제 미션 TYPE 표를 Factory에 넘기는 WB05 Fort Load에서 연결한다. 이번 단위의 Factory는 레코드 배열 계약까지다.
- **Form 호스트 단계(D5):** Depend/Gump/Content/ContentLifetime Form 호스트, `SquidProcessHost`·Regular, `SquidFormDispatch`와 `ValidatePop`·`RawFormLinked`, 정렬 Content 부착(`…AtTable`)은 D5에서 다룬다. cpppj에서 Depend 생성에 의존하는 `FormAttachCurrent_ContentVtableReallyUsesCommonThunk`와 Content 호스트에 의존하는 `FormPopCurrent_SortedContentBorrows…`도 D5다.
- **실제 World/Session:** cpppj의 RawSurfaceWorld/Session 검사는 웹에 World가 없어서 같은 Factory에 GameAuxState·깊이 소유자·종속 분리를 직접 빌려 주는 형태로 옮겼다. 해당 검사는 `FormAttachCurrent_World…`, `FormPopCurrent_ActualWorld…`, `FormUnpopCurrent_WorldTake…`, `FactoryTakeDiagnostics_ActualWorlds…`, `SquidCreationParent_ActualWorld…`다. 세션이 풀·Factory·Unpop·Form 층을 함께 소유하는 연결은 WB05에서 한다.
- **SID 할당 소진 회수(3.5절 ③):** 웹 `allocate`는 진단 정상 복귀와 다리 회수 없이 오류로 멈춘다. Factory도 그대로 전달한다. 현재 World 삭제 대여(`SquidFormDispatch`·World 수명)와 함께 반영한다.
- `ObjectConversion`(retype 소비자)·`RawFortRandomPlacement`·`RawContainedCreate`·`RawDependCreate` 등 Factory 소비자는 각 단계에서 옮긴다.

## 5. 검사 수치와 다음 단계

- squid-factory 10개(약 22초, 파생 3300단계·Unpop 1809단계가 대부분), squid-factory-contract 31개.
- 다음은 D5다. Form 호스트(Depend/Gump/Content·ContentLifetime), `SquidProcessHost`·Regular, `SquidFormDispatch`, `ValidatePop`·`RawFormLinked`를 다룬다. 계획은 [공통 Form 층 문서 5절](webpj-squid-forms-20261106.md)의 D5 행을 따른다. D5를 시작할 때 `node webpj/tools/fixture-manifest.mjs --write`로 이번 9개 항목과 `squid-destroy`의 `webCommit`(`worktree`)을 실제 커밋으로 갱신한다.
