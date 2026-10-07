# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 작업 계획 및 인수인계

> 최종 갱신: 2026-10-07 (호스트 `vm-debian-codex`, Debian 13 / .NET SDK 10.0.401). **사용자 요청으로 병행 개발 착수, 1~2단계의 자산 타입·자료 판독 범위 완료.** 원본 x86 14,615개 입력과 정적 C++ 타입 116개 대조. 최종 빌드·테스트 결과는 바로 아래 완료 절 참조. 게임·클론 창 실행 없음.
> 이 문서는 [LEFT_JOBS.md](LEFT_JOBS.md) 맨 끝 "인수인계 문서 임시 분할" 지침에 따라 **dotnetpj 의 진행 상황과 인수인계만** 담는다. cpppj·디컴파일 기록은 LEFT_JOBS.md 에 적는다.
> 2026-10-05 이전의 dotnetpj 작업 이력(구현 범위·검증 기록)은 LEFT_JOBS.md 의 해당 날짜 절에 그대로 남아 있다. 여기로 옮기지 않았다.

---

## 2026-10-07 ✅ 완료: x86 기대값 연결과 dotnetpj 자료 계층 정정

- [x] **착수/분리:** 사용자 요청 "현재 PC의 디컴파일 상황 파악 후 dotnetpj 진행, cpppj는 다른 AI 작업"에 따라 이미 확정된 범위를 병행 개발했다. AGENTS.md와 두 인수인계 문서를 확인했다. `cpppj/`·AGENTS.md·LEFT_JOBS.md·원본 파일은 수정하지 않는다. dotnetpj 기록은 분할 지침대로 이 문서에 남긴다.
- [x] **이 PC:** `vm-debian-codex`, Debian 13, .NET 10.0.401. 기본 10.78 디컴파일은 4,506개 함수 머리말이다. CD/별도 10.37 각 3,711, 10.62 3,729, 10.82 V10 10,130/업데이터 2,618, 10.62 패치 148. 정밀 `extracted/refined/`와 V12 디컴파일은 없다. 원본 V12 파일 자체는 있다. 최신 Windows 전체 복원 기록과 이 PC 산출물을 구분한다([상세](docs/dotnet-reconstruction-20261007.md)). 추가 디컴파일·원본 실행 없이 기존 C++ 소스/TSV를 이용했다.
- [x] **기준선:** Release 빌드 성공·기존 CA2014 1개, 단위 테스트 **803개 통과**(Assets 262 + Core 541). CA2014는 테스트 stackalloc을 루프 밖으로 옮겨 해소했다.
- [x] **원본 기대값:** `.csproj`의 링크와 `tests/Shared/X86Fixture.cs`로 같은 C++ TSV를 읽는다. Draw 10,005·Random 1,056·Decode 752·FrameFind 1,536·Subst 495·Get 77·options 16·원시 Config 270·Section 408 = **14,615개 입력 전부 일치**. 최초 불일치는 Draw 4·FrameFind 42·Subst 285·Get 9였다. 나머지 묶음은 이번에 처음 연결했다. 원본 exe/Python/Ghidra 없는 CI에서도 TSV 검사가 실행된다.
- [x] **자료 계층:** 미등록 키 `{KEY}`/미션 `{@MISSION}`, 환경 허용·버퍼 없는 객체·LF/CR/NUL, 전역 설정 우선순위, 32비트 프레임 마스크/부호 확장, 타입 후처리·긴 이름 해시·zorder·float 비용·level, 버전 0 상태 바이트·소유자 정규화 함수·부호 있는 섹션 길이·첫 TypeNames 번호·목록 플래그·타입 0 덱 제외를 구현했다. 기존 실제 소유권 결정과 설정 저장 경로는 보존했다.
- [x] **정적 C++ 대조:** 순수 자료 소스만 임시 콘솔로 빌드해 클론 자산 타입 **116개의 두 플래그·깊이·비용·level·목록·해시 전부 일치**를 확인했다. x86 직접 기대값과 구분하여 `Fixtures/type-metadata-1078.tsv`에 출처 SHA를 남겼다. 재생성: `python3 dotnetpj/tools/export_cpp_reference.py`(g++ 필요, 원본/클론 게임 실행 없음).
- [x] **패턴/SHP:** CanonDecoder의 다리 26개·영역 68개 전체 출력·기존 `BridgePiece.Cells`를 대조했다. JSON 기존 64모양 유지·변형/라벨/추가 4모양 보존. Squid 추가 헤더 36바이트를 읽고 **3,783개 참조** 전수 확인. VFX 내부 값과 표시 폭 3,711·높이 3,697·hotspot X 3,388·Y 3,434개가 달라 뷰어/클릭 판정 교체는 5-2에 남긴다. 기존 **465개 요새 전수 판독**도 통과했다.
- [x] **최종 검증:** Release 빌드 **경고 0·오류 0**. 전체 테스트 **825개 통과·실패/스킵 0**(Assets 281 + Core 544). 원본 x86 **14,615개 입력 일치**, 정적 타입 116개 일치, 자료 재생성 도구 실행 성공, `git diff --check` 통과. 보호된 원본 7개 디렉터리의 **2,782개 파일 SHA·목록 동일**(수정/삭제/새 파일 0). `cpppj/`·AGENTS.md·LEFT_JOBS.md 변경 없음. 게임·클론 창 검사는 하지 않았다.
- [ ] **다음:** 3-1~3-5의 객체 단위 연결/이웃/열린 방향/수명/붕괴 방문 목록(월드와 분리한 라이브러리) → 4단계 Graph/점유/SID → 5-1·5-2 표시 순서/경계. 전체 188타입 표·SID 실제 월드 연결·소유자 적용은 후속이다. cpppj가 미복원한 8절 항목과 영어 글꼴 정책은 이번에 구현하지 않았다.

---

## 0. 이 문서가 다루는 것

cpppj(원본 10.78 을 디컴파일해 C++ 로 복원하는 프로젝트)와 디컴파일 분석이 2026-10-05~10-07 사이에 **원본 기계어와 대조해 확정한 사실** 가운데, dotnetpj 가 아직 다르게 구현했거나 구현하지 않은 것을 정리하고 작업 순서를 정했다.

- 대조한 자료: `docs/exe/cpp-*.md` 27개 가운데 25개(`cpp-rawgraph`·`cpp-graphrecovery` 두 개는 LEFT_JOBS.md 의 요약만 읽었다), `cpppj/src/o/` 의 `Bridge.cpp`·`CanonDecoder.cpp`·`RiftTypeTable.cpp`·`VFXDraw.cpp` 일부, `cpppj/tests/fixtures/*-x86.tsv` 의 행 구성, `docs/exe/bridge-pieces.md`·`main-loop.md`·`docs/core-rules.md`, 그리고 `dotnetpj/src/` 의 대응 코드.
- **모든 C# 파일을 전수 대조한 것은 아니다.** 설정·타입·요새·SHP·다리·점유·그리기 순서·글꼴·커서·메뉴 진입부를 읽었다. 전투·AI·희생·생산 흐름은 cpppj 에 대응 복원이 아직 없어 비교하지 않았다.
- 아래 "C# 현재" 칸은 2026-10-07 에 코드를 직접 읽어 확인한 내용이다. "원본" 칸의 근거 수준은 다음 표기를 쓴다.

| 표기 | 뜻 |
|---|---|
| **[x86]** | 원본 실행 파일의 기계어를 격리 실행한 기대값이 `cpppj/tests/fixtures/` 에 있다 |
| **[정적]** | 두 판본 역어셈블·실제 자료 전수 비교로 확인했다. x86 실행 기대값은 없다 |
| **[어댑터]** | cpppj 도 관찰 기반으로 임시 구현한 부분이다. 원본 확정이 아니다 |

### 먼저 알아 둘 조건

1. **기준 판본은 10.78(`originals/Netstorm.exe`)이다.** CD 10.72(실행 파일 표시 10.37)·10.62·10.82(V10, V12)는 비교 자료다. 판본마다 다른 값은 패치 10.78 값을 쓴다 — 다리 가중치 합 287, 타입 188개, Sun Cannon `hpPerSec` 16, `manabolt` 8프레임, 선택 표시 위쪽 여유 15픽셀, 점유 충돌 시 조기 반환.
2. **AGENTS.md 는 "dotnetpj — cpppj 완성 후 이를 분석하여 개발"로 적혀 있다.** 한편 LEFT_JOBS.md 끝의 분할 지침은 병행 작업을 전제로 한다. 그래서 이 문서는 "cpppj 에서 이미 확정돼 지금 반영해도 뒤집힐 가능성이 낮은 것"(1~6단계)과 "cpppj 가 더 진행돼야 하는 것"(8절)을 나눴다. **2026-10-07 사용자 요청으로 병행 착수했다.** 현재는 확정된 범위부터 진행한다(10절).
3. **`DESKTOP-HJOW` 에서는 원본/복사본 게임 실행과 모든 창 검사가 금지다**(LEFT_JOBS.md 머리말의 사용자 지시). 이 PC 에서는 `dotnet build`·`dotnet test`(헤드리스)만 돌리고, `tools/clone_*_smoke.ps1` 처럼 클론 창을 띄우는 검사는 다른 PC 에 넘긴다. 다른 PC 에서도 원본 게임 실행은 AGENTS.md 의 확인 규칙을 따른다.
4. 2026-10-07 착수 기준선은 단위 테스트 **803개 통과**(Assets 262 + Core 541)였다. 최신 결과는 이 문서 맨 위 완료 절에 있다.
5. 정밀 디컴파일(`extracted/refined/`)은 Git 제외라 이 PC 에 없다. 함수 본문을 직접 읽으려면 `tools/ghidra/refine_all.ps1`(약 20분)을 먼저 돌린다. 기본 디컴파일 `extracted/decomp/Netstorm.c` 는 이 PC 에 있다.

---

## 1. 작업 순서 한눈에 보기

| 단계 | 내용 | 창 필요 | 선행 |
|---|---|---|---|
| **1** | cpppj 의 x86 기대값을 C# 테스트에서 읽는다 (검증 기반) | 없음 | — |
| **2** | 자료 계층 정정 — 설정·타입 표·요새·SHP 헤더 | 없음 | 1 |
| **3** | 다리·표면 계산을 기대값에 맞춘다 | 없음 | 1, 2 |
| **4** | 표면 그래프·SID 순서·점유 비트를 Core 에 추가한다 (라이브러리 수준) | 없음 | 3 |
| **5** | 화면 표시 — 그리기 순서, 표시 경계, 원본 글꼴, 커서, 타이틀 그림 | **다른 PC** | 2 |
| **6** | 미션 시작 값·통계 정합과 회귀 검사 보강 | 없음 | 2 |
| **8절** | cpppj 진행을 기다리는 항목 (지금 하지 않는다) | — | cpppj |

1~4·6단계는 헤드리스 단위 테스트만으로 끝낼 수 있어 이 PC 에서도 진행할 수 있다. 5단계는 코드 작성은 가능하지만 화면 확인은 다른 PC 가 필요하다.

---

## 2. 1단계 — cpppj 의 x86 기대값을 C# 테스트로 연결

**2026-10-07 완료:** 아래 비교표의 "예상"·"C# 현재"는 착수 전 상태다. 완료 결과와 유지한 차이는 맨 위 작업 절 및 [상세 기록](docs/dotnet-reconstruction-20261007.md)에 있다.

**이유:** cpppj 는 원본 기계어의 결과를 TSV 로 저장해 두었다(누적 61,122개 입력). 이 파일은 실행 파일·Python·Ghidra 없이 읽을 수 있다. C# 이 같은 파일을 읽으면 "원본과 같다"를 팬게임이나 영상 추정이 아니라 원본 기계어로 판정할 수 있다. 이후 단계의 수정이 맞았는지도 이 파일로 확인한다.

- [x] **1-1. 공용 로더.** `cpppj/tests/fixtures/*.tsv` 를 복사하지 말고 테스트 프로젝트(`.csproj`)에서 링크로 포함한다(사본이 두 벌이 되면 어긋난다). 첫 줄 `#` 은 주석이고 열은 탭으로 나뉜다. 행 종류별 열의 뜻은 문서에 따로 없으므로 **같은 파일을 읽는 C++ 테스트**(`cpppj/tests/<이름>Tests.cpp`)와 생성 도구(`tools/decomp_<이름>_oracle.py`)를 보고 맞춘다.
- [x] **1-2. 이미 옮긴 C# 코드의 기준선.** 코드를 고치기 전에 아래를 먼저 돌려 통과·실패 수를 기록한다. 실패는 2~3단계의 작업 목록이 된다.

| 기대값 파일 (행 종류: 개수) | 대조할 C# | 예상 |
|---|---|---|
| `bridge-x86.tsv` `Random`: 1,056 | `Simulation/NetstormRandom` | 식이 같아 통과 예상 |
| `bridge-x86.tsv` `Draw`: 10,005 | `Bridges/BridgePatternCatalog.Draw` | 패치 열만 쓴다. 표·가중치(합 287)·`<=` 비교는 같다. **음수 입력에서 C# 은 예외를 던지고 원본은 부호 있는 나머지로 첫 조각을 고른다** |
| `bridge-x86.tsv` `Decode`: 752 | `Bridges/BridgePiece.Cells`, `BridgeDirections.Rotate` | 다리 26모양 × 4회전. 영역 68모양 행은 `TerritoryPatterns.json` 쪽 |
| `original-x86.tsv` `FrameFind*`: 1,536 | `Assets/TypeFrameTable.Find`·`FindExactFlags` | 플래그 마스크가 부호 있는 8비트 확장(`movsx`)인 입력 포함 |
| `config-x86.tsv` `Subst`: 495, `Get`: 77 | `Assets/ConfigStore.Expand`·`Get` | **실패 예상** (2-1, 2-2) |
| `options-x86.tsv`: 16 | `Assets/ConfigFile.Encode` | 서명 없는 예외 입력에서 실패 예상 (2-4) |

- [x] **1-3. 완료 기준:** 위 여섯 묶음이 테스트로 돌고, 실패한 입력의 수와 원인이 이 문서에 기록된다. 원본 파일이 없는 환경(CI)에서도 돈다.

---

## 3. 2단계 — 자료 계층 정정

### 2-1. 설정 치환: 찾지 못한 키 **[x86]**

| | 내용 |
|---|---|
| 원본 | 찾지 못한 키는 `{Not Found:키}` 가 아니라 **`{키}`**(대문자)로 남는다. `{@absent.name}` 은 `{@ABSENT}`('.' 에서 잘린 키). 최상위 조회는 빈 문자열. `00440760` 이 `Not Found:` 를 쓴 자리에 키를 덮어쓰기 때문이다 |
| C# 현재 | [ConfigStore.cs](dotnetpj/src/Netstorm.Assets/ConfigStore.cs) `Lookup` 이 `{Not Found:키}` 를 붙인다. `Get` 은 null 을 돌려준다 |
| 근거 | [cpp-config-reconstruction.md](docs/exe/cpp-config-reconstruction.md) "이번에 확인한 원본 동작" 1번 |

- [x] `Lookup` 을 원본대로 고친다. `ConfigStore.NotFoundPrefix` 를 쓰는 곳을 정리한다.
- [x] 이 문자열을 기대하는 테스트를 고친다: `TextResourceTests.cs` 91·99·115행, `GameResourcesTests.cs` 70행, `TutorialDialogScriptTests.cs` 290행(부재 검사).
- [x] 고치기 전에 **이 표시가 실제 화면에 나오는 경로가 있는지** 확인한다(브리핑·안내 창 본문). 있으면 화면 문구가 바뀐다.

### 2-2. 설정 층의 조회 순서 **[정적]**

| | 내용 |
|---|---|
| 원본 | 언어 용어표가 전역 설정보다 **먼저** 등록되고, 조회는 마지막에 등록한 것부터다. 그래서 **전역 설정(options·setup)이 용어표보다 우선**한다. 그 위에 `local`·`mission` 임시 객체가 쌓인다 |
| C# 현재 | [GameResources.cs](dotnetpj/src/Netstorm.Assets/GameResources.cs) 생성자가 기본값 → options+setup → 영어 용어표 → 번역 용어표 → 언어 덮어쓰기 순으로 쌓는다. 조회는 위에서부터라 **용어표가 전역 설정보다 우선**한다 |
| 근거 | 같은 문서 3번(정적 초기화 표 `005280ac`·`005280b0`) |

- [x] 같은 키가 용어표와 전역 설정에 함께 있는 실제 사례를 먼저 찾는다(`config.<언어>` 와 `setup.cfg` 의 키 교집합). 없으면 결과가 같으므로 순서만 맞추고 끝낸다.
- [x] 순서를 바꾸면 `currentLanguage` 를 지키려고 넣은 "언어 덮어쓰기" 층이 필요 없어질 수 있다. 한국어처럼 원본에 없는 언어를 고르는 경로가 깨지지 않는지 `GameResourcesTests` 로 확인한다.
- [x] 영어 용어표를 바닥에 깔아 누락 키를 보충하는 것은 **C# 의 추가 기능**이다(원본에 없다). 유지하되 주석에 명시한다.

### 2-3. 인코딩된 설정 파일의 첫 줄 **[x86]**

- 원본은 복호화한 내용을 서명 `mQdsT` 째로 버퍼에 붙인다. `options.cfg` 는 `mQdsTInstallDir = "…"` 로 시작하므로 **`InstallDir` 키가 조회되지 않는다**.
- C# 의 `ConfigFile.Decode` 는 서명을 떼므로 `InstallDir` 가 조회된다.
- [x] 클론이 `InstallDir` 를 읽는 곳이 있는지 확인한다. 없으면 주석으로 차이만 남긴다. 있으면 원본처럼 조회되지 않게 한다.

### 2-4. 설정 값 쓰기와 저장 형식 **[x86]** — 낮은 우선순위

- 원본 `Set` 은 버퍼 전체에서 같은 키의 줄을 모두 지우고 `[END]` 섹션 끝에 `키 = "값"` 을 붙인다. 저장은 `[options.cfg]` 본문 + `[END]` 본문이며, 파일 이름 섹션이 없으면 서명을 붙이지 않는다.
- C# 의 `ConfigFile.Set` 은 처음 나온 줄을 제자리에서 바꾸고 `Encode` 는 항상 서명을 붙인다.
- 지금 이 함수를 쓰는 곳은 `analyzeManager/SessionStore.cs`(원본 복사본의 `options.cfg` 를 창 모드로 바꾸는 용도)뿐이다. 원본은 처음 나온 값을 읽으므로 현재 방식으로도 동작한다. **dotnetpj 는 `options.cfg` 를 쓰지 않는다**(9절).
- [x] 1-2 의 `options-x86.tsv` 결과를 보고, analyzeManager 에 영향이 없으면 차이를 주석으로만 남긴다.

### 2-5. 타입 표: 플래그와 번호 체계 **[정적]**

[TypeCatalog.cs](dotnetpj/src/Netstorm.Assets/TypeCatalog.cs) 의 `ComputeFlags` 는 플래그 단어 표와 파생 규칙 2개(container, saveQA)만 적용한다. cpppj `o/RiftTypeTable.cpp` 의 `ApplyDefinition`·`PostProcess` 가 원본 `0049c3b0`·`0049b0d0` 을 옮긴 기준이다([cpp-fort-reconstruction.md](docs/exe/cpp-fort-reconstruction.md)).

- [x] **`group` 속성이 플래그 2 를 켠다.** battery → `0x800`, archer·cannon → `0x8000000`, blocker → `0x4000000`. C# 은 계산하지 않고 `GetString("group")` 문자열 비교를 여러 곳에서 쓴다.
- [x] **후처리 파생 규칙을 모두 옮긴다.**
  - `emplacement` → `dropblocking`(`0x10`). **동작에 영향이 있다:** `Bridges/BridgeAnchors` 는 typeflags 에 `dropBlocking` 단어가 있는 타입만 다리 부착 금지 칸으로 모은다. 원본은 모든 emplacement(포대·궁수 등)의 발자국 칸도 금지다.
  - 3×3 emplacement(vortex·factory 제외) → 플래그 1 에 `6`(섬·테두리 모두 놓기 가능)
  - `mayDropOnRim` 이 없으면 `mayDropOnIsle` 을 켠다. 내장·빈 타입도 그래서 플래그 1 이 2 다
  - vortex·factory·walker·balloon → 목록 플래그(walker·balloon 0x20, 그 밖 0x10), bomb → 내용물 목록 플래그 1
  - `maxHitPoints` 가 있으면 플래그 1 `0x10`, 사용량(`minUsage`·`maxUsage` 계열)이 0 이 아니면 `0x4000`
  - 그 밖의 비트(`0x10000000`, `0x4000000`, `0x84000`, `0x8000`)는 `PostProcess` 본문을 그대로 따른다
- [x] 플래그 단어 `focus`·`predictable` 을 표에 넣는다(비트는 없지만 `focus` 는 종류 단어다).
- [x] **타입 이름 해시.** `TypeCatalog.NameHash` 는 이름을 20글자로 자른다. 원본은 이름이 20글자 이상이고 `description` 이 있으면 **"이름 앞 20글자 + 설명"** 이 이름으로 읽힌다. `fakeThreeByThreeSurface` 의 실제 해시는 `0534a54b`(= `fakeThreeByThreeSurf` + `fake3x3Surface`)이고 C# 계산값과 다르다. 지금 보유한 `.fort` 에는 이 타입의 오브젝트가 없어 드러나지 않을 뿐이다.
- [ ] **번호 체계.** C# 은 `.type` 타입(70 + 로딩 순서)만 안다. 원본은 0~69 가 내장 타입(1·2·3·4·6 에 이름, 10~62 가 프로세스 타입 53개)이고 186·187 은 빈 번호다. `.fort` 의 `TypeNames` 변환표는 **현재 타입마다 해시가 같은 첫 파일 번호**에 자기 번호를 적는다(C# 은 파일 번호마다 사전 조회). 해시 충돌·이름 없는 타입(해시 0)에서만 결과가 다르다. 내장 타입 표를 추가할지는 4단계(SID·생성자)의 필요에 따라 정한다.
- [x] `zorder` 속성을 읽는다(5-1 에서 쓴다). 숫자 또는 깊이 이름 23개 + 선택적 `+`/`-` 오프셋이다. 이름 표와 값은 `RiftTypeTable.cpp` 의 `kZOrders` 에 있다(`zoISLAND` 0, `zoBRIDGE` 0, `zoEMPLACEMENTS` −20, `zoFLYERS` −30, `zoBATTLE` 10 등). 이름 비교는 대소문자를 구분하는 부분 문자열 첫 일치다.
- [x] `cost` 는 float 로 읽는다(누락 0, 중복은 뒤의 값). `level` 은 읽은 값 − 1 로 저장된다.
- [x] **완료 기준 달성:** 순수 C++ RiftTypeTable 콘솔에서 내보낸 자산 타입 116개의 플래그 1·2와 부가 메타데이터가 같다. Windows 전용 cpppj 전체 빌드 대신 기존 순수 자료 소스만 임시 빌드했다. 기존 `.fort` 465개 전수 판독 통과.

### 2-6. 요새 파일 읽기 **[정적]**

[FortFile.cs](dotnetpj/src/Netstorm.Assets/FortFile.cs) 의 레코드 구조는 맞다(cpppj 가 같은 Python 판독기와 313,712개 오브젝트를 전수 비교했다). 남은 차이:

- [x] **소유자 정규화.** 원본은 저장된 소유자 바이트가 0 이거나 8 보다 크면 1 로 바꾼다. geyser·buried(`0x10002000`)와 island 는 소유자 0 이다. C# 은 저장값을 그대로 쓴다(`Owner ?? 0`). 값을 실제로 쓰는지는 실행 상태에 달려 있고 아니면 영역 소유자를 쓴다 — cpppj 도 "파일 값 보존, 결정은 하지 않음"이므로 **정규화 함수만 추가**하고 적용 지점은 6-1 에서 정한다.
- [x] **버전 0.** 원본은 버전 0 에서 모든 타입 뒤에 상태 1바이트를 읽는다(`or edx, 0x4000000` 뒤 분기가 항상 참). C# 은 버전 0 에서 그 바이트를 읽지 않는다. 보유 파일은 버전 2(2,523개 섹션)와 1(2개)뿐이라 드러나지 않는다.
- [x] 섹션 길이는 부호 있는 16비트다. `Deck` 에서 변환한 타입이 0 인 항목은 덱에 넣지 않는다. 내용물의 목록 플래그 바이트는 "항목 타입의 `+0x100` → 그릇 타입의 `+0xfc` → 둘 다 0 이면 파일 1바이트" 순서다(C# 의 `Technology` 읽기는 결과가 같다. 규칙으로 일반화할지는 선택).

### 2-7. SHP 의 Squid 전용 헤더 **[x86]**

| | 내용 |
|---|---|
| 원본 | 프레임 표가 가리키는 VFX 레코드 **앞에 36바이트의 추가 헤더**가 있다. 처음 두 float 는 해시 단계 계산용 칸 크기, VFX 직전 12바이트의 부호 있는 16비트 네 개는 **표시 폭·높이·hotspot X·Y** 다. 화면 경계 계산은 이 값을 쓴다 |
| C# 현재 | [ShapeDatabase.cs](dotnetpj/src/Netstorm.Assets/ShapeDatabase.cs) 는 이 36바이트를 읽지 않는다. VFX 레코드 **안**의 `Bounds0/1`·`Origin0/1`(부호 없는 16비트, 주석에 "추정")을 `FortMapViewer.Combat.cs:71`·`Placement.cs:271` 에서 쓴다 |
| 근거 | [cpp-display-reconstruction.md](docs/exe/cpp-display-reconstruction.md), `cpppj/src/client/VFXDraw.cpp` `SquidMetrics`. 실제 자산 6,950개 헤더(패치 3,783)를 읽었다 |

- [x] `ShapeDatabase` 에 `SquidMetrics(블록, 프레임)` 을 추가한다. 순수 VFX(글꼴 등)에는 이 헤더가 없으므로 있다고 가정하지 않는다.
- [x] VFX 안의 `Bounds`/`Origin` 과 새 값이 실제 자산에서 어떻게 다른지 표로 뽑아 문서에 남긴다. 같으면 이름·부호만 정리하고, 다르면 5-2 에서 교체한다.

---

## 4. 3단계 — 다리·표면 계산을 기대값에 맞춘다

[BridgeGrid.cs](dotnetpj/src/Netstorm.Core/Bridges/BridgeGrid.cs) 는 2026-09-30 에 원본 `004227e0`·`004218b0`·`004217f0` 을 칸 단위로 옮겼고 근사한 부분을 주석에 적어 두었다. cpppj 는 같은 함수들을 기계어로 대조했다([cpp-bridge-reconstruction.md](docs/exe/cpp-bridge-reconstruction.md), [cpp-surface-reconstruction.md](docs/exe/cpp-surface-reconstruction.md)). 기준 구현은 `cpppj/src/o/Bridge.cpp`·`SquidFinder.cpp` 다.

- [ ] **3-1. 연결 판정 `00441e40` [x86: `surface-x86.tsv` `Connect` 3,016].** 짝수 방향은 프레임의 측면 글자, **홀수(대각) 방향은 변형 글자**를 본다. 타입 플래그 2 의 emplacement(`0x40000`)는 글자를 `P` 로 만든다. 다리·폭탄(`0x104`)과 섬·건물군(`0x1044202`)이 만나면 섬·건물 쪽을 `A`(사방)로 본다. 이 보정의 순서와 if/else 우선순위도 원본대로다. C# 은 "섬 칸은 늘 이어진다"로 근사한다(`NeighborsOf`, `SurfacesConnect`).
- [ ] **3-2. 이웃 탐색 [x86: `Neighbor` 125].** 원본은 **표면 객체 단위**다. 같은 객체는 여러 칸에서 발견해도 한 번만 낸다. 후보의 기준점이 자기 사각형 안이면 제외하고, 죽음 비트와 후보 기준점의 spot 안쪽 비트 8 을 본다. 후보 중심 → 자기 중심의 네 방향을 고르며 절댓값이 같으면 세로가 먼저다. 1×1 은 북 → 서 → 자기 → 동 → 남 순서로 C# 의 `ScanOrder` 와 같다. C# 은 **섬 칸마다 이웃 하나**로 세므로 접합 칸이 같은 섬에 두 면으로 닿으면 이웃 수(`< 3`, `< 2` 판정)가 달라진다.
- [ ] **3-3. 열린 방향 `00421770`·`004217f0` [x86: `Open` 5,760, 전체 방향은 패치판만].** 좌표에 `0.9999f`(비트 `3f7ff972`)를 더해 0 쪽으로 자른다. 지도 밖이나 번호 0 은 열린 것이다. B~E 는 두 번째, F~K 는 첫 번째 열린 방향을 만난 즉시 그 방향을 돌려주고, L·M·N·O 는 고정 방향 0·2·4·6, A·P 는 −1 이다. C# `HasOpenSide` 는 참/거짓만 계산한다. 반환 방향을 쓰는 후속(끝 칸 변환 등)을 위해 방향을 돌려주게 바꾼다.
- [ ] **3-4. 수명 감소 `00421c30` [x86: `Life` 480, 첫 외부 효과 전 구간만].** 수명은 상태 단어의 비트 3~6(`0x78`)이다. Battle 모드에서 0 이 되면 **상태 단어를 그대로 두고** 제거를 예약하며, J·K 판자는 제거 플래그 `0x02000000` 을 넘긴다. C# `LowerTimeLeft` 는 결과가 같은 단순화다. 수명 7 초과 입력은 원본 디버그 검사 대상이므로 C# 도 거부한다.
- [ ] **3-5. 붕괴 방문 목록 `004218b0` [x86: `Collect` 309].** 규칙은 C# `Walk` 와 같다(부모만 제외, 목록 용량 100, 목록이 차도 재귀는 계속, 경계에서 같은 번호의 모든 항목 제거). **고리 처리 정책이 다르다:** 원본은 접합 칸 고리에서 끝없이 재귀한다. cpppj 는 고리나 깊이 256 에서 "불완전, 붕괴 없음"으로 중단하고, C# 은 재귀 경로에 있는 칸을 건너뛰고 계속한다. 둘 다 새 코드의 안전 처리다. **cpppj 정책(중단, 수명 변화 없음)으로 맞춘다** — 불완전한 목록으로 수명을 바꾸지 않는 쪽이 원본에 없던 결과를 덜 만든다.
- [x] **3-6. 조각 추첨의 음수 입력.** 1-2 표 참고. `Draw` 가 음수를 원본처럼 처리하게 한다(실제 호출은 0~9999 라 게임 결과에는 영향이 없다).
- [ ] **완료 기준:** 3-1~3-5 의 기대값(합 9,690개 입력)이 C# 테스트로 통과한다. 기존 `BridgeGridTests`·`BridgePieceTests` 와 원본 맵 회귀(`OriginalMaps_StoredBridgesMostlySurviveFiveMinutes`)가 통과하거나, 바뀐 수치의 이유가 기록된다.

> 3-2 는 "섬이 어떤 표면 객체로 놓이는가"를 알아야 월드에 붙일 수 있다. cpppj 도 섬·지면·`noIsland`·받침의 실제 생성·등록을 아직 월드에 연결하지 않았다(8절). 이번 단계에서는 **객체 번호 지도와 spot 지도를 입력으로 받는 계산 함수**까지 만들고 기대값으로 검증한다. `BridgeGrid` 의 칸 단위 근사는 그때까지 유지한다.

---

## 5. 4단계 — 표면 그래프·SID 순서·점유 비트 (Core 에 라이브러리로 추가)

이 단계의 결과물은 **월드에 아직 연결하지 않는 순수 계산 클래스와 테스트**다. 원본의 메모리 배치(50바이트 슬롯, vtable 주소)는 복제하지 않고 의미만 옮긴다.

### 4-1. 표면 그래프 **[x86: `graph-x86.tsv` 2,560]**

`BridgeGrid.SurfaceGraphReaches` 는 붕괴 스캔 때마다 너비 우선으로 "무리 크기 ≥ 5"를 **칸 수로** 다시 센다. 원본 Graph 는 지속되는 표이며 규칙이 다르다([cpp-graph-reconstruction.md](docs/exe/cpp-graph-reconstruction.md), `cpppj/src/o/Graph.cpp`).

- **표면 수는 객체 방문마다 1 증가한다. 여러 칸 발자국도 한 객체다.** (`docs/exe/bridge-pieces.md` 8.1 의 1번도 이렇게 정정돼 있다.)
- 정상 번호 0~250, 무효 254. 레코드는 표면 수(부호 있는 16비트)와 사용 여부다.
- 등록: 이웃 그래프 가운데 표면 수가 가장 큰 것에 붙는다. 같으면 **먼저 탐색한 번호**가 남는다. 다른 연결은 승자로 flood 하고 패배 레코드를 반납한다. 연결이 없으면 새 번호.
- flood 는 LIFO 스택이고 마지막 이웃부터 방문한다. 카운터는 16비트로 감긴다. 같은 객체에 등록을 다시 부르면 수가 또 증가한다(원본 그대로).
- 삭제 준비(`004637b0`, [cpp-graphremove-reconstruction.md](docs/exe/cpp-graphremove-reconstruction.md)): 남은 무리를 새 번호로 나누고 표면 수를 1(특수 타입은 9) 줄인다.
- 주변 영역 무효화(`00462d40`)와 번호 소진 시 전역 재구성(`00463110`)은 [cpp-regiongraph-reconstruction.md](docs/exe/cpp-regiongraph-reconstruction.md)·[cpp-graphrebuild-reconstruction.md](docs/exe/cpp-graphrebuild-reconstruction.md)에 있다.

- [ ] `Netstorm.Core` 에 그래프 표(할당·반납·감소·flood·등록·분할)를 추가하고 `graph-x86.tsv` 의 `Add`·`Flood`·`Remove`·`Free`·`Allocate` 각 512개로 검증한다. 기대값의 표·스택 열은 Adler-32 체크섬이므로 같은 체크섬을 계산해 비교한다.
- [ ] 영역 무효화·분할·재구성은 `rawgraph`·`regiongraph`·`graphremove`·`graphlookup`·`graphrebuild`·`graphrecovery` 기대값이 raw 슬롯 바이트를 입력으로 삼는다. C# 이 raw 배치를 복제하지 않으므로 **이 파일들은 그대로 쓰기 어렵다.** 정수 스냅샷 수준의 `graph-x86.tsv` 까지를 이번 단계의 검증 범위로 한다.

### 4-2. 오브젝트 번호(SID) 할당 순서 **[x86: `sid-x86.tsv` 3,168]**

`BridgeCellState.Sequence` 주석: "원본은 붕괴 스캔이 오브젝트 번호 순으로 칸을 훑는데, 번호 재사용 규칙을 몰라서 만든 순서로 대신한다." **이 규칙은 이제 알려져 있다**([cpp-sid-reconstruction.md](docs/exe/cpp-sid-reconstruction.md), `cpppj/src/o/SidPool.cpp`).

- 10.78 의 범위: 일반 클라이언트 5..14999, 일반 서버 15000..23000, 예측 머리 23001(첫 예측 할당 23002). 0..4 는 일반 할당에서 제외.
- 일반 할당은 free 목록 머리에서 떼는 FIFO 이고 **마지막 free 슬롯은 예약 꼬리로 남긴다.** 반납한 번호는 그 목록의 **꼬리**에 붙는다. 그래서 반납된 번호는 한참 뒤에야 다시 쓰인다.
- 예측 번호는 커서로 증가하며 반납해도 커서가 돌아가지 않는다.
- 이 경로에는 세대 비트가 없다.
- `bridge-pieces.md` 8.1 에 따르면 붕괴 스캔은 번호 15000~23001 을 훑는다. 서버 범위 + 예측 머리와 같다.

- [ ] `Netstorm.Core` 에 번호 할당기(범위·FIFO·예약 꼬리·예측 커서)를 추가한다. `sid-x86.tsv` 는 raw 풀 체크섬을 비교하므로 그대로 쓰기 어렵다. 머리·꼬리·카운터 열만 비교하거나 `cpppj/tests/SidTests.cpp` 의 경계 검사를 C# 으로 옮긴다.
- [ ] **확인 필요(디컴파일):** 싱글 플레이에서 다리 조각이 어느 범위(서버·예측)로 할당되는지. `Construction.cpp` `00442c80` 의 할당 플래그를 본다. 이것이 정해져야 `Sequence` 를 번호로 바꿀 수 있다.
- [ ] 정해지면 `BattleMap.NextId()`(1부터 증가)와 `BridgeCellState.Sequence` 를 이 할당기로 바꾸는 방안을 검토한다. 결정론 검사합(`Checksum`)이 바뀌므로 관련 테스트의 기대값을 함께 갱신한다.

### 4-3. 점유(spot) 비트와 해시 단계 **[x86: `hash-x86.tsv` `Genus` 3,216, `Level` 605]**

`BattleMap` 은 칸마다 "겹친 오브젝트 수"만 세고 비행체·풍선을 예외로 뺀다. 원본은 256×256 바이트 지도에 **타입 플래그 2 의 하위 바이트를 OR** 한다([cpp-hash-reconstruction.md](docs/exe/cpp-hash-reconstruction.md), [cpp-spatial-reconstruction.md](docs/exe/cpp-spatial-reconstruction.md)).

- 비트: `0x2` 섬, `0x4` 다리, `0x8` 발자국 안쪽, `0x10` dropBlocking, `0x20` shotBlocking.
- 안쪽 비트 8 은 건물군(마스크 `0x50444200`)의 발자국에서 **네 변이 아닌 칸**에만 붙는다. 지붕 비트(`0x400000`)가 있으면 정확한 중심 열의 위쪽 절반·중앙은 제외한다.
- 등록 루프의 칸 좌표는 "float 절삭 → 정수에 0.9999 더하기 → 절삭"이고, 발자국 경계 계산은 "float 좌표 + 0.9999 → 절삭"이다. 소수 좌표에서 둘을 한 식으로 합치면 결과가 달라진다.
- 10.78 은 등록 중 `(기존 spot & 새 genus) != 0` 이면 즉시 돌아간다(앞서 쓴 칸은 되돌리지 않는다). **이것은 등록 함수의 동작이지 배치 가능 여부 판정이 아니다.** 배치 판정은 `0049b510` 이다(8절).
- 해시 단계는 발자국이 아니라 **현재 SHP 헤더의 크기**(2-7 의 두 float)로 정한다. 섬·다리는 0, 가로·세로 최댓값이 2 이하면 1, 4 이하면 2, 그보다 크면 3.

- [ ] `EffectiveGenus`(칸별 점유 비트)와 해시 단계 계산을 `Netstorm.Core` 에 추가하고 기대값으로 검증한다.
- [ ] spot 지도 클래스(발자국 OR 등록, AND 해제)를 추가한다. `BattleMap._occupied` 를 바꾸는 것은 8절의 배치 판정이 복원된 뒤에 한다.

### 4-4. 공통 postPop 의 집계 **[x86: `postpop-x86.tsv` 984]**

[cpp-postpop-reconstruction.md](docs/exe/cpp-postpop-reconstruction.md): 오브젝트가 **처음** 월드에 등록될 때 타입 통계 표가 1 증가하고(재등록에서는 다시 늘지 않는다), 비용 집계·공급 목록·작업장 목록이 갱신된다. 공급 목록은 중복을 넣지 않고 작업장 목록은 중복 검사가 없다. 이 비용 집계는 플레이어 SP 차감이 아니다.

- [ ] `docs/core-rules.md` 의 "원본은 다리의 **누적 제작 수**를 보지만 현재 클론은 살아 있는 내 다리 칸 수를 센다"(튜토리얼 1 의 단계 B·C)를 고친다. `PlayerState.Made` 계열에 다리 칸의 누적 수를 넣고 `TutorialStages` 가 그것을 보게 한다. 통계가 "첫 등록 시 1 증가, 감소 없음"이라는 점은 cpppj 가 확정했다. 튜토리얼 1 함수(`004c3a20`)가 실제로 어느 표를 읽는지는 디컴파일로 한 번 더 확인한다.

---

## 6. 5단계 — 화면 표시 (코드는 이 PC 에서, 화면 확인은 다른 PC 에서)

### 5-1. 그리기 순서 **[x86: `renderer-x86.tsv` `Order` 261]**

| | 내용 |
|---|---|
| 원본 | 정렬 비교 `00497900`: **깊이(부호 있는 16비트) 내림차순 → y 오름차순 → x 오름차순**. 깊이는 타입의 `zorder`(2-5)에서 온다 |
| C# 현재 | [FortMapViewer.cs:111](dotnetpj/src/Netstorm.Game/FortMapViewer.cs#L111) 은 `surface` 플래그 여부 → y → x 로 정렬한다 |

- [ ] 2-5 의 `zorder` 를 써서 정렬 키를 원본대로 바꾼다. 완전 동률의 순서는 원본 `qsort` 라 미확정이다. cpppj 처럼 입력 순서를 유지한다.
- [ ] 비교 함수를 `Netstorm.Core` 에 두고 `Order` 261개로 검증한다.

### 5-2. 오브젝트의 화면 위치와 경계 **[x86: `display-x86.tsv` `Bounds` 368]**

- 화면 좌표는 `trunc(x × 16 + 0.5) − 카메라X`, `trunc(y × 11 + 0.5) − 카메라Y` 에서 hotspot 을 뺀다. 폭·높이는 2-7 의 표시 폭·높이 + 1 이다.
- 선택 표시가 켜진 오브젝트는 폭이 19 미만이면 양쪽 9, 항상 양쪽 3, 위쪽 15픽셀(10.78)을 더 무효화한다.
- 타입 플래그 1 `0x40000`(shadow)이면 그림자 프레임은 `프레임 수 + 현재 프레임` 이다.

- [ ] C# 의 스프라이트 배치(`FortMapViewer.Sprites.cs`, `Units.cs`)가 같은 식인지 확인하고 다르면 맞춘다. 클릭 판정 사각형(`Combat.cs:71`)도 2-7 의 값으로 다시 계산한다.
- [ ] **그림자 합성은 지금 바꾸지 않는다.** 원본은 불투명 영역 아래 **화면의 팔레트 색을 변환표로 바꾸고**, C# 은 검정 알파 92 를 덮는다. 변환표의 생성·선택은 cpppj 도 아직 복원하지 않았다(8절).

### 5-3. 원본 글꼴 `.chfnt` **[정적, 실제 캐시 18개·4,608글리프 전수 대조]**

C# 은 모든 글자를 D2Coding(FontStashSharp)으로 그린다. `dotnetpj` 에 `.chfnt` 판독기가 없다. AGENTS.md 는 "그래픽을 가능한 한 동일하게"와 "한국어에 글꼴이 필요하면 D2Coding"을 함께 요구하므로, **영어는 원본 비트맵 글꼴, 한국어는 D2Coding** 이 목표에 맞는다(사용자 확인 필요 — 10절).

형식([cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md) "글꼴 형식의 추가 확정"):

| 파일 오프셋 | 내용 |
|---|---|
| `0x00` | `BitmapFontData`·`1A 00`, 16바이트 |
| `0x10` | 버전 1, 높이, 어센트, 디센트, 문자 수 256 |
| `0x24` | i32 × 256 측정 폭 |
| `0x424` | i32 × 256 ABC A |
| `0x824` | i32 × 256 그리기 진행 폭 |
| `0xC24` | u32 × 256 글리프 블록 바이트 수 |
| `0x1024` | VFX `1.10` 블록 256개(각 한 프레임) |

- 측정은 첫 표, 실제 출력의 진행은 세 번째 표를 쓴다. 저장된 ABC A 를 x 에 다시 더하지 않는다.
- 글자의 불투명 픽셀은 팔레트 번호 100 이고, 출력할 때 100 만 요청 색으로 바꾼다. 공백은 좌표 `7FFFFFFF/80000001` 의 특수 빈 프레임이다.
- 원본 슬롯: 0 = 14/700, 1 = Courier New 13/0, 3 = 20/700, 4 = 48/0, 5 = 14/0, 6 = 12/0(2 는 미사용). 크기는 포인트가 아니라 픽셀 높이다. 문맥 스타일은 normal/italic/bold/strikeout/underline.
- 그림자와 네 방향 외곽선 출력이 있다(`004a3430`·`004a3f70`).

- [ ] `Netstorm.Assets` 에 `.chfnt` 판독기를 추가한다. VFX 글리프 해독은 기존 `ShapeDatabase` 의 RLE 해독을 재사용한다. `assets/game-data/d/` 의 캐시 18개(`!Arial.bold.12.0.chfnt` 등)를 전수로 읽는 테스트를 넣는다.
- [ ] `OriginalUiSkin` 에 비트맵 글자 출력을 추가하고, 문자열이 Windows-1252 범위면 원본 글꼴, 아니면 D2Coding 으로 그린다. 폭 측정도 같은 기준으로 나눈다.
- [ ] 화면 확인(메뉴·브리핑·도움말)은 다른 PC 에서 `tools/clone_ui_smoke.ps1` 등으로 한다.

### 5-4. 커서 **[정적]**

- 원본 커서 번호 1~18 의 리소스 그룹: 113, 110, 111, 108, 107, 109, 115, 116, 131, 117, 130, 129, 132, 133, 134, 136, 141, 148(패치 `005423a8`). 번호 0 은 미설정.
- C# `GameCursor` 는 다섯 가지(113·109·148·111·110)만 있다.
- [ ] `GameCursor` 와 `OriginalCursor.Resources` 를 18개 표로 넓힌다. `assets/game-data/cursors/` 에는 지금 다섯 개(`RT_CURSOR_5·6·7·8·20.bin`)만 있다. 나머지는 `docs/formats/README.md` 의 추출 순서로 실행 파일 리소스에서 뽑고, `tools/cursor_catalog.py` 로 그룹 번호와 그림을 맞춘다(C# 은 그룹 번호가 아니라 개별 `RT_CURSOR` 번호로 읽는다).
- [ ] **어떤 상황에 어떤 번호를 쓰는지는 넣지 않는다.** UserInput(`004d62b0`)을 cpppj 가 복원한 뒤에 연결한다(8절). 지금 다섯 상태의 매핑은 TEST02 관찰 기반이므로 유지한다.

### 5-5. 타이틀·구름 그림의 색 **[정적]**

- 원본(`00419ec0` → `004dae60`)은 GIF 의 **색 번호를 화면 팔레트에 그대로 복사**한다. GIF 안의 RGB 표로 색을 다시 맞추지 않는다.
- C# [MainMenuView.cs](dotnetpj/src/Netstorm.Game/MainMenuView.cs) `LoadImage` 는 `Texture2D.FromStream` 으로 GIF 자체의 RGB 표를 쓴다.
- [ ] `d/titleMenu.gif`·`d/Gifcloud.gif` 의 내장 RGB 표와 게임 팔레트(`GamePalSpec` + `fortPal`)가 같은지 먼저 비교한다. 같으면 할 일이 없다. 다르면 색 번호를 해독해 게임 팔레트로 칠한다(GIF 해독은 `cpppj/src/client/GifImage.cpp` 가 인터레이스·투명·LZW 폭 증가까지 처리한 참고 구현이다).

### 5-6. 메인 메뉴 — 확인만

[cpp-menu-reconstruction.md](docs/exe/cpp-menu-reconstruction.md) 의 수치와 C# 을 맞춰 본다. C# 은 폭 75·피치 79·시작 `(356,311)` 을 이미 쓴다.

- [ ] 버튼 높이 19, 둘째 줄 y 334, 아래쪽 판정 +1픽셀.
- [ ] 돌 버튼은 **누름을 잡고 같은 영역에서 뗄 때** 실행하며 호버 변화가 없다. 목록 항목은 호버 강조가 있고 **누르는 즉시** 실행한다. 오른쪽 버튼·잠금 항목·밖에서 뗀 돌 버튼은 실행하지 않는다.
- [ ] 캠페인 목록은 `offical*.english`(원본 철자)에서 모으고 `Done...` 값으로 완료·잠금을 읽는다. 클론이 임의로 완료 값을 쓰지 않는다.

---

## 7. 6단계 — 미션 시작 값과 회귀 검사

- [ ] **6-1. 소유자 결정.** 2-6 의 정규화를 어디에 적용할지 정한다. cpppj 의 현재 월드는 "소유자 정규화/중립 처리"를 적용한 어댑터다([cpp-world-reconstruction.md](docs/exe/cpp-world-reconstruction.md)). `cpppj ... --dump-world originals thewarbegins`(콘솔, 창 없음)의 오브젝트 소유자 열과 C# `FortMap` + `BattleSessionFactory` 결과를 미션 4개에서 줄 단위로 비교하는 테스트를 만든다.
- [ ] **6-2. 시작 값 회귀.** 같은 `--dump-world` 출력의 시작 SP(1-1 은 저장 `Money` 100000 이 아니라 미션의 3000, Save the Island 2000, tutorial1 0, TEST01 50000)·동맹·색과 `MissionStart` 결과를 비교한다.
- [ ] **6-3. 본섬 마스크.** cpppj 가 기록한 256×256 마스크 SHA-256 네 개를 C# 회귀에 모두 넣는다. `MapRenderingTests.cs` 에 이미 일부가 있다.

| 미션 | 본섬 칸 | SHA-256 |
|---|---:|---|
| The War Begins! | 1,505 | `b24af563875ef935aa1053d521a6abf8490c4ddea840ac1845b43223fd0436d1` |
| Save the Island! | 2,082 | `68e699f55ad34041f316f4466f6c33d2edb6807833a648462a64ebbc2c505cc8` |
| tutorial1 → BridgeTheGap | 763 | `e16dd045520434a91d8ba43d078c7a11f56f3b8a87303ab8a6b2fa2496882584` |
| TEST01 | 5,172 | `ad2dafb1e5225e6be32e3a70a77cdc5b0475841dead39a4490804f6a404e6079` |

- [ ] **6-4. 받침 표시.** 완전한 `noIsland` 3×3 저장 묶음은 island/islandStalag 로 그리며 기준점은 오른쪽 아래, **일반 절벽과 달리 y 를 옮기지 않는다.** `FortIslandSupports` 가 같은지 확인한다.
- [ ] **6-5. 최대 HP.** 시작 HP 는 타입 `maxHitPoints` 다. mana(타입 158)는 약화 옵션이 켜지면 최대 HP 를 **0 쪽으로 자르는 정수 ÷ 4** 로 계산한다([cpp-creation-reconstruction.md](docs/exe/cpp-creation-reconstruction.md)). C# 에 이 옵션이 있는지 확인한다.
- [ ] **6-6. 문서.** 위 단계가 끝날 때마다 `docs/core-rules.md` 의 "근사한 부분"·"근사·미구현" 목록과 `docs/exe/bridge-pieces.md` 8.2 표에서 해소된 항목을 고친다.

---

## 8. cpppj 진행을 기다리는 항목 — **지금 구현하지 않는다**

cpppj 가 아직 복원하지 않았거나 월드에 연결하지 않은 부분이다. 지금 C# 에 넣으면 다시 추정이 된다. cpppj 쪽 진행은 [LEFT_JOBS.md](LEFT_JOBS.md) 맨 위 절과 [cpp-playable-plan.md](docs/cpp-playable-plan.md) 의 4단계 체크 목록에서 확인한다.

| 항목 | 원본 함수 | C# 의 현재 근사 | cpppj 상태 |
|---|---|---|---|
| 다리 배치 판정(겹침·영역 소유·법적 위치) | `0049b510`, `0048fdb0` | "섬 가장자리 또는 내 다리의 열린 끝" | 미복원 |
| 다리가 닿은 섬의 소유자 전파 | `00421240`, `004213b0` | `BridgeReach` | 미복원 |
| 조각 생성·품질 적용 | Construction `00442c80` | `BridgeGrid.Place` | 미복원 |
| 붕괴 스캔의 번호 범위 진행·한 칸 처리 전체 | `00422bc0`, `004227e0` | 10초 경계에서 한 번에 처리 | 방문 목록까지만 복원 |
| 금 간 프레임·낙하·지연 낙하(이벤트 `0x2692`)·칸 위 이동체 낙하 | `00421c30` 뒷부분, `00421f90` | 일부 미구현 | 수명 함수 접두 구간만 |
| 섬·지면·`noIsland`·받침이 어떤 표면 객체로 등록되는가 | Islandbuilder, `0046da70` 이후 | 섬 칸 단위 | 월드 미연결 |
| 점유 지도를 쓰는 실제 배치 판정 | `0049b510` | 칸별 점유 수 | 미복원 |
| 생산 덱·자원·SP 차감, `.fort` 의 `Deck`·`Technology` → 초기 덱 | Deck.cpp 등 | 미션 `myTech` 로만 시작 | 읽기·보존만 |
| 건설·채집·전투·AI·포획/희생·승패 | — | 관찰·팬게임 기반 구현 | 미복원 |
| 상황별 커서 선택, 입력 처리 전체 | UserInput `004d62b0` | TEST02 관찰 5종 | 미복원 |
| 그림자·소유자 색 변환표의 생성과 선택 | `00498220` 등 | 검정 알파 / `ObjectColorRemap` | 미복원 |
| 구름 움직임·시차 | Screen | 정적 타일 | 미복원 |
| 소리 초기화·재생 순서 | `004aa600` | `AudioPlayer`(분석 문서 기반) | 미복원 |
| 번호(SID) 소진 시 다리 최대 50개 삭제 | `004af1d0` | 없음 | 정적 대조만 |
| 미션 종류 객체·스크립트 명령 실행 | Mission, State `004b88c0` | `MissionConditions`·`TutorialStages` | 부분 |

**붕괴 스캔에 대해 지금 알려진 것(참고):** `bridge-pieces.md` 8.1 의 정적 분석으로는 스캔이 번호 15000~23001 을 10초에 걸쳐 나누어 훑는다. 그렇다면 칸마다 스캔 시각이 번호에 따라 0~10초 어긋나고, 이것이 관찰된 "스캔 위상이 조각마다 다르다"(같은 문서 8.3)와 맞는다. 4-2 의 할당기가 있으면 이 위상을 재현할 수 있다. 다만 프레임당 진행 식과 실측(금 → 낙하 40.0초)의 정합은 cpppj 가 `00422bc0` 을 기계어로 대조한 뒤에 확정한다. **지금 C# 의 스캔 방식을 바꾸지 않는다.**

---

## 9. 반영하지 않는 것 (dotnetpj 가 일부러 다르게 두는 것)

cpppj 는 "원본을 그대로 되살린다"가 목표라 아래를 원본대로 한다. dotnetpj 는 AGENTS.md 의 요구(Windows 10/11 + Linux, 와이드 화면, 다국어, 전체화면 재실행 오류 없음) 때문에 따르지 않는다. **cpppj 문서에 있다는 이유로 dotnetpj 에 옮기지 않는다.**

| cpppj(원본) | dotnetpj | 이유 |
|---|---|---|
| `<게임 폴더>/d/options.cfg` 를 원본 시점·형식으로 읽고 쓴다. `fullscreenStateFile.dat` 생성·삭제 | 자체 `settings.json`. `options.cfg` 는 읽기만 하고 쓰지 않는다 | 원본의 전체화면 저장 뒤 재실행 오류를 피한다(`DisplaySettings.cs` 주석, LEFT_JOBS.md 1.4절) |
| Win32 API 직접 호출, DIB + 팔레트, DirectDraw | MonoGame | Linux 지원 |
| 프레임 제한은 바쁜 대기 | MonoGame 의 대기. 간격(14ms)만 같다 | CPU 점유([main-loop.md](docs/exe/main-loop.md) 7절) |
| 고정 틱 없음("지금 + 간격" 타이머) | 24Hz 고정 틱 + 화면 보간 | 결정론·리플레이·이후 60/120프레임 |
| 입력 사건 큐 40칸, 넘치면 새 사건 버림 | MonoGame 입력 | 플랫폼 차이. 조작 결과가 달라지는 사례가 나오면 그때 본다 |
| 변경 사각형 100개 표(부분 다시 그리기) | 매 프레임 전체 그리기 | 화면 결과가 같다 |
| 뮤텍스, 레지스트리, CD 찾기, 창 크기 고정 | 없음 | 원본 실행 환경 전용 |
| raw 50바이트 슬롯·vtable 주소·생성자 주소 표 | 옮기지 않는다 | 의미만 옮긴다 |
| CD 10.72 의 다른 값(가중치 306, 타입 171, 점유 충돌 시 계속 등록 등) | 쓰지 않는다 | 기준은 10.78 |
| 10.82(V10·V12)의 Sun Generator·난이도·새 단축키·SID 77바이트·그래프 50,000 한도 | 쓰지 않는다 | 후대 판본 기능. AGENTS.md 도 Sun Generator 를 구현 대상이 아니라고 적었다 |

---

## 10. 사용자 확인이 필요한 것

1. **착수 시점 — 해결(2026-10-07).** 사용자 요청으로 병행 개발을 시작했다. 이미 확정된 1~2단계를 완료했으며 미복원 범위는 8절대로 기다린다.
2. **영어 UI 글꼴(5-3).** 영어를 원본 `.chfnt` 비트맵 글꼴로 바꾸고 한국어만 D2Coding 으로 둘지, 지금처럼 D2Coding 으로 통일할지.
3. **설정 층 순서(2-2) — 해결(2026-10-07).** 실제 용어표 4개와 setup의 중복 키는 0개다. 전역 우선순위를 원본대로 정정하고, 명시 언어 층·영어 누락 보충으로 한국어 선택을 유지했다. 관련 회귀 통과.
4. **번호 할당기 도입(4-2).** 오브젝트 번호를 원본 범위·순서로 바꾸면 결정론 검사합과 관련 테스트 기대값이 한꺼번에 바뀐다. 지금 할지, cpppj 가 붕괴 스캔을 복원한 뒤에 할지.

---

## 11. 디컴파일을 직접 읽어 풀 수 있는 C# 쪽 미확인 항목

`docs/core-rules.md` 가 "근사·미확인"으로 남긴 것 가운데 cpppj 진행과 무관하게 디컴파일만으로 답이 나올 수 있는 후보다. 주소는 기존 문서에 적힌 것이다. 읽는 방법: `python tools/decomp_refine.py --show <주소>`(정밀 디컴파일 필요), 함수 하나만 다시 뽑으려면 `tools/ghidra/decompile_at.ps1`.

- [ ] 싱글 플레이 다리 조각의 번호 할당 범위 — `00442c80`(4-2 의 선행).
- [ ] 튜토리얼 1 이 보는 "지은 다리 수"의 출처 — `004c3a20`(4-4).
- [ ] 건설 시간 `constructionRate` 의 계산식(지금은 모든 건물 10초) — [priest-construction.md](docs/exe/priest-construction.md).
- [ ] 손상된 유닛의 회수 금액 감소 공식 — 회수 `0044c420`.
- [ ] 워크샵 생산 칸 수(2/3/4)의 패치판 판정.
- [ ] 튜토리얼 단계 잠금 카운터(`+0x84`, 프레임 함수 `004c34c0`).
- [ ] 건물 제거 시 주변 다리 약화가 회수에도 적용되는지 — `0044b9e0` 의 수신 쪽 호출 경로(`bridge-pieces.md` 8.5).

---

## 12. 검증 명령과 기록 규칙

```powershell
# 기준선과 단계별 검증 (헤드리스, 이 PC 에서 실행 가능)
cd dotnetpj
dotnet build -c Release
dotnet test -c Release

# cpppj 콘솔 출력과 대조할 때 (창이 뜨지 않는 검사 명령만 쓴다. --run 은 창을 띄우므로 이 PC 에서 금지)
cpppj/build/bin/Release/NetstormCpp.exe --dump-types originals
cpppj/build/bin/Release/NetstormCpp.exe --dump-world originals thewarbegins
cpppj/build/bin/Release/NetstormCpp.exe --inspect-bridges originals
```

- 클론 창을 띄우는 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 `DESKTOP-HJOW` 가 아닌 PC 에서 한다.
- 새 코드의 상수·함수·반복문에는 한국어 주석을 달고, 새 파일은 UTF-8 로 쓴다(AGENTS.md).
- 원본 주소를 주석에 적을 때는 10.78 주소를 쓰고, 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.
- 작업이 끝난 항목은 이 문서에서 체크하거나 지운다. 중단하면 변경 파일·검사 결과·다음 항목을 이 문서 맨 위에 날짜 절로 남긴다.
- AGENTS.md 와 원본 디렉터리(`originals/`, `originalCD/` 등)는 수정하지 않는다.

## 13. 참고 문서

| 주제 | 문서 | cpppj 기준 구현 |
|---|---|---|
| 설정 조회·치환·저장 | [cpp-config-reconstruction.md](docs/exe/cpp-config-reconstruction.md), [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md) | `o/Config.cpp`, `o/ConfigInterface.cpp` |
| 타입 표·요새 파일 | [cpp-fort-reconstruction.md](docs/exe/cpp-fort-reconstruction.md), [cpp-assets-reconstruction.md](docs/exe/cpp-assets-reconstruction.md) | `o/RiftTypeTable.cpp`, `o/Template.cpp` |
| 다리 표·추첨·열린 방향·수명 | [cpp-bridge-reconstruction.md](docs/exe/cpp-bridge-reconstruction.md), [bridge-pieces.md](docs/exe/bridge-pieces.md) | `o/Bridge.cpp`, `o/CanonDecoder.cpp` |
| 표면 이웃·연결·붕괴 방문 | [cpp-surface-reconstruction.md](docs/exe/cpp-surface-reconstruction.md) | `o/SquidFinder.cpp` |
| 표면 그래프 | [cpp-graph-reconstruction.md](docs/exe/cpp-graph-reconstruction.md) 외 graph* 5개 | `o/Graph.cpp`, `o/RawGraph.cpp` |
| 점유 비트·해시 단계·등록/해제 | [cpp-hash-reconstruction.md](docs/exe/cpp-hash-reconstruction.md), [cpp-spatial-reconstruction.md](docs/exe/cpp-spatial-reconstruction.md) | `o/SquidHash.cpp`, `o/SquidSpatial.cpp` |
| 번호(SID) 할당 | [cpp-sid-reconstruction.md](docs/exe/cpp-sid-reconstruction.md) | `o/SidPool.cpp` |
| 첫 등록 통계·비용·목록 | [cpp-postpop-reconstruction.md](docs/exe/cpp-postpop-reconstruction.md) | `o/SquidPostPop.cpp` |
| 그리기 순서·글꼴·커서 | [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md), [cpp-display-reconstruction.md](docs/exe/cpp-display-reconstruction.md) | `client/Renderer.cpp`, `o/SquidDisplay.cpp` |
| 메뉴·브리핑 | [cpp-menu-reconstruction.md](docs/exe/cpp-menu-reconstruction.md) | `client/UberGump.cpp`, `client/Gump.cpp` |
| 지형·시작 값·조작 | [cpp-world-reconstruction.md](docs/exe/cpp-world-reconstruction.md) | `o/TerrainBuilder.cpp`, `client/GameWorld.h` |
| 메인 루프·프레임 | [main-loop.md](docs/exe/main-loop.md) | `client/ClientMain.cpp` |
| 판본 비교(10.37·10.62·10.82·V12) | [cpp-reference-versions.md](docs/exe/cpp-reference-versions.md) | — |
| C# 규칙 코어의 현재 범위와 근사 목록 | [core-rules.md](docs/core-rules.md) | — |
