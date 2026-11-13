# webpj RawCampaignParticles·ParticlePaletteMemory — 입자 표시 본체 (WB04-E390, 2026-11-13, 5-A 열아홉 번째 단위)

5-A 의 시각 효과 마지막 부품인 client **RawCampaignParticles**(cpppj `client/RawCampaignParticles.cpp/.h`, 기준선 513줄)와 그 색 표 저장소 **ParticlePaletteMemory**(`client/ParticlePaletteMemory.cpp/.h`, 65줄, adapt)를 각각 `render/raw-campaign-particles.js`·`render/particle-palette-memory.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 이미 이식된 `core/` 의 아홉 표시 시스템(불꽃 꼬리·줄기·번개 빔·연기·악마 광선·파편·분해·연소·건설, E195~E201)을 **한 캠페인의 한 풀/Host/Kernel 과 공통 표시 목록(`RenderProcessSystem`) 위에 묶고**, 색 표 메모리·dirty 통지·VFX 선/타원/픽셀 출력·위치 음향·카메라·현재 TYPE 전역을 클라이언트 경계로 연결한다. 게임 계산은 바꾸지 않는다.

## 1. 규칙

- **구성:** `new RawCampaignParticles(core, random, palette, { sound, shrapnel, adjacent })`. `core` 는 `RawSurfaceWorld.core()`(풀·TYPE·SHP·Host·이웃 탐색), `palette` 는 `GamePalette`. `shrapnel`(더미 생성/Pop/의존/캡처 장치 + 부착 전 삭제 + 공유 CRT)이 있으면 파편(16)·분해(18)가 생기고 CRT 가 없으면 거부한다. 불꽃·빔·줄기·연기·악마 광선은 항상 만든다. 건설(17)·연소(14)는 `connectConstruction`/`connectBurning` 으로 한 번 연결한 뒤에만 쓴다(미연결·중복·함수가 아닌 경계는 거부).
- **공유 상태:** 모든 시스템이 같은 프레임 상태(`now`·배율 16/11·`cameraX/Y`)를 쓴다. 줄기(12)는 빔과 같은 `0055a758` 공유 색 객체를 쓰므로 빔 수명 상태의 팔레트 토큰을 별칭 상자로 가리키고, 줄기 첫 생성이 빔 색 표를 먼저 만들며 `addStreak` 은 그 메모리를 현재 색 읽기 뷰로 받는다. 색 표 토큰은 불꽃 1·빔 2·연기 3·악마 광선 4이고 건설(3)·연소(4)는 각 시스템 안의 별도 토큰이다.
- **색 표:** 불꽃/빔/연기/악마 광선/건설/연소 각자의 `ParticlePaletteMemory` 를 갖고, 팔레트 256색(`GamePalette.logical`)에서 원본 색 띠(`buildFieryTailPalette`·`buildLightningPalette`·`buildSmokePalette`·`buildEvilRayPalette`·`buildParticleGradient`+`mergeParticleGradients`)로 만든다. **매 Draw 마다 현재 메모리의 읽기 뷰를 다시 받는다**(`색수 + 인접 DWORD`).
- **출력:** `sprites(clip)` 는 공통 목록을 수집(`{ left/top/right/bottom }` 월드 사각형 = `(clip + 카메라 원점) ÷ 16/11`)해 항목마다 `{ database: null, clip, order: { x, y, depth }, procedure: { bounds, draw } }` 를 만든다. `bounds` 는 `renderProcessRemovalBounds`(공통 삭제 영역), `order` 는 수집 항목의 float x/y(부모 raw +0.001)와 signed 깊이로 종류별 정렬을 하지 않는다. `draw(pixels, pitch, height, cut)` 는 그 Draw 동안만 출력 대상을 잡고(중첩 거부, 예외 뒤에도 해제) 토큰의 현재 생존 프로세스를 그린다.
  - VFX 선(`drawVfxLinePixels`)은 불꽃/빔/연기, 타원(`drawVfxEllipse`)은 악마 광선(원문 pane `0,0,639,479` 닫힌 구간을 반개방으로 바꿔 강제), 픽셀은 파편/분해(현재 clip 밖은 건너뛰고 clip 안인데 표면 밖이면 경계 불일치), 건설은 캡처 사각형을 clip 안에서만 복사(255 는 투명), 연소는 현재 dirty clip 의 5점.
- **dirty:** 통지는 `display.invalidate(rect, flags)` 로 가고, 장치가 없으면 100항목 `DirtyRegions` 대기표에 쌓았다가 `setView` 에서 순서(플래그 포함)대로 전달한다. 카메라 원점/표시 영역이 바뀌면 영역 전체 한 건을 요청하고 같은 view 호출은 대기분만 전달한다. `bindCameraView(CameraView)` 를 빌리면 카메라 정수 픽셀은 그 snapshot 이 소유하고 fallback 원점은 쓰지 않으며, 프레임 카메라 float 은 snapshot 의 56/60 float 이다.
- **생성 래퍼:** `add(11, 부모)`(그 밖의 TYPE 은 미복원 거부)·`addStreak`·`addEvilRay`·`addBeam`(`createLightning` 래퍼: 죽은 표적은 `TargetUnavailable`)·`addScreenBolt`(화면 정수 양끝 + 카메라 원점, 파일명 `shortZap.wav`)·`addShrapnel`·`addDissolve`·`addConstruction`·`addBurning`·`addCooling`·`addSmoke`. `createTailByConstructor(주소, 부모, 진단)` 는 내장 Form 네 생성자(VT 만 씀)·Kaboom/Particle(진단 뒤 NULL)·줄기/불꽃/악마 광선 주소를 구별하고 나머지는 거부한다.
- **빌려 오는 전역:** `bindTailProcessTypes(GameAuxState)`(불꽃/줄기/악마 광선 TYPE 과 원천을 getter 로 매번 읽음)·`bindHitPointTypes`(사제/다이스 TYPE)·`bindWind`·`bindWeakenedMana`(getter 함수). 같은 소유자 재호출은 허용하고 다른 소유자는 거부한다. 악마 광선 장치(`connectEvilRayDevices`: soundId·sound·frameError·assertion)는 도달한 위치에서만 요구한다.
- **정리:** `dispose()` 는 선언 역순(악마 광선 → 연기 → 연소 → 분해 → 파편 → 건설 → 줄기 → 빔 → 불꽃)으로 남은 효과를 Host 삭제(flags 0x10)한다.

## 2. ParticlePaletteMemory 의 웹 대체 (결정: "명시 인접 입력, 기본 관찰값 1")

원본은 색 배열을 `HeapAlloc(heap, 0, 색수 × 4)` 로 요청 크기 그대로 확보하고, 수명이 최대인 입자(색 번호 = 색수)가 배열 바로 뒤 DWORD 를 읽는다. cpppj 는 실제 Win32 heap 과 `ReadProcessMemory` 로 재현했지만 "새 64비트 프로세스의 인접 값이 원본 32비트 실행과 같다고 보장할 수 없어" 고정 값으로 숨기지 않았다. 웹에는 OS heap 이 없으므로 **인접 DWORD 를 명시 입력**으로 받는다: `new ParticlePaletteMemory(adjacent = 1)`. 기본값 1 은 원본 데모 실행(단위101)에서 관찰된 값이며 모든 실행에서 같다는 증거는 아니다(`OBSERVED_ADJACENT_DWORD`). `RawCampaignParticles` 는 `options.adjacent` 로 여섯 메모리에 같은 값을 준다. 시험은 다른 값(`0xabcdef01`)을 넣어 읽기 경로를 구별한다. `address` 는 실제 주소가 없으므로 확보 세대 번호이다.

## 3. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 10.78 전용. 실행 환경에 `bad_alloc` 이 없어(`reserve` 는 항상 성공) C++ 의 "확보 실패 → nullptr" 경로는 없다(`createLightning` 의 `AllocationFailed` 상태 값은 시스템이 null 을 돌려줄 때를 위해 그대로 둠).
- `Renderer`(직접 그리기 절차를 깊이 정렬·dirty 합성하는 본체)와 `RenderSprite` 구조체는 아직 웹에 없다. `sprites()` 는 그 구조체의 필드 이름(`database/clip/order/procedure`)을 그대로 쓰는 평범한 객체를 돌려주며 Renderer 단위에서 이를 소비한다. cpppj `Renderer_ProceduresShareSortClipAndDirtyRemoval` 은 Renderer 단위의 몫이고, 이 단위의 시험은 절차를 같은 clip 으로 불러 합성하는 시험용 대역을 쓴다.
- `camera-view.js` 에 `projectCameraCoordinates(cameraX, cameraY, x, y)` 를 내보냈다(`CameraView.project` 의 몸체 분리, 같은 값). 위치 음향 `SoundToScreen` 과 497220 이 같은 몸체이므로 오디오 단위가 재사용한다.
- 연소 프레임의 `deltaMultiplier`(timer +44)는 C++ 와 같이 이 본체가 설정하지 않는다(클라이언트 소유). 따라서 연소 픽셀 출력은 이 단위의 시험 범위가 아니며 단계 진행·삭제 dirty·소리 정지까지 확인했다.
- 세션 수준 시험(`CampaignSessionTests`·`DamageableBurningSessionTests` 의 이 클래스 사용부)은 4-D 세션 본체와 함께 건다.

## 4. 검사

- `tests/particle-palette-memory.test.mjs` 2건: 확보 전 거부, 색수 11 + 인접 DWORD 12(cpppj ActualAllocationAndAdjacentRead 대응), 재확보 교체, 인접 입력/색/길이 범위 거부와 이전 배열 보존.
- `tests/raw-campaign-particles.test.mjs` 36건:
  - cpppj `ParticleRenderingTests` 이식 2건 — `CampaignParticles_ActualHostPaletteAndVfxPixels`(실제 GIFCLOUD 팔레트의 불꽃 꼬리가 첫 프레임 10입자·둘째 12입자, 부모 화면 위치 (328, 234) 에 색 196 선, 색은 196 과 배경뿐, 카메라 (300, 200) 에서 (28, 34), TYPE 12 거부, 부모 삭제 뒤 목록/Kernel 비움)와 `CampaignParticles_BeamRendererSoundAndRemoval`(`shortZap.wav` 한 번 (20.5, 21.25), 색 195/85 선만, 소진 뒤 비움, 죽은 표적 `TargetUnavailable`) — **둘 다 첫 실행에서 C++ 기대값과 일치**했다.
  - 웹 계약: `projectCameraCoordinates`(CameraView.project 와 45조합 일치), 생성자·연결 계약, 생성자 주소 분배, 장기 전역 바인딩, dirty 대기/전체 갱신, sprites 키·bounds·영역 밖 제외, draw 예외 뒤 대상 해제, 줄기 공유 팔레트, 건설(캡처 사각형에만 복사), 연소(실제 Dude 캡처·삭제 dirty·소리 정지), 악마 광선 두 건(장치 연결·pane 강제·누락 장치·범위 밖 프레임의 오류 순서), 파편 세 종류와 분해(clip·표면 경계·캡처 실패 잔재 없음), 혼합 정리.
- 최초 실패와 교정(모듈 변경 없음): 내 빔 시험이 TYPE 발자국을 주지 않아 이웃 중심 조회가 거부됐고(합성 발자국 1×1 추가), dirty 시험이 첫 프레임만으로 요청이 나온다고 가정했으나 첫 프레임은 입자 생성만 하므로 둘째 프레임 뒤로 고쳤고, 파편 시험이 생성 시각 0 에 만들고 시각 1 에 실행해 입자 수명이 모두 소진된 것을 `prepareFrame(1)` 선행으로 고쳤다.
- 변형 확인: 아래 5절.

## 5. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 262개(입자 본체) + 22개(색 표 저장소)를 적용했다. 입자 본체 1차 생존 131건 → 가장자리/경계 독립 시험 보강 뒤 63건 → 두 번째 보강 뒤 41건 → 회귀 고정(`scaledDelta` 배율) 뒤 **40건**이며 색 표 저장소는 22개 모두 잡았다(1차에 남은 첫 항목 검사 건너뛰기는 첫 항목이 잘못된 입력 시험으로 보강). 기준 시험 통과를 변형 실행 전에 별도 호출로 확인했다.
- 보강으로 잡은 공백(요약): 모든 float/DWORD 바이트(비정수 좌표)·카메라 float 오프셋·창 한 축씩 자르기·표면 폭/높이 경계(표면 메모리를 크게 잡아 감김 드러냄)·0 열/0 행 픽셀·투명 255 비복사·pane 639/479 경계·TYPE/프레임 범위 경계·연소 프레임 번호 주기(슬롯 8)·수집 영역 첫 픽셀 경계·기본 영역·`rate`.
- **회귀 고정(원본 대조가 아님):** 불꽃 둘째 프레임 정확히 16픽셀과 이후 잔여 19픽셀, 파편 세 종류의 픽셀 수/위치, 냉각 연기 10픽셀은 cpppj 관측이 아니라 이 이식의 결과를 고정한 값이다(`rate` 같은 입력 상수가 바뀌면 드러나게 함). cpppj 가 확인한 것은 "(328,234)=196, 색은 196 과 배경뿐, 둘째 프레임 12입자" 등이며 위 첫 두 시험이 첫 실행에서 그대로 일치했다.
- 남은 동등/비관측 변형 40건: ① 공유 색 객체 호스트 토큰 값 여섯(불꽃 1·빔 2·연기 3·악마 광선 4·건설 3·연소 4) — 생성과 구성 검사가 같은 상수를 쓰는 불투명 값, ② 건설/연소 `buildPalette` 의 토큰·띠 개수 일치 검사 둘과 표면 훅의 `!target ||` — 시스템 내부 계약이라 공개 API 로 어긋나게 만들 수 없음, ③ 초기값이 첫 사용 전에 덮이는 상태 스물한 칸(프레임 `now/cameraX/cameraY/diagnostic`, 색 읽기 뷰 `count/readable`, 악마 광선 pane 초기값, 건설 색 `token/count`, 연소 독립 기본값 158/165/약화 마나와 첫 프레임 번호, 줄기 수명 팔레트(공유 상자가 가림), 악마 광선 독립 원천) — 연소 독립 기본값은 `RawBurningSources` 단위(E183)가 대조, ④ `ArrayBuffer(4)→5` 같은 무해한 크기, ⑤ 카메라를 빌릴 때 fallback 원점을 0 으로 만드는 대입 넷(현재 원점이 항상 snapshot 으로 덮임), ⑥ SHP 폭 반환 배열의 둘째 원소(호출자가 쓰지 않음)와 16비트 감김 폭 +1 비트(프레임 몸체가 같은 감김을 다시 하며 실제 SHP 폭은 640 미만), ⑦ 악마 광선 clip 왼쪽 `rect[0]→rect[1]`(둘 다 0), ⑧ `bindHitPointTypes` 첫 `!aux ||`(null 은 어느 쪽이든 TypeError), ⑨ 수집 반복 `index < count → <=`(0 토큰 항목은 목록에 없어 건너뜀).
- 다음 5-A 항목: `RawCursorPreviews`(185, 커서 전역 K 영역 선행)와 렌더러 프레임 본체의 `Renderer`·`ClientRendererFrame`·`ScreenPresentation`·`Screen`(+`RenderSprite`/직접 그리기 절차 합성), cpppj `RainDrawTests` Renderer 합성 2건 재대조. 이 단위가 미룬 것: Renderer 의 `Renderer_ProceduresShareSortClipAndDirtyRemoval`, `CampaignSessionTests`/`DamageableBurningSessionTests` 의 이 클래스 사용부(4-D).
