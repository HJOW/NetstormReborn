# webpj 캠페인 하위 목록 검증: Early Missions 등 5종 (WB03-E, 2026-10-29)

2026-10-29, **HJOW-X3D**. 사용자 요청("AGENTS.md·LEFT_JOBS.webpj.md 를 읽고 webpj 다음 단계 진행")에 따라 직전 단위(WB03-D)의 '다음' 첫 항목, 즉 Campaign 대화상자 아래의 **캠페인 하위 목록 5종**(`$Checked` 체크 표시·잠금·`MissionBegin`)을 원본 스크린샷과 대조했다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md 는 수정하지 않았다(cpppj 는 읽기만 했고 다른 PC 의 세션이 cpppj 를 진행 중이다). 원본 게임·analyzeManager 는 실행하지 않았다.

## 1. 디컴파일 호스트와 근거

현재 PC 는 **HJOW-X3D** 이고 공통 인계의 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23** 이라 불일치한다. 이 PC 의 `extracted/refined/originals/` 는 2026-10-27 에 중단된 재디컴파일 잔재(`raw.c` 등이 없는 불완전한 자료)다. 사용자가 "cpppj 가 어느 정도 플레이 가능한 수준이니 디컴파일 산출물 대신 cpppj 소스를 참고하라" 고 지시해 **재디컴파일을 하지 않았고 디컴파일 소스도 읽지 않았다.** 마지막 디컴파일 호스트는 HJOW-Athlon 그대로다.

| 항목 | 근거 |
|---|---|
| `$Checked` 규칙(체크 표시 = 4번째 값 ≠ 0, 활성 = 5번째 값 ≠ 0 \|\| `userCheat` \|\| 특수 모드) | cpppj `HtmlWindowMetadata.cpp`, [Checked 전역 기록](exe/cpp-html-checked-client-20261025.md) |
| 값 해석(`ConfigParseLong`) | cpppj `o/ConfigInterface.cpp` |
| 체크 표시 위치·행 그리기 | cpppj `MenuGumpDraw.cpp`(체크 SHP 는 항목 위치 +5, +7) |
| 진행 값(`DoneTutorial1` 등) | [설정 형식](formats/config.md), 읽기 전용으로 해독한 `originals/d/options.cfg` |
| 시각 기준 | `screenShots/mainMenu - Campaign - *.png` 5장 |

## 2. 스크린샷의 상태는 저장된 진행 값 그대로다

`originals/d/options.cfg`(XOR 인코딩, 읽기만 함)에 `DoneTutorial1~6`, `DoneThewarbegins`·`DoneMasterofwhirligigs`·`DoneSavetheisland`·`DoneFragilefortune`·`DoneThunderingpower`·`DoneDissolvedalliance`, `DoneThenoose`·`DoneRainvsrain`·`DoneRunforit` 이 `1` 이고 나머지(`DoneGuardMyBack`·`DoneSurrounded`·`Donebc*` 등)는 없다. 이 값을 설정에 넣으면 다섯 스크린샷의 행 상태가 모두 그대로 나온다.

| 화면 | 행 상태(체크 표시 / 활성 / 잠김) |
|---|---|
| Early Missions | 6행 모두 체크 표시 |
| Priest Training | 체크 표시 없음, 앞 7행 활성(고정 인자 1~7), `Fury War`·`Secret` 잠김(활성 식이 `{Donebc*}` 이어 붙임 → 빈 문자열 → 0) |
| Struggle For Freedom | 6행 모두 체크 표시 |
| A Nation Rises | 앞 3행 체크 표시, 4행 활성(앞 미션 완료·자신은 미완료), 5행 잠김 |
| Complete Victory | 5행 모두 잠김(`{DoneSurrounded}` 없음) |

처음 설치한 상태(진행 값 없음)에서는 묶음마다 첫 행(고정 인자 1)만 활성이다. 키 이름은 대소문자를 구분하지 않아 저장 파일의 `DoneThewarbegins` 가 목록의 `{DoneTheWarBegins}` 와 같다.

## 3. 대조 결과

구현(WB03-D)은 이미 형식을 옮겨 둔 상태였고, 코드 변경 없이 그린 결과가 스크린샷과 일치했다. 신규 검사 `dialog.test.mjs` 의 두 항목이 이를 고정한다.

- **위치:** 다섯 화면 모두 목록 영역 정렬 탐색에서 최적 클라이언트 원점이 이웃 원점보다 **3.5~19배** 좋고(Early 5.2배·Priest 6.9배·Struggle 19배·Nation 5.4배·Victory 3.5배), 원점이 모두 x 2~7, y 33~35 안이다. 패널 크기는 338×311·418×404·458×353·258×251·258×247 이며 [메인 메뉴 문서](screens/main-menu.md)의 대략 범위(±5)와 맞는다.
- **글자:** 목록 영역의 흰 글자 마스크(채널 최솟값 ≥ 215)의 교집합/합집합이 Early **0.995**·Priest **0.997**·Struggle **1.000**·Nation **0.994** 이다(Complete Victory 는 흰 글자가 없어 비교 대상이 아니다).
- **평균 채널 차이:** 목록 1.8~6.6, 본문 2.8~5.2, 버튼 1.6~6.5. 차이의 대부분은 **캡처 노이즈**다. 같은 화면에서도 행·열별 차이가 번갈아 오르내리는 디더링 무늬이고(Early 목록 열별 3~10), 글자 마스크가 거의 완전히 일치하며, 같은 구현이 Struggle 에서는 1.8 이다. 패널 질감 전체의 픽셀 일치는 주장하지 않는다.
- **변이 검사:** 목록 세로 여백 1→2(4개 실패), 체크 표시 x +5→+9(신규 스크린샷 검사만 실패), 잠금 판정 반전(4개 실패)이 모두 검사에 걸림을 확인했다.

## 4. 코드 변경: ConfigParseLong

`$Checked` 의 완료·활성 값은 원본 `ConfigParseLong`(앞 공백 건너뜀·부호 하나·십진 숫자에서 멈춤·부호 없는 32비트 감김)으로 읽는다. 기존에는 `parseInt(…)|0` 이라 `0x10`·부호·감김은 같지만 20자리가 넘는 이어 붙임에서 정밀도를 잃었다. `www/assets/config.js` 에 `configParseLong` 을 추가하고 `www/ui/html-text.js` 의 `$Checked` 가 쓴다(검사 `config.test.mjs` 신규 1).

## 5. 웹에서 지원하지 않는 부분

- 활성 식의 `userCheat`(설정 값이 `0x845fed` 일 때만 유효한 숨은 우회)와 특수 모드(`0055a480`, 설정 해석에서 항상 0)는 지원하지 않는다. 웹 판본에는 설정 파일을 직접 편집하는 경로가 없다.
- `MissionBegin` 행은 누르면 대화상자를 닫고 콘솔 안내만 한다(미션 단계에서 연결).
- `User Made Campaigns`(`%{userSpec}`) 목록은 사용자 맵이 웹 자산이 아니므로(AGENTS.md: 커스텀 맵 제외) 만들지 않았다. 메뉴에서 뺄지 빈 목록으로 둘지는 다음 단위에서 정한다.

## 6. 확인하지 않은 것

저장된 진행 값은 이번에 **고정 입력**으로 넣었다. 웹에서 미션 완료 후 값을 쓰고 브라우저 저장소에서 읽는 경로(설정 저장)는 아직 없다. 실제 브라우저에서의 이 화면 클릭 확인은 직전 단위(Credits→Campaign→Early Missions 목록 열기)에서 했고 이번에는 Node 합성만 검사했다.
