# 10.78 미니맵의 음수 crop과 실제 raw 복사 경계

2026-10-23, 현재 호스트와 마지막 전체 refined 디컴파일 호스트는 모두 `VM-W11-Codex`다. `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md` 및 `reverse-engineer-anything` 지침을 재확인했다. 등록 제품 C++/CMake, 보호 원본, 기존 독립 자료를 변경하지 않았다. 실제 게임/GUI/빌드/새 디컴파일/커밋은 실행하지 않았다.

이 조사의 결론은 **음수 X crop 자체는 잘못된 주소가 아니며, 실제 flat BYTE 주소를 대상 clip 뒤에 판정해야 한다**는 것이다. 동시에 원본이 bitmap 할당 앞뒤를 실제 읽을 수 있다는 별도의 경계가 있다. 그 인접 BYTE 값은 이번 격리 관측에서 실제 게임 heap 값으로 확인하지 않았다.

## 원문 주소와 정적 연결

| 역할 | 원문 주소 | refined C 행 및 현재 제품 연결 |
|---|---|---|
| 부착 시 상한만 crop 보정 | `004805e0` | `cpppj/src/client/MinimapGeometry.cpp:54`, `RawMinimapControls.cpp:79`. 폭130과 crop0이면 X를-2로 만든다. 음수 crop을0으로 재보정하지 않는다. |
| 자동 카메라 추적 | `00480270` | `MinimapGeometry.cpp:33`. `scrolling!=0`이면 생략한다. 폭128 초과 시 max(0,min(candidate,128-width)) 결과는0이다. Attach와 같은 공식으로 합치면 안 된다. |
| 전체 Draw | `00480660` | `extracted/refined/originals/Netstorm.c:103923`. Bounds→ClipGump→raw blit→카메라 frame→clip 복구다. |
| 실제 소스 포인터 | `00480720`~`00480733` | refined C `103950`. `imul`→cropX 덧셈→bitmap 덧셈→push→blit CALL이며 `bitmap + cropY*stride + cropX`를 이미 이동한 포인터로 넘긴다. `005949c4`는 bitmap 주소, `005949cc`는128 stride다. |
| 임시 Gump clip/복구 | `00464200` / `00464290` | refined C `81455` / `81480`. 현재 화면 clip과 전체 Gump Bounds를 교차하고 복구한다. |
| 사각형 clamp | `00445290` | refined C `56615`. 네 좌표를 각각 주어진 경계로 제한한다. |
| 화면 clip 전역 쓰기 | `004a1580` | refined C `128131`. `005c7a44/48/4c/50` half-open과 `005b5de0/e4/e8/ec` inclusive를 함께 쓴다. |
| 화면 wrapper→실제 복사 | `004a1fe0`→`004a0c10` | refined C `128669`→`127657`. 목적지 clip/화면 크기만 알고 소스의128×128 크기는 받지 않는다. |
| 외곽선 | `004a24b0`→`004dade0`→`004011f1` | refined C `128892`→`170934`. right/bottom에서1을 빼고 위/아래/왼쪽/오른쪽 선을 그린다. 원문 VFX 선 본체까지 실제 실행했다. |
| bitmap 실제 할당 | `00481320` | refined C `104399`~`104410`. `new(0x4000)`, stride128, 정확히0x4000BYTE를0으로 초기화한다. 앞뒤 guard는 이 할당에 포함되지 않는다. |

소스 포인터 산술은 메모리 한 덩어리의 주소다. `cropX=-2,cropY=1`은 offset126이며 앞 행 마지막2BYTE 다음으로 이어진다. X를0으로 clamp하거나 매 행을128 경계에서 잘라 버리면 원문과 다른 그림이다. `cropX=-2,cropY=0`은 offset-2이므로 첫 행에서 실제 할당 앞2BYTE를 읽는다. 대상의 왼쪽 또는 위쪽 clip이 그 픽셀을 제거하면 원문도 해당 BYTE를 읽지 않는다.

## 독립 PE 실행

도구는 `tools/decomp_minimap_blit.py`, fixture는 `cpppj/tests/fixtures/minimap-blit-x86.tsv`, 상세 보고서는 `extracted/cpp-minimap-blit-20261023-native.json`이다.

- 고유 입력60개, 두 x87 제어어 `0x27f/0x37f`, 총120행이다. 실제648124명령을 실행했으며 정밀도 차이는0이다.
- 전체 `00480660`부터 정상 RET까지120회, 실제 `004a0c10`120회, 실제 VFX 선 `004011f1`480회를 실행했다. 실제 clip 복구도120회다.
- 정상 RET, ESP, SEH `FS:0`, EBX/ESI/EDI/EBP, x87 제어어와 TOP0, 실행한 모든 본문 바이트 불변, 대상 앞뒤32BYTE 보호값을 확인했다.
- 가로·세로 `(80,75),(128,128),(130,75),(130,130),(160,140)`, 음수 crop, 정상 flat offset, 화면/창 clip, 소수 크기, 화면 밖 목적지, 앞뒤 guard 입력을 교차했다. 임의 source clamp나 래스터 기대값 모델은 쓰지 않았다.
- 읽기 hook은 실제 소스 read BYTE 주소 집합을 관측했다.40행에서 실제 할당 밖 read가 있었다. `guardReads`는 CPU 메모리 read 명령의 건수이며 BYTE 수와 다르다.
- `Screen.type=1`, 이미 locked인 명시8BIT DIB 표면을 사용했다. DirectDraw IsLost/restore는 제외다. 원문 `004648e0` Bounds만 명시 destination 및 현재 width/height의 절삭값을 반환하는 장치다. Gump 전체 위치 캐시·부모 좌표·실제 프레임 Refresh 순서는 이 관측의 범위가 아니다.

fixture 앞24열은 다음 입력이다.

```text
name control widthBits heightBits cropX cropY destX destY
clipLeft clipTop clipRight clipBottom cameraXBits cameraYBits
viewLeft viewTop viewWidth viewHeight pixelsX pixelsY
bitmapSeed guardSeed background ink
```

뒤9열은 `completed changedRuns readRanges guardReads blit frame clipout instructions stackdelta`다. 실제 TSV에서는 탭으로 구분한다. 대상 표면은192×160이며 `changedRuns`의 `시작offset:hexBYTE;...`를 입력 background 위에 적용하면 전체 표면을 무손실 복원한다. `readRanges`는 bitmap 기준 signed BYTE offset과 길이다. `blit`은 대상X:Y:bitmap기준포인터:stride:sourceLeft:sourceTop:sourceRight:sourceBottom이며 `frame`은 left:top:right:bottom:ink다. `clipout`은 정상 복구 뒤의 half-open clip이다.

`bitmapSeed`와 `guardSeed`는 raw 입력 메모리 패턴의 값이다. 패턴은 `(offset*73 + floor(offset/128)*19 + seed)&255`다. 이것은 소스 입력을 만드는 공식이며 결과 blit·clip·선 픽셀의 기대값을 계산하는 공식이 아니다. 명시 guard는 앞뒤0x8000BYTE이며 **실제 게임 heap의 관측값이 아니다.**

관측 예시는 다음과 같다.

| 입력 | 실제 source read / 실제 대상 결과 |
|---|---|
| `cross-016`, 폭130×75, crop(-2,0), 목적지(8,7), 전체 clip | 실제 source `[-2,9598)`, blit 크기128×75다. |
| `guard-043`, 같은 크기, crop(-2,1) | 실제 source `[126,9726)`, guard read0. 첫 대상2BYTE는13,86이다. X<0만으로 거부해서는 안 된다. |
| `cross-017`, 같은 음수 crop이나 clip(11,12,101,82) | 첫 실제 source641부터 행별90BYTE를 읽으며 guard read0이다. 보이지 않는 첫2BYTE를 선행 거부하면 원문보다 엄격하다. |
| `guard-040/041/042`, bitmap 동일·guardSeed0/1/255 | 대상(8,7),(9,7)은 각각91/164,92/165,90/163이다. 인접 메모리 값에 따라 실제 픽셀이 바뀜을 관측했다. |
| `edge-057`, 폭130×75, 목적지(-2,-1) | blit 오른쪽126 밖인 대상(126,64),(127,64)은 원문 outline 색155다. 그 다음 행(126,65)은 기존 배경211이다. |
| `edge-059`, 폭160×140, 목적지(8,7) | blit 오른쪽136 밖인(136,72),(150,80)은 원문 outline 색155다. (167,80)은 기존 배경211을 유지한다. |

## 제품 수정 제안과 검사 준비

현행 `RawMinimapControls.cpp:36`의2차원 crop 거부는 정상 flat source까지 거부한다. 같은 파일42행은 outline을 min(128,width) 크기의 임시 IndexedImage에 먼저 그리므로 더 넓은 창의 outline을 잃는다. 원문은 bitmap 복사와 전체 창 outline을 별도로 실행한다.

`extracted/cpp-minimap-blit-proposal-20261023.patch`는 **미적용 제안**이다. `git apply --check`만 성공했고 등록 제품은 바꾸지 않았다. 제안의 핵심은 다음과 같다.

1. `GumpPixelTarget::RawBitmap`이 현재 화면∩전체 Gump∩블릿 목적지를 먼저 제한한다. 보이는 픽셀만 sourceBase+(y-destY)*128+(x-destX)의 flat BYTE 주소로 읽는다.
2. `LinesClipped`가 같은 표면에 전체 Gump clip으로 외곽선을 별도 출력한다. bitmap 크기128 밖의 여백은 Fill하지 않아 기존 배경을 유지한다.
3. source span 안의 정상 flat 주소는 그대로 읽는다. source span 밖은 명시 `outside(offset)` 장치로만 받는다. 그 장치가 없으면 실제 보이는 미확인 read만 명시 거부한다. crop, 카메라, 블릿 크기는 바꾸지 않는다.
4. RawMinimapControls를 이 둘로 바꾸는 연결도 patch에 있다. 다만 현재 연결안에는 실제 heap guard 장치가 없으므로 일부 상태의 명시 거부는 남는다. 이 patch만 적용해서 전체 dock의 동일성 완료를 주장하면 안 된다.

새 `cpppj/tests/MinimapBlitTests.cpp`는 제안 patch의 두 API를 전제로 한 **미등록·미빌드·미실행 검사**다. `NETSTORM_MINIMAP_BLIT_FIXTURE`가 위 TSV를 가리키도록 다음 적용 단위에서 등록한다. 앞24열만 입력으로 사용하고, 실제 전체120개 표면과 모든 할당 밖 BYTE 주소 집합을 대조한다. native의 guard read 명령 횟수를 제품 callback BYTE 수로 비교하지 않는다. outline이128 밖에 남는 픽셀과 기존 배경 보존을 전체 표면 비교에 포함한다. 현행 등록 제품 검사 통과 수에 더하지 않았다.

## 확인 완료와 남은 경계

- 확인 완료: 음수 crop의 flat 주소 의미, 대상 clip 뒤 source 접근, 행 경계 넘김,128을 넘는 창의 배경 보존, 전체 창에 그리는 outline, 현재 DIB8BIT 경로의 실제 픽셀.
- 부분 확인: 실제 법적 dock 폭130의 Attach가 crop-2를 만드는 정적 경로는 이미 복원되어 있다. Rebuild는 width>128에서 crop0을 쓴다. 실제 Attach/Idle/Refresh/Draw 순서와 capture 동안 어떤 상태가 표시되는지는 이 격리 Draw 관측으로 단정하지 않는다.
- 미확인: 원본 `new(0x4000)` 앞뒤의 실제 allocator/이웃 할당 BYTE. black/0/기존 배경으로 정해진다고 볼 근거가 없다. 원본 game heap의 입력 없이 인접 BYTE까지 동일하게 재현할 수 있다고 주장하지 않는다. 원본 할당 밖 접근을 host C++의 범위 밖 포인터 읽기로 직접 옮기는 것도 수정안이 아니다.

보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`다. 도구 SHA256은 `e330b7aa19f16d8403c24a4553bef86b9d0ec8f12cfbe5456c1480abcc0c55d8`, TSV SHA256은 `7b6eec53b0d69a138b41d7d438bc04495b5b4b1e7ee438d96194fed2930497b3`다. 실제 명령 범위별 SHA와 실행 수는 native JSON에 있다. 이 결과는 신규 게임 실행0회, 신규 디컴파일0회의 격리 x86 관측이다.
