# webpj 한국어 지원 5단계: F1 일반 도움말 멀티플레이어 설명·주문 등 산문 주제 번역 마무리 (WB03-R, 2026-11-01)

2026-11-01, **HJOW-Athlon**(마지막 전체 정밀 디컴파일 호스트와 같은 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 원본 아카이브의 영어 `help.english`·[4단계 기록](webpj-korean-help-20261101.md)·[한국어 명칭 규칙](korean-unit-names.md)·공유 명칭 JSON·기존 webpj 도움말 변환 코드다). 사용자 요청("webpj 다음 작업 진행")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목 **나머지 도움말 주제 번역** 중 **산문 주제**를 마쳤다. 작업 시작 때 `git status` 에 다른 세션의 cpppj 변경이 있었으므로 `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md`·`README.md`(이전 단위에서 받은 '공통 파일은 나중에 몰아서 갱신' 지시를 이어 지킴)·공유 명칭 JSON 은 수정하지 않았다. 원본 게임·analyzeManager 는 실행하지 않았고, 브라우저는 우리 웹 클론만 띄운 Edge 헤드리스다. **코드 변경은 없다**(번역 자료·자산 복사본·매니페스트·테스트만 바뀜).

## 1. 범위와 결과

| 대상 | 상태 |
|---|---|
| 새로 번역한 본문 15개(영어 약 33KB) | **한국어** (`help.korean`) |
| 새로 더한 별칭 앵커 6개 | 영어와 같은 자리에 두어 같은 본문이 나온다 |
| 번역 전체 | 본문 49개 + 별칭 17개 = 앵커 66개. 영어 도움말 고유 앵커 128개 중 약 52%, 영어 글자 수로는 약 **71%**(78KB / 110KB) |
| 아직 영어로 대체되는 주제 | `…Type` 유닛·건물·주문 설명 55개(약 31KB)와 그 별칭 5개(`daisType`·`runeType`·`bombExplodeMediumType`·`geyserType`·`thunderWalkerType`), 원문 `map`(그림 하나), `serverHelp`(아래 3절) |

새 본문 15개: `chatViewHelp`(채팅 창)·`formatCommandHelp`(.format 명령, 색 코드 표)·`chathelperHelp`(채팅 도우미 메뉴)·`outpostHelp`(아웃포스트)·`rankHelp`(랭크)·`multiSafeHelp`(멀티플레이어 생존 가이드)·`reliabilityHelp`(신뢰도 등급)·`battlemasterHelp`(배틀마스터)·`battleIslandType`(챌린지 링)·`challengeIslandType`(지역)·`playerListViewHelp`(플레이어 목록 창)·`battleStatusViewHelp`(전투 상태 창)·`playerOptionsHelp`(플레이어 설정)·`bombHelp`(주문)·`battleOptionsHelp`(전투 옵션). 별칭 앵커 6개: `outpostType`→`outpostHelp`, `zoneHelp`→`challengeIslandType`, `playerListView`→`playerListViewHelp`, `battleStatusView`→`battleStatusViewHelp`, `obeliskType`·`buriedType`→`bombHelp`. 이제 산문 주제는 `serverHelp` 하나를 빼고 모두 번역했고, 주문 아이콘 목록의 오벨리스크 링크(`#buriedType`)가 한국어로 열린다.

## 2. 구현

| 파일 | 내용 |
|---|---|
| `translations/help.korean` | 새 앵커 21개 추가(약 37KB, UTF-8·LF·BOM 없음). 머리 설명 갱신. 4단계의 `multiQuickHelp` 에서 "같은 수준의 플레이어" 를 "같은 레벨의 플레이어" 로 고쳤다(플레이어 레벨은 `레벨`, 유닛·워크샾의 단계는 `단계` 로 구분). |
| `www/data/d/help.korean`, `www/data/manifest.json` | `prepare-assets` 로 복사본·매니페스트 갱신(크기 51582 → 89146 바이트). |
| `tests/korean-help.test.mjs` | 번역 주제 목록 66개·별칭 표 17개로 갱신, **검사 보강 2가지**(아래 5절). |

## 3. 번역 방식과 결정

- **구조는 영어와 똑같이, 보이는 문구만 번역했다**(1~4단계와 같다). 태그·그림·링크 주소(`multiplayerHelp` 처럼 `#` 이 없는 주소 포함)·`~` 글자 모양 표시·화면에 글자 그대로 찍히는 코드(`~~y` 등)·원본 그림 코드(`~[I93.113]`·`~[B.3]`)는 그대로다.
- **게임 안 메뉴·옵션 이름은 영어 그대로 `<c>` 안에 두고 설명만 번역했다**: `<c>Ignore Chat</c>`·`<c>Allow Chat Sound:</c>`·`<c>Bridge Slots</c>`·`<c>Netstorm Standard</c>`·`<c>ADVANCE TO RANK X</c>` 등. 게임 안 메뉴가 아직 한국어가 아니기 때문이다(앞 단계의 `"Drop ..."`·`"About"` 과 같은 결정). 메뉴 묶음 제목(`<h3>`)은 `서식 (Format)`·`채팅 효과 (Chat Effects)` 처럼 한국어 뒤에 영어 이름을 병기했다. 메인 메뉴에 이미 번역표가 있는 항목(`옵션`·`서버 진단 통과`)은 한국어 번역표의 말을 썼다. 게임 안 `Game` 하위 메뉴는 번역표에 없어 영어로 적었다.
- **`serverHelp` 는 번역하지 않았다.** 영어 원문이 `[[ DO NOT TRANSLATE YET ]]` 로 시작하는 제작진의 미완성 틀이고, 어느 주제에서도 링크되지 않으며, 도움말 창은 이를 영어로 대신 보여 준다(원문 그대로).
- **용어**: AGENTS.md 명칭 규칙·공유 표·앞 단계 용어를 그대로 따랐다. 이번에 새로 정한 말: `Chat Window → 채팅 창`, `Chat Helper → 채팅 도우미`, `Whisper → 귓속말`, `Allies → 동맹`, `Observer → 관전자`, `Firewall → 방화벽`, `Handicap → 핸디캡`, `Battle Master → 배틀마스터`(기존), `Level → 레벨`(플레이어 레벨), `Battle Status Window → 전투 상태 창`, `Player List Window → 플레이어 목록 창`, `Zone → 지역`(기존), `pip → 표시`, 인물 `Sar Quinset → 사르 퀸셋`·`Oltor Hammershold → 올토르 해머숄드`, 색 이름 17개(파란색·연한 파란색·청록색·빨간색·초록색·연한 초록색·검은색·자홍색·짙은 자홍색·주황색·연한 주황색·갈색·짙은 황갈색·연한 황갈색·분홍색·노란색·연한 노란색·은색). 주문 18개 이름은 4단계에서 정한 음역을 `bombHelp` 에도 그대로 썼다.
- **원문의 모순·오타는 그대로 번역했다**: ① `chathelperHelp` 는 `Edit Back Format` 과 `Edit Front Format` 이 둘 다 이름 "앞"에 글을 붙인다고 쓴다(`.format`·`.back` 은 메시지 앞, `.front` 는 이름 앞이라 원문이 앞뒤가 어긋남). ② `playerListViewHelp` 의 `robst`·`fire-walled`·알아보기 어려운 한 문장은 의미만 옮겼다. ③ `rankHelp` 의 "43레벨까지 올라오며" 와 `Level` 표기의 `battleMasterHelp`/`battlemasterHelp` 대소문자 혼용 링크는 그대로다(주제 이름은 대소문자를 구분하지 않고 앞부분이 같으면 찾는다).

## 4. 새로 알게 된 영어 도움말 파일 구조

- **`battleOptionsHelp` 는 파일 맨 끝의 앵커이고 닫는 태그가 없다.** 그래서 한국어도 이를 맨 끝에 두고 닫는 태그를 더하지 않았다(영어와 같은 구조). 처음에 `bombHelp` 와 이어져 있다고 잘못 읽어 한국어 파일을 그렇게 만들었다가, 영어와 한국어의 링크 목적지 비교 테스트가 어긋나 바로잡았다.
- **`bombHelp`(주문)는 별칭 `obeliskType`·`buriedType` 앞 묶음이다**(닫는 태그 3개). 오벨리스크 링크가 닿는 곳이 이 묶음이다.
- **별칭 이름이 본문 앵커 이름의 앞부분이면(`playerListView` ⊂ `playerListViewHelp`, `battleStatusView` ⊂ `battleStatusViewHelp`) 두 앵커가 모두 주제로 잡혀 본문 맨 앞의 머리 표시(`~5~w `)가 겹쳐 나온다.** 영어도 같고 눈에 보이는 차이는 없다. 4단계의 '별칭으로 열면 끝에 링크 색 복원 코드가 앵커 수만큼 더 붙는다' 와 같은 원본 파서의 동작이다.

## 5. 검증(HJOW-Athlon, Node v24.21.0, Edge 헤드리스)

- `npm --prefix webpj test` **141개 통과**(기존 141 그대로 — 새 검사는 기존 검사 안에 넣었다. 이 PC 에서는 `prepare-assets` 로 맞춘 상태 기준, 아래 환경 메모 참고).
  - **검사 보강 2가지**: ① 구조 토큰에 화면에 글자 그대로 찍히는 코드(`~~y`)와 원본 그림 코드(`~[B.3]`) 전체를 넣고 글자 모양 표시의 대소문자를 구분한다 — 변이 검사에서 "그림 코드 숫자 변경"과 "찍히는 코드의 글자 변경"을 첫 실행에 놓쳐 보강했다. ② 영어 낱말 잔존 검사에서 **게임 안 메뉴 이름을 그대로 둔 `<c>` 문구는 영어 원문의 같은 주제에 똑같은 `<c>` 문구가 있을 때만** 허용한다(오타·새로 만든 영어 문구는 걸린다). 원본 그림 코드 `~[I…]` 도 검사에서 뺀다.
  - 별칭 검사는 이름이 앞부분으로 겹치는 별칭의 머리 표시 중복을 영어와 대조해 허용한다.
  - 구조 대조·별칭 대조(한국어·영어 모두)·링크 목적지·영어 낱말 잔존·66개 앵커가 4개 화면 크기(800×600·1024×768·1366×768·1280×800)에서 모두 열려 화면 안.
- **변이 검사** 한국어 자료 19종(링크 목적지 변경·별칭 앵커 삭제/이동 5·주문 묶음 닫는 태그 하나 빠짐·색 코드·그림 코드·그림 이름·글자 모양 표시 대소문자·찍히는 코드 글자·문장 영어로·영어 원문에 없는 `<c>` 문구·`<p>` 삭제·링크 `#` 추가·중복 앵커·본문 비우기)이 **모두 걸렸다**(첫 실행에서 2종을 놓쳐 위 ① 로 보강하고 다시 확인, 자료는 소스·복사본을 같이 바꿔 실행하고 끝나면 복원 확인).
- **실제 브라우저(진짜 마우스 클릭·휠·키보드, CDP):** 1024×768·1920×1080·1366×768·1280×800 × 한국어·영어, 8회 × 381개 = **3048개 모두 통과**(콘솔 오류·예외 0, 경고는 그림 미지원뿐). 마지막에 `rankHelp` 문구 하나를 다듬은 뒤 한국어 1024×768 을 다시 돌려 381/381 통과를 확인했다. 같은 흐름을 매번 반복한다: 클릭하여 시작 → 메인 메뉴 → F1 → 4단계의 첫 경로(링크 15개, Back 7번)와 OK 로 닫기 → 다시 F1 → **두 번째 경로**를 링크 위치를 찾아 **진짜 클릭**으로 따라간다(`campaignHelp`(별칭)→`multiQuickHelp`→`chatViewHelp`→`formatCommandHelp`→`chathelperHelp`→Back 3번→`rankHelp`→`playerOptionsHelp`→`battleOptionsHelp`→Back 3번→`outpostType`(별칭)→Back→번역하지 않은 `altarType`→Back→`playerlistView`(별칭)→`battleIslandType`→`battlemasterHelp`→Back 4번→Back→`sphereHelp`→`pyrosphereHelp`→`buriedType`(주문 묶음 별칭)) → OK 로 닫기. 매 단계 주제 이름·한글 유무(영어 모드는 한글 없음, 한국어 모드의 `altarType` 은 영어 대체)·창이 화면 안인지 확인하고, 본문이 긴 주제는 휠로 링크가 보일 때까지 스크롤한다. 이어 번역한 56개 주제(별칭 포함, 4단계 35개 + 이번 21개)를 모두 열어 본문 줄·한글·화면 안을 확인하고 번역하지 않은 `altarType` 은 한글이 없음을 확인한 뒤 Escape 로 닫는다. 스크린샷으로 한국어 `formatCommandHelp`(색 코드 표가 각 색으로 그려짐)·`buriedType`(주문)·`battleOptionsHelp`(영어 메뉴 이름은 강조색, 설명은 한국어)의 글자 모양과 줄바꿈을 눈으로 확인했다.
- 확인하지 않은 것: Chrome/Firefox·터치·소리 출력·기기 픽셀 비율 2 이상의 한글 모양·번역의 어색함(검수 몫)·그림(아이콘) 표시(미지원 그림은 경고만 남는다).
- 환경 메모: 4단계 기록과 같다. 이 PC 는 `fonts/License.txt` 작업 트리 사본이 CRLF 라 `prepare-assets` 가 License 복사본·매니페스트 항목을 바꾸므로 그 두 곳은 되돌리고 커밋했고, 커밋한 상태로 이 PC 에서 테스트를 돌리면 assets 관련 2개가 그 한 파일 때문에 실패한다(내용 차이 아님).

## 6. 남은 일

(2026-11-01 WB03-S 에서 아래 1번의 `…Type` 주제 55개와 별칭 5개 번역을 마쳐 도움말 번역이 끝났다. [6단계 기록](webpj-korean-help-types-20261101.md) 참고. 남은 일은 그 문서 6절이 최신이다.)

1. **`…Type` 주제 55개(영어 약 31KB)와 별칭 5개 번역**: 유닛·건물·주문 설명(`sunWalkerType`·`altarType`·`priestType`·`bombExplodeLargeType` 등)이다. 영어 원문은 `<info>` 머리글(타입 그림·능력치)을 두는데 타입 표(`.type`)가 없어 지금은 그려지지 않으므로, 머리글 없이 본문만 먼저 번역해도 된다. 별칭(`daisType`·`runeType`→`altarType`, `geyserType`→`emptyGeyserType`, `bombExplodeMediumType`→`bombSpecialOneType`, `thunderWalkerType`→`bulfType`)은 본문 앵커 바로 앞에 그대로 두고 같이 옮긴다. 코드 변경은 없고 테스트의 `TRANSLATED`·`ALIAS_TARGETS` 와 영어 낱말 허용 목록만 갱신한다.
2. 번역 문구 개발자 검수(특히 주문 이름 18개), 에너지 기호·간헐천·건물·주문 아이콘 등 그림 지원.
3. 맨 위 인계의 나머지(미션 스크립트 한국어·Auto-Demo·Options −40 규칙·본문 끌기 Overlay·커서 2~14), 그 뒤 정적 맵 화면(M2).
