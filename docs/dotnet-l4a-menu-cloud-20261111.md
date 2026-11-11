# dotnetpj L4a 첫 비교 — 메인 메뉴 화면과 구름(Kencloud) 이식 (0.380~0.381, 2026-11-11)

dotnetpj 새 경로(`Netstorm.Client` + `Netstorm.Client.Host`)의 화면이 cpppj 오라클과 같은지를 **같은 ui-script의 논리 캡처(L4a)**로 처음 비교했고, 드러난 차이 둘을 모두 닫았다. 이 문서는 비교 방법, 차이의 원인, Kencloud 이식의 구조와 C# 모양 차이를 한곳에 둔다. 단위 기록은 [LEFT_JOBS.dotnetpj.md 8절](../LEFT_JOBS.dotnetpj.md#dotnet-unit-records)의 0.380·0.381이다.

## 1. 비교 방법

- 도구: [`dotnetpj/tools/l4a_compare.ps1`](../dotnetpj/tools/l4a_compare.ps1). 같은 steps 템플릿(`{OUT}` 자리표시자)을 cpppj `Netstorm.exe --run`과 `Netstorm.Client.Host.exe`에 각각 넣어 `extracted/dotnet-oracle-captures/<이름>/cpp`·`dn`에 BMP를 만들고 `cpp_frame_compare.py --dir`로 모두 비교한다. 종료 코드 0 일치·1 차이·2 오류.
- 조건: 1024×768, `--fixed-frame-ms 25`, `--set "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0"`, 양쪽 모두 빈 사용자 루트(`NETSTORM_USER_DATA`), 같은 데이터 사본 `extracted/dotnet-oracle-data/`(`originals/` 1,394파일 + 클론 `cursors/`). 실행 전후 `originals/` 해시 목록이 같아야 한다.
- 오라클: `cpppj/build/bin/Release/Netstorm.exe`(SHA-256 `09042F35…E996`, cpppj `src` 마지막 커밋 `5b4b80874` 제품 1301과 같은 빌드).
- 첫 장면 [`l4a/menu-main.tsv`](../dotnetpj/tools/l4a/menu-main.tsv): 프레임 10·40·120의 메인 메뉴. 세 프레임은 서로 바이트 동일이다(메뉴 구간에서 화면이 정지).

## 2. 드러난 차이 둘과 원인

첫 비교 결과: **38,793픽셀(4.93%) 다름.**

| 번호 | 차이 | 원인 | 처리 |
| --- | --- | --- | --- |
| 1 | 버튼 8개 모두 아래 테두리가 한 줄 위(3행 폭, 채널 최대 차이 145) | cpppj는 컨트롤 영역을 `{x, y, x+75, y+kButtonHeight(19)+1}`로 만들고(`UberGump.cpp` 2019행) 돌 버튼을 그 크기 **75×20**으로 구성한다(`RawButtonControls.cpp` 294행). `HostMainMenu`는 75×19였다 | `HostMainMenu`를 75×20으로 교정(0.380). 라벨 클릭 좌표도 영역 중심 (393,321)이 맞다 |
| 2 | 화면 바깥(GIF 밖) 배경 구름에서 3행짜리 띠 24개가 타일에서 1픽셀 왼쪽으로 어긋남(37,054픽셀, 채널 최대 차이 17). 타이틀 GIF가 639×480이라 640×480 영역의 마지막 열 x=831도 구름이다 | Kencloud 미이식. cpppj는 팔레트 전환이 구름 파일을 읽고(`BuildKencloud`) 한 번 이동마다 구름 캐시를 `phaseRows[phase]`로 시작하는 16행 간격에서 989바이트(약 1.93행)씩 왼쪽으로 1바이트 민다(`StepKencloud`). `cloudSpeed`가 2라 두 번 이동한다. dotnetpj는 이 단계 없이 512×512 타일을 원점 0에서 반복했다 | Kencloud를 이식하고 팔레트 전환을 연결(0.381). 차이 0 |

원인 2의 단서: 어긋난 행의 간격이 31행 가까이였고(989×16÷512 ≈ 30.9), 행 번호가 `phaseRows[1]=8`→15행(0x3dd×8÷512)과 `phaseRows[2]=4`→7행에 정확히 맞았다. 어긋난 행에 대해 dotnetpj 행을 512 감김으로 −1 이동하면 cpppj 행과 일치함을 확인했다.

## 3. cpppj 메인 메뉴 진입의 구름 순서

1. 시작: `ClientMain.cpp` 376행이 `fortPal` 팔레트를 직접 읽는다(`LoadNativePaletteFile(…, clear 1)`). 구름은 아직 없다.
2. 메인 메뉴 상태(State 4) 진입에서 `Client::ChangeFortPalette(fortPal)`(004b6100) → `TransitionFortPalette`: 이름 비교(다르면) → 이름 복사 → 구름 캐시 반납 → `BuildKencloud`(구름 파일 읽기 + 게임 난수 801번: seed 400개×2 + spacer 1) → `cloudSpeed`번 `StepKencloud` → `GamePalSpec` 팔레트 읽기(clear 1) → 색 표 → dirty.
3. 화면: 렌더러 앞단이 전체 표시 요청을 하고 `DrawKencloud`가 카메라 원점 0으로 512 타일을 전체 복사한 뒤 `CleanKencloudDirty`로 정리한다. 구름은 이후 움직이지 않는다(이동은 이 전환의 두 번뿐).

## 4. 이식 구조

| cpppj | dotnetpj | 상태 | 검증 |
| --- | --- | --- | --- |
| `o/RawKencloud.cpp/.h` | `Netstorm.Core/Original/RawKencloud.cs` | L1 | `kencloud-v2-x86.tsv` 246행(정상 208·부분 중단 38) 전수, 같은 난수·전체 할당 3개·인접 전역·파일 scratch·호출 순서 |
| `o/RawKencloudRender.cpp/.h` | `Netstorm.Core/Original/RawKencloudRender.cs` | L1 | `kencloud-render-x86.tsv` 944행 전수(상태 FNV·clip·속도·반환·trace), 자연 생략·물리 접두 |
| `client/ClientKencloudGlobals.cpp/.h` | `Netstorm.Client/Display/ClientKencloudGlobals.cs` | port | 물리 전역·주소 수명·실제 파일·fallback·옵션·팔레트 전환·타일·부분 표시 10건 |
| `client/ClientMain.cpp`(`ChangeFortPalette`) | `Netstorm.Client/Host/HostStartupScene.cs` | 부분 | 메인 메뉴 L4a 3프레임 차이 0(BMP 바이트 동일) |

C# 모양 차이(한국어 주석에도 적었다):

- 전역 참조 묶음(`uint32_t&` 여러 개)은 `KencloudGlobals` 클래스가 같은 배열(`State` 2054칸·`Options` 8칸·`PhaseRows` 16칸)을 빌려 속성으로 읽고 쓴다.
- 구름 이름 버퍼는 연속 전역 뒤쪽(8112바이트부터) 별칭이라 `MemoryManager<byte>` 파생 관리자로 같은 배열을 바이트로 다시 본다.
- cpppj의 `CurrentAllocationBytes`(미초기화 힙 내용 복사)는 결정적이지 않아 기본을 0으로 둔다(생성자 인자로 바꿀 수 있다).
- 구름 파일 읽기는 cpppj `ReadBackgroundGifFile`(입력 길이 선계산·치수 선행 쓰기·Decode 중 파일 수명) 대신 `GameFileSystem` 전체 읽기 → `GifImage.DecodeBackgroundGif`다. 정상 경로의 색 번호·512 판정·반납 순서는 같고 부분 실패의 중간 효과만 다르다.
- 팔레트 전환의 색 표 갱신(`RebuildColorSortMaps`·`RefreshPaletteConsumers`)은 이 호스트의 `Screen`이 팔레트 읽기 때 직접 계산하므로 비운다. 진단 로그(`Palette 29 init palette`)는 호스트 로그 장치가 없어 남기지 않는다.
- alternate 표시(004a15d0/1650 parallax DirectDraw 장치)는 cpppj도 미복원이라 같은 지점에서 명시 예외다.

## 5. 남은 것

- `Renderer` 연결(R5): 렌더러 존재 토큰(`RendererToken`)·월드 전체 다시 그리기(`WorldRedraw`)·`BindRenderer`는 값만 둔다. 카메라 이동·시차·`PrepareRendererFrame`의 행 스캔 갱신 호출은 R5에서 옮긴다.
- 새 화면(Help·Options·Credits·Campaign 등 하위 메뉴) L4a 장면 추가는 다음 단위부터 `l4a/*.tsv`로 늘린다.
- 호스트 게임 난수의 시작 상태(cpppj는 시간 기반 재시드)와 `session-dump`의 `random` 줄 비교(L2)는 세션 조립(0.4 2단계)에서 한다. 구름이 소비한 801번은 같은 `GameRandom` 객체를 쓰므로 소비량은 같다.
