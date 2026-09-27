# LEFT_JOBS — NetStorm 클론 프로젝트 작업 계획 및 인수인계

> 최종 갱신: 2026-09-28
> 프로젝트 목표(AGENTS.md): 원본 NetStorm: Islands at War 를 디컴파일/분석하여 클론 코딩하고,
> **Windows 10/11** 과 **GUI 환경의 Linux** 에서 동작하며 **여러 언어를 지원**하는 게임을 만든다.
> **1차 목표 언어: 영어, 한국어** (그 외 언어는 이후 확장).
> **우선순위: Windows 10/11 > Linux** (Linux 지원은 우선순위가 낮다 — 설계상 이식성은 유지하되 검증·배포는 Windows 먼저).

---

## 0. 진행 현황 요약

| 단계 | 내용 | 상태 |
|---|---|---|
| 1 | 설계 (본 문서 작성: 원본 조사, 단계·작업 정의) | ✅ 완료 (2026-09-27) |
| 2 | 개발 환경 구축 | ✅ 완료 (2026-09-27) — C# + MonoGame(net10.0), 솔루션·테스트·CI |
| 3 | 원본 분석 — 데이터 포맷 | 🔶 거의 완료 (TAFF·셰이프·팔레트·.type·설정·번역 체계·`.fort` 컨테이너/오브젝트 완료 / `.fort` 일부 섹션·HLP 남음) — [docs/formats/](docs/formats/README.md) |
| 4 | 원본 분석 — 실행 파일(게임 로직) | 🔶 착수 (전체 디컴파일·모듈 맵 완료) |
| 5 | 원본 분석 — 플레이 영상 | ⬜ 대기 |
| 6 | 자산 로더 / 개발용 뷰어 | 🔶 진행 중 (TAFF·팔레트·셰이프·.type·.cfg·TTC·.fort 로더 완료) |
| 7 | 엔진 코어 (플랫폼 계층) | ⬜ 대기 |
| 8 | 게임 월드 / 규칙 구현 | ⬜ 대기 |
| 9 | UI · 미션 스크립트 · 튜토리얼 | ⬜ 대기 |
| 10 | AI | ⬜ 대기 |
| 11 | 캠페인 · 저장(fort) | ⬜ 대기 |
| 12 | 다국어 지원 | ⬜ 대기 (설계는 초기부터 반영) |
| 13 | 멀티플레이 | ⬜ 대기 |
| 14 | 패키징 · 배포 (Windows 우선, Linux 후순위) | ⬜ 대기 |
| 15 | 검증 · QA | ⬜ 상시 |

단계 번호는 대략적인 순서이며, 3~5 단계(분석)는 병행 가능하다. 6 단계 이후는 분석 결과가 나오는 대로 점진적으로 진행한다.

---

## 1. 원본 조사 결과 (1단계 조사 + 3단계 분석으로 갱신)

`originals/` 전체 약 310MB, 파일 1,388개.

### 1.1 실행 파일 / 라이브러리

| 파일 | 내용 |
|---|---|
| `Netstorm.exe` (1.5MB) | x86 32bit PE, MSVC 빌드, **패킹되지 않음**(.text 약 1MB). 빌드 타임스탬프 2006-03 → 원본(1997) 이 아닌 **Ticonderoga Entertainment 비공식 패치 빌드(10.7x)**. 임포트: DDRAW, DSOUND, WINMM, WSOCK32, smackw32, GDI32, COMCTL32, ADVAPI32, SHELL32, VERSION, ole32, mscoree. 문자열에 `TAFF v%d.%d`, `_shapes.shp`, `*.tarc`, `IPX`, `SPX/IPX` 존재 |
| `NSENGLISHRES.DLL` | **설치·진단 프로그램용** 문자열 24개·다이얼로그 13개 (게임 UI 문자열 아님). 게임 UI 문자열은 exe 에 영어로 하드코딩되어 있고 `xlat.<언어>` 로 번역된다 → [xlat.md](docs/formats/xlat.md) |
| `Smackw32.dll` | Smacker 동영상 코덱. 단, `movie/` 폴더는 비어 있음 |
| `R.exe`, `unpack.exe`, `bzip2.exe`, `TMaker.exe` | 보조 도구(업데이터/압축 해제/요새 생성기로 추정). 게임 본체 분석 대상은 아님 |
| `PatchFixs.txt` | 패치 변경 이력(892줄). 원본 대비 **변경된 규칙·버그 수정 목록** → 어느 버전 동작을 기준으로 할지 결정 시 참고 |

### 1.2 데이터 파일

| 파일/패턴 | 확인된 내용 | 비고 |
|---|---|---|
| `netstorm.tarc` (1.1MB) | TAFF 아카이브, 엔트리 246개. 데이터는 XOR 키 `mydoghasfleas` 로 인코딩 → **해독 완료** ([taff.md](docs/formats/taff.md)) | `.type`(오브젝트 정의), 공식 미션, 번역 테이블(`xlat.*`), 용어표(`config.*`) 포함 |
| `d/_shapes.shp` (5.6MB) | Miles VFX 셰이프 블록 116개(타입당 1개), 프레임 3,783개, 행 단위 RLE → **해독 완료** ([shp.md](docs/formats/shp.md)) | 모든 스프라이트. 블록 순서 = exe 의 타입 로딩 순서 |
| `d/*.COL` | Autodesk Animator Pro 팔레트 (8바이트 헤더 + 256×RGB) | **`GIFCLOUD.COL` = 게임 팔레트** (`setup.cfg` 지정). `TITLE*.COL` = 타이틀 화면. 나머지 유닛 이름 팔레트는 그래픽 제작용으로 추정 |
| `d/!color.dat` | 256바이트 색상 변환 테이블 9개 (항등·검정·단색·어둡게 등) | 개별 용도는 4단계에서 확인 ([config.md](docs/formats/config.md)) |
| `d/!*.chfnt` (18개) | Windows GDI 글꼴(Arial 등)로 게임이 **실행 중에 만든 캐시** (없으면 재생성) | 라틴 문자 전용 → 한국어는 `fonts/` 의 D2Coding 사용 (1.5절, [chfnt.md](docs/formats/chfnt.md)) |
| `d/*.english`, `*.german` 등 (약 580개) | 미션 스크립트. INI 유사 섹션(`[Header]`, `[A.]`, `[Succeeded]`, `[Failed]`, `[aiNPriestDead]`, `[aiNTempleHalfDead]`, `[@1]` …) + `key = value` + HTML 부분집합(`<h2>`, `<p>`, `<i>`) + 명령(`$Button=`, `$Timeout=`, `$Menu=`, `$Checked=`, `$OnExit=`, `<$Config,…>`) | 팬 제작 미션 다수 포함. 오타(`$Buton=`, `$button=`)가 섞여 있어 **관대한 파서** 필요 |
| `d/*.fort` (431개) | `'F'` + 플래그 + 길이 접두 섹션 35개(이름 고정). 월드 16×16 청크의 오브젝트 레코드 → **해독 완료** (일부 섹션 내부 제외) | 섬/요새·미션 맵 ([fort.md](docs/formats/fort.md)) |
| `d/*.gif` | 팬 제작 이미지로 추정 | 우선순위 낮음 |
| `d/options.cfg`, `setup.cfg`, `!rootservers.dat` | 설정 / 온라인 서버 목록. `.cfg` 는 TAFF 와 같은 키로 XOR 인코딩 → **해독 완료** | `tools/nscfg.py` 로 읽기·수정 가능. **화면 해상도 설정 `SCREENW/SCREENH` = 1024×768** (640×480 고정 아님) |
| `music/*.mus` (9개) | 확장자만 다른 **RIFF WAV** (PCM 16bit, 스테레오, 22,050Hz) | 그대로 재생 가능 |
| `sound/*.wav` (218개) | 표준 PCM WAV 효과음 (대부분 22,050Hz 8bit 모노) | 그대로 재생 가능 |
| `help/*.HLP` | WinHelp 도움말 (GAME, HELP, VOCAB, VENDOR, README) | 유닛·규칙·용어 설명 → 규칙 분석의 1차 자료 |

### 1.3 주의 사항

* **원본 자산 재활용 가능**: 개발사 지원이 오래전에 끊겨 저작권 행사가 사실상 없으므로, 원본 그래픽·사운드·미션·도움말 등 **기존 파일을 필요 시 그대로 재활용**한다. 대체 자산을 새로 만들 필요는 없다.
  * 다만 `originals/` 는 약 310MB 이므로 **git 저장소에는 커밋하지 않는다**(`.gitignore` 에서 제외). 배포 패키지에는 필요한 자산을 동봉할 수 있다(14단계).
  * 게임 엔진은 원본 포맷을 직접 읽는 구조로 만든다(자산 변환 없이 원본 폴더 또는 동봉 폴더에서 로드).
* (Linux 대응 시) Linux 는 파일 시스템이 **대소문자를 구분**한다. 원본 데이터 파일명은 대소문자가 뒤섞여 있고(`Battle1.fort` / `battle1.english`), 아카이브 내부 경로는 `\` 구분자를 쓴다 → 파일 탐색은 반드시 대소문자 무시 + 경로 구분자 정규화 계층을 거친다.
* 원본 텍스트 파일 인코딩은 Windows-1252(영어/독일어)로 추정 → 로드 시 UTF-8 로 변환한다. 새로 만드는 문서·코드는 UTF-8.

### 1.4 원본 실행 환경 (AGENTS.md 기준)

* 원본 `Netstorm.exe` 는 현재 **Windows 10/11 에서 제한적으로 구동 가능**하며, 실행 시 **창 모드**로 동작한다.
* 게임 옵션에서 전체화면으로 전환하면 전환 자체는 되지만, **게임 종료 후 재실행 시 오류가 발생**한다. 오류 후 한 번 더 실행하면 다시 창 모드로 실행된다.
  * 원본을 분석용으로 실행할 때는 **전체화면 전환을 하지 않는다**.
  * **분석 결과(정적)**: 설정은 `d/options.cfg` 의 `startInFullScreen` 에 저장된다. 시작 시 이 값이 참이면 `workingFullScreenFlags` 모드로 화면을 초기화하는데, 실패하면 초기화 함수가 그대로 반환되어 게임이 뜨지 않는다 → DirectDraw 독점 전체화면 초기화 실패로 추정. 자세한 내용은 [config.md](docs/formats/config.md) "전체화면 재실행 오류".
  * **복구 방법(추정)**: `python tools/nscfg.py set originals/d/options.cfg startInFullScreen 0` (원본은 `.bak` 으로 백업됨). **현재 `originals/d/options.cfg` 는 `startInFullScreen = "1"` 상태**라 다음 실행 때 오류가 날 수 있다.
  * 오류 후 창 모드로 돌아오는 과정은 동적 분석으로 확인한다. 클론에서는 이 버그를 재현하지 않는다.
* 원본을 직접 실행할 수 있으므로 다음이 가능하다.
  * **동적 분석**: x64dbg 등으로 실행 중인 원본의 메모리·함수 호출·파일 접근 관찰 (정적 분석 결과 검증)
  * **직접 대조**: 영상뿐 아니라 원본 화면 캡처·수치 측정과 클론 결과를 나란히 비교

### 1.5 한국어 글꼴 (AGENTS.md 기준)

* 한국어 표시가 필요한 경우 **`fonts/D2Coding-Ver1.3.2-20180524-all.ttc`** 를 사용한다.
* TrueType Collection(TTC)이며 글꼴 4종을 담고 있다. 로드할 때 **face 인덱스**를 지정해야 한다.

  | 인덱스 | 글꼴 |
  |---|---|
  | 0 | D2Coding (일반) |
  | 1 | D2Coding Bold |
  | 2 | D2Coding ligature |
  | 3 | D2Coding ligature Bold |

* 라이선스: SIL Open Font License 1.1 → 게임과 함께 배포 가능 (배포 시 OFL 고지 동봉).
* **monospace(고정폭)** 글꼴이다. 원본 UI 는 Arial(가변폭) 기준으로 배치되어 있으므로, 같은 문장이라도 폭이 달라진다 → 텍스트 영역은 글꼴 메트릭으로 측정해 줄바꿈·크기를 정해야 한다(좌표 하드코딩 금지).
* 영어 텍스트는 원본 느낌을 살리기 위해 원본 `.chfnt` 비트맵 폰트 사용을 우선 검토하고, 한국어(및 원본 폰트에 없는 문자)는 D2Coding 으로 대체하는 **폰트 폴백 체인**을 둔다. 영어도 D2Coding 으로 통일할지는 구현 시 화면을 보고 결정.

### 1.6 외부 참고 자료 (AGENTS.md 기준)

* **플레이 영상** (YouTube): 튜토리얼, 캠페인 1-1~4, 1-5, 1-6, 2-1, 2-2, 2-3 — 주소는 5단계 표 참고
* **게임 플레이 방법 소개 홈페이지 (한국어)**: https://hjow.duckdns.org/netstorm/learnmain.htm (2026-09-28 AGENTS.md 에 추가)
  * 하위 페이지 (2026-09-28 목차 확인):

    | 페이지 | 주제 | 주로 참고할 단계 |
    |---|---|---|
    | [learninstall.htm](https://hjow.duckdns.org/netstorm/learninstall.htm) | 설치 및 시작, 메뉴, 옵션 | 9단계(메뉴·옵션 화면) |
    | [learninterface.htm](https://hjow.duckdns.org/netstorm/learninterface.htm) | 화면 구성·UI 설명 | 5단계(화면 관찰), 9단계(HUD) |
    | [learnmineral.htm](https://hjow.duckdns.org/netstorm/learnmineral.htm) | 광물 자원 | 4·8단계(경제) |
    | [learnresource.htm](https://hjow.duckdns.org/netstorm/learnresource.htm) | 자원 수집·관리 (Storm Power 등) | 4·8단계(경제) |
    | [learntemple.htm](https://hjow.duckdns.org/netstorm/learntemple.htm) | 신전 시스템 | 4·8단계(신전·기술) |
    | [learnpriest.htm](https://hjow.duckdns.org/netstorm/learnpriest.htm) | 사제(프리스트)·고위 사제 | 4·8단계(승패 조건·프리스트) |
    | [learnmovie.htm](https://hjow.duckdns.org/netstorm/learnmovie.htm) | 플레이 영상 | 5단계 |
    | [unitmain.htm](https://hjow.duckdns.org/netstorm/unitmain.htm) | 전체 유닛 목록·설명 | 4·8단계(유닛·건물 규칙), [types.md](docs/gameplay/types.md) 수치와 대조 |

  * 활용 방침: 게임 규칙의 **1차 근거는 원본 데이터·실행 파일 분석**이고, 이 홈페이지는 동작 이해와 교차 검증용으로 쓴다. 내용이 분석 결과와 다르면 원본 동작을 우선하고 차이를 문서에 남긴다.
  * **한국어 용어 참고 자료**로도 쓴다 (12단계 한국어 번역 시 유닛·건물·자원 이름의 기존 한국어 표기 확인).

---

## 2. 기술 스택 (**확정: C# + MonoGame**, 2026-09-27 사용자 결정)

| 영역 | 선택 | 비고 |
|---|---|---|
| 언어 / 런타임 | **C#** / .NET (MonoGame 공식 템플릿의 대상 프레임워크를 따름, 현재 net8.0 LTS) | 이 PC 에는 .NET SDK 9.0.318 설치됨 (net8.0 대상 빌드 가능) |
| 게임 프레임워크 | **MonoGame 3.8.x — DesktopGL 플랫폼** | OpenGL 기반이라 **같은 코드로 Windows 10/11 과 Linux 모두** 지원 (WindowsDX 는 Windows 전용이라 사용하지 않음) |
| 빌드 | `dotnet` CLI + 솔루션(`.sln`) / SDK 스타일 `.csproj` | Visual Studio 2022, VS Code(C# Dev Kit), Rider 모두 사용 가능 |
| 렌더링 | 원본 자산은 Content Pipeline 을 거치지 않고 **런타임에 직접 로드**. 8bit 인덱스 프레임 → 팔레트 적용 방식은 (a) CPU 에서 RGBA 변환 후 `Texture2D.SetData` 또는 (b) 인덱스 텍스처 + 팔레트 텍스처 + 픽셀 셰이더 중 7단계에서 결정 | 해상도 가변(원본 설정 `SCREENW/SCREENH`, 기본 1024×768) + `RenderTarget2D` 비율 스케일링. 원본은 640×480 고정이 아니므로 해상도별 UI 배치 확인 필요 |
| 폰트 | **FontStashSharp** (런타임 TTF 래스터화) + **D2Coding**(`fonts/`) | MonoGame 기본 `SpriteFont` 는 빌드 시 글자를 미리 구워야 해서 한글(1만 자 이상)에 부적합. D2Coding 은 TTC 이므로 face 인덱스 지정 가능 여부를 확인하고, 불가하면 필요한 face 만 TTF 로 분리해 사용 |
| 오디오 | MonoGame `SoundEffect`(효과음, PCM WAV 직접 로드) + `DynamicSoundEffectInstance`(음악 스트리밍) | 원본 오디오는 모두 표준 PCM WAV. 음악 파일이 18~20MB 이므로 스트리밍 필요 |
| 네트워크 | **LiteNetLib** (UDP) 또는 `System.Net.Sockets` | 결정론적 락스텝 구현. 13단계에서 확정 |
| 설정 / 번역 파일 | `System.Text.Json` (설정) + 원본 `xlat` 형식 호환 번역 파일(UTF-8) | 원본 번역 체계 재활용 ([xlat.md](docs/formats/xlat.md)) |
| 테스트 | **xUnit** | 포맷 로더·규칙·결정론 테스트 |
| 배포 | `dotnet publish` self-contained (win-x64 / linux-x64), 단일 파일 옵션 검토 | 사용자 PC 에 .NET 설치 불필요 |
| 분석/추출 도구 | Python 3.13 (기존 `tools/` 유지) | 포맷 리버싱·검증용. 게임 본체는 C# 로더를 새로 작성하고 Python 결과와 비교 테스트 |
| 디컴파일 | Ghidra + x64dbg (변경 없음) | 디컴파일 결과(C 의사코드)를 C# 로 옮길 때 정수 오버플로·부호·구조체 레이아웃 차이에 주의 |

### 결정론(멀티플레이·리플레이) 관련 주의

* 게임 시뮬레이션 코드에서는 `float`/`double` 연산 결과가 플랫폼·JIT 에 따라 달라질 수 있으므로 **고정소수점(정수) 연산**을 우선 검토한다 (원본 동작 분석 후 결정).
* `System.Random` 대신 원본 난수 생성기를 그대로 옮긴 자체 구현을 쓴다.
* `Dictionary`/`HashSet` 순회 순서에 의존하는 로직을 만들지 않는다.

### C++ 전제로 이미 준비된 항목의 처리

* `PREPARE.ps1` 의 VS Build Tools(C++)·CMake·Ninja·vcpkg 항목은 게임 빌드에 더 이상 필수가 아니다 → 2단계에서 **.NET SDK·MonoGame 템플릿 항목을 추가**하고 C++ 항목은 선택으로 내린다.
* `src/` 아래 C++ 기준으로 만든 빈 폴더(`platform/`, `engine/` 등)는 3절의 C# 프로젝트 구조로 재구성한다.

---

## 3. 디렉터리 구조 (C# + MonoGame 기준)

```
Netstorm/
├─ originals/                 # 원본 (읽기 전용, 재활용 가능, 용량 문제로 저장소에 커밋하지 않음)
├─ docs/
│  ├─ formats/                # 파일 포맷 명세 (taff.md, shp.md, type.md, fort.md, mission-script.md …)
│  ├─ gameplay/               # 게임 규칙·수치·공식 명세
│  ├─ exe/                    # 실행 파일 분석 노트 (모듈 맵, 구조체)
│  ├─ videos/                 # 영상 분석 노트
│  └─ screens/                # 원본 화면 캡처(screenShots/) 관찰 노트
├─ tools/                     # Python 추출·검증 도구, Ghidra 스크립트
├─ extracted/                 # 도구가 원본에서 추출한 결과물 (커밋하지 않음, 필요 시 생성)
├─ fonts/                     # 한국어 글꼴 D2Coding (TTC, SIL OFL)
├─ locale/                    # 번역 파일 (en, ko 1차 / de 등 이후)
├─ screenShots/               # 사용자가 찍은 원본 게임 캡처 (외부 캡처 도구 — 창 제목 표시줄·테두리 포함)
├─ Netstorm.sln
├─ src/
│  ├─ Netstorm.Assets/        # 원본 포맷 로더 (TAFF, .type, _shapes.shp, 팔레트, .fort, 설정, 번역) — MonoGame 비의존
│  ├─ Netstorm.Core/          # 게임 규칙·엔티티·전투·경제·다리·AI·미션 스크립트 — MonoGame 비의존 (결정론·테스트 용이)
│  ├─ Netstorm.Net/           # 멀티플레이 (락스텝)
│  └─ Netstorm.Game/          # MonoGame DesktopGL 실행 프로젝트: 렌더링·입력·오디오·UI·화면 전환
└─ tests/
   ├─ Netstorm.Assets.Tests/  # xUnit: 원본 전체 파일 파싱, Python 도구 결과와 비교
   └─ Netstorm.Core.Tests/    # xUnit: 규칙·결정론 테스트
```

* 게임 로직(`Core`)과 자산 로더(`Assets`)는 MonoGame 에 의존하지 않게 분리한다 → 헤드리스 테스트·리플레이 검증·서버 재사용이 쉽다.

---

## 4. 단계별 작업 상세

각 작업은 `[ ]` 체크박스로 관리하고, 완료 시 `[x]` 로 바꾸고 산출물 경로를 적는다.

### 2단계. 개발 환경 구축 — ✅ 완료 (2026-09-27)

- [x] 기술 스택 확정: **C# + MonoGame (DesktopGL)** — 2026-09-27 사용자 결정 (2절)
- [x] `git init`, `.gitignore` — 2026-09-27 완료. .NET 산출물(`bin/`, `obj/`, `publish/`, `TestResults/`)도 제외
- [x] 개발 도구 점검·설치 스크립트 `PREPARE.ps1` — 2026-09-27 완료 (C# 기준으로 갱신)
  - 사용법: `powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1` (항목 선택 후 점검·설치), `-CheckOnly`(점검만), `-All`(선택 화면 생략), `-ToolsDir`(Ghidra·vcpkg 설치 폴더, 기본 `C:\Tools`)
  - 필수: Git, **.NET SDK 10**, Python 3 + 패키지(pillow·pefile·capstone), JDK 21+, Ghidra, x64dbg, Process Monitor, git safe.directory / 권장: MonoGame 템플릿, VS Code 확장(C# Dev Kit·C#·Python) / 선택: VS Build Tools(C++)·CMake·Ninja·vcpkg(C# 확정으로 불필요), yt-dlp, FFmpeg
  - 2026-09-27 점검 결과: 필수 전부 설치됨 (.NET SDK 8.0.425 / 9.0.318 / 10.0.401 확인). MonoGame 템플릿은 미설치(프로젝트는 템플릿 없이 구성했으므로 필수 아님)
- [x] `.editorconfig` — 2026-09-27 완료. UTF-8·LF 기본, `.ps1` 은 UTF-8 BOM·CRLF, `.sln` CRLF, C# 스타일 규칙(파일 범위 네임스페이스, Allman 중괄호, `_camelCase` private 필드)
- [x] 솔루션 골격 — 2026-09-27 완료. `src/` 의 C++ 기준 빈 폴더 삭제 후 3절 구조로 생성
  - `Netstorm.sln`, `global.json`(SDK 10.0.x), `Directory.Build.props`(공통: **net10.0**, Nullable, ImplicitUsings), `Directory.Packages.props`(패키지 버전 중앙 관리)
  - 대상 프레임워크는 **net10.0 (LTS, 2028-11 지원 종료)**. .NET 8 은 2026-11 지원 종료라 제외. MonoGame 3.8.5.1 패키지는 net8.0 대상이지만 net10.0 에서 그대로 동작함을 확인
  - 패키지: MonoGame.Framework.DesktopGL 3.8.5.1, FontStashSharp.MonoGame 1.6.1, xunit.v3 3.2.2 (+ xunit.runner.visualstudio 3.1.5, Microsoft.NET.Test.Sdk 17.14.1)
  - xUnit v3 를 쓴 이유: 원본 데이터가 없는 환경(CI)에서 원본이 필요한 테스트를 `Assert.SkipWhen` 으로 건너뛰기 위함. xunit.v3 4.x 는 새 테스트 플랫폼(MTP v2) 설정이 필요해 3.2.2 사용
- [x] MonoGame DesktopGL 창 + **D2Coding 한글 출력** + 원본 스프라이트 애니메이션 표시 — 2026-09-27 완료 (Windows 에서 확인)
  - FontStashSharp 는 TTC face 선택을 지원하지 않으므로 `TrueTypeCollection.ExtractFace` 로 face 0 을 TTF 로 분리해 로드
  - 검증용 `--screenshot <경로>` 옵션: 30프레임 후 화면을 PNG 로 저장하고 종료
- [x] CI — 2026-09-27 완료. `.github/workflows/ci.yml` (windows-latest + ubuntu-latest, `dotnet build` + `dotnet test`)
  - 원본이 없는 환경을 흉내 내 실행: 21개 중 원본 필요 테스트는 건너뛰고 나머지 통과 확인. **원격 저장소가 아직 없어 실제 GitHub Actions 실행은 미확인**
- [x] Ghidra 프로젝트 생성, `Netstorm.exe` 임포트·자동 분석·전체 디컴파일 — 2026-09-27 완료
  - `tools/ghidra/run_decomp.ps1` (헤드리스, 약 10~20분) → 프로젝트 `extracted/ghidra/`, 결과 `extracted/decomp/Netstorm.c` (함수 4,506개, 실패 1개)
- [ ] (후순위) Linux 에서 빌드·실행 확인

#### 빌드·실행 방법

```powershell
dotnet build Netstorm.sln                     # 전체 빌드
dotnet test Netstorm.sln                      # 테스트 (원본 데이터가 있으면 원본 검증 테스트도 실행)
dotnet run --project src/Netstorm.Game        # 게임 실행 (Esc 종료)
dotnet run --project src/Netstorm.Game -- --screenshot extracted/screens/shot.png
```

원본 데이터 폴더는 `NETSTORM_DATA` 환경 변수 → 실행 파일/현재 폴더에서 상위로 올라가며 `originals/` 탐색 순으로 찾는다 (`GameDataLocator`).

### 3단계. 원본 분석 — 데이터 포맷

각 포맷마다 (1) `docs/formats/*.md` 명세, (2) `tools/` 의 Python 파서·추출기, (3) 원본 전체 파일에 대한 파싱 성공 검증을 산출물로 한다.
exe 내부의 파일 로딩 함수를 Ghidra 로 함께 추적하면 빠르다(`TAFF v%d.%d`, `_shapes.shp` 문자열 참조 지점부터 시작).

- [x] **TAFF 아카이브 (`netstorm.tarc`)** — 2026-09-27 완료: [taff.md](docs/formats/taff.md), `tools/taff.py`. XOR 키 `mydoghasfleas`, 246개 전부 추출
- [x] **`.type`** 문법·속성·플래그 — 2026-09-27 완료: [type.md](docs/formats/type.md), `tools/typefile.py`, 수치표 [docs/gameplay/types.md](docs/gameplay/types.md) (70종)
  - [ ] 남은 일: 속성·플래그의 정확한 게임 내 의미 확정 (4단계와 연계)
- [x] **`_shapes.shp`** — 2026-09-27 완료: [shp.md](docs/formats/shp.md), `tools/shp.py`. 116블록·3,783프레임 전부 디코딩, PNG 추출 검증
  - [ ] 남은 일: 헤더 bounds/origin 의 정확한 의미, 특수 레코드 91개의 용도
- [x] **팔레트** — 2026-09-27 완료: 게임 팔레트 = `d/GIFCLOUD.COL` ([shp.md](docs/formats/shp.md) "색상")
  - [ ] 남은 일: `!color.dat` 테이블 9개의 개별 용도, 플레이어 색 구현 방식 (4단계)
- [x] **`.chfnt`** 용도 확인 — 2026-09-27 완료: GDI 글꼴의 런타임 캐시 ([chfnt.md](docs/formats/chfnt.md))
  - [ ] 남은 일(낮은 우선순위): 글리프 데이터 디코딩 (영어 원본 글꼴 재현이 필요할 때)
- [x] **`.fort`** 컨테이너·오브젝트 레코드 — 2026-09-27 완료: [fort.md](docs/formats/fort.md), `tools/fort.py` (`dump`, `verify`)
  - 원본 463개(느슨한 파일 431 + 아카이브 32) 전부 섹션 끝까지 정확히 해석
  - 섹션 35개(Subscriber … Deck, Reserved2~4, Terr00~19), 타입 번호 변환(TypeNames 해시), 청크 레코드, 타입 플래그(typeflags 단어 → 비트, 파생 규칙) 해독
  - [x] 2026-09-28: 위치 바이트 x/y, `Territory` 모양·생성 플래그·청크 위치 및 `TerrNN` 배치 해석 — [영역 배치 분석](docs/exe/territory-layout.md), C# `FortMap`. 원본 463개 청크 수와 캡처 좌표 대조 완료
  - [ ] 남은 일: `State`·`Technology`·`Deck`·`Badges`·`CoreData`·`Mission` 섹션 내부 구조, `Territory` 외관·나머지 플래그, 회전된 영역 동적 검증, 내부 class 값에 따른 파생 플래그
- [ ] **미션 스크립트** (`.english` 등): 문법 명세 — 섹션 종류와 발생 조건(이벤트), `[Header]` 키 전체 목록(`missionType`, `myTech`, `aiNName/Tech/StartMoney/Collectors/GeyserAttachments/color/BridgeDrawRate/Ability` …), `$` 명령과 인자, 인라인 HTML 태그, `{mission.filename}` 등 치환 변수. 전 파일 대상 키·명령 빈도 통계
  - [x] 문법 개요·빈도 통계·공식 캠페인 구성 — 2026-09-27 완료: [mission-script.md](docs/formats/mission-script.md)
  - [ ] 남은 일: 각 명령·버튼 동작·이벤트 섹션의 정확한 의미 (스크립트 인터프리터 코드 분석, 4단계와 연계)
- [x] **설정 파일** — 2026-09-27 완료: [config.md](docs/formats/config.md), `tools/nscfg.py` (복호화·값 변경, 바이트 단위 라운드트립 검증)
- [x] **리소스** — 2026-09-27 완료: `tools/peres.py` (비트맵 3·커서 18·다이얼로그·문자열). DLL 문자열은 설치 프로그램용
- [ ] **도움말**: `help/*.HLP` 텍스트 추출 (WinHelp 형식 — helpdeco 등 도구 필요). 참고: 게임 내 도움말 텍스트는 아카이브의 `help.english` 에도 있음
- [x] 오디오 전수 확인 — 2026-09-27 완료: 229개 모두 표준 PCM WAV
  - [ ] 남은 일: 사운드 파일명 ↔ 게임 이벤트 매핑 표 (`.type` 의 `*Sound` 속성 + exe 문자열)
- [x] **다국어 체계** (계획에 없던 발견) — 2026-09-27 완료: [xlat.md](docs/formats/xlat.md). 원본에 `xlat.<언어>` 번역 테이블(독일어 871항목)과 `config.<언어>` 용어표가 있어 12단계 설계 기반으로 사용

**완료 기준**: 게임에 필요한 모든 원본 자산을 Python 도구로 읽어 사람이 볼 수 있는 형태(PNG/JSON/텍스트)로 내보낼 수 있다.

### 4단계. 원본 분석 — 실행 파일 (게임 로직)

목표는 "코드를 그대로 옮기는 것"이 아니라 **동작 명세를 뽑아내는 것**이다. 결과는 `docs/exe/`(함수 맵, 구조체), `docs/gameplay/`(규칙·수치) 에 정리한다.
규칙 이해·교차 검증에는 플레이 방법 소개 홈페이지(1.6절: 자원·신전·사제·유닛 페이지)를 함께 참고한다.

- [ ] 라이브러리 함수(CRT, DirectX, WinSock 래퍼) 식별·명명 → 게임 고유 코드 범위 축소
- [ ] 동적 분석 환경: 원본을 창 모드로 실행 + x64dbg 연결, 파일 접근(Process Monitor)·메모리 관찰로 정적 분석 결과 검증
- [x] 전체화면 설정 저장 위치·시작 흐름 확인 (정적 분석, 1.4절) — 2026-09-27
  - [ ] 남은 일: 동적 분석으로 실패 지점·플래그 초기화 과정 확인
- [ ] 서브시스템 경계 식별: 메인 루프/틱, 렌더, 입력, 사운드, 파일 로딩, 네트워크, 스크립트, AI
  - 출발점: assert 메시지로 복원한 원본 소스 파일 114개 모듈 맵 [docs/exe/modules.md](docs/exe/modules.md) (`tools/ghidra/module_map.py`)
- [ ] 핵심 구조체 복원: 게임 오브젝트, 플레이어, 섬, 다리, 타일/좌표계
- [ ] **게임 틱**: 고정 프레임 여부, 틱 레이트, 난수 생성기(결정론 확보에 필수)
- [ ] **좌표계·맵**: 아이소메트릭 투영, 타일 크기, 섬 형태, 높이/레이어, 그리기 순서(정렬 규칙)
- [ ] **경제**: 가이저(geyser) → Storm Power(게임 내 재화) 수집 흐름, 수집 유닛(collector) 이동, 자원 운반, 제단(altar)·희생, 비용
- [ ] **다리 건설**: 다리 조각 생성 규칙(모양 풀, 순서, `BridgeDrawRate`), 배치 판정, 연결·붕괴 조건
- [ ] **건물/유닛**: 배치 규칙, 건설 시간, 원소(Sun/Rain/Wind/Thunder) 별 기술 트리, 연구(기술 획득) 방식
- [ ] **전투**: 사거리·명중·피해 공식, 발사체 궤적, 특수 효과(`bomb*` 계열: 마비, 중력, 치유, 반역 등), 방어(차단벽·실드)
- [ ] **승패 조건**: 프리스트(priest) 사망/포획, 신전(temple) 파괴, 미션 스크립트 이벤트 발생 지점
- [ ] 미션 스크립트 인터프리터 동작(3단계 문법 명세와 교차 검증)
- [ ] AI 의사결정 루틴 (10단계 입력)
- [ ] 네트워크: 프로토콜 방식(TCP/IP, IPX), 동기화 모델(락스텝 여부), 패킷 형식 (13단계 입력)
- [ ] `PatchFixs.txt` 와 대조하여 **기준 버전** 결정 (원본 1.x 동작 vs 패치 10.7x 동작)

**완료 기준**: `docs/gameplay/` 만 보고도 게임 규칙을 재구현할 수 있다.

### 5단계. 원본 분석 — 플레이 영상

영상별로 `docs/videos/<이름>.md` 에 타임스탬프 기반 관찰 노트를 작성한다.
화면 구성은 홈페이지의 [게임 인터페이스](https://hjow.duckdns.org/netstorm/learninterface.htm) 설명과 대조해 UI 요소 이름을 정한다 (1.6절).
원본을 직접 실행할 수 있으므로(1.4절), 영상으로 확인하기 어려운 부분은 원본을 실행해 캡처·측정한다.

| 영상 | 주소 | 주요 관찰 항목 |
|---|---|---|
| 튜토리얼 | https://www.youtube.com/watch?v=CI3dCrUt4tY | 기본 조작, UI 레이아웃, 튜토리얼 메시지 흐름 |
| 캠페인 1-1~4 | https://www.youtube.com/watch?v=AMEzorbjQYQ | 초반 미션 흐름, 메뉴·브리핑 화면 |
| 캠페인 1-5 | https://www.youtube.com/watch?v=adR1Kap60hw | |
| 캠페인 1-6 | https://www.youtube.com/watch?v=PUeIPg1BEzo | |
| 캠페인 2-1 | https://www.youtube.com/watch?v=zpZsx4dRac8 | |
| 캠페인 2-2 | https://www.youtube.com/watch?v=2NxTN314RnE | |
| 캠페인 2-3 | https://www.youtube.com/watch?v=LHkgSp0J73E | |

- [ ] 화면 구성(HUD, 사이드바, 미니맵, 버튼 배치, 커서 종류) 캡처 및 레이아웃 좌표 기록
  - [x] 원본 캡처 1: 튜토리얼 "Bridge the Gap!" 시작 직후 — 2026-09-28 완료: [docs/screens/bridge-the-gap-start.md](docs/screens/bridge-the-gap-start.md)
    (클라이언트 1024×768 위치, 사이드바·미니맵 배치, 맵 데이터 대조. 처음의 칸 크기 가설 32×22px 은 캡처 2 로 정정됨)
  - [x] 원본 캡처 2: 캠페인 1-1 "The War Begins!" 시작 직후 — 2026-09-28 완료: [docs/screens/the-war-begins-start.md](docs/screens/the-war-begins-start.md)
    (**좌표계 확정**: 위치 바이트 상위=x·하위=y, 청크 i → (i%16, i/16), 미니맵 1px = 2칸, 칸 = 16 × 약 11.25px(→ 캡처 3 에서 16 × 11 로 정정). noIsland = 가이저 받침 바위, 섬 지형 테마 = 신전 원소 추정)
  - [x] 원본 캡처 3: 캠페인 1-3 "Save the Island!" 시작 직후 (유닛·다리·바리케이드 배치) — 2026-09-28 완료: [docs/screens/save-the-island-start.md](docs/screens/save-the-island-start.md)
    (**템플릿 매칭으로 정밀 확정**: 칸 = 정확히 16 × 11px, 모든 타입이 칸 좌표→화면 점에 스프라이트 기준점(0,0)을 그림, TerrNN 청크 = 영역 소속 청크를 y·x 순으로 훑은 것. Sun Barricade = 두 기둥 사이 광선, 미니맵은 다리도 소유자 색으로 표시)
- [ ] 메뉴 흐름도(타이틀 → 캠페인/멀티 → 브리핑 → 게임 → 결과)
- [ ] 애니메이션 속도·연출(건설, 다리 설치, 폭발, 승리/패배), 사운드·음악 재생 타이밍
- [ ] 수치 검증용 관찰(건설 시간, 공격 간격, 자원 증가 속도) → 4단계 결과와 대조
- [ ] 각 미션의 시작 상태·목표·스크립트 이벤트 발생 순서

**완료 기준**: 구현 결과를 영상과 나란히 비교할 체크리스트가 준비된다.

### 6단계. 자산 로더 / 개발용 뷰어 (C#, `Netstorm.Assets` + `Netstorm.Game`) — 🔶 진행 중

`src/Netstorm.Assets/` (MonoGame 비의존), 테스트 `tests/Netstorm.Assets.Tests/` (39개, 전부 통과)

- [x] `XorCipher` — TAFF·설정 파일 공용 XOR — 2026-09-27 완료
- [x] `TaffArchive` — 아카이브 읽기, 대소문자·구분자 무시 이름 검색 — 2026-09-27 완료
- [x] `Palette` — .COL(0x308)·RGBX(0x400) 팔레트 — 2026-09-27 완료
- [x] `ShapeDatabase` — 블록·프레임 헤더·RLE 디코딩(8bit 인덱스 + 투명 마스크) — 2026-09-27 완료. 3,692개 이미지 프레임 전부 디코딩 테스트
- [x] `TypeLoadOrder` — 타입 로딩 순서 116개 (셰이프 블록 대응) — 2026-09-27 완료
- [x] `TypeDefinition` — `.type` 파서(머리·플래그·속성·클러스터) — 2026-09-27 완료. "클러스터 × 레이어 = 프레임" 규칙 115/116 블록 검증 (예외 manabolt)
- [x] `ConfigFile` — `.cfg` 복호화·값 읽기/쓰기·재인코딩(원본과 바이트 동일) — 2026-09-27 완료
- [x] `TrueTypeCollection` — TTC 에서 face 분리 (D2Coding) — 2026-09-27 완료
- [x] `GameDataLocator` — 데이터 폴더 탐색 + 대소문자 무시 파일 찾기 (Linux 대비) — 2026-09-27 완료
- [ ] 가상 파일 시스템: 원본 폴더(`d/`) + TAFF 아카이브 통합 조회 (어느 쪽이 우선인지는 4단계에서 확인)
- [ ] 자산 경로 결정: 동봉 자산 폴더 우선 → 없으면 원본 설치 경로 탐지·선택 (필수 파일 검증 — Steam 설치본 포함)
- [x] `TypeCatalog` (타입 목록 + 플래그 계산, 이름 해시) · `FortFile` (.fort 섹션·청크·오브젝트·내용물) — 2026-09-27 완료. 원본 463개 전부 해석 테스트
- [x] `FortMap` (영역 패턴·저장된 오브젝트의 월드 좌표) — 2026-09-28 완료. 실행 파일에서 패턴 64개 추출·내장, 원본 463개 영역 청크 수와 Save the Island! 캡처 좌표 검증
- [x] 개발용 정적 맵 뷰어 (`--map 이름/경로`) — 2026-09-28 완료. 이동·확대·사제 중심·오브젝트 정보·PNG 캡처. [실행 방법과 제한](docs/map-viewer.md)
- [x] 저장된 다리 렌더링 — 2026-09-28 완료. bridge 값은 각 칸의 클러스터 번호임을 확인, `MapSpriteFrames` 구현, 원본 463개 파일의 다리 프레임 범위 전수 검사
- [x] 지면 미리보기 — 2026-09-28 완료. `FortTerrainPreview`: 청크 연결 통로·시드 성장·빈 틈 보정·원소별 원본 타일·작은 받침 표시. G로 청크 윤곽 전환. 두 공식 맵의 반복 생성·타일 유효성 검증
  - [x] 본섬 밀도 시드 수정·마스크 대조 — 2026-09-28 완료: 원본 어셈블리에서 청크별 밀도 시드 초기화 확인, 월드 y·x 처리 순서 반영. `IslandCells`·`tools/terrain_mask.py` 추가. 두 공식 맵의 전체 마스크가 독립 Python 재현 결과와 일치하며, 두 원본 캡처에 경계를 겹쳐 형태를 확인. 회귀 검사 2개 추가, 전체 39개 통과 ([수치·재현 명령](docs/exe/terrain-and-bridges.md))
  - [ ] 남은 일: 원본의 타일 변형·원소 선택·큰 타일·가장자리 fringe·하단 절벽·받침 생성·그림자·플레이어색. 원본 실행 중 마스크 메모리 대조·픽셀 단위 외관 검증은 아직 미완료 ([분석과 제한](docs/exe/terrain-and-bridges.md))
- [ ] 미션 스크립트 로더, 번역(`xlat`·`config.<언어>`) 로더
- [ ] 셰이프 헤더 bounds/origin 의 의미 확정 후 기준점 처리 정리 (현재는 xmin/ymin 오프셋만 사용)
- [ ] 개발용 뷰어: 스프라이트·애니메이션·팔레트·`.type` 속성 탐색 (현재는 2단계 확인 화면에 5개 애니메이션만 표시)

### 7단계. 엔진 코어 (플랫폼 계층)

- [ ] 창·전체화면·해상도 스케일링(MonoGame `GraphicsDeviceManager` + `RenderTarget2D`, 해상도 가변·레터박스), HiDPI
  - 전체화면 ↔ 창 모드 전환과 설정 저장 후 재실행이 안정적으로 동작해야 한다 (원본의 재실행 오류 재현 금지, 1.4절)
- [ ] 8bit 인덱스 → 팔레트 적용 방식 결정 및 구현 (CPU 변환 vs 팔레트 셰이더, 2절) — 그림자·색상 변환 테이블(`!color.dat`) 효과 대응
- [ ] 입력(마우스·키보드·단축키), 커서
- [ ] 오디오: `SoundEffect` 효과음 다중 재생, `DynamicSoundEffectInstance` 음악 스트리밍, 볼륨
- [ ] 고정 틱 게임 루프(시뮬레이션과 렌더 분리), 결정론적 난수
- [ ] 로깅, 설정 저장(사용자 데이터 경로: Windows `%APPDATA%`, Linux `$XDG_CONFIG_HOME`/`$XDG_DATA_HOME`)

### 8단계. 게임 월드 / 규칙 구현

4단계 명세를 기반으로 구현하며, 모든 규칙 코드는 단위 테스트를 둔다.

- [ ] 맵·섬·아이소메트릭 렌더링, 카메라 스크롤, 오브젝트 그리기 순서
- [ ] 엔티티 시스템 (`.type` 데이터 구동)
- [ ] 애니메이션 시스템
- [ ] 다리 조각 생성·배치·연결·붕괴
- [ ] 건물 배치·건설
- [ ] 경제(가이저, 수집, 운반, Storm Power)
- [ ] 이동 유닛(수집 유닛, 프리스트 등) 경로 탐색
- [ ] 전투·발사체·특수 효과
- [ ] 기술 획득·원소별 기술 트리
- [ ] 승패 판정

**완료 기준**: 스크립트 없이 1:1 스커미시(인간 vs 더미)가 끝까지 진행된다.

### 9단계. UI · 미션 스크립트 · 튜토리얼

- [ ] UI 위젯 프레임워크(버튼, 체크박스, 메뉴, 스크롤 텍스트, 대화상자)
- [ ] 인게임 HUD (5단계 레이아웃 기준)
  - UI 요소 이름·역할은 홈페이지 [게임 인터페이스](https://hjow.duckdns.org/netstorm/learninterface.htm)·[설치 및 시작](https://hjow.duckdns.org/netstorm/learninstall.htm)(메뉴·옵션) 참고
  - Storm Power(게임 내 재화) 표시: 충분하면 흰색, 부족해지기 시작하면 노란색, 더 부족하면 빨간색 (사용자 확인. 기준값은 실행 파일에서 확인 — [save-the-island-start.md](docs/screens/save-the-island-start.md))
- [ ] 미션 스크립트 파서(관대한 파싱: 대소문자 무시, 알려진 오타 허용, 경고 로그) 및 인터프리터
- [ ] HTML 부분집합 렌더러(`<h2>`, `<p>`, `<i>`, `<br>` …) — 한국어 줄바꿈(어절 단위) 지원
- [ ] 이벤트 섹션 트리거 연결 (`[Succeeded]`, `[Failed]`, `[aiNPriestDead]` 등)
- [ ] 메인 메뉴 / 설정 / 브리핑 / 결과 화면
- [ ] 튜토리얼 재현 → 튜토리얼 영상과 대조

### 10단계. AI

- [ ] 미션 헤더의 AI 파라미터 해석(`Tech`, `StartMoney`, `Collectors`, `GeyserAttachments`, `BridgeDrawRate`, `Ability` 플래그)
- [ ] 4단계에서 분석한 AI 루틴 재구현 (건설 순서, 다리 확장, 공격 대상 선택)
- [ ] 난이도·행동을 원본 미션에서 비교 검증

### 11단계. 캠페인 · 저장

- [ ] 캠페인 진행 상태 저장(`$Config` 명령 대응 — 완료 미션 기록 등)
- [ ] `.fort` 읽기/쓰기 (플레이어 요새 저장)
- [ ] 튜토리얼(Early Missions: Bridge the Gap! 등 6개)과 캠페인(Struggle For Freedom 이후)을 영상과 대조하며 순서대로 플레이 가능하게 만들기 — Early Missions 는 캠페인이 아니라 튜토리얼임 (2026-09-28 사용자 확인)

### 12단계. 다국어 지원 (설계는 2단계부터 반영)

- [ ] 모든 UI 문자열을 코드에서 분리 → 문자열 ID + 번역 파일(`locale/<언어>/`)
- [ ] 원본 문자열 소스 통합: `NSENGLISHRES.DLL`/exe 리소스, 도움말, 미션 스크립트
- [ ] 미션 스크립트 언어 선택 규칙: `<미션>.<언어>` 파일 우선 → 없으면 영어 폴백 (원본의 `.english`/`.german`/`.french` 규칙 확장, 예: `.korean`)
- [ ] 원본 텍스트 Windows-1252 → UTF-8 변환 로딩
- [ ] 한국어 글꼴 렌더링: `fonts/` 의 D2Coding TTC 로드(face 인덱스 0 일반 / 1 Bold), 폰트 폴백 체인(원본 `.chfnt` → D2Coding), 고정폭 글꼴에 맞춘 텍스트 박스 자동 크기 조정 (1.5절)
- [ ] 게임 내 언어 선택 메뉴 + OS 로케일 자동 감지
- [ ] **1차 지원 언어: 영어, 한국어** (AGENTS.md). 독일어 등 원본 제공 언어는 원본 파일이 있으므로 이후 확장 시 우선 추가
- [ ] 한국어 번역: UI 문자열, 도움말, 튜토리얼·캠페인 미션 스크립트(`.korean` 파일 신설)
  - 용어 표기는 한국어 플레이 방법 소개 홈페이지(1.6절)의 기존 표기를 참고해 정하고, 용어집(`locale/ko/` 등)으로 관리

### 13단계. 멀티플레이

- [ ] 결정론적 락스텝 모델 (명령만 전송, 동기화 검사용 체크섬)
- [ ] LAN 게임 (직접 IP 접속 / 로컬 브로드캐스트 탐색)
- [ ] 인터넷 게임: 로비 서버(별도 프로그램) — 후순위
- [ ] 원본 클라이언트와의 프로토콜 호환 여부 결정 (기본 방침: 호환 안 함, 분석 결과에 따라 재검토)
- [ ] 리플레이 저장(락스텝 명령 기록 재사용) — 디버깅·QA 에도 활용

### 14단계. 패키징 · 배포

- [ ] Windows 10/11 x64: `dotnet publish -r win-x64 --self-contained` 결과를 zip 또는 설치형으로 배포
- [ ] 원본 자산 동봉 배포 (재활용 가능, 1.3절) — 필요한 파일만 선별해 패키지 크기 최소화
- [ ] D2Coding 글꼴 동봉 + SIL OFL 1.1 라이선스 고지 포함
- [ ] (후순위) Linux: `dotnet publish -r linux-x64 --self-contained` + AppImage 우선(필요 시 Flatpak), X11·Wayland 양쪽 동작 확인
- [ ] 라이선스·면책 고지 문서

### 15단계. 검증 · QA (상시)

- [ ] 포맷 로더 회귀 테스트 (원본 전체 파일 파싱)
- [ ] 규칙 단위 테스트 (4단계 수치 기반)
- [ ] 영상 대조 체크리스트 통과 여부 기록
- [ ] 원본 직접 실행 결과(화면·수치)와 클론 비교
- [ ] 결정론 테스트: 동일 입력 리플레이 → 동일 체크섬 (Windows 먼저, Linux 지원 시 교차 검증)

---

## 5. 결정이 필요한 사항

1. ~~**기술 스택**~~ → **결정됨** (2026-09-27): C# + MonoGame (DesktopGL)
2. **기준 버전**: 원본 1997 동작 vs Ticonderoga 패치(10.7x, 보유 exe) 동작 — 보유 exe 는 패치판이므로 기본적으로 패치판 동작을 따르되, 차이점은 `PatchFixs.txt` 로 문서화
3. ~~**자산 정책**~~ → **결정됨** (AGENTS.md): 원본 파일 재활용 가능. 저장소에는 커밋하지 않고, 배포 시 필요한 자산을 동봉한다
4. **멀티플레이 범위**: LAN 만 / 인터넷 로비 포함 / 원본 호환

## 6. 바로 다음 작업

1. 4·6단계: 지면 미리보기를 원본 캡처와 대조하여 정확도를 높이기
   - 출발점: `Chunkmap.cpp`, `Islandbuilder.cpp`, `Terrainbuilder.cpp`, `Renderer.cpp`, 청크 크기 16×16 (Template.cpp `FUN_004be020`)
   - **작업 시작 전 참고**: [save-the-island-start.md](docs/screens/save-the-island-start.md) 4절(칸 16×11px·기준점 규칙·영역 청크 배치 — 템플릿 매칭으로 확정), [the-war-begins-start.md](docs/screens/the-war-begins-start.md) 4절(미니맵 대조), [bridge-the-gap-start.md](docs/screens/bridge-the-gap-start.md)
   - **완료(2026-09-28)**: 영역 청크 결정 규칙·맵 뷰어 기본 구현, `FortMap` 및 [영역 배치 분석](docs/exe/territory-layout.md), [맵 뷰어 실행](docs/map-viewer.md)
   - **추가 완료(2026-09-28)**: 저장된 다리 표시와 지면 미리보기 구현. [다리·지면 분석](docs/exe/terrain-and-bridges.md). 이전 추정 정정: 저장된 bridge 값은 각 칸의 프레임이며 VA 0x52f998은 새 조각 배치용 복합 패턴
   - **후속 완료(2026-09-28)**: 본섬 밀도 시드 오류 수정, 독립 재현 도구와 두 공식 맵의 전체 마스크 일치 검증, 원본 캡처의 섬 형태 대조. `extracted/terrain/*-overlay.png`와 수정 뷰어 PNG 생성, 테스트 39개 통과·솔루션 빌드 성공 (NuGet 취약성 조회 연결 실패 경고 NU1900 1개)
   - 남은 핵심: 가장자리 fringe·타일 변형·원소 선택·하단 절벽·그림자·작은 받침 생성, 일반 전투 섬 재배치, 카메라 원점. 원본 실행 중 마스크 메모리·픽셀 외관 비교는 미완료. 뷰어는 현재 미션 저장 위치 원점 (1,1)을 사용
   - 방향 주의: 원본 활성 영역은 모두 방향 0. 섬 생성과 크기 계산 함수의 방향 전달 방식 차이를 분석 문서에 기록했으며 회전된 파일의 동적 확인 필요
2. `.fort` 남은 섹션(`Territory` 외관·잔여 플래그, `Technology`, `Deck`, `State`) 해석 — `Template.cpp` 의 해당 읽기 함수(디컴파일 126,100~127,840행)
3. 6단계 나머지: 가상 파일 시스템(d/ + 아카이브, 원본 우선순위 확인), 미션 스크립트·번역 로더, 개발용 스프라이트 탐색 뷰어
4. 4단계: 메인 루프/틱, 다리 생성(`Deck.cpp`, `Bridge.cpp`), 경제 분석
5. 5단계(영상 분석)와 병행: 공식 캠페인 구성([mission-script.md](docs/formats/mission-script.md))과 영상 대응 확인
6. (원격 저장소가 생기면) CI 실제 실행 확인

### 참고: 3단계에서 만든 도구 사용 순서

[docs/formats/README.md](docs/formats/README.md) "추출 순서" 참고. 추출 결과(`extracted/`)는 git 에 포함되지 않으므로 새 환경에서는 다시 실행해야 한다.
