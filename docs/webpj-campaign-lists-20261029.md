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
- `User Made Campaigns`(`%{userSpec}`) 목록은 사용자 맵이 웹 자산이 아니므로(AGENTS.md: 커스텀 맵 제외) 만들지 않았다. **2026-10-30 사용자 결정: 웹 판본은 이 항목을 제공하지 않고 Campaign 목록에서 노출하지 않는다**(빈 목록으로 두지도 않는다). 목록의 마지막 행(User Made Campaigns, `offical6.overview`)과 그 앞 구분선을 뺀 6행이 웹의 Campaign 목록이 된다. **2026-10-30 WB03-I 에서 코드에 반영했다**(§7). 이 문서의 스크린샷 대조(§2~§3)는 8행 원본 기준이며, 웹 6행은 제외한 두 행을 뺀 부분에만 그 대조가 적용된다.

## 6. 확인하지 않은 것

저장된 진행 값은 이번에 **고정 입력**으로 넣었다. 웹에서 미션 완료 후 값을 쓰고 브라우저 저장소에서 읽는 경로(설정 저장)는 아직 없다. 실제 브라우저에서의 이 화면 클릭 확인은 직전 단위(Credits→Campaign→Early Missions 목록 열기)에서 했고 이번에는 Node 합성만 검사했다. *(진행 값 쓰기·저장은 2026-10-30 WB03-H 에서 했다 — [진행 값 저장 기록](webpj-progress-storage-20261030.md). Campaign 목록의 6행 변경과 실제 브라우저 확인은 아래 §7.)*

## 7. 웹 Campaign 6행 (WB03-I, 2026-10-30, HJOW-Athlon)

사용자 결정(맨 위 §5)을 코드에 반영했다. 디컴파일 소스·원본 게임·analyzeManager 는 쓰지 않았고(마지막 디컴파일 호스트 HJOW-Athlon 과 현재 호스트가 같다) `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 도 수정하지 않았다.

| 항목 | 내용 |
|---|---|
| 구현 | `www/ui/tell-lists.js` 의 `WEB_EXCLUDED_LIST_OBJECTS = ['offical6']` 와 `expandFileList(..., { excluded })`(기본값이 이 상수, 대소문자 무시). 제외한 객체는 행도 **설정 객체 등록도 하지 않아** `offical6.overview` Tell 섹션이 열리지 않는다. 무언가를 뺀 목록은 맨 앞·맨 뒤·연달아 놓인 구분선을 지운다(`offical5sep` 이 사라짐). 아무것도 빼지 않은 목록은 원본 그대로다. |
| 제외 범위 | 목록 층에서만 뺀다. `GameData.findFiles` 와 `files.find` 는 여전히 `offical*.english` 8개를 센다. 남은 객체의 `index`(`{offical3.index}` 등)는 뺀 객체까지 센 원래 순번 그대로다. |
| 결과 배치 | 6행: Early Missions · Priest Training · 구분선 · Struggle For Freedom · A Nation Rises · Complete Victory. 패널 **238×268**(원본 8행 238×301 − 구분선 16 − 행 17 = 33). 화면 가운데에 다시 놓여 1024×768 에서 (393,250)~(631,518), 목록 (436,372)~(589,473), 본문 (443,270), Back 버튼 (493,483). 목록 폭은 가장 긴 라벨(`User Made Campaigns`)이 빠져 155→153. |

**원본과 같은 부분의 근거.** 원본 스크린샷 `mainMenu - Campaign.png` 는 8행이라 6행 화면과 직접 대조할 수 없다. 대신 (1) 8행 배치(`excluded: []`)를 스크린샷과 기존대로 대조하고(정렬 유일성·평균 차이), (2) 같은 코드의 6행 배치가 8행 배치에서 두 행만 뺀 것임을 확인했다: 본문의 흰 글자 1,122픽셀이 **한 픽셀도 다르지 않고**, 목록 앞 6행의 흰 글자 1,654픽셀도 테두리 경사면(안쪽 2픽셀, 6행 끝과 8행 끝의 위치가 달라 생기는 차이)을 뺀 영역에서 완전히 같다. 본문→목록→버튼 사이 간격도 원본과 같다. 한 칸 어긋나면 겹침이 59~66% 로 떨어져 이 비교가 위치에 민감함을 검사로 고정했다. 이 근거는 '제외한 두 행을 뺀 부분은 원본과 같다'까지이며 6행 화면 자체의 스크린샷 일치를 뜻하지 않는다(메인 메뉴 Edit·Quit·Multiplayer 칸을 뺀 것과 같은 '웹 제외 칸' 처리).

**검증.**
- `npm --prefix webpj test` **77개 통과**(신규 3: 웹 기본 목록·구분선 정리·6행 글자 모양, 기존 3개 갱신). 변이 검사 10종(제외 목록 비우기·대소문자 구분·index 계산·구분선 정리 4종·제외 객체 등록·Tell 창의 제외 무시·제외 없는 목록에도 정리 적용)이 모두 걸렸다. 첫 실행에서 '제외 없는 목록에도 정리 적용'만 놓쳐(원본 8행에는 가장자리 구분선이 없어 드러나지 않음) 구분선 파일 하나만 맞는 패턴 검사를 더했다. 변이는 Git 제외 폴더의 사본에서 했다.
- 실제 브라우저(Edge 156 헤드리스, 마우스 클릭) 1024×768 한국어·1024×768 영어·1920×1080 한국어·1366×768 영어 각 12개, 합계 **48/48**: 메인 메뉴→Campaign→6행(텍스트 5 + 구분선 1)·`User Made Campaigns` 없음·패널 238×268 가운데·목록과 Back 사이 클릭 무반응→Early Missions 하위 목록(MissionBegin 6행) 열림→Back 으로 Campaign 6행 복귀·콘솔 오류 0. 확인하지 않은 것: Chrome/Firefox·터치·소리 출력·`mainMenu - Campaign.png` 와 6행 화면의 직접 픽셀 대조(원본에 해당 화면 없음).
