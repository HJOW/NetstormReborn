# Netstorm.Client — 8비트 논리 화면·팔레트·입력 큐 (R1-1)

2026-10-28, 작업 PC `VM-W11-Codex`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)의 결정 1·2·3에 따라 cpppj `src/client`의 화면 계층을 C#으로 옮긴 첫 단위다. MonoGame 호스트(R1-2)는 이번 범위가 아니다. 게임·창·디컴파일은 실행하지 않았고 `cpppj/`·원본·AGENTS.md는 수정하지 않았다. 대응표는 [PORT_MAP.md](../dotnetpj/PORT_MAP.md)·[PORT_MAP.tsv](../dotnetpj/PORT_MAP.tsv)다.

## 1. 만든 것

새 프로젝트 `dotnetpj/src/Netstorm.Client`(MonoGame·Win32 비의존, `Netstorm.Core` 참조)와 `dotnetpj/tests/Netstorm.Client.Tests`를 솔루션에 추가했다.

| C# 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Display/ScreenTypes.cs` | `Screen.h` | `ScreenRect`·`ScreenPoint`·`ScreenColor`(RGBQUAD 순서)·`PaletteColor`·`ScreenMode` 플래그 |
| `Display/GamePalette.cs` | `Screen.cpp` | 팔레트 파일(0x308 COL / 0x400 BGRX) 판독, 가장 가까운 색 검색 `PaletteSearch.FindPaletteColor` |
| `Display/ScreenGeometry.cs` | `Screen.cpp` | 모드 합법성(004a13d0), 사각형 자르기(00445290), 창 위치(004a4fe0) |
| `Display/ScreenPresentation.cs` | `ScreenPresentation.cpp/.h` | 화면 스크롤(4a1ad0), 현대 모드 정규화, 비율 유지 출력 사각형, 좌표·변경 사각형 역/정변환, 정수 최근접 확대 |
| `Display/ScreenResolutions.cs` | `ScreenResolutions.h` | 원본 4:3 세 항목 + 호환 세 항목 |
| `Display/ScreenPaletteApply.cs` | `ScreenPaletteApply.h` | 팔레트 clear 관문(004a4923..004a49a2)과 하위 장치 묶음 |
| `Display/Screen.cs` | `Screen.h/.cpp` | 행 간격이 있는 8비트 인덱스 버퍼, 현재·저장·논리 팔레트, 모드, 잠금 깊이, 자르기, FillRect/Clear/BlitBuffer/Update/Scroll, 호스트 출력 인터페이스 `IScreenOutput` |
| `Input/InputEvent.cs` | `InputEvent.cpp/.h` | 입력 코드 비트, 40칸 사건 큐(가득 차면 새 사건 버림, DEL→Ctrl+Backspace, 명령 사건) |

## 2. cpppj와 다르게 한 점과 이유

- **창·DIB·GDI 없음.** cpppj `ScreenSurface`·`Screen::Init`·`InitDibSection`의 Win32 호출은 옮기지 않았다. 대신 `byte[]` 버퍼와 `IScreenOutput.Present`(원본 BitBlt 자리)·`WaitVerticalBlank`(DwmFlush 자리)로 호스트에 넘긴다. 행 간격은 Win32 DIB와 같은 4바이트 정렬이다(폭 1366 → 1368). 플랫폼이 달라도 같은 픽셀이 나오게 하려는 결정 1의 적용이다.
- **`SetMode`는 모드 계산만 한다.** 창 스타일·위치·메뉴·커서 제한·활성 상태는 호스트(R1-2)의 몫이다. `PresentationRect`·`ToLogicalPoint`는 클라이언트 크기를 인자로 받는다.
- **`PaletteLoaded`는 로컬 필드다.** cpppj는 Client 전역 DWORD(00555e08)를 빌린다. R4에서 세션 전역에 연결한다.
- **`LoadPalette`는 색 표 계산을 뺐다.** `BuildPaletteColorTable`(원본 기본·표시·날씨 색 표)과 `BuildGumpShadeMaps`(돌 배경 명암 표)는 `PaletteColors`·`PaletteShade` 이식(R2)에서 연결한다. 저장 팔레트 채우기, clear 관문, `SetPalette(0,256,null,true)`는 cpppj와 같은 순서다.
- **`Unlock`은 잠금 깊이 0에서 예외를 던진다.** cpppj는 그 경우 널 표면을 참조한다. 같은 상황에서 조용히 진행하지 않으려는 방어다.
- **누락 하위 장치.** C++의 `std::bad_function_call`은 C#에서 `InvalidOperationException`이다. 연결하지 않은 장치에 도달하면 빈 성공으로 바꾸지 않는다는 cpppj 방침은 같다.

## 3. 검증

Netstorm.Client.Tests 33개(공통 매니페스트 검사 4개 포함), 전체 dotnetpj **1,384개(Assets 363 + Core 988 + Client 33) 통과**, Release 경고·오류 0.

| 검사 | 근거 | 범위 |
| --- | --- | --- |
| `NearestColorMatchesNativeBodies` | fixture `palettecolor-x86.tsv` | **원본 기계어 786행** 전부 같은 색 번호 |
| `ScrollMatchesNativeDibPixels` | fixture `screen-scroll-x86.tsv` | **원본 기계어 357행**, 144바이트(padding 포함) 전체 바이트 |
| `PaletteClearMatchesFirstPePrefixFixture` | fixture `screen-palette-clear-x86.tsv` | **원본 기계어 166관측**(정상 checkpoint 122), 순서·중단 원인·필드 13개·물리 64바이트·trace |
| `ScreenTests`의 모드·사각형·창 위치·비율 맞춤·역변환·확대·스크롤 | `cpppj/tests/ScreenTests.cpp`·`ScreenPresentationTests.cpp`·`ScreenScrollTests.cpp`의 입력과 기대값 | cpppj 단위 검사와 같은 값 |
| `InputQueueTests` 3개 | `cpppj/tests/ScreenTests.cpp`의 InputQueue 검사 | 순서·넘침·거르기·DEL·명령 코드 |
| `ScreenTests`의 `Screen` 인스턴스 검사 | **C# 쪽 계약**(원본 fixture 없음) | 행 간격, 모드 정규화, 잠금 깊이, FillRect 자르기, Update 알림, 팔레트 고정색, LoadPalette clear, BlitBuffer, 전체화면 좌표 변환 |

- 판별력: `PaletteSearch`의 동률 비교(`<`→`<=`)와 스크롤의 행 순서를 일부러 바꾸자 해당 fixture 검사가 실패했고 원래대로 되돌렸다.
- 최초 실패: `SetModeNormalizesAndGatesDrawing`의 마지막 단언은 작성 중 잘못 쓴 기대값(창 모드는 시차를 정규화로 꺼서 `true`가 맞다)이었다. 제품 결함이 아니라 테스트 오류라서 그 줄만 지웠고, 같은 사실은 `SetModeWindowedDropsParallax`가 검증한다.
- 연결 fixture 3개를 `cpp-fixture-manifest.tsv`에 `Client` 사용 프로젝트로 등록했다(`cpp_sync.py`와 `CppFixtureManifestTests`가 `Client`를 인정하도록 고쳤다). 반영 커밋(`f95a8db2`)은 올리지 않았다.

## 4. 남은 것

- **R1-2:** MonoGame 출력 호스트(인덱스→RGBA 텍스처, 정수 확대·검정 여백, 전체화면 창, 마우스 좌표 역변환, 입력 → `InputQueue`), `--ui-script`·`snapshot`·`cpp_frame_compare.py`.
- 이 단위는 화면 버퍼의 **규칙**을 옮긴 것이며 실제 화면에 나온 픽셀을 cpppj와 비교한 것이 아니다. 픽셀 비교는 R1-2의 캡처 도구가 생긴 뒤 한다.
- `Screen`의 GDI 팔레트(번개·날씨의 일시 `SetPalette` 호출 순서)와 `PaletteColors`/`PaletteShade`는 R2다.
