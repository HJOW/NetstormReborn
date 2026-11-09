# webpj Burning 프레임(RawBurningFrame)·BurningFrameCapture 이식 — WB04-E200

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 이백 번째 부분이다. Burning(process 14)의 캡처(00423d50)와 실제 프레임(00424530)을 옮기고, E199 `RawShapeToBuffer` 를 Burning 프레임 hooks 에 설치하는 client `BurningFrameCapture` 를 함께 옮겼다. 이로써 Burning 표시 몸체(생성 E183·연소점 E183·출력/소멸 E184·프레임 E200)가 모두 웹에 있고, 남은 것은 Host 부착 `BurningVisualSystem`(E201) 이다.

- cpppj 기준선 `1f00a25a`. 두 소스는 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋: `RawBurningFrame` `e5a198df`, `BurningFrameCapture` `ab9361f1`.
- 원본 fixture: `burningframe-x86.tsv` originals 213행(SHA-256 `8b09c3d5…`, 근거 `ab9361f1`, 기준선 뒤 변경 없음). originalCD 213행은 범위 밖이다.
- 공유 장치:
  - E199 `render/raw-shape-to-buffer.js`·`squid-renderer.js`
  - E183 `raw-burning-visual.js`·`raw-burning-sources.js`
  - E76 `raw-burning-victim.js`
  - E26 `raw-sid-robust.js`
  - E59 `render/dirty-regions.js`
  - D2 `random.js`(GameRandom)
  - E129 `RawRenderRegistry`

## 1. 옮긴 것

`www/core/raw-burning-frame.js`(DOM 없음)

- `createBurningFrameFields()`: 생성자가 초기화하지 않는 원본 +40/+44 `{ delta, scaledDelta }` 이다.
- `createBurningFrameState()`: 프레임이 매 호출 읽는 현재 전역이다.
  - 게임 시각·월드→픽셀 배율·카메라·바람·timer 배율·viewport·inset 모드를 담는다.
  - 피해 대상 진단 상태 `victim` 과 현재 Client 바람 getter `sharedWind` 도 담는다.
- `RawBurningFrame(pool, types, visual, sources, random, state, sink, hooks)`:
  - 같은 풀(visual/sources)인지, 캡처 hooks 가 있는지 생성 때 확인한다.
  - sink 는 Squid 표시 출력 대상이다(`invalidate(rect, flags)`).
- `capture(fields)`(423d50):
  1. 이전 이미지/행을 반납한다.
  2. capacity 만큼 입자를 zero 로 만든다.
  3. 피해 대상 앞단(진단 포함)을 거친다. 조기 반환이면 메타데이터를 유지한다.
  4. 현재 `fields.victim` 으로 캡처한다(앞단이 고정한 번호가 아님).
  5. (폭+높이)/5 개 행 표식을 zero 로 만들고 selected 를 0 으로 한다.
- `step(fields, frame)`(424530):
  1. capacity 0 이면 반환한다.
  2. 단계 0 이면 시각 초기화 뒤 캡처한다.
  3. 피해 대상 앞단을 거치고, 선택 슬롯의 좌표·깊이를 읽는다(진단 뒤에도 선택 슬롯).
  4. 단계 1 이면 첫 좌표를 넣고 off 로 둔다.
  5. visible 판정을 한다. 이전 범위를 ftol 하고 카메라를 뺀 뒤 다시 ftol 하며, inset 이면 100 픽셀을 줄인다. 처음 꺼질 때 nonzero 소리는 정지 소비자로 넘긴다.
  6. 다시 켜졌으면 시각을 맞추고, timer(10배·배율)를 계산한 뒤 연소점을 선택한다.
  7. capacity 전체 입자를 처리한다(같은 프레임에 새로 추가된 후방 입자 포함).
     - source 는 float 난수 < delta 일 때 방출한다. 수명 난수 4~9, 상승 −2 이며 Append 가 성공하면 상자에 넣는다.
     - 이동 입자는 수명을 줄이고, 넓은 속도+현재 바람으로 위치를 옮긴다.
  8. 상자를 오른쪽 +2·아래 +1·왼쪽 최소(right−17)·위 −2 로 보정한다.
  9. 이전/새 영역을 SetRect(패치: 여백 → 카메라 순서) 뒤 viewport 로 자르고, 빈 영역이 아니면 dirty 로 넘긴다.
  10. 등록 범위를 갱신한다.
  - x87 의 float 저장 위치는 `Math.fround` 로 보존한다. 패치 helper 의 ordered 비교에서 NaN 은 새 값을 고르고, NaN 수명은 비활성이다.
- `makeBurningFrameCapture(capture, pool, hooks)`(client `BurningFrameCapture`):
  - 같은 풀의 `RawShapeToBuffer` 만 받는다.
  - 호출자가 준 다른 소비자(소리 정지)는 유지하고 `capture` 만 설치한다.

## 2. 검증

`tests/burning-frame.test.mjs`(5건). 실제 자료 장치는 E199 시험과 공유하도록 `tests/capture-assets-support.mjs` 로 분리했다(`shape-to-buffer.test.mjs` 도 이를 쓴다).

- 원본 213행: 피해 SID 50(TYPE 70, 좌표 20.75/21.9, HP 10)에 다음 입력을 넣고 전체 출력 15칸을 대조한다.
  - capacity 0(1행)/12(212행), 단계 0/1/2, off, 화면 안/밖
  - 입자 패턴 네 가지: 없음·source·수명 감소·NaN/무한/−0 수명
  - 난수 시드·시각 비트·바람·깊이 바이트·frame
  - 캡처 배율 1/1.5 와 월드→픽셀 배율 1/0.75
  - 15칸: 등록 범위·깊이·total·timer 24바이트·단계·캡처 메타데이터·전체 픽셀·행 표식·selected·off·입자 바이트·난수 상태·전체 갱신·dirty 표(병합 포함)·캡처 크기.
- 바람 owner: 같은 213행을 생성 뒤 바뀐 `sharedWind` getter 로 다시 실행해도 같다(cpppj AmbientWind 시험의 10.78 절반).
- 세 소비자 진단: 캡처(0x6f)·첫 좌표(0x11f)·입자 프레임(0x11f → 연소점 0x91)에서 첫 robust 실패 진단이 복귀하면 기존 출력과 같다.
- 실제 피해 대상 어댑터:
  - `isSidRobust` 상세 owner(lastRobust/type)를 진단 시점(2/70)과 복귀 뒤(0/70)에 확인한다.
  - checking 0 에서는 owner 를 읽지 않는다.
  - Dais 형 HP 진단 복귀를 확인한다.
- 웹 계약 검사: 풀 불일치·캡처 누락 거부, 소리 정지 소비자 유지, capacity 0 무동작, 화면 밖 첫 off 의 nonzero 소리 정지와 반복 off 무정지, 정지 소비자 누락 거부, 입자 배열 크기 거부.

최초 실패와 교정: 없다. 모듈과 시험 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 확인하려고 상승 속도 상수를 −3 으로 바꾸면 213행 중 69행째에서 등록 범위 칸이 어긋나 실패했고, 확인 뒤 되돌렸다.

## 3. 경계와 남은 것

- 실제 Host 부착·Kernel 수명·자기 삭제는 E201 `BurningVisualSystem` 에서 E192 `render-process-system.js` 위에 연결한다.
- 실제 소리 정지 장치·실제 Client 바람/카메라/viewport 전역·Session 월드 연결은 후속이다(PORT_MAP session 0 유지).
- 10.72(CD)의 무진단 인라인·signed DWORD life·저장 vx 재읽기·ordered 비교 차이는 범위 밖이다.

## 4. 다음 시작점

WB04-E201 은 `o/BurningVisualSystem` 이다(cpppj `BurningVisualTests`, E192 공통 표시 저장소와 `smoke-visual-system.js` 방식). 이것으로 일반 `*VisualSystem` 이 모두 웹에 있게 된다.
