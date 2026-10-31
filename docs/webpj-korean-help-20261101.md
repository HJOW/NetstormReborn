# webpj 한국어 지원 4단계: F1 일반 도움말 34개 주제와 별칭 앵커 11개 번역 (WB03-Q, 2026-11-01)

2026-11-01, **HJOW-Athlon**(마지막 전체 정밀 디컴파일 호스트 HJOW-Athlon/2026-10-23 과 같은 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 원본 아카이브의 영어 `help.english`·[3단계 기록](webpj-korean-help-20261031.md)·[한국어 명칭 규칙](korean-unit-names.md)·공유 명칭 JSON·기존 webpj 도움말 변환 코드다). 사용자 요청("AGENTS.md, LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목 **나머지 도움말 주제 번역** 중 이미 번역한 주제에서 링크로 닿는 것부터 했다. 작업 시작 때 `git status` 에 다른 세션의 cpppj 변경(`cpppj/`·공통 `LEFT_JOBS.md`)이 있었으므로 `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md`·`README.md`(이전 단위에서 받은 '공통 파일은 나중에 몰아서 갱신' 지시를 이어 지킴)·공유 명칭 JSON 은 수정하지 않았다(읽기만). 원본 게임·analyzeManager 는 실행하지 않았고, 브라우저는 우리 웹 클론만 띄운 Edge 헤드리스다. **코드 변경은 없다**(번역 자료·자산 복사본·매니페스트·테스트만 바뀜).

## 1. 범위와 결과

| 대상 | 상태 |
|---|---|
| 새로 번역한 주제 본문 19개(영어 약 25KB) | **한국어** (`help.korean`) |
| 새로 더한 별칭 앵커 11개(본문 없이 다음 앵커와 묶이는 앵커) | 영어와 같은 자리에 두어 같은 본문이 나온다 |
| 번역 전체 | 본문 34개 + 별칭 11개 = 앵커 45개. 영어 도움말 고유 앵커 128개 중 **약 35%**, 영어 글자 수로는 약 **41%**(45KB / 110KB) |
| 아직 영어로 대체되는 주제 | 산문 14개(약 30KB)·`…Type` 57개(약 35KB)·별칭 12개 |

새 본문 19개: `transportHelp`(수송 유닛)·`technologyHelp`(지식)·`statsHelp`(유닛 정보와 분류 목록, 링크 69개)·`stormPowerHelp`(스톰 파워)·`influenceHelp`(에너지)·`vortexHelp`(신전)·`spVortexHelp`·`golemVortexHelp`·`influenceVortexHelp`(신전 하위 3개)·`thunderFactoryType`(= 워크샾 설명)·`upgradeHelp`(워크샾 업그레이드)·`shooterHelp`(사격 유닛)·`tacticsHelp`(전술)·`salvageHelp`(회수)·`minimapGumpHelp`(하늘 전경)·`moneyGumpHelp`(스톰 파워 보유량 창)·`teleportViewHelp`(생산 창)·`multiplayerHelp`(캠페인과 멀티플레이어 모드)·`multiQuickHelp`(멀티플레이어 빠른 시작).

별칭 앵커 11개: `researchHelp`→`technologyHelp`, `moneyHelp`→`stormPowerHelp`, `tacticHelp`→`tacticsHelp`, `campaignHelp`→`multiplayerHelp`, `rainVortexType`·`windVortexType`·`thunderVortexType`→`vortexHelp`, `factoryHelp`·`sunFactoryType`·`windFactoryType`·`rainFactoryType`→`thunderFactoryType`. F1 도움말 첫 화면의 "캠페인과 멀티플레이어 모드"와 유닛 분류 목록의 신전·워크샾 행이 이제 한국어로 열린다(이전에는 영어 대체였다).

## 2. 구현

| 파일 | 내용 |
|---|---|
| `translations/help.korean` | 새 주제 30개 앵커 추가(약 28KB, UTF-8·LF·BOM 없음). 머리 설명에 별칭 앵커 규칙을 한 줄 더했다. |
| `www/data/d/help.korean`, `www/data/manifest.json` | `npm --prefix webpj run prepare-assets` 로 복사본·매니페스트 갱신(크기 23217 → 51582 바이트, 설명 문구는 그대로). |
| `tests/korean-help.test.mjs` | 번역 주제 목록을 45개로 늘리고 **별칭 앵커 검사**를 더했다(아래 5절). 번역하지 않은 주제의 예로 쓰던 `statsHelp` 를 `altarType` 으로 바꿨다. 8개 중 1개 신규(전체 140 → 141). |

도움말 창·변환 코드(`help-window.js`·`help-topic.js`)와 영어 대체 구조는 3단계 그대로다.

## 3. 번역 방식과 결정

- **구조는 영어와 똑같이, 보이는 문구만 번역했다**(3단계와 같다). 태그·그림(`<!"mana.8">` 등)·조건·`~y`/`~r`/`~.` 같은 글자 모양 표시·링크 주소(`#suncannontype` 같은 영어 대소문자 불일치 포함)·앵커 이름은 그대로다. 같은 이름의 앵커가 둘인 주제(`vortexHelp`·`spVortexHelp`·`golemVortexHelp`·`influenceVortexHelp`)는 영어의 첫 앵커만 담았다.
- **별칭 앵커**: 영어 원문은 `<a name="moneyHelp">` 처럼 본문이 없는 앵커를 본문 앵커(`stormPowerHelp`) 바로 앞에 붙여 두어, 별칭 주제를 열면 그 아래 본문이 나온다(도움말 변환이 `</a>` 에서 멈추기 때문). 한국어 파일도 같은 자리에 그대로 두었다. 신전(`</a>` 4개)·워크샾(`</a>` 5개) 묶음은 앵커 수만큼 닫는 태그를 맞췄다.
- **새로 알게 된 동작**: 별칭 앵커로 열면 본문 뒤에 링크 색 복원 코드(`~[T0]~N~.`)가 앵커 수만큼 더 붙는다. 영어도 똑같고 눈에 보이는 차이는 없다(원본 파서의 동작을 그대로 재현). 새 테스트가 이를 영어와 대조한다.
- **원문의 모순은 그대로 번역했다**: `stormPowerHelp` 는 적 유닛을 파괴하면 스톰 파워의 "절반"을 얻는다고 하고 `tacticsHelp`·`themeHelp` 는 25% 라고 한다(영어 원문이 이미 어긋남). 번역으로 고치지 않았다.
- **용어**: AGENTS.md 명칭 규칙과 공유 표를 그대로 따랐다(유닛·건물 이름 71개는 `unit-names.korean.json`, 개념어는 1~3단계와 같다). 이번에 새로 정한 말: `Alignment → 계열`, `Class → 분류`, `Hits → 체력`, `Range → 사거리`, `Damage → 피해`, `Cost in Storm Power → 스톰 파워 비용`, `Energy to Build → 건설에 필요한 에너지`, `Holy Place → 성지`, `Source of Storm Power → 스톰 파워 공급원`, `Shooter → 사격(유닛)`, `Offensive/Defensive/Summons Spell → 공격/방어/소환 주문`, `Production Window → 생산 창`, `Production Slot → 생산 칸`, `Storm Power Available Window → 스톰 파워 보유량 창`, `Sky Overview → 하늘 전경`, `Rank → 랭크`, `Reliability Ratings → 신뢰도 등급`, `Battle Options → 전투 옵션`, `Player Settings → 플레이어 설정`, `Multiplayer Survival Guide → 멀티플레이어 생존 가이드`, `Original Campaigns → 오리지널 캠페인`, "Early Missions" → "초기 미션"(캠페인 목록 이름과 같음), `Tenth Circle → 열 번째 원`(앞서 `Sacrificial Circle → 제물 의식의 원` 과 같이 Circle → 원), 인용문 인물 `Haldor Hurlmen → 할도르 헐멘`·`Spen Lodel → 스펜 로델`, `Nimbian → 님버스인`. `Salvage` 는 팁·`unitHelp` 처럼 `회수(Salvage)` 로 적었다(게임 안 우클릭 메뉴가 아직 영어이기 때문).
- **주문(스펠) 이름 18개는 새로 음역했다**(공유 명칭 JSON 에 없음): 포인트 블라스트·데버스테이션·데시메이션·트리즌·밤바드먼트·그래비톤·썬더스톰·썬더 스트라이크·다리 하든·힐·인비저빌리티·패럴리시스·훨윈드·트위스터·보텍스·하이드라·하이드라 웨이브·하이드라 플러드. 영어 발음을 한국어로 적는다는 명칭 규칙을 따랐고, `Bridge Harden` 만 `Bridge` 가 번역 허용 낱말이라 `다리 하든` 으로 했다. **개발자 검수 대상**이다(나중에 주문 이름 표가 생기면 거기에 맞춘다).
- **게임 화면 문구는 영어 그대로 따옴표로 남겼다**: 우클릭 메뉴 항목 "Drop ...", 명령 `.format`, 시험용 이름 `devo12`(`multiQuickHelp` 의 플레이어 목록 그림 설명). 게임 안 메뉴가 아직 한국어가 아니기 때문이다.
- **예시 문구**: `moneyGumpHelp` 의 건설 비용 예시 `"Cannon 1000"` 은 유닛 이름이 들어가는 자리라 `"캐논 1000"` 으로 적었다.

## 4. 원본과 다르게 둔 점·가정

1. **번역 문구는 이 프로젝트가 새로 쓴 것이다**(원본에는 한국어가 없다). 개발자 검수 대상이며 특히 위 주문 이름 18개·인용문의 말투·`하늘 전경`·`스톰 파워 보유량 창`·`랭크` 가 선택이 갈리는 부분이다.
2. **번역한 주제에서 번역하지 않은 주제로 링크가 닿는다**(`…Type` 유닛·건물 설명, 멀티플레이어 상세 주제). 그 주제는 영어로 열린다. 번역을 늘리면 이 경계가 줄어든다.
3. **그림 미지원 경고**: `mana.*`(에너지 기호)·`geyser.*`·`nugget.*`·`*vortex.*`·`*battery.*`·`*Cannon.*` 같은 그림은 아직 그려지지 않는다(WB03-G 부터 알려진 미지원, 영어도 같음). 그림이 많은 주제(`influenceHelp`·`stormPowerHelp`)에서 경고가 늘었다. `fortgump.3`(스톰 파워 아이콘)은 이미 그려진다(`moneyGumpHelp`·`salvageHelp` 스크린샷에서 확인).
4. `<info>` 머리글(`vortexHelp`·`thunderFactoryType` 의 `<info>`)은 타입 표(`.type`)가 없어 여전히 그려지지 않는다(영어도 같음).
5. 영어 원문의 오타·옛 문구(`Golem are dumb` 등)는 의미만 옮겼다.

## 5. 검증(HJOW-Athlon, Node v24.21.0, Edge 헤드리스)

- `npm --prefix webpj test` **141개 통과**(신규 1: `renderHelpTopic: 별칭 앵커가 영어와 같은 본문 앵커의 본문을 보여 준다`, 기존 140 그대로·번역 주제 목록과 허용 영어 낱말 세 개(`Drop`·`devo`·`format`) 갱신).
  - **구조 대조:** 번역한 45개 앵커가 영어 첫 앵커와 태그·그림·링크 주소·조건·`~` 표시가 같다(정렬 비교). 별칭 여부(영어에서 보이는 글이 없는 앵커)가 테스트의 별칭 표와 일치하는지도 함께 본다.
  - **별칭 앵커:** 한국어·영어 모두에서 별칭 11개가 본문 앵커와 같은 본문·링크를 보여 주고(끝의 링크 색 복원 코드 꼬리는 영어와 같음). 첫 실행에서 이 꼬리 때문에 "별칭과 본문 앵커의 결과가 완전히 같다"는 단언이 실패했는데 원본 파서의 동작이라 자료 문제가 아니었고, 꼬리를 뺀 본문이 같고 꼬리 자체가 영어와 같은지를 대조하도록 비교 방식을 고쳤다.
  - **영어 문장 잔존:** 번역하지 않고 남은 네 글자 이상 영어 낱말은 고유 이름·키·메뉴 이름뿐이다.
  - **창:** 45개 앵커가 4개 화면 크기(800×600·1024×768·1366×768·1280×800)에서 모두 열려 화면 안에 들어가고 본문이 유니코드 코드다. 번역하지 않은 `altarType` 은 영어 파일로 대체된다.
- **변이 검사** 한국어 자료 18종(링크 목적지 변경 2·링크 하나 풀기 1·별칭 앵커 삭제 4·닫는 태그 하나 빠짐 1·글자 모양 표시 변경/삭제 2·그림 이름 변경 1·영어 문장·낱말 남김 3·중복 앵커 추가 1·영어에 없는 앵커 1·태그 하나 추가 1·주제 본문 비우기 1)이 **모두 걸렸다**(처음 시도한 링크 변이 하나는 같은 문구가 두 곳에 있어 적용되지 않아 고유한 문구로 바꿔 다시 했다)(자료는 소스·복사본을 같이 바꿔 실행하고 끝나면 원래 내용으로 복원 확인). 이번 단위는 코드를 바꾸지 않아 소스 변이는 하지 않았다.
- **실제 브라우저(진짜 마우스 클릭·휠·키보드 입력, CDP):** 1024×768·1920×1080·1366×768·1280×800 × 한국어·영어, 8회 × 217개 = **1736개 모두 통과**(콘솔 오류·예외 0, 경고는 그림 미지원뿐). 같은 흐름을 매번 반복한다: 클릭하여 시작 → 메인 메뉴 → F1 로 도움말 → 링크 위치를 찾아 **진짜 클릭**으로 15개 링크를 따라가고(`unitHelp`→`statsHelp`→`sunFactoryType`(별칭)→Back 2번→`salvageHelp`→Back→`tacticsHelp`→Back 2번→`sacrificeOutline`→`transportHelp`→번역하지 않은 `priestType`→Back 2번→`technologyHelp`→`factoryHelp`(별칭)→`upgradeHelp`→`moneyHelp`(별칭)→`moneyGumpHelp`→`teleportViewHelp`→`interfaceHelp`) Back 7번, 매 단계 주제 이름·한글 유무(영어 모드는 한글 없음, 한국어 모드의 `priestType` 은 영어 대체)·창이 화면 안인지 확인, 본문이 긴 주제는 휠로 링크가 보일 때까지 스크롤, 방문 기록을 확인하고 OK 단추 클릭으로 닫는다. 이어 번역한 35개 주제(별칭 포함)를 모두 열어 본문 줄·한글·화면 안을 확인하고 번역하지 않은 `altarType` 은 한글이 없음(영어 대체)을 확인한 뒤 Escape 로 닫는다. 스크린샷으로 한국어 `statsHelp`·`moneyGumpHelp` 의 글자 모양·줄바꿈·링크색·노랑/빨강 숫자·스톰 파워 아이콘을 눈으로 확인했다
- 확인하지 않은 것: Chrome/Firefox·터치·소리 출력·기기 픽셀 비율 2 이상의 한글 모양·번역의 어색함(검수 몫)·에너지 기호 등 그림(미지원).
- 환경 메모: 이 PC(HJOW-Athlon)는 `core.autocrlf=true` 라 `fonts/License.txt` 의 작업 트리 사본이 CRLF 다(저장소 blob 은 LF, CRLF 를 LF 로 바꾸면 매니페스트의 해시 `5755b8a0…` 와 정확히 같다). `prepare-assets` 를 실행하면 이 파일의 복사본·매니페스트 해시가 CRLF 기준으로 바뀌므로 **그 두 곳은 되돌리고 커밋한다**(저장소 상태는 LF 기준이라야 다른 PC 에서도 맞는다). 그래서 **커밋하는 상태로 이 PC 에서 `npm --prefix webpj test` 를 돌리면 141개 중 2개(`자산 복사본과 매니페스트가 assets/game-data 원본과 같다`·`한국어 글꼴 자산: fonts/ 의 woff2 를 그대로 복사했다`)가 이 한 파일 때문에 실패한다**(139 통과). 내용 차이가 아니라 이 PC 의 줄바꿈 현상이며 이번 변경과 무관하다. 위 141개 통과는 `prepare-assets` 로 이 PC 기준에 맞춘 상태에서 돌린 결과이고, 그 상태에서 `check-assets` 도 통과했다.

## 6. 남은 일

(2026-11-01 WB03-R 에서 아래 1번의 산문 주제 14개는 `serverHelp`(원문이 제작진의 미완성 틀) 하나를 빼고 번역을 마쳤다. [5단계 기록](webpj-korean-help-multiplayer-20261101.md) 참고. 남은 주제는 그 문서 6절이 최신이다.)

1. **나머지 도움말 주제 번역**: 산문 14개(약 30KB)는 대부분 멀티플레이어 설명이다 — `chatViewHelp`·`formatCommandHelp`·`chathelperHelp`·`serverHelp`·`bombHelp`·`outpostHelp`·`rankHelp`·`multiSafeHelp`·`reliabilityHelp`·`battlemasterHelp`·`playerListViewHelp`·`battleStatusViewHelp`·`playerOptionsHelp`·`battleOptionsHelp`. 별칭 앵커가 본문과 묶인 것(`zoneHelp`·`playerListView`·`battleStatusView`)은 영어에서의 자리를 확인해 같이 옮긴다. `…Type` 57개(약 35KB)와 그 별칭(`daisType`·`runeType`·`obeliskType`·`buriedType`·`bombExplodeMediumType`·`geyserType`·`thunderWalkerType`·`outpostType`)은 타입 표(`.type`)가 생겨 `<info>` 머리글을 그릴 수 있게 될 때 함께 한다(지금은 본문만 나옴). 번역 주제를 더할 때 코드 변경은 없고 `tests/korean-help.test.mjs` 의 `TRANSLATED`·`ALIAS_TARGETS` 와 영어 낱말 허용 목록만 갱신한다.
2. 번역 문구 개발자 검수(특히 주문 이름 18개), 에너지 기호·간헐천·건물 그림 지원(`<!"mana.*">` 등).
3. 맨 위 인계의 나머지(미션 스크립트 한국어·Auto-Demo·Options −40 규칙·본문 끌기 Overlay·커서 2~14), 그 뒤 정적 맵 화면(M2).
