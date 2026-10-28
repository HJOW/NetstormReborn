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

## 5. R1-2 부분: 캡처·스크립트 도구와 오라클 확인 (같은 날)

이 단위에서 MonoGame 호스트 없이 가능한 R1-2 도구를 옮겼다. 호스트(R1-2a)는 아직 없다.

| 파일 | 내용 |
| --- | --- |
| `Display/ScreenCapture.cs` | 논리 화면 → RGBA → cpppj와 byte 동일한 32비트 BMP(122바이트 V4 머리, 위→아래, BGRA, sRGB). `Client::Capture`+`WriteBitmap`의 이식 |
| `Automation/UiScript.cs` | `--ui-script` 파서(프레임 양수·오름차순, 탭 구분)와 실행기. esc·key·keyup·pointer·좌표/라벨 클릭·snapshot 지원, report·refresh는 건너뛰며 기록, 그 밖의 동작은 명시 예외 |
| `dotnetpj/tools/cpp_frame_compare.py` | 두 BMP 비교(파일 바이트 → 다른 픽셀 수·영역·채널 최대 차이·목록·차이 PNG), `--dir`로 폴더 전체 비교. 종료 코드 0/1/2 |

**오라클 캡처 확인(VM-W11-Codex, 예외 시스템 `10.0.0.17`).** `extracted/dotnet-oracle-data/`에 `originals/` 사본(1,394파일)을 만들고 HEAD `0cc1d33d`로 빌드한 `NetstormCpp.exe`(SHA-256 `63DF9A10…`)를 실행했다.

```powershell
NetstormCpp.exe --run extracted\dotnet-oracle-data --window --no-audio --frames 34 `
  --set "SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0" `
  --ui-script main-menu.tsv --ui-report states.tsv      # main-menu.tsv: "30<TAB>snapshot<TAB>...main-menu-1024.bmp"
```

- 종료 코드 0, 두 번 실행한 `main-menu-1024.bmp`가 같은 SHA-256 `1229c3661d51f6d7…`(3,145,850바이트 = 122 + 1024×768×4). 프레임 30의 메인 메뉴는 원본의 "Not Validated" 시작 대화상자가 올라간 모습이며 R3의 픽셀 일치 목표 화면이다.
- 그 BMP의 앞 122바이트는 `CaptureAndScriptTests`의 명세 기대값(필드 명세에서 직접 쓴 바이트)과 byte 단위로 같다. 따라서 C# 작성기는 같은 화면 내용이면 같은 파일을 만든다. **화면 내용(픽셀)의 일치는 R2·R3 이후 확인하며 지금 주장하지 않는다.**
- 실행 전후 `originals/` 전체 파일의 SHA-256 집계가 같다(변경 0). 남은 프로세스 없음. 원시 파일은 `extracted/dotnet-oracle-captures/`(Git 제외)다.
- 이 PC의 PowerShell 5.1에서 `Start-Process -PassThru -NoNewWindow`는 종료 코드를 비워 두므로 `& exe args; $LASTEXITCODE`로 읽는다.

**검증:** Client 검사 45개(R1-1의 33개 + 캡처·스크립트 12개), `cpp_frame_compare.py`를 일치/차이/입력 오류/폴더 모드로 직접 실행해 종료 코드와 보고를 확인했다. 스크립트 단계는 cpppj 소스에서 옮긴 계약이며 cpppj의 같은 스크립트 결과와 직접 비교한 것은 아니다(호스트가 없다). 검사 작성 중 최초 실패 1건은 `AfterFrame` 없이 프레임을 건너뛴 테스트 오류였고, 앞 단계가 남으면 뒤 단계가 막히는 cpppj 동작을 `RunnerRejectsUnsupportedAndBlockedSteps`가 따로 검증한다.

## 6. R1-2a: MonoGame 출력 호스트 (같은 날, HJOW-Athlon)

`--run` 진입·창·입력 펌프·프레임 고리의 호스트 몫을 옮겼다. 기존 `Netstorm.Game`은 동결하고 별도 실행(`NetstormClientHost`)으로만 돈다.

| 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Netstorm.Client/Host/HostOptions.cs` | `src/app/main.cpp` (--run 인자) | --window/--fullscreen, --width/--height/--fps, --frames, --ui-script/--ui-report, --set 뭉치 해석. `SCREENW`·`SCREENH`·`maxFPS`는 바로 반영하고 나머지는 세션용으로 보관한다 |
| `Netstorm.Client/Host/ScreenTextureConverter.cs` | `src/client/Screen.cpp` (004a1800) | 변경 사각형의 인덱스를 현재 팔레트 RGBA로 푼다 |
| `Netstorm.Client/Host/HostPresentation.cs` | `ScreenPresentation.cpp` | 정수 최근접 맞춤 사각형(비율 유지, 1배율 이상, 가운데). 마우스 역변환과 그리기가 같은 값을 쓴다 |
| `Netstorm.Client/Host/HostInputMapper.cs` | `src/client/InputEvent.cpp` (창 프로시저) | 키(가상 키 위로 밈·수식·뗌 비트, F11 제외)·글자(아래 코드)·버튼 비트, 전체화면 커서 제한 판정 |
| `Netstorm.Client/Host/HostFrameDriver.cs` | `src/app/main.cpp` (--run 고리) | 화면·큐 소유, 프레임 번호, 스크립트 Before/After 호출. 따라잡기 Update 뒤에도 밀린 프레임 관측을 한 프레임씩 처리한다 |
| `Netstorm.Client.Host/ClientHostGame.cs` | `Screen.cpp`·`InputEvent.cpp` | Game 골격: F11 전환(borderless, 장치 모드 유지), 정수 배율 그리기(PointClamp), 마우스 역변환·제한, 키·글자·버튼 펌프, 프레임 제한 종료 |
| `Netstorm.Client.Host/MonoGameScreenOutput.cs` | `Screen.cpp` (004a1800) | `IScreenOutput` 구현: 변경 사각형만 텍스처에 부분 업로드 |
| `Netstorm.Client.Host/Program.cs` | `src/app/main.cpp` (--run 진입) | 별도 진입. 종료 코드 0(정상)·1(실행 오류)·2(인자 오류). 보고서는 실패해도 쓴다 |

- 창 모드는 논리 크기 1:1, 전체화면은 디스플레이 크기의 borderless(장치 모드를 바꾸지 않는다). 모드 전환은 저장 팔레트를 다시 적용한다(원본과 같은 순서).
- `Screen.ToLogicalPoint`가 쓰는 비율 맞춤 대신 호스트 정수 맞춤으로 마우스를 되돌린다. 그린 위치와 같은 사각형이므로 어긋나지 않는다. 창 모드에서는 커서를 가두지 않는다.
- `PORT_MAP.tsv`에 `port` 8개 행을 새로 추가했다(기대값을 C# 결과로 바꾸지 않는 이식이며 fixture 대조가 아니라 L1이 아니다).

**검증:** Release 경고·오류 0. Client 검사 56개(R1-2b까지 45개 + 호스트 11개) 통과, 전체 **1,441개(Assets 363 + Core 1022 + Client 56) 통과**.
검사 작성 중 변이 1건(변환 R/B 뒤바꿈)에 기대값 실패를 확인하고 원복했다.
실행 확인(HJOW-Athlon, 예외 시스템이라 별도 창 확인 없이 진행): `NetstormClientHost.dll --window --width 320 --height 200 --frames 5 --ui-script steps.tsv(3프레임 snapshot) --ui-report report.tsv` →
종료 0, BMP 256,122바이트(= 122 + 320×200×4), 빈 보고서. 인자 오류·없는 스크립트는 종료 2.
실행 중 찾은 결함 1건: 고정 시간 따라잡기로 Update가 몰리면 중간 프레임 snapshot이 건너뛰어졌다. `HostFrameDriver.EndFrame`이 밀린 프레임을 한 프레임씩 처리하도록 고치고 `FrameDriverDrainsSkippedFrames`로 고정했다.

## 7. R2a: 팔레트 3종과 LoadPalette 연결 (같은 날, HJOW-Athlon)

그리기 원시·팔레트·글꼴(R2)의 첫 묶음이다. R1에서 미룬 색 표 계산과 번개·날씨 일시 팔레트를 옮겼다.

| 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Display/PaletteColors.cs` | `PaletteColors.cpp/.h` (원본 004a4850) | 58개 검색 인자·기본 9색·날씨 별칭·호출 순서 색 표. CD판은 12~19번을 빼고 50개로 계산한다 |
| `Display/PaletteShade.cs` | `PaletteShade.cpp/.h` (원본 0043c010) | 밝음(229~245 제외)·어두움 목표 색의 번호 변환표. double 비교·float 최솟값 저장을 유지한다 |
| `Display/PaletteFlash.cs` | `PaletteFlash.cpp/.h` (원본 00470d40~00471820) | 번개 팔레트 프로세스. Init(읽기 2회·밝히기/목표 채우기)→Frame(시간 경과분 1씩 복구)→Cancel(저장 복구). 창 모드에서는 화면을 바꾸지 않는다 |
| `Display/Screen.cs` | `Screen.cpp` | `LoadPalette`에 판본 인자를 받고 파일 로더에서만 색 표·명암 표를 다시 계산한다. 일시 효과(SetPalette·번개)는 표를 바꾸지 않는다 |

- `PORT_MAP.tsv`에 `L1` 6개 행을 새로 추가하고 `Screen.cs` 행의 base를 올렸다. fixture 2개(`palettecolors` 24행·`paletteflash` 768행)를 연결했다.
- 명암 표는 대조 fixture가 없어 L1이 아니다. 기대값은 같은 알고리즘의 별도 Python 참조 구현으로 계산한 세 무작위 팔레트(768값)와 회색 자명 경우로 고정했다. Gump 그리기 L1(R3)에서 끝까지 대조한다.

**검증:** Release 경고·오류 0. Client 검사 66개(+10) 통과, 전체 **1,451개(Assets 363 + Core 1022 + Client 66) 통과**.
검사 작성 중 실패 3건은 모두 테스트의 오류였다(부분 팔레트 +start 채우기 누락·열 번호 밀림·리틀 엔디언 double 해석). hex는 리틀 엔디언 바이트 덤프이며 선행 0 바이트 위치가 낮은 자리다.
변이 1건(밝음 제외 시작 229→230)에 참조 기대값 실패를 확인하고 원복했다.

## 8. R2b: VFX 그리기 원시 (같은 날, HJOW-Athlon)

그리기 원시·팔레트·글꼴(R2)의 두 번째 묶음이다. VFX 셰이프 저장소·기본/mapped 그리기·일반 선을 옮겼다.

| 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Display/VfxDraw.cs` | `VFXDraw.cpp/.h` (원본 00401d92) | 셰이프 저장소(블록·프레임·RLE 풀기·Squid 전용 헤더)와 8비트 합성. 반환 0/-1/-2/-3/-4를 유지한다 |
| `Display/VfxMappedDraw.cs` | `VFXMappedDraw.cpp/.h` (원본 0040229f) | 불투명 픽셀만 256바이트 치환표로 바꾸는 그리기. 배경은 바꾸지 않는다 |
| `Display/VfxLine.cs` | `VFXLine.cpp/.h` (원본 004011f1, mode 0) | 단색 8비트 선. 클립·양 끝 포함·0.32 분수 위상·DIV 역산을 유지한다 |

- `PORT_MAP.tsv`에 6개 행을 새로 추가했다(`VfxDraw.cs` 2행은 L1, `VfxMappedDraw.cs`·`VfxLine.cs` 4행은 대조 fixture가 없어 port). fixture `graphics` 709행을 연결했다.
- `VfxLine`의 기대 픽셀은 같은 알고리즘의 별도 Python 참조 구현으로 계산한 10가지 경우이며 L1이 아니다.
  mapped 그리기는 기본 그리기와의 관계(불투명만 치환·배경 유지)로 고정했다.
- `VFXTransform`(회전 없는 16.16 확대·축소)은 `Renderer`·`ColorMap`을 빌려야 하므로 색 표 묶음(R2c)에서 잇는다.
- `fence-line` fixture는 `o/RawFenceLine` 월드 모듈 대조용이라 이번에 연결하지 않았다(R4·R6 몫).

**검증:** Release 경고·오류 0. Client 검사 72개(+6) 통과, 전체 **1,457개(Assets 363 + Core 1022 + Client 72) 통과**.
검사 작성 중 실패 1건은 테스트의 오류였고(거리 상한의 등호 착각, C++ 조건 `ax > INT32_MAX`에 맞게 수정) 소스 수정은 없었다.
변이 1건(투명 마스크 무시)에 L1 기대값 실패를 확인하고 원복했다.

## 9. R2c: 색 표 (같은 날, HJOW-Athlon)

그리기 원시·팔레트·글꼴(R2)의 세 번째 묶음이다. 스프라이트·커서·생산·소유자 색 표와 ColorSort primary를 옮겼다.

| 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Original/RawColorSortPrimary.cs` | `o/RawColorSortPrimary.cpp/.h` (원본 00439de0) | raw 56바이트 × 256 기록의 H/L/S 계산과 명도 읽기. 관측 경계(checkpoint)를 그대로 둔다 |
| `Display/SpriteColorMaps.cs` | `SpriteColorMaps.cpp/.h` (원본 0043c010) | 밝음·건설(명암 재사용)과 제곱 거리 그림자 표. 228~245·254·255 항등 덮개를 유지한다 |
| `Display/PreviewColorMaps.cs` | `PreviewColorMaps.cpp/.h` | 커서 파괴(빨강 채움)·불법(빨강 2.1배)·글자 그림자(0) 표 |
| `Display/ProductionColorMaps.cs` | `ProductionColorMaps.cpp/.h` | 생산 아이콘 비활성(빨강 2.1배)·선행(0) 표 |
| `Display/OwnerColorMaps.cs` | `OwnerColorMaps.cpp/.h` (원본 0043b950·0043ba00·0043b750) | 표준 9표(228~245 띠·다리 색 2·7 특례)와 전용 5종(111·141·137·94·97) 표. 명도 읽기는 논리 adapter와 primary 두 경로를 둔다 |

- `PORT_MAP.tsv`에 `L1` 10개 행을 새로 추가했다. fixture 5개(`sprite-colors`·`preview-colors`·`production-colors` 각 3행, `owner-hsl-colors` 15행, `owner-capture` M행 18개)를 연결했다.
- `owner-capture`의 C행 480개(세션 자산·SHP 필요)와 `SpriteDraw` 클래스·그림자 alias 세션 연결은 R4·R5에서 잇는다.
- `owner-hsl` 대조는 논리 adapter와 primary 두 경로가 같은 기대값과 일치해야 하며, 64비트 별도 관측이 다른 행이 있어 검사가 헛돌지 않음을 확인한다.

**검증:** Release 경고·오류 0. Client 검사 77개(+5)·Core 검사 1025개(+3) 통과, 전체 **1,465개(Assets 363 + Core 1025 + Client 77) 통과**.
변이 1건(그림자 어두움 곱 0.6000000238418579→0.61)에 L1 기대값 실패를 확인하고 원복했다.
