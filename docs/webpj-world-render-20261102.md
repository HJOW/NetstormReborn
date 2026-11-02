# webpj 월드 그림과 카메라 기반 — WB03-Z

2026-11-02, HJOW-Athlon, Node v24.21.0. **M2 4단계의 공통 그리기 기반**을 옮겼다. 아직 메뉴에서 미션 맵을 여는 단계는 아니다.

## 1. 근거와 이식 범위

AGENTS.md·공통/dotnetpj/webpj 인계를 읽었다. 마지막 전체 정밀 디컴파일 호스트(HJOW-Athlon/2026-10-23)와 현재 호스트가 같다. 이번에는 디컴파일 산출물을 읽지 않고 검증된 cpppj 복원 소스·원본 실행 fixture·기존 웹 자산만 읽었다. dotnetpj 구현을 근거로 쓰지 않았다.

검토 기준은 `ee73f365` 트리다. 이전 웹 단위 `310c5ba6` 이후 아래 소스의 변경 커밋은 없었다. 이 기록은 나머지 cpppj 변경을 모두 이식했다는 뜻이 아니다.

| 웹 구현 | cpppj 근거 | 원본 10.78 |
| --- | --- | --- |
| `render/draw-sort.js` | `client/DrawSort.cpp`, `Renderer.cpp` | 표시 비교 `00497900`, qsort `004e6010`, shortsort `004e5fa0` |
| `render/camera-view.js` | `client/RawCameraView.cpp/.h`, `RawCursorPosition.cpp` | 초기화 `004993c0`, 월드 원점 `004973e0`, 픽셀 원점 `00497490`, 투영 `00497220`, 커서 역변환 `004971e0` |
| `render/world-sprites.js` | `client/SpriteDraw.cpp`, `Renderer.cpp`, `SquidRenderer.cpp` | 타입 발 기준점 차감, SHP 레이어와 객체 내부 출력 순서 |
| `IndexedSurface.drawImage`의 그림자 분기 | `Renderer::DrawIndexedImage` | 투명 마스크 안에서 기존 대상 색을 표로 치환 |

원본 게임·analyzeManager·cpppj 실행 파일·브라우저는 실행하지 않았다. 외부 창도 열지 않았다. AGENTS.md·원본/CD·cpppj·dotnetpj·공통 인계·README·자산/매니페스트는 수정하지 않았다. 착수 때 있던 dotnetpj 변경은 보존하고 커밋에서 제외한다.

## 2. 동작과 사용법

### 표시 순서

`sortDrawEntries(entries)`는 항목 전체를 제자리 교환한다. 키는 `{ x, y, depth }`: x/y는 저장된 유한 float32, 깊이는 signed WORD다. 깊이 내림차순, y/x 오름차순이며 완전 동률도 비교 결과가 **1**이다. JS의 안정 정렬을 사용하지 않고 원본 CRT의 8개 이하 선택 정렬, 세 지점 중앙값 피벗, 피벗 위치 갱신, 구간 스택을 옮겼다. 같은 키 9개를 1~9 순서로 제출하면 `9,5,3,4,2,6,7,8,1` 순서가 된다.

잘못된 키는 첫 교환 전에 거부한다. JS에 아직 저장되지 않은 double 좌표는 호출자가 float32로 저장한 뒤 전달해야 한다. CD판 정렬은 구현하지 않았다.

### 카메라

`CameraView`는 원본 앞 64바이트(16DWORD)를 보관한다. 생성 직후는 BSS 0이며 `initialize()`를 명시 호출한다. `setWorld(x,y)`는 입력 float32의 분수를 보존하고 칸 크기의 정수와 double로 곱한 뒤 signed64 절삭의 하위 DWORD를 쓴다. `setPixels(x,y)`는 원점 정수를 저장하고 원본 float32 역배율(특히 Y)을 곱해 월드 원점을 만든다. 다른 DWORD는 그대로 둔다. `copy()`는 독립 저장소이며 부분 Buffer 입력도 64바이트로 복사한다.

`project(x,y)`는 `ftol(float32(x)*16+0.5)-cameraX`, `ftol(float32(y)*11+0.5)-cameraY`다. 음수에서 `Math.round`와 다르다. `screenToWorld(x,y)`는 **커서**의 원본 변환으로, 화면 좌표에 한 칸(16/11)을 먼저 더한다. 그림 기준점의 단순 역함수가 아니다.

진단은 모든 store 뒤 X→Y 순서로 실행한다. 진단 callback이 상태를 바꾸면 Y는 현재 상태를 다시 읽는다. 카메라 목표/제한/보간, 스크롤 입력, 화면 크기별 맵 영역 계산은 이 클래스의 정책으로 추가하지 않았다.

### 월드 그림

```js
const camera = new CameraView();
camera.initialize();
camera.setPixels(1600, 1100);
const renderer = new WorldSpriteRenderer({ table, shapes, ownerColors });
renderer.draw(surface, camera, [
    { type: 97, frame: 100, x: 104, y: 104, depth: 30, ownerColor: 3 },
], { clip: surface.bounds });
```

위 숫자는 API 예시이며 미션 초기 상태나 타입별 깊이 규칙을 뜻하지 않는다. 실제 장면 조립자가 깊이·현재 프레임·소유자 **색 번호**를 제공해야 한다. 플레이어 번호를 색 번호로 간주하지 않는다.

- `physicalShapeFrame(table, shapes, type, frame, layer)`는 `논리 클러스터 + 레이어 × 클러스터 수`를 실제 SHP 블록에서 검사한다. modulo로 없는 프레임을 대신하지 않는다.
- `plan()`은 입력 배열/객체/카메라를 바꾸지 않고 float32 월드 정렬 키, 타입 발 기준점을 뺀 화면 위치, 프레임과 색표를 한 항목에 묶는다. 실제 게임의 SID/표시 수집기와는 별개의 **출력 계획**이다.
- `following: [{ frame, layer }]`는 같은 객체 안에서 연속 출력한다. 후속 레이어만 다른 객체와 따로 정렬하지 않는다.
- `ownerColors`는 WB03-Y의 `createOwnerColorTables` 결과다. 없는 타입은 원색, 명시한 `colorMap`은 해당 표를 사용한다. 배너 기본값과 플레이어→색 번호 선택은 호출자 계약이다.
- `shadow: true`는 명시적인 배경 변환표가 필요하다. SHP의 불투명 픽셀을 마스크로 삼고 **그림 색이 아닌 현재 표면의 색**을 변환한다. 그림자 색표 생성/등록은 아직 이식하지 않았다.
- 클립은 right/bottom 제외다. 화면 밖 프레임과 특수 레코드는 해독하지 않는다. 제출한 항목 전체를 정렬한 뒤 출력에서 클립하므로, 원본의 사전 가시 수집 단계와 혼동하면 안 된다.
- 배경 지우기·구름/배경 위상·Canvas 출력·dirty 영역은 상위 호출자가 소유한다. 기존 메뉴 `drawImage` 호출은 기본 인자가 같아 동작이 유지된다.

## 3. 검증

`node --test webpj/tests/world-render.test.mjs`: **8개 통과, 실패/건너뜀 0**.

| 확인 | 결과 |
| --- | --- |
| 원본 CRT fixture | 10.78 325줄의 전체 16바이트 항목 순서 일치. 8/9 경계·signed 깊이·중복·극단 유한 float 입력 포함 |
| 원본 카메라 fixture | 16줄의 전체 64바이트·진단 메시지/행 번호 일치: 초기화1, world7, pixels5, project3 |
| 합성 픽셀 | 타입 발 기준점, 동률9개 마지막 색, 정렬 끔, 객체 내부 후속 레이어, 소유자 색표와 그림자 표의 구별, 투명/음수 위치/clip의 양 끝, 특수/화면 밖 해독 생략 |
| 실제 SHP 자산 | 116타입의 모든 물리 프레임 번호↔논리 프레임/레이어 대응. isle100을 타입 기준점·카메라·소유자 색표로 합성한 전체 버퍼 일치 및 실제 색 치환 발생 |
| 카메라 저장 수명 | 분수 보존, 독립 snapshot, 부분 Buffer 복사, 음수 투영, 커서 한 칸 보정, ftol indefinite/감김, 진단 후 현재 상태 재조회 |

fixture는 원본 실행 관측을 읽기만 했으며 웹 출력으로 덮어쓰지 않았다. 배포 폴더에도 복사하지 않았다. fixture 없는 환경에서는 해당 두 검사를 건너뛴다.

| 경로 | SHA-256 |
| --- | --- |
| `cpppj/tests/fixtures/drawsort-x86.tsv` | `4cf42c2dbc1f1e2ca93df8157c392fe9dc816e7634f839768282cb6039c7abd8` |
| `cpppj/tests/fixtures/camera-view-x86.tsv` | `fb22a21df5be3ca9201d1c8d635f0e4b49b6f974efbacfe46e012a86536887d0` |

전체 검사의 최종 값은 인계 문서 WB03-Z 절에 기록한다. 기존 두 실패는 `fonts/License.txt` 원본 작업 트리 CRLF와 웹 복사본 LF의 차이다. 이번 단위에서 자산·기대값을 갱신하지 않았다.

## 4. 남은 일과 완료 판정

**정적 맵 M2는 미완료**다. 이번 작업은 월드 출력의 공통 기반이며 `netstorm.js`의 메뉴 동작이나 캠페인 시작은 바꾸지 않는다. 기존 WB03-X/Y 계산 모듈의 난수/원소/소유자 가정도 그대로 남는다. 원본 화면 동일성(L4), 자연 플레이(L5), 브라우저/다른 OS 검증을 완료로 세지 않는다.

다음은 지면·절벽(y+4)·작은 받침·저장 오브젝트를 이 API에 제출하는 **장면 조립**이다. 실제 타입/생성자에서 깊이·프레임·발 기준점 적용 위치를 확인하고, 신전 theme 및 소유자 마감/색 번호를 정한다. 원본 그림자 색표, 가상 Draw/다층 신전·제단·트리 등, 구름과 카메라 배경 위상, 카메라 시작/제한/스크롤을 이어 붙인 뒤 메뉴의 미션 명령을 정적 뷰에 연결한다. 그때 원본 캡처와 대조한다. cpppj의 옛 근사 `GameWorld` 깊이 상수/원점 처리로 대신하지 않는다. `edgeFarm`은 기존 계획대로 WB04 이후다.
