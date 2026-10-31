# webpj 한국어 지원 3단계: F1 일반 도움말 주제 단위 영어 대체와 게임 도움말 15개 주제 번역 (WB03-P, 2026-10-31)

2026-10-31, **HJOW-X3D**(마지막 전체 정밀 디컴파일 호스트 HJOW-Athlon/2026-10-23 과 다른 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 원본 아카이브의 영어 `help.english`·[2단계 기록](webpj-korean-tell-20261031.md)·[한국어 명칭 규칙](korean-unit-names.md)·기존 webpj 도움말 변환 코드다). 사용자 요청("AGENTS.md, LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행, cpppj 는 다른 세션이 진행 중이니 수정하지 말고 README.md 같은 공통 파일은 나중에 몰아서 갱신할 예정이니 지금은 수정하지 말 것")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목 **한국어 F1 도움말 본문**을 했다. `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md`·`README.md`·공유 명칭 JSON 은 수정하지 않았다(읽기만). 원본 게임·analyzeManager 는 실행하지 않았고, 브라우저는 우리 웹 클론만 띄운 Edge 헤드리스다.

## 1. 범위와 결과

| 대상 | 상태 |
|---|---|
| 도움말 창의 **주제 단위 영어 대체** | 구현. 한국어 도움말 파일(`help.korean`)에서 주제를 못 찾으면 영어 `help.english` 에서 찾는다. |
| 번역한 주제 15개(영어 약 20KB, 전체 118KB 의 17%) | **한국어** (`help.korean`) |
| 나머지 주제 113개(앵커 138개에서 중복 앵커를 빼면 128개 주제 중) | **영어 그대로**(한국어 모드에서는 D2Coding 으로 그려짐) |

번역한 주제: `F1Help`(도움말 첫 화면)·`interfaceHelp`(사용자 인터페이스·단축키)·`sphereHelp`·`deusphereHelp`·`serenisphereHelp`·`pyrosphereHelp`·`bountyislandHelp`(님버스 세계와 중립 섬)·`themeHelp`(세 퓨리)·`unitHelp`(유닛 개요)·`vesselPriestHelp`(사제의 건설 능력)·`sacrificeOutline`·`immobileHelp`·`pickupHelp`·`moveyourpriestHelp`·`performsacrificeHelp`(포획과 제물 바치기 5단계). F1 도움말 첫 화면에 있는 "GAME HELP" 목록의 주제와 그 아래 연쇄다.

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/assets/game-data.js` | `readEnglishText(path)` 신규: 언어 파일 경로(끝이 `.언어`)의 영어 대응 파일을 읽는다(영어이거나 파일이 없으면 null). 기존 `fallbackTellText` 도 이 함수를 쓰도록 합쳤다. |
| `www/ui/help-window.js` | `HelpWindow` 가 `fallbackHelpText`(영어 도움말 전체 문자열)를 받아, `_show` 에서 현재 언어 파일에 주제가 없으면(`found` 가 거짓) 영어 파일에서 다시 찾는다. `createHelpWindow` 가 `readEnglishText` 로 영어 파일을 함께 넘기며, 같은 파일이거나 없으면 null 이다(영어 실행·한국어 묶음 실패 시 영어 파일을 두 번 쓰지 않음). |
| `www/assets/korean-assets.js`, `tools/prepare-assets.js`, `www/data/manifest.json` | 한국어 묶음에 `d/help.korean` 추가(묶음 파일 수 14 → 15, 원본 `webpj/translations/help.korean`). |
| `translations/help.korean`(신규, UTF-8, LF, BOM 없음) | 번역 자료. 첫 앵커 앞의 글은 화면에 나오지 않으므로 거기에 용도·규칙을 적었다. |

## 3. 번역 방식과 결정

- **구조는 영어와 똑같이, 보이는 문구만 번역했다.** 태그(`<a href>`·`<h2>`·`<b>`·`<c>`·`<br>`·`<p>`)·그림(`<!"mana.8">`·`<!"windVortex.*">`)·조건(`<?{global.inMission}>`)·글자 모양 표시(`~i`·`~.`)·링크 주소·앵커 이름은 원문 그대로다. 원문의 오타 태그(`<a herf=…>`·`<b><c>F1</c></c></b>`·닫히지 않은 `<b>`)도 그대로 둬 구조 대조가 가능하게 했다.
- **`~lblue~.` → `~l파란색~.`**: 이 표시는 "blue" 라는 낱말을 연한 파랑으로 칠하는 것이라 낱말도 번역해야 한다(`파란색 글자를 누르면…`).
- **소스 줄바꿈은 줄바꿈이 아니다.** 도움말 변환은 소스의 LF 를 공백으로 바꾸고 `<br>`·`<p>` 만 줄을 나눈다(Tell 대화상자와 다름). 그래서 한국어 줄 길이는 신경 쓰지 않았고, 문장 중간에서 줄을 바꿀 때는 낱말 경계에서만 했다.
- **같은 이름의 앵커가 둘인 주제는 첫 앵커만 보인다.** 영어 파일에는 `themeHelp`·`unitHelp`·`vortexHelp`·`spVortexHelp`·`golemVortexHelp`·`influenceVortexHelp` 등이 두 번씩 있는데(10.72 패치 때 남은 옛 본문으로 보인다), 렌더 결과로 첫 앵커 본문만 나옴을 확인했다(`unitHelp` 는 첫 앵커에만 있는 Whirlibase 가 나온다). 그래서 한국어 파일에는 첫 앵커만 담았다.
- **용어:** AGENTS.md 명칭 규칙과 공유 표를 따랐다. 유닛 이름은 공유 표(썬 디스크 발사기·훨리베이스·세일 스케이터·크로스보우·애시드 바리케이드·아크 스파이어·벌프·불워크·반더 타워 등), 개념어는 2단계와 같다(`Furies → 퓨리`·`Knowledge → 지식`·`Energy → 에너지`·`Storm Power → 스톰 파워`·윈드/레인/썬더). 이번에 새로 정한 말: 세 구체 `Serenisphere/Pyrosphere/Deusphere → 세레니스피어/파이로스피어/데우스피어`, `Spell → 주문`, `Hit Points → 체력`, `Transport → 수송 유닛`, `BattleMaster → 배틀마스터`, `Challenge Ring → 챌린지 링`, `Zone → 지역`, `Level One → 1단계`, `Ground Transport → 지상 수송`, `Aerial Attack → 공중 공격`, `Source of Energy → 에너지 공급원`, `Sacrificial Circle → 제물 의식의 원`.
- **게임 화면 문구는 영어 그대로 따옴표로 남겼다**: 'Start Battle'·"About"(우클릭 메뉴 항목)·`Escape`·`Shift`·`Pause`·`Ctrl`·`TAB` 등 키와 메뉴 이름. 게임 안 메뉴는 아직 한국어가 아니기 때문이다.

## 4. 원본과 다르게 둔 점·가정

1. **번역 문구는 이 프로젝트가 새로 쓴 것이다**(원본에는 한국어가 없다). 개발자 검수 대상이며 특히 세 구체 이름(`세레니스피어`·`파이로스피어`·`데우스피어`)·`제물 의식의 원`·`지상 수송` 같은 분류어가 선택이 갈리는 부분이다.
2. **주제 단위 영어 대체는 웹 적응이다.** 원본은 도움말 파일 하나만 읽는다. 대체 판정은 `renderHelpTopic` 의 `found`(앵커를 찾았는가)이며, 앵커가 있으나 본문이 비어 있는 주제(`campaignHelp` 등 앵커만 있는 별칭)는 한국어 파일에 없으므로 영어 파일의 빈 본문 그대로 나온다.
3. **한국어 모드의 영어 주제 본문은 D2Coding 으로 그려진다**(2단계와 같은 글꼴 결정). 한 줄이 Arial 보다 넓어 스크롤바 아래로 몇 픽셀 겹칠 수 있다(기존 배치 그대로).
4. **번역한 주제에서 번역하지 않은 주제로 링크가 닿는다**(예: `unitHelp` → `statsHelp`·`transportHelp`). 그 주제는 영어로 열린다. 번역을 늘리면 이 경계가 줄어든다.
5. 도움말의 에너지 기호 그림(`<!"mana.8">` 등)은 기존처럼 아직 그려지지 않고 콘솔에 경고 한 줄씩이 남는다(WB03-G 부터 알려진 미지원, 이번 변경과 무관하며 영어에서도 같음).
6. `NO_HELP_TEXT` 번역은 2단계 기록 4번처럼 닿지 않는 경로라 하지 않았다.

## 5. 검증(HJOW-X3D, Node v24.21.0, Edge 156 헤드리스)

- `npm --prefix webpj test` **140개 통과**(신규 7: `tests/korean-help.test.mjs`, 기존 133 그대로). `check-assets` 통과.
  - **구조 대조:** 번역한 주제 15개가 영어 첫 앵커와 태그·그림·링크 주소·조건·`~` 표시가 같다(정렬 비교). 영어에 없는 앵커·중복 앵커·한글이 없는 주제가 없고 번역 주제 목록이 고정되어 있다.
  - **영어 문장 잔존 검사:** 번역하지 않고 남은 네 글자 이상 영어 낱말은 고유 이름·키·메뉴 이름 16개(`NetStorm`·`Escape`·`Shift`·`Salvage` 등)뿐이다.
  - **렌더:** 각 주제의 링크 목적지 집합이 영어와 같고 한글 본문이며, 앵커가 둘인 `themeHelp`·`unitHelp` 가 한 번만 나온다. 번역하지 않은 주제(`statsHelp`)는 한국어 파일에서 찾지 못하고 도움말 창이 영어로 대체한다.
  - **창:** 한국어 창에서 번역 주제는 한국어(유니코드 코드), 번역하지 않은 주제로 열면 영어, 뒤로 가기는 방문 기록대로 한국어 주제로 돌아온다. 번역 15개 주제가 4개 화면 크기(800×600·1024×768·1366×768·1280×800)에서 모두 열려 화면 안에 들어간다. 영어 창과 한국어 묶음 실패 창은 영어 파일을 두 번 쓰지 않는다.
- **변이 검사** 한국어 자료 11종(링크 목적지·`<br>`·조건·그림·글자 모양 표시·링크 색 표시·오타 태그 수정·영어에 없는 앵커·앵커 중복·주제 삭제·주제를 영어로 되돌림)과 소스 4종(도움말 창의 영어 대체 제거·영어 파일 미전달·같은 파일도 전달·영어 파일 경로 오류)이 **모두 걸렸다**(첫 실행에서 '주제를 영어로 되돌림'을 놓쳐 영어 문장 잔존 검사를 보강했다). `readEnglishText` 의 `language === 'english'` 조기 반환은 경로 비교와 겹치는 중복 방어라 지워도 결과가 같다(동치 변이, 가독성을 위해 남김).
- **실제 브라우저(마우스 클릭·키보드, CDP)** 1024×768·1920×1080·1366×768·1280×800 × 한국어·영어 118개 중 117개 통과(첫 1024×768 실행에서 콘솔 검사 하나가 4번 5의 기존 그림 경고를 잡아 이후 실행부터 그 경고만 제외하고 전부 통과): F1 → 첫 화면 한국어(유니코드)·영어는 8비트 영어 → 링크 `interfaceHelp`(한국어 제목) → 뒤로 → 번역하지 않은 `priestType`(영어 문장) → 뒤로 → `unitHelp` → `statsHelp`(영어) → 뒤로 두 번 → `sacrificeOutline`(한국어) → `immobileHelp` → 확인으로 닫기. 스크린샷으로 한국어 도움말 첫 화면·사용자 인터페이스·유닛 개요의 글자 모양과 줄바꿈을 눈으로 확인했다(강조색·링크색 정상).
- 확인하지 않은 것: Chrome/Firefox·터치·소리 출력·기기 픽셀 비율 2 이상의 한글 모양·번역의 어색함(검수 몫)·에너지 기호 그림(미지원).

## 6. 남은 일

(2026-11-01 WB03-Q 에서 아래 1번의 우선순위 14개 주제와 그 연쇄(신전·워크샾 하위 주제, 별칭 앵커 11개)는 번역을 마쳤다. [4단계 기록](webpj-korean-help-20261101.md) 참고. 남은 주제는 그 문서 6절이 최신이다.)

1. **나머지 도움말 주제 번역**: 산문 주제 41개(영어 약 53KB)와 유닛·건물 설명 `…Type` 주제 72개(약 36KB). 우선순위는 이미 번역한 주제에서 링크로 닿는 것: `transportHelp`·`technologyHelp`·`statsHelp`·`stormPowerHelp`·`influenceHelp`·`vortexHelp`·`factoryHelp`·`tacticsHelp`·`salvageHelp`·`minimapGumpHelp`·`moneyGumpHelp`·`teleportViewHelp`·`multiplayerHelp`·`multiQuickHelp`. `…Type` 주제는 타입 표(`.type`)가 생겨 `<info>` 머리글을 그릴 수 있게 될 때 함께 한다(지금은 본문만 나옴).
2. 번역 문구 개발자 검수, 에너지 기호 그림(`<!"mana.8">`) 지원(WB03-G 때부터 미지원).
3. 맨 위 인계의 나머지(미션 스크립트 한국어·Auto-Demo·Options −40 규칙·본문 끌기 Overlay·커서 2~14), 그 뒤 정적 맵 화면(M2).
