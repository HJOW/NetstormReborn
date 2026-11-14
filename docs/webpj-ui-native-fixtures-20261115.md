# webpj UI 부품 5모듈 원본 관측 연결 (WB04-E477, 2026-11-15)

3.8절 이월 목록(이미지/메타/HUD, 텍스트 입력/진단 일부)에서 이미 이식한 UI 부품 다섯 모듈을 원본 10.78 독립 관측 fixture 다섯 개 **3,410행**에 대조했다. 모듈 코드는 고치지 않았고(주석 오기만 정정) 다섯 모두 **첫 실행에서 전체 일치**했다.

| fixture | 행 | 대상 | 비교한 것 |
| --- | --- | --- | --- |
| `budget-refresh` | 120 | `refreshStormPowerGump` | 현재 serial 조회(없는 대상/루트/첫째·마지막 슬롯/죽음/중복)와 Push/Attach 순서·좌표·깊이 복원·중단 prefix, 두 정밀도의 전체 노드 바이트·등록표·serial·호출 순서 |
| `meta-image` | 326 | `MetaImageGump` | 생성/크기/부착 축소/가상 출력의 배율·기하 float 비트·command/vtable/flags·공통 등록표 count/serial·호출 인자/순서 |
| `meta-image-list` | 514 | `MetaImageList` | 생성/부착/히트/입력/그리기/ID/이미지/삭제/추가/소멸의 tail 0xbc·자식 22개 기하·반환·노드 필드·호출 순서 |
| `editor-image` | 1,570 | `RawImage` | x87 두 모드·삼색 전 조합·긴 run·실제 지형 Bitmap 의 변환/crop/shape 인코딩/복사/소멸 전체 shape 바이트·크기 |
| `text-clipboard` | 880 | `TextClipboard` | CF_TEXT 읽기 240·쓰기 640의 용량·NUL 패딩·장치 실패별 호출 순서와 접근 실패 80행 |

## 한 일

- `tests/ui-native-fixtures.test.mjs` 신설: cpppj `StormPowerRefreshTests`·`MetaImageGumpTests`·`MetaImageListTests`·`EditorImageTests`·`TextClipboardTests`의 입력 구성(앞 열만 읽음)과 직렬화를 그대로 옮겼다(편집기 Image 의 기존 RLE renderer 픽셀 대조는 렌더러 단위 시험의 몫이라 옮기지 않았다).
- manifest에 다섯 항목(`unit WB04-E477`)을 연결했고 PORT_MAP 다섯 행(`RawStormPowerRefresh`·`MetaImageGump`·`MetaImageList`·`RawImage`·`TextClipboard`) 메모에 연결 사실을 덧붙였으며 모듈 다섯과 시험 다섯 파일의 "cpppj에도 직접 시험이 없다"는 오기를 정정했다(E465 감사가 찾은 같은 오기).

## 결과

- 다섯 fixture 모두 첫 실행에서 전체 일치했다. 첫 시도에서 `meta-image-list`의 열 수 단언(25→26) 하나만 하네스 실수로 어긋났다. 모듈 결함은 없었다.
- 기준선 뒤 변경: `1d5b1f437`(K1 한국어 후보 병합 등)이 다섯 파일 중 하나를 건드렸으나 3.6절 ①(한국어 K1)로 웹 제외 판정이 이미 있다.

## 남은 것

3.8절 이월 목록의 나머지(`html-input-dialog`·`htmlcondition`·`dot-command-*`·`hud-nsinfo-*`·`floating-sp`·`production-hint`·`minimap-*`·`gump-pointer-boundary`·`pointer-special-trace`·`menugump`·`inventory-about*`·`text-entry-*`·`user-highlight`·`priest-regen-hit-points-current`·`rifttype-hit-points`·`basefile-*`·`startup-*`·`xlat-file-read`·`options`·입력 명령 계열·`renderer-projection`·`scenemusic`·`chat-window*`·`chat-completion`·`chat-callbacks`·`chat-input-recipients`)는 후속 단위에서 같은 방식으로 연결한다.
