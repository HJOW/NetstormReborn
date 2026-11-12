# webpj Client 수명 전역 — AI 진단·카메라 (WB04-E333, 2026-11-12, 4-D A 구획)

3.1절 4-D A 구획의 client **ClientAiDebugGlobals**(기준선 34줄, 헤더만, 변경 없음)와 **ClientCameraGlobals**(93줄 = `.cpp` 41 + `.h` 52, 변경 없음)를 `core/client-ai-debug-globals.js`·`core/client-camera-globals.js` 로 옮겼다. 둘은 `ClientGameplayGlobals`(다음 항목)가 합쳐 소유하는 하위 소유자라 먼저 옮겼다(앞 단위 E325·E331·E332 와 같은 방식).

## 1. 규칙

- **ClientAiDebugGlobals:** 보호 PE 의 AI 진단 전역을 Client 수명으로 보관한다. `checking()`(005e4794, 최초 BSS 0, **signed 32비트** `Int32Box` — BOOL 로 좁히지 않음), `assertionDetail()`(005e4798, 최초 `null`, 마지막 오류 상세 문자열 — 월드 대여/반납으로 지우지 않음), 7로그 DWORD 일곱 칸(주소 0x52f3cc·d0·d4·d8·dc·e0·e4, 초기값 99·99·29·99·99·99·29)은 `word(address)`/`read(address)` 로 접근하며 **정확히 일치하는 주소만** 허용하고 아니면 `RangeError`(임의 포인터 역참조 없음). 접근자는 호출마다 같은 객체를 돌려준다.
- **ClientCameraGlobals:** 같은 형식이라는 이유로 alias 하지 않는 여섯 저장소 — 전역 snapshot(0059a900, `CameraView` 16DWORD, 최초 BSS 0)·논리 Renderer 앞 64BYTE(`rendererView()`)·목표(`target()`: 한계 `limits[2]`(+48/+4c)·다음 픽셀 원점 `pixels[2]`(+50/+54)·다음 Q16 배율 `zoom`(+58))·분모(`denominator()`: PE 초기 15/15)·최근 건설 위치 이력(`constructionHistory()`)·숫자 키 저장 80BYTE(`savedPositions()`).
  - `initializeNativeRenderer(width, height, worldCells, worldRedraw, mapRevision)` (실제 4993c0): 논리 Renderer 기본 view → 한계 `ftol(칸 크기 × fround(세계 크기) − 화면)` 두 개(배율은 +58 에 둘 사이에 씀)와 목표 원점 0 → 논리 Renderer 64BYTE 를 snapshot 으로 복사 → worldRedraw 1 → mapRevision +1. 화면 크기가 0 이하면 `RangeError`.
  - `updateTargetLimits`(497f80): 칸 크기(double 곱의 중간 정밀도 유지) × **단정도 세계 크기** − 화면 크기의 ftol 하위값. `requestPixels`: 목표 원점만 쓴다. `requestCenter`(497f20): `0 − ftol(−16·fround(x)) − (width/2 절삭)` 과 Y 의 11배를 목표에 쓰고 두 store 뒤에만 revision(005c85dc)을 올린다(화면 가장자리 제한은 뒤 Present 의 별도 경로). `centerImmediately`(433aa0): worldRedraw 증가 → `requestCenter` → commit 순서이며 commit 이 없으면 앞선 쓰기를 보존한 채 `즉시 카메라의 실제497960 장치 미연결` 로 거부.
  - `applyPixelsToRendererAndSnapshot`(497960 조각): 두 view 에 같은 픽셀 원점. `copyRendererSnapshot`(436450): 논리 Renderer 64BYTE 를 snapshot 으로 복사(목표/viewport 는 복사하지 않음). `selectDenominator`(4978d0): 0 이면 기본 분모, 현재와 다를 때만 현재 분모를 먼저 저장하고 worldRedraw 1.

## 2. 웹 범위와 기준선 뒤 변경

- 두 파일 모두 기준선 뒤 추가 줄이 없다(3.6절 ①).
- 카메라 64BYTE 상태와 `project`/`setWorld`/`setPixels`/`initialize` 는 기존 `render/camera-view.js`(`CameraView`, WB03-Z, PORT_MAP `RawCameraView.cpp` partial)를 재사용한다. 실제 카메라 시작·스크롤·제한 정책(`CameraCommit`/`Center`/`Home`/`Pan`/`Present` 계열)은 별도 단위이다.

## 3. 검사 (`tests/client-ai-camera-globals.test.mjs` 12건)

- **원본 독립 관측:** `camera-view-x86.tsv` 의 `init` 행(실제 4993c0 Native Renderer 초기화 한 행): 논리 Renderer 입력 64BYTE 를 두고 초기화하면 Renderer 64BYTE·snapshot 복사·worldRedraw·mapRevision(`0xffffffff` → 0 감김)이 관측과 같고 목표가 한계 3072/2048·원점 0·배율 65536 이다. **첫 실행에서 일치했다.** manifest 의 `camera-view` 항목에 이 시험과 모듈을 소비자로 더했다.
- **cpppj 시험 이식:** `CameraView_BssExplicitInitializeTargetSnapshotAndDenominatorStayDistinct`(목표/논리 Renderer/snapshot/분모가 서로 다른 주소, 명시 복사 위치만 현재 값을 바꿈, 다른 소유자는 영향 없음).
- **웹 계약:** AI 진단의 초기값·주소 일치·폭(signed/unsigned)·상세 보존, 목표 한계의 단정도 입력과 중간 정밀도(비유한 입력은 하위 DWORD 0), 목표 중심 수식(양/음/홀수 화면), 즉시 중심의 순서와 부분 효과, 분모 선택, 건설 이력과 저장 위치의 초기 BSS 0·안정된 소유, 초기화가 이전 목표 원점을 지움, 497960 조각이 두 view 모두에 씀.
- 첫 실행의 실패 1건은 시험 쪽 기대값 오류였다: 음수 중심의 목표를 `56 − 512` 로 기대했으나 식은 `0 − ftol(56) − 512 = −568` 이다(모듈은 C++ 식 그대로).

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 54개(AI 12·카메라 42)를 돌려 AI 는 0개, 카메라는 첫 실행에서 8개가 남았다(초기화의 목표 원점 0·세계 크기/중심 입력의 단정도 저장·497960 의 두 view 쓰기 등). 시험을 보강(이전 목표를 지움, 2^24+1 세계 크기와 경계 중심 입력, 두 view 모두 확인)한 뒤 **2개가 남았고 모두 동등**하다: ① `wrap(value)` 의 `| 0` 제거 — 결과를 `Int32Array` 에 저장할 때 같은 32비트 감김이 일어난다 ② mapRevision 증가의 `>>> 0` 제거 — `DwordBox` 가 쓰기에서 같은 DWORD 감김을 한다.
- **앱 수명:** 아직 Client 가 이 객체들을 만들지 않는다(PORT_MAP session 0). 4-D 의 `Client`/`ClientGameplayGlobals` 조립이 한 번씩 만든다.
- 3.1절 4-D 는 5/27 이다(A 구획의 ClientFortGlobals·ClientCoreDataGlobals·ClientPlayerRecords/Runtime·ClientAiDebugGlobals·ClientCameraGlobals 다섯 줄 완료; 묶음 줄을 풀어 분모가 25 → 27). 다음: A 구획 `ClientCursorGlobals`·`ClientUserSelectionGlobals` (둘 다 client 커서 모듈 `RawCursorLifecycle`/`RawCursorPlacement`/`RawCursorDrop` 의 상태 구조체에 기대므로 선행 판단 필요).
