# webpj 사용자 선택·후보 선택·다중 선택 이식과 원본 관측 연결 (WB04-E495, 2026-11-15)

3.8절 이월 목록의 사용자 선택 계열을 이식하고 원본 10.78 독립 x86 관측 **3,108행**(user-selection 1,074·user-selection-clear 87·escape-selection 154·user-pick 1,092·multiple-selection 601)에 대조했다. 5개 fixture 모두 첫 실행에서 전체 일치했고 이식 결함은 나오지 않았다. 새 모듈 3개로 `PORT_MAP.tsv`가 todo 98 → 95, port 765 → 768이 되었다.

## 이식한 것

| 웹 모듈 | cpppj 대응 | 내용 |
|---|---|---|
| `www/core/raw-user-selection.js` | `client/RawUserSelection.cpp/.h` | 세 인자 `select`(관문·다중/단일 정리·덤프·마나/사정거리 표시·제단 위임·선택 통지·기억 좌표·이동 강조·소리)·`clear`·`clearMultiple`·`remember`·`recheckMouse`·`toggleMultiple`·`extendSelection`·`moveMultiple`·`carrierMenu`/`archerMenu` 메뉴 입구와 `WORLD_DESTINATION_SELECTED_COMMAND`(0x02000044) |
| `www/core/raw-user-pick.js` | `client/RawUserPick.cpp/.h` | 4d5840 후보 순위(소유자·이동·깊이·좌표)·필터·가상 hit 호출 순서·마지막 1.6 근접 이동 후보 교체. 객체 상태는 쓰지 않는다 |
| `www/core/client-user-selection-globals.js` | `client/ClientUserSelectionGlobals.cpp/.h` | 선택·강조 아홉 DWORD와 P/R 누적·다중 선택 30칸 배열의 장기 owner. `appendMultiple`/`removeMultiple`/`resetForGeneralWorld`/`readDword`/`writeDword`와 처리기에 빌려 주는 `selectionState`/`highlightState` 접근자 보기 |

모든 외부 효과는 호스트 훅이며 도달한 경계에 훅이 없으면 그 위치에서 `Error`다(선행 효과는 되돌리지 않는다). 풀은 10.78(`edition === 'patch1078'`)과 188 TYPE만 받는다.

## 비교한 것

`tests/user-selection-native-fixtures.test.mjs`·`tests/multiple-selection-native-fixtures.test.mjs`가 cpppj `UserSelectionTests`·`UserPickTests`·`MultipleSelectionTests`의 입력 구성을 옮긴다. 입력 열만 장치/전역에 공급하고 관측 열(반환·경계 사건 순서·현재 전역)은 비교에만 쓴다.

- **user-selection(1,074)**: 31DWORD 입력(연산 0 `select`, 1 `clearMultiple`, 2 `remember`, 3/4 Carrier/Archer 메뉴)을 같은 물리 풀(24000, 서버)과 실제 `RawUserHighlight`에 공유 접근자 보기로 연결한다. 반환·경계 사건 문자열(`P;B;N;R;F;Q;D;T;V;C;I;A;M;E;S;!;O`)·열 개의 현재 전역/물리 필드가 일치한다. 열두 개 변경 경계(`mutate`)로 가상 통지가 개수·선택·TYPE·좌표를 바꾸는 경로를 포함한다.
- **user-selection-clear(87)·escape-selection(154)**: `clear`와 `select(0,0,0)`의 enabled 관문·다중 정리·강조 0 순서와 현재 전역.
- **user-pick(1,092)**: 후보 순위와 가상 hit 호출 순서(`paused;project;begin:r1,r2;hit:sid…`)·반환 SID·전체 슬롯 바이트 불변성. 근접 후보 거리는 기존 `RawSquidNeighbors.center`와 `aiPointDistance`와 같은 배정도 계산이다.
- **multiple-selection(601)**: `toggleMultiple`(기본 선택 비트·승격/제외)·`moveMultiple`(현재 개수/배열 재조회)·`appendMultiple`/`removeMultiple`(고정 용량 30·중복·안정된 제거)의 사건 순서와 전역/표시 비트/30칸 배열. 이 fixture는 `ClientUserSelectionGlobals`가 있어야 닿았고 이번 단위에서 연결했다.
- **독립 시험(cpppj 시험 이식)**: 조기 관문·미연결 하위의 위치 오류·선행 쓰기 보존, 10.78/TYPE 188 계약, 가상 통지 중 재할당된 DWORD 배열·signed 개수 재조회, 풀 밖 DWORD 후보 자연 생략과 후반 배열 실패의 선행 통지 보존, 마우스 재확인 여섯 경우, Shift 확장 승격·고정 용량·실패 시 enabled 0 보존, 아홉 DWORD 비트와 원시 마우스 좌표.

## 변형 확인

세 모듈에 자동 변형을 돌렸다. 처음에는 fixture만으로 `raw-user-selection.js` 168개 중 77개, `raw-user-pick.js` 91개 중 19개, `client-user-selection-globals.js` 67개 중 24개가 살아남았다(토글·확장·이동·재확인과 owner 쪽은 선택 1,074행 fixture가 닿지 않는 경로였다). multiple-selection 601행과 독립 시험을 더하고 `tests/user-selection-boundaries.test.mjs`(경계 12개)로 상수 비트·WORD 링크의 이웃 바이트·TYPE 범위·선택 허용 비트·관문 순서·float 저장 경계·좌표 필드 오프셋·초기값/reset/중복 검색 범위/쓰기 폭을 직접 확인한 뒤 남은 것은 모두 동등 변형이다.

- `raw-user-selection.js` 12개: SID 상한(`MAX_SID`) 관련 6개는 `SidPool` 용량 상한(65535)이 이미 막아 닿을 수 없는 방어 검사다. `value >= capacity`/`||` 변형은 풀 자체의 범위 오류(`RangeError`)가 대신 던진다. `DataView` 버퍼 크기 4→5와 `#read` 기본 폭 4→5는 `>>> 0` 절단으로 같은 값이다. `activateMove` 1→2 세 곳은 참/거짓으로만 쓰인다.
- `raw-user-pick.js` 4개: `MAX_SID`·`candidate > MAX_SID` 방어 검사(용량 상한이 막음), 초기 깊이 9999→10000(첫 후보의 `dz`는 어느 쪽이든 음수라 순위가 같다), `DataView` 길이 4→5.
- `client-user-selection-globals.js`: 변형 24개 모두 잡혔다.

## 남은 것

사용자 선택 계열의 `ClientUserSelectionCleanup`은 `RawCampaignSession.core()`(Session 허브)에 기대어 대기한다. 3.8절의 남은 항목은 `chat-callbacks`·`chat-input-recipients`(허브), `html-input-dialog` 생성/공장 20행(SaveFortNameDialog), `detonate-input`(RawObjectConversion), `audio-reinterpret`·`font-face-*`(적응 판정), `renderer-projection`(ClientRendererFrame), `scenemusic`(ClientAudio 적응)이다.
