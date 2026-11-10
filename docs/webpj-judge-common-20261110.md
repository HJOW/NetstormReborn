# webpj 3-E 판정-공통층 — WB04-E259 (2026-11-10)

3.1절 3-E의 첫 항목 **판정-공통층** 22단위를 판정했다. 이식 원본은 언제나 기준선 `1f00a25a` 판이므로, 기준선 뒤 cpppj 변경(대부분 CP 세션의 `Actual*`/`shared*` 실제 연결)이 단독 미션의 웹 결과를 바꾸는지만 본다. 판정 기준 커밋은 착수 때의 `HEAD` `4789039e2`이고 단위마다 행 base 를 "그 단위 파일을 마지막으로 바꾼 커밋"으로 올렸다(3.6절 절차).

- ① 웹 결과 영향 없음(C++ 수명·접근 제어·메타맵/일반 전투/LAN 전용·독립 typed 경로 불변)
- ② 웹 반영 필요 → 이 단위에서 재반영(시험 포함) 2건
- ③ 아직 웹에 없는 후속 단계 몫 → LEFT_JOBS.webpj.md 3.6절 표에 시점을 적음

판정은 `git diff 1f00a25a HEAD -- <단위 파일>`(22단위 `.cpp/.h`, 합계 약 700줄)을 모두 읽고, 새 API 의 소비자를 `cpppj/src` 전체에서 찾아(`grep`) 단독 경로 도달 여부를 가려 정했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(LEFT_JOBS.md 마지막 디컴파일 PC 와 무관한 정적 판정).

## 1. 판정 표

| 단위 | 판정 base(기준선 뒤 커밋) | 기준선 뒤 변경 | 판정 | 처리·시점 |
| --- | --- | --- | --- | --- |
| ConfigInterface | `eefded86` (1) | `ReadStringBytes`(+10): 고정 C 문자열 목적 버퍼에 첫 불일치부터 NUL 까지 복사 | ③ | 소비자 `ClientMain`·`ClientLocalStateGlobals`(ClientMetaPacketReceivers 는 메타 ①). **4-A** Config 이식 때 필요 여부 판정. 웹은 JS 문자열 저장소 |
| ContentFormHost | `3f766a97` (1) | `AttachCurrent`: Regular 재부착과 contained 생성의 마지막 부착이 호출 순간의 현재 가상+50 장치를 선택 | **②** | **재반영(`#attachCurrent`)**. 실제 World 가 `BindReattach` 로 설치하는 장치 연결은 ③ **4-D** |
| ContentFormLifetime | `cfbd6420` (1) | `ContentLifetimeState::LocalOwner()`(`sharedLocalOwner` 주소)·`hooks.selectedRaw` 전달 | ③ | 실제 Client 의 로컬 소유자 getter 연결 **4-D**. 독립 typed 입력은 값형 `localOwner` 라 결과 동일 |
| DependFormHost | `5635f21a` (1) | `BindVirtualDestroy`/`beforeFind_`: Native 는 상대 현재 VT 의 삭제·FormFinder 시작 진단·TYPE<70 후보·무검사 getter | ③ | 소비자 `RawSurfaceDepend`. **4-D**. 독립 typed 경로 불변 |
| GumpFormHost | `4ee69f4f` (1) | `BindCurrent`(같은 Client Gump TYPE 전체 DWORD·현재 가상 삭제·무검사 getter/setter), 생성자의 TYPE 사본 제거 | ③ + ① | Native 장치는 **4-D**. 사본 제거는 ①(크기 검사 유지) |
| RawFortObjectLoad | `0d276d0d` (2) | `FortObjectLoadState` 가 `loadingDepth`·`scope`·두 mask 를 참조로 보관, `FortSerializationGlobals` 대여 생성자 | ③ | 실제 Client 의 장기 전역 대여 **4-B**(Fort 적재). 독립 typed 생성자는 로컬 값이라 결과 동일. `.cpp` 순변경 0 |
| RawGameAuxState | `8defb347` (19) | 새 장기 전역 칸 약 100개(프로세스·TYPE 역할·패킷 헤더·미션 설정 7칸·Quick 건설·`typeUse` 256B·중앙 X/Y 등) | ③ | **기존 51칸의 값/폭 변화 0**(주석 3줄만). 칸은 소비자 단위에서 보호 PE 초기값과 함께 추가 |
| RawMissionBattleOptions | `76e7e04a` (2) | `OptionByte`/`RangeChoice` 로 읽기 구체화·순수 함수 3개·`MinimumDensity`/`MaximumDensity`/`InitializeDensity` | ① | 순수 함수 소비자는 `ClientMetaPacketReceivers`(메타, 웹 제외), 밀도는 일반 전투 State7 로그인(웹 제외). 기존 getter 결과 동일(오류 문구만 구체화) |
| RawSquidContainedUnpop | `be567d46` (1) | `BindRuntimePin`(C++ 호출 수명 유지)·`BindChecking` | ① | JS 는 GC·살아 있는 state 객체라 결과 영향 없음 |
| RawSquidDestroy | `417f22d2` (2) | `DestroyPatch`·`ReleasePatch`·`SquidDestroyDiagnostics`·`selectedRaw`(실제 PATCH 공통 삭제) | ③ | **4-D** World 수명 때 `SquidDestroyLifecycle` Native 와 함께(cpppj `DestroyLifecycleTests`). typed 경로는 선택 비교가 `selectedRaw` 우선 형태로 바뀐 것뿐이라 typed 결과 동일 |
| RawStateSidLists | `c9140743` (1) | `RemoveStateSidList`: PATCH 0040ea00 의 signed count 압축 제거 | **②** | **재반영(`removeStateSidList`)**. 소비자 `RawDudeBehavior`·`RawPriestPreDestroy`·`RawSurfaceRegistryDeletion` |
| RiftType | `513b6e85` (5) | 레코드 새 칸 6개: `range`·`useAirDamage`·`airDamagePerSecond`·`damagePerSecond`·`directions`·`frameCount` | ③ | 타입 로더(`buildTypeRecords`) 연결 때 파서와 함께 **4-A**(기존 `defaultFrame` 행과 같은 시점) |
| RiftTypeTable | `513b6e85` (5) | 위 칸을 채우는 `range`/`hpPerSec`/`useairdamage`/`airdamage` 속성 파서·`ReadRawArcherTypeData` 호출 | ③ | 위와 같음 **4-A** |
| SidPool | `541624a9` (2) | `PredictableCursorValue`(9cb0547f 에서 ① 판정)·`friend` 선언 두 줄 | ① | C++ 접근 제어. 소진 진단/다리 회수는 기존 ③(4-D) 유지 |
| SquidDestroyLifecycle | `7ba0130b` (3) | PATCH(Native) 전용 분기 | ③ | **E213 판정 유지**(4-D). 새 변경 없음 |
| SquidDisplay | `2c6ab7cc` (2) | `BindView`/`CurrentView`(투영·clip 마다 현재 Client 카메라 재조회)·메타/일반 Sprite VT(0x50c9f0/0x50cc08)의 `MetaRefresh` 분배 | ① + ③ | 메타·일반 Sprite 는 ①(메타 섬/일반 전투 웹 제외). `BindView` 는 웹 `setView` 가 값 복사라 실제 카메라와 묶을 때 getter 가 필요 → **5-B** |
| SquidFactory | `be567d46` (1) | `BindUnpop`·`BindRuntime`(`SquidFactoryRuntimeBindings`: 현재 World 의 Unpop·Form 분리·내용물 TYPE·전송·pin) | ③ | **4-D**. 독립 공장 불변 |
| SquidFormDispatch | `417f22d2` (2) | `HasPatchDiagnostics` 일 때 현재 VT 로 자산/Form 가상 +10 선택·`selectedRaw`·`SetAssets` 의 최초 부모 보존 | ③ | **4-D**. typed/CD 분배 불변 |
| SquidFrame | `be567d46` (1) | `BindCurrentTypes`(Storage 가 고정한 현재 TYPE 표 대여) | ③ | **4-D**. 웹은 생성 때 받은 `types` 를 그대로 읽으므로 같은 표를 공유하는 한 결과 동일 |
| SquidProcess | `417f22d2` (3) | 기본 장치(`BindDefaultLifecycle`/`FormFactory`/`FormTransmission`)·World 없는 Host 폴백·`AssertionDevice`·Native `Kill` 진단 복귀·`ReceiveSharedRegular`·`WriteFormPacket`·`Shared()` | ③ + ① | 02b00b12 판정 유지. Native 장치는 **4-D**. 공유 Regular 수신/기록은 LAN 동기화(① 유지)이나 **4-C** 패킷 writer(RawFormPacket) 이식 때 단독 도달을 다시 확인 |
| SquidUnpop | `2c6ab7cc` (2) | `SupportsDisplay` 에 메타/일반 Sprite VT 두 개 추가 | ① | 메타 섬·일반 전투(웹 제외) |
| Xlat | `cd63f421` (1) | `TranslateView`(string_view 반환) | ① | C++ 메뉴 포인터 정체성 보존용. JS 문자열에는 해당 없음(웹 `RawXlat` 은 E242) |

괄호 안 숫자는 기준선 뒤 해당 단위 파일을 바꾼 커밋 수다. 합계: ② 재반영 2(ContentFormHost·RawStateSidLists; ContentFormHost 의 실제 World 장치 연결만 ③ 4-D 로 남는다)·① 5·③ 12·①+③ 혼합 3(GumpFormHost·SquidDisplay·SquidProcess) — 단위 22개가 모두 판정됐다.

## 2. ② 재반영 2건

### 2.1 ContentFormHost 의 현재 가상 +50 부착 (`www/core/content-form-host.js`)

cpppj `ContentFormHost::AttachCurrent`(3f766a97)를 `#attachCurrent(form, parent, flags)` 로 옮겼다.

- 장치(`bindReattach`)가 있으면 호출 순간에 읽어 그 장치로 `(form, parent, flags)` 를 그대로 호출한다. 호출 뒤 기존 정렬 부착은 실행하지 않는다.
- 장치가 없으면 현재 VT DWORD 가 Content(6)일 때만 기존 정렬 `attach` 를 쓰고, 다른 미설치 가상 표는 앞선 생성/필드 쓰기 뒤 `'ContentForm 현재 가상+50 부착 장치 미연결'` 오류로 거부한다(TYPE BYTE 를 VT 의 대용으로 쓰지 않는다).
- 사용처: `damage()`(양수 피해의 Unpop 뒤 재부착, 기존 동작과 같음)와 `bindCreate()`(contained 생성기의 마지막 부착). 후자는 이전에 항상 `attach` 였으므로 이것이 바뀐 점이다. 표면 통지 콜백이 장치를 설치하거나 VT 를 바꾸는 장면이 결과를 가른다.

### 2.2 `removeStateSidList` (`www/core/raw-state-sid-lists.js`)

cpppj `RemoveStateSidList`(c9140743, PATCH 0040ea00)를 그대로 옮겼다: 현재 signed count 아래의 DWORD 를 순서대로 읽어 일치하면 세고 아니면 `entries[index - removed]` 로 압축한다(일치가 여럿이면 모두 제거). 선검사·꼬리 지우기가 없고, 음수 count 는 읽기/쓰기 없이 지나가며, count 가 물리 배열 밖을 가리키면 그 칸에서 `RangeError` 로 멈춘다(앞선 압축은 되돌리지 않고 count 는 마지막에만 감산·DWORD 감김).

소비자(`RawDudeBehavior`·`RawPriestPreDestroy`)는 3-E 의 다음 항목(판정-전투/건설·운반·Dude, 판정-Player·사제·신전)에서 판정하며, 이 함수를 쓰도록 맞춘다.

## 3. 검사와 최초 실패/교정

- 신규 `tests/judge-common-layer.test.mjs` 7건(Content 부착 3·`removeStateSidList` 4): cpppj 에 두 변경 전용 시험이 없어 C++ 본문의 호출 순서·감김·범위 오류를 손으로 옮겼다. 모두 첫 실행에 통과했다.
- 변형 시험(상수 한 개씩 바꿔 시험이 실패하는지 확인, 모듈은 항상 원본으로 복원): `removeStateSidList` 10개 중 9 탐지, `ContentFormHost` 6개 중 6 탐지. 처음 살아남은 변형 3개 가운데 둘은 입력 부족이었다 — 부호 있는 수(-1)로 저장된 항목(DWORD 비교)과 부착 flags 가 0x10 이 아닌 값(0x90)을 시험에 추가해 닫았다. 남은 1개(`list.count - removed` 의 `>>> 0` 제거)는 count 가 uint32 범위 안이면 `removed ≤ count` 라 음수가 되지 않는 **동등 변형**이다(범위 밖 입력 -1 만 다르다).
- 단위 검사: `node --test tests/judge-common-layer.test.mjs tests/form-hosts-contract.test.mjs tests/form-hosts.test.mjs tests/region-sid-lists.test.mjs` 통과.
- **Node 버전과 NaN 시험 2건 교정:** 이 PC 의 기본 Node 가 v24.21.0 에서 v26.13.0 으로 올라 전체 검사에서 `carrier-fall-start`·`construction-data`(원본 10.78 NaN 비트 대조)가 실패했다. 클린 `HEAD` 의 별도 worktree 에서도 같아 이번 변경과 무관함을 확인했다. 최소 재현(`getFloat64` 로 만든 신호 NaN `0x7ff00000_00000001` 을 객체 필드에 담았다가 `setFloat64` 로 읽기)에서 약 1,000~3,000회 반복 뒤에야 quiet 비트가 켜졌다 — Node v26 의 최적화 JIT(TurboFan·Maglev) 동작이고, 두 NaN 의 덧셈에서 남는 페이로드 순서도 같은 이유로 바뀐다. 처음에는 NaN 비교를 quiet 비트 무시로 느슨하게 했으나 페이로드 순서 차이(`low 0` ↔ `low 1`)가 이어서 드러나 접고, 두 시험 파일이 `tests/nan-exact.mjs`(`v8.setFlagsFromString('--no-turbofan'/'--no-maglev')`)를 가장 먼저 가져와 그 프로세스에서만 최적화 JIT 를 끄는 방식으로 바꿨다(NaN 대조는 그대로 엄격). 사용자 지시(Node 22·24·26 모두 지원)에 따라 Node v26.13.0 과 v22.23.3(`C:\Program Files\nodejs22\node.exe`)에서 전체 검사 **2366/2366** 을 확인했다(각각 약 358초·421초). Node v24 는 이 PC 에 없어 확인하지 못했다. `webpj/package.json` 에 `engines: node >=22` 를 적었다.

## 4. 대응표·문서 반영

- `PORT_MAP.tsv` 22행의 base 를 위 표의 커밋으로 올리고 메모에 판정을 덧붙였다. `node webpj/tools/port-map.mjs` 오류 0, 22단위의 `cpppj 변경` 보고가 사라진다.
- LEFT_JOBS.webpj.md 3.6절 표에 ③ 묶음 4행(4-D Native 대여/가상 분배 묶음·4-B Fort 적재 전역·4-A 설정 버퍼와 타입 레코드 칸·5-B 표시 카메라 getter)을 더하고 `RawGameAuxState`·`RiftTypeTable` 행을 갱신했다. [대응표 문서](webpj-port-map-20261106.md) 9절에 이 묶음을 가리키는 줄을 두었다.

## 5. 남은 경계와 다음 시작점

- 이 문서의 ③ 은 기능이 아니라 **연결 시점**이다. 4-D World 수명 때 `SquidDestroyLifecycle`·`RawSquidDestroy`·`SquidFactory`·`SquidFormDispatch`·`SquidProcess`·`DependFormHost`·`GumpFormHost`·`ContentFormLifetime` 의 Native 분기를 cpppj `DestroyLifecycleTests` 등으로 한 묶음으로 대조한다.
- 다음은 3-E 두 번째 항목 **판정-Player·사제·신전**(12단위)이다. `RawPriestPreDestroy` 는 이 단위의 `removeStateSidList` 를 소비한다.
