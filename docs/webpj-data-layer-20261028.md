# webpj 데이터층: TAFF·설정·Tell 섹션 (WB02-D, 2026-10-28)

2026-10-28, **VM-W11-Codex**. 사용자 요청("LEFT_JOBS.webpj.md 를 읽고 webpj 다음 단계 진행, 다른 세션에서 cpppj 진행 중이니 cpppj 쪽 파일은 수정하지 말 것")에 따라 두 가지를 했다. (1) AGENTS.md 변경(2026-10-27)의 코드 후속인 **메인 메뉴 Edit 버튼 제거**, (2) 메뉴 버튼이 여는 대화상자의 본문을 읽기 위한 **WB02 나머지 — TAFF 아카이브·설정 객체 층·Tell 섹션 조회**. 화면에 보이는 새 대화상자는 아직 없다. 대화상자(HTML 본문·돌 패널·목록 행) 그리기는 이 데이터층 위에서 다음 단위로 한다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md 는 수정하지 않았다. cpppj 소스와 fixture 는 읽기만 했다. 원본 게임·분석 프로그램은 실행하지 않았고, 브라우저 검사는 Edge 헤드리스(창 없음)로 했다.

## 1. 근거와 디컴파일 호스트

| 항목 | 근거 |
|---|---|
| TAFF 형식·XOR 키·이름 규칙 | [taff.md](formats/taff.md), 원본 `FUN_0041a240`(이름 비교), 실물 `assets/game-data/netstorm.tarc`(= `originals/netstorm.tarc` 와 SHA-256 동일) |
| 설정 조회·값 해석·치환·섹션 | [config.md](formats/config.md), 원본 `0043fd70`·`00440150`·`00440760`·`00440a00`·`00440b60`·`0043fe60`, **cpppj `o/Config.cpp`(두 판본 x86 기대값과 대조 완료된 복원)를 읽어 옮김** |
| 설정 구성 순서 | cpppj `o/ConfigInterface.cpp`(원본 `00440e00`·`00441790`·`004410f0`): 언어 용어표 → 전역 설정 → `global.` → `fort.`, 전역 버퍼 `[ARGS]`+버전+`[options.cfg]`+`[setup.cfg]`+`[END]` |
| Tell 섹션 조회 | cpppj `client/TellSection.cpp`(원본 `004cf730`): 이름의 점 → 등록 객체 → (현재 미션) → 기본 tell 스크립트 `missionSpec + local.1="tell"` |

**디컴파일 호스트:** 현재 PC 는 VM-W11-Codex 이고 공통 인계의 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23(21:16 재실행)이다. 이 PC 의 `extracted/refined/originals/` 는 같은 날 오전 VM-W11-Codex 산출물이며, [재실행 기록](exe/cpp-decompilation-20261023.md)은 "이전 결과와 텍스트 산출물이 모두 동일"이라고 남겼고 `tools/ghidra` 는 그 뒤 바뀌지 않았다. **이번 단위는 디컴파일 소스를 분석하지 않았으므로 재디컴파일을 하지 않았다.** 다음 단위(대화상자 그림)는 디컴파일 소스를 읽어야 하므로 착수 전에 AGENTS.md 의 PC 불일치 규칙을 다시 적용해야 한다([인계](../LEFT_JOBS.webpj.md) 참고).

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/assets/taff.js` | `TaffArchive`(머리·디렉터리 검증, 대소문자 무시 이름 조회, `.\` 접두 제거, 와일드카드 `find`, 엔트리 XOR 복호화), `xorDecode`, `archiveNameMatches` |
| `www/assets/config.js` | `findKey`·`getRaw`·`findSection`·`sectionRange`(원본 줄 이동·값 해석·섹션 경계), `ConfigRegistry`·`Config`(등록 순서 층, 이름 접두어, `append`·`loadBytes`·`get`·`substitute`·`section`, 환경/레지스트리/미션 훅) |
| `www/assets/game-files.js` | `GameFiles`: 느슨한 파일(웹 `www/data`) → TAFF 아카이브 순 조회(원본 열기 순서), `\D\tell.english`·`d/tell.english` 표기 통일, 와일드카드 목록(첫 중복 제거·원래 대소문자) |
| `www/assets/game-data.js` | `GameData.build({files, language, options})`: 설정 층 구성·`pathSpec`(임시 `local.1~3`)·`readText`(언어 파일 영어 대체)·`tellSection(name)` |
| `www/assets/text-encoding.js` | `decodeWindows1252`·`decodeOriginalText`(UTF-8 판별 후 Windows-1252) 추가 |
| `tools/prepare-assets.js` | `netstorm.tarc`(1.1MB)·`d/setup.cfg` 를 `menu` 묶음에 추가(변환 없는 복사, 원본과 같은 blob). 묶음 크기 약 5.9MB → 약 7.0MB |
| `www/netstorm.js` | 자산 적재 뒤 `GameData` 를 만들어 `app.gameData` 로 노출. 메뉴 Tell 명령 실행 시 섹션을 실제로 찾아 콘솔에 줄 수를 알림(화면은 그대로) |
| `www/ui/main-menu.js` | `Edit` 칸 `web: false`(이유 `note` 기록). 원본 8버튼 비교(`includeAll`)에서는 계속 만든다 |

웹 적응(원본과 다른 점): 언어 파일이 없으면(`tell.korean` 등) 영어 파일로 대체한다(원본은 대체하지 않음, 한국어 자료가 갖춰질 때까지의 임시 정책). `options.cfg` 자리에는 파일 대신 호스트 설정(언어·음량)을 넣으며 해상도·전체화면·설치 경로는 넣지 않는다. 값 쓰기(`Set`)·줄 삭제·저장은 필요해질 때 옮긴다.

## 3. 검증 (VM-W11-Codex, Node v24.19.0, 2026-10-28)

`npm --prefix webpj test` **25개 통과**(기존 17 → Edit 제거로 1개 이름/기대값 수정, 신규 8).

- **원본 x86 기대값 2,025행:** `cpppj/tests/fixtures/config-x86.tsv` 의 Get 77 + Subst 495 + FindSection 1,045 + Section 408 행이 모두 같다(두 판본 원본 기계어가 같은 결과를 낸 입력). 행 수를 고정해 fixture 가 바뀌면 실패한다. 치환 키 대문자 변환을 일부러 끄는 변이로 검사가 실패함을 확인한 뒤 되돌렸다. Set·Remove 행은 웹에 옮기지 않았으므로 쓰지 않는다.
- **TAFF 246개:** Python 독립 구현(`tools/taff.py`)이 만든 `tests/fixtures/taff-entries.tsv`(이름 순서·복호화 크기·SHA-256)와 전부 같다. 손상 입력(짧음·매직·범위 밖)은 구분된 오류, XOR 은 대칭.
- **실제 자료:** `config.english` 용어(`{a factory}`→a Workshop 등)·없는 키 `{KEY}`·기본값·이스케이프·`setup.cfg` 스타일 코드(`{H2}`=`~w`, `{CText}`=`~i`)·경로 지정값(`missionSpec`→`\D\tell.english`, `languageSpec`)·임시 `local` 객체 제거, 느슨한 파일 우선, Credits·UCampaign 섹션 본문 줄 단위 일치, 한국어 요청 시 영어 대체.
- **실제 브라우저(Edge 헤드리스 CDP, 1024×768 / 1920×1080):** 로딩 → 클릭 → 메인 메뉴에서 버튼 5개(3+2, Edit 없음), 좌표가 Node 기대값과 같다(4:3: 407/478/549 · 443/514, 16:9: 578/649/720 · 614/685). Credits·Campaign·Demo 클릭 시 콘솔에 `Tell 섹션 Credits: 5줄`, `UCampaign: 7줄`, `Demoville: 10줄`. 콘솔 오류 없음(`favicon.ico` 404 제외). 이 PC 의 브라우저 언어가 한국어여서 영어 대체 경로가 실제 브라우저에서도 동작함을 함께 확인했다.

## 4. 새로 확인한 사실

- **캠페인 목록 파일 8개:** `\d\offical*.english` 는 6개가 아니라 **8개**다. `offical2sep`·`offical5sep` 이 그룹 구분선 행이다(스크린샷 `mainMenu - Campaign.png` 의 구분선 2개, 공통 인계의 "Campaign 8행(구분선 2)"과 일치). 이름 인덱스 순서는 `offical1, 2, 2sep, 3, 4, 5, 5sep, 6`이다.
- **메뉴 Tell 섹션 이름:** `Credits`(5줄: 배경·제목·메뉴 2·취소), `UCampaign`(7줄, 메뉴는 `@{guideSpec}` 로 위 목록을 펼침), `DEMOVILLE`(대소문자 무시 조회).
- **스타일 토큰:** 본문의 `{H2}`·`{Text}`·`{CText}` 는 `setup.cfg` 의 `~w`·`~i` 같은 StyleText 코드로 풀린다. 이 코드를 해석하는 StyleText(`~w` 흰색, `~i`, `~B~E`, `~[C.84]` 등)가 다음 단위의 한 부분이다.

## 5. 남은 일 (다음 단위)

1. **Tell 대화상자 그리기:** 본문 파서(`<background>`·`<h2>`·`<p>`·`<br>`·`$Menu=`·`$Button=`·`$Timeout=`), StyleText 해석, 돌 패널(모서리 장식·질감)·목록 행·버튼, 대화상자 크기 맞춤·가운데 배치, 입력. 먼저 Credits(정적 5줄)로 `screenShots/mainMenu - Credits.png` 와 대조하고, Campaign(`@{guideSpec}` 목록·구분선)으로 넓힌다. **디컴파일 소스(HtmlWindow·StyleText·MenuGump·UberGump 배경 Gump)를 읽어야 하므로 PC 불일치 규칙을 먼저 적용한다.**
2. Tell 이름의 지역 인자 `Name(arg1,arg2)` 해석(원본 `004c8ea0`, cpppj fixture `tell-parameters-x86.tsv` 1,070관측으로 대조 가능), 현재 미션 스크립트 조회 연결.
3. 한국어: 용어표·Tell 한국어 파일, D2Coding 글리프 경로.
4. 사용자 설정 저장(`Set`·저장)과 옵션 메뉴 연결.
