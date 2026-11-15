# 게임 메뉴 일반 명령 이식 (WB04-E535)

6단계의 "게임 메뉴" 중 `client/GameMenuActions`(원문 UserInput 4d62b0에서 Guide/Gump 전처리가 소비하지 않은 일반 메뉴 명령 몸체)를
`webpj/www/core/game-menu-actions.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
전체 UserInput의 OS 메시지 폴링·Guide/Gump 전처리는 이 단위의 범위가 아니며, 앞단이 소비하지 않은 명령만 `handle(event, first, second)`로 넘긴다.

## 옮긴 범위

`handle`은 대상이 아니면 `null`, 처리된 일반 명령은 `0`, QuitApp의 원문 즉시 반환은 `1`을 돌려주며 release(상위 비트)는 대상 명령이어도 효과 없이 `0`이다.
현재 명령 ID는 `command` 훅에서 호출 순간에 읽고, `MenuJustClosed`는 부모 Gump 몫이라 대상에 넣지 않는다.

| 명령 | 효과 |
| --- | --- |
| QuitApp | `second`가 99가 아니면 진단 993(복귀 시 계속) → `selectTab.wav` 재생 → 즉시 1 반환 |
| RequestQuitApp | Editor면 Tell `QuitEditMode`, 아니면 Abandon 쌍이 모두 거짓일 때 `QuitAbandon`, 그 밖은 QuitApp을 99로 예약 |
| SoundVolume/MusicVolume | `soundVolume=%d`/`musicVolume=%d`(signed 해석) 설정문 → `priestmove.wav` 재생 |
| Pause | mission→Editor 순서로 하나라도 참이면 현재 깊이 질의 → 상태 안내(11, 0, 0/1) → 재개(clock·sound) 또는 정지(커서 취소·clock·음소거) |
| ViewHome | 595344가 참이고 594fc8이 거짓일 때 소유자 0이면 진단 1895(계속) 뒤 소유자를 다시 읽어 Player의 +7c, 없으면 +84 SID로 카메라 이동 |
| ViewYourPriest | `second`를 owner로 사제 보기 |
| HideBuildings | redraw=1, 건물 숨김 토글 |
| ReloadTheams | 594fc8이 거짓일 때 두 SID 목록을 차례로(콜백 뒤 현재 개수를 다시 읽으며) Form +0e/+12의 X/Y로 `reloadAt(x, y, 0)` |
| GetFullDeck | Editor 또는 mission이면 하위 전체 Deck 부모 호출 |
| AddSp | Editor 또는 mission이면 signed `second`를 float32로, 아니면 0으로 `addLocalSp`(0 가산도 생략하지 않음) |
| PlaySound | 모드·소유자 관문 없이 `text(second)`로 얻은 이름을 `sound(name, 0, 0, 0, 0, 0)` |

선택한 분기가 요구하는 효과만 필수이며 미연결 효과는 `Error`(`/미연결/`), Player/Form의 범위 밖 읽기는 `RangeError`로 거부한다. 전역은 사본 없이
`read(주소)` getter와 `{ value }` 상자(hideBuildings, redraw)로 현재 값을 읽고 쓴다.

## 시험

`webpj/tests/game-menu-actions.test.mjs` 6건:

1. 원본 관측 `game-menu-actions-x86.tsv` **610행**: 종료 종류·최종 전역 16개·장치 바이트·큐 커서/물리 큐·본체 효과 순서(부분 중단 26·QuitApp 즉시 반환 16·
   Guide/Gump 앞단이 소비해 건너뛴 270 포함)를 독립 장치(Player/Form/SID/큐)와 함께 대조했다. **첫 실행에서 610행 전부 일치**했다.
2. `cheat-money-x86.tsv` **144행**: AddSp의 관문 읽기 순서·signed→float32 비트·중단 시 부분 효과. 첫 실행 전부 일치.
3. `full-deck-input-x86.tsv` **384행**: GetFullDeck의 Guide/Gump 앞단·Editor/mission 관문·release와 하위 호출 60회. 첫 실행 전부 일치.
4. 계약: 비대상(MenuJustClosed·미등록 명령)·release는 효과 장치 없이 반환, 선택한 효과 누락은 성공으로 대체하지 않음, 생성자는 전역 getter/명령 조회 필수.
5. 현재 전역 읽기 순서와 단락(원본 fixture가 읽기 순서를 관측하지 않아 직접 확인): Pause는 mission(594fa4)→Editor(5c85a4), ViewHome은 소유자(540c70)를 두 번 재조회,
   RequestQuitApp은 Editor→594fb8→594fc8.
6. GetFullDeck의 Editor 읽기 단락·release 무읽기, PlaySound의 text 뒤 고정 0 다섯 개(음수 입력은 하위 32비트)와 text 훅 누락 거부.

**변형 확인:** 모듈에 16가지 변형(Pause 인자 반전·ViewHome 둘째 SID 제거·HideBuildings 반전·Reload X/Y 교환·AddSp 관문 순서·RequestQuit Editor 분기 제거·
QuitApp 99 비교 반전·볼륨 signed 해석 제거·release 검사 제거·진단 제거·AddSp 0 생략·Reload 개수 signed 비교 제거·redraw 쓰기 제거 등)을 차례로 넣어
시험이 모두 실패함을 확인했다. 처음 한 변형(Pause 두 전역 읽기 순서 교환)만 살아남았는데, 원본 610행이 읽기 순서를 관측하지 않기 때문이라
5번 시험을 추가해 잡았다.

`webpj/tests/fixtures/manifest.json`에 `game-menu-actions`(610행)·`cheat-money`(144행)·`full-deck-input`(384행) 항목(원본 10.78)을 추가했다.

## 남은 경계

- `PauseKeyInput`(E20)이 예약한 Pause 명령을 이 처리기가 소비하는 시점의 결합은 UserInput 전체 부모(`ClientInput` 계열) 조립에서 대조한다.
  이 단위에서는 Pause 명령(0x020000cf) 몸체를 위 610행으로 대조했다.
- `GetFullDeck`가 부르는 하위 전체 Deck 갱신 부모(`GetFullDeckCommand`, cpppj `o/RawDeckRestore`)는 PORT_MAP에서 `exclude-edit`라 웹에 옮기지 않으며, 이 모듈에는 `getFullDeck` 훅으로만 연결한다(웹이 훅을 두지 않으면 미연결 거부).
- 명령 ID 표(`GameMenuCommands`)와 실제 Client·Session 장치(큐·Tell·소리·시계·Player·Form)는 아직 연결하지 않았다(PORT_MAP `port`).
