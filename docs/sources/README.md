# 원본 동봉 문서와 추가 매뉴얼 (`originals/help`, `originals/*.txt`)

> 기존 동봉 문서는 2026-09-29 읽고 정리함. 같은 날 사용자가 외부 사이트에서 확보한 공식 매뉴얼 PDF를 추가했다. PDF의 내용과 판본은 아래 HLP 기반 정리와 아직 대조하지 않았다. HLP 추출 방법은 [hlp.md](../formats/hlp.md) (helpdeco 소스 `I:\Workspace\git\helpdeco`, VS 2022 Build Tools Win32 Release 빌드).
> 추출 결과 `extracted/helpdeco/<파일>/<파일>.txt` 는 git 에 포함되지 않으므로 새 환경에서는 `python tools/hlp.py` 로 다시 만든다.

## 1. 자료 목록과 쓰임새

| 자료 | 내용 | 클론 관련도 | 정리 문서 |
|---|---|---|---|
| [`help/manual.pdf`](../../originals/help/manual.pdf) | 사용자가 외부 사이트에서 찾아 추가한 원본 게임 공식 매뉴얼 PDF. 기존 설치 파일에 동봉된 자료와 구분한다 | **높음** (내용 대조 전) | PDF 원문. 기존 [game-manual.md](game-manual.md)는 `GAME.HLP` 기반 |
| `help/GAME.HLP` | 게임 매뉴얼 "The Book of Nimbus" (원판 1997): 규칙·화면·조작·튜토리얼·멀티플레이·유닛/주문 핸드북 | **높음** | [game-manual.md](game-manual.md), 짧은 요약 [help-manual.md](../gameplay/help-manual.md) |
| `PatchFixs.txt` | Ticonderoga 비공식 패치 10.70~10.78 변경 이력 | **높음** (보유 exe 의 동작) | [patch-history.md](patch-history.md) |
| `help/README.DOC` | 1997-10-23 최신 정보: 매뉴얼 이후 유닛 수치 변경, 멀티플레이 연결 | 중간 | [support-docs.md](support-docs.md) 3·4절 |
| `help/readme.hlp` | 설치·사양·DirectX·전체화면 옵션·멀티 접속·제작진 | 중간 (화면 모드) | [support-docs.md](support-docs.md) 1·2절 |
| `help/HELP.HLP` | Windows 95 장치 문제 해결 | 낮음 | [support-docs.md](support-docs.md) 5절 |
| `help/VENDOR.HLP`, `VOCAB.HLP` | 하드웨어 제조사 목록, 컴퓨터 용어 | 없음 | — |
| `help/*.CNT`, `*.GID`, `HELP.EXE` | 도움말 목차, WinHelp 캐시, 도움말 실행기 | 없음 | [support-docs.md](support-docs.md) 5절 |
| `Readme.txt`, `TMaker.txt`, `disclaimer.txt`, `steam_appid.txt` | 패치판 사양·실행 방법, 요새 생성기, 배포 고지, Steam 앱 번호 | 낮음 | [support-docs.md](support-docs.md) |

`manual.pdf`는 PDF 1.5 형식의 7,394,011바이트 파일이다(SHA-256 `B6584D5D105A126B8A0240618F533442D2F434A8C8B4B4EA522E715077ED8C0D`). 제공 사이트 주소와 PDF의 정확한 판본은 현재 기록되지 않았다. `GAME.HLP`와 내용이 같은지, 수치·그림에 추가 정보가 있는지는 확인 전이다.

게임 안 F1 도움말은 별개로 아카이브의 `help.english`(HTML 부분집합 스크립트)에 있고, 튜토리얼 안내는 `tutorial1~6.english` 에 있다.

## 2. 보유 exe 의 버전

* `PatchFixs.txt` 의 최신 항목은 **10.77/10.78**. exe 에 버전 문자열은 그대로 들어 있지 않다.
* exe 의 전투 옵션 표([battle-options.md](../exe/battle-options.md))에 **10.75/10.76 에서 추가된 옵션**(Island Dynamics, Geysers Placement, Geysers Respawns, Resource Injections, Game Type)이 모두 있다 → **10.75 이상**. 동봉 문서로 보아 10.77/10.78 일 가능성이 크다.
* 게임 안 Credits 의 "Netstorm 10.72 Patch Credits" 는 제작진 명단 제목이다 (`tell.english`).
* 원판 매뉴얼(1997)과 패치판 사이에는 유닛 레벨·비용·체력, 주문, 조작 키, 전투 옵션, 섬 테마 그래픽이 크게 다르다. **구현 기준은 패치판 exe·`.type`** 이고, 매뉴얼은 동작 설명·용어의 근거로 쓴다 (LEFT_JOBS.md 5절 "기준 버전").

## 3. 기존 분석·사용자 설명과 어긋나거나 새로 확인할 것

| # | 항목 | 동봉 문서 | 현재 분석·구현 | 할 일 |
|---|---|---|---|---|
| 1 | **level 1 원소 유닛의 에너지** | 매뉴얼: Bulf = **Thunder 1**, Sail Skater = Wind 1, Acid Barricade = Rain 1. Generator 는 Sun 1 | 사용자 설명: level 1 은 아무 공급원 1개 ([elements-energy.md](../gameplay/elements-energy.md)) | **exe 정적 분석 완료(2026-09-29)**: `Rifttype.cpp`가 기본 요구 문자열을 생성하고 `Mana.cpp`가 건설 위치에서 검사한다. 패치판 level 1 Bulf·Arc Spire = Thunder 1, Crystal Crab = Rain 1. Generator 3종은 명시 `mana = "s"`로 아무 공급원 1개. 게임 내 교차 원소 배치 재현은 남음 — [energy-requirements.md](../exe/energy-requirements.md) |
| 2 | Thunder Cannon 레벨 | 매뉴얼 개요: Level III (Thunder 2 + Sun 1) / 유닛 항목: Level II (Thunder 1 + Sun 1) | `.type` level 2 | 유닛 항목·`.type` 을 따름 (개요 문장은 오기로 봄) |
| 3 | 파일 조회 순서 | 패치 10.70 V5.3: "치트 방지로 하드디스크 파일보다 **tarc 를 먼저**" | 정적 분석: 데이터 폴더 디스크 → tarc ([vfs.md](../formats/vfs.md)) | **재확인 완료(2026-09-29)**: 열기·이름 해석 함수와 모든 호출부가 디스크 우선, 아카이브 우선인 존재 검사도 결과를 있음/없음으로만 씀 → 보유 exe 는 **디스크 우선** 유지. 클론 구현 변경 없음. 원하면 동적 확인 방법은 vfs.md |
| 4 | `bridgeDrawRate`·`stuffRefreshRate` | 패치 V9.0 에서 CFG 명령 제거 | `options.cfg` 에 값이 남아 있음 | **확인 완료(2026-09-29)**: exe 설정 키 등록(`00441270` 호출)에 두 키가 없다 → `options.cfg` 값은 무시됨. `edgeScrollSpeed`·`maxFPS` 는 등록됨. 미션 머리 값 `aiNBridgeDrawRate`(AI 다리 속도)는 별개로 여전히 쓰임 |
| 5 | Storm Power 숫자 색 | 매뉴얼: **1000 미만이면 빨강** | 사용자: 흰/노랑/빨강 3단계, 캡처 450·200 빨강, 1400·1650 노랑 | **확인 완료(2026-09-29)**: `Combatgump.cpp` `0043da10` — **SP ≤ 1000 빨강(`~r`), 1001~2000 노랑(`~y`), 2001 이상 흰색(`~w`)**. 형식 `~3~E~%c%d~[I%d.3]`(크기 3·엠보스·색·숫자·SP 아이콘). 캡처 값 전부 일치 |
| 6 | 건물 배치 위치 | 매뉴얼: 건물은 섬 위에만(다리 끝 불가). 유닛은 내 섬·아군 다리 끝·중립 섬 | 사용자: 빈 섬은 워크샵·알타 가능(사제 도달), 건물형 유닛은 다리 연결 필요 ([island-ownership.md](../gameplay/island-ownership.md)) | 일치. "Stream of Power"(워크샵·아웃포스트에서 SP 줄기가 도달해야 활성화)가 다리 연결 조건의 이유 → 규칙 문서에 반영 |
| 7 | 알타 위치 | 매뉴얼: 본섬·점령한 적 섬·중립 섬 | 사용자: 빈 섬에도 가능 | 캠페인 "빈 섬" = 멀티의 중립 섬에 해당하는 것으로 보임 |
| 8 | 멀티 Generator Range | 매뉴얼: short/normal/long 3단계 | exe: 4단계(Very Long 추가), 14/22/30/38 중 30 상한 | 패치판 기준 ([battle-options.md](../exe/battle-options.md)) |
| 9 | 원판 해상도 | 매뉴얼·readme: 640×480 256색 최소 / 패치 V9.0 에서 **1152×864, 1280×960 제거** | 캡처: 640/800/1024 세 가지 | 기록만 |
| 10 | 가장자리 스크롤 | 전체화면 또는 **640×480 창 모드**에서만, `edgeScrollSpeed` | AGENTS.md: 풀스크린에서 필수 | **확인·구현 완료(2026-09-29)**: exe 조건은 "창 테두리 오프셋 = 0"(전체화면 또는 바탕화면 = 게임 해상도인 창)이고 속도는 프레임당 픽셀, 상한 `edgeScrollSpeed` — [edge-scroll.md](../exe/edge-scroll.md) |
| 11 | 결정 1개 가치 | 매뉴얼: **200 SP**, 가이저 2000 SP | `.type` `nugget` cost 200 | 일치 |
| 12 | 튜토리얼 범위 축소 | 매뉴얼: 튜토리얼 3 에서 "템플 범위가 짧다" | exe: 튜토리얼 2 처리 함수에서 14칸 | 튜토리얼 3 에서도 14칸이 이어지는지 확인 ([game-manual.md](game-manual.md) 5절) |

## 4. 다음 작업에 바로 쓸 수 있는 규칙·수치 (요약)

* **생산 흐름**: 지식 획득(희생) → 워크샵 등록(Level I 2칸 / II 3칸 / III 4칸) → Production 창에서 집어 배치 → 워크샵·아웃포스트에서 SP 줄기가 날아가 닿으면 활성화.
* **Production 창**: 다리 조각(무료, 2/4/6칸 — 멀티 옵션, 놓으면 무작위 새 조각, 처음엔 금 간 상태), 유닛, 위 SP, 아래 미니맵.
* **다리 십계명**: [game-manual.md](game-manual.md) 2절. 가이저 연결 시 소유, Edge Farm 위치 불가, 적 다리 연결은 가능하나 거기서 생산 불가.
* **사제**: 체력 절반에서 기절·보호막, 템플이 있으면 회복, 운반 유닛에서 풀리면 완전 회복, 다리가 없어져도 떠 있음.
* **전투 AI**: 가장 가까운 목표 자동 선택·고정, 폭발 연쇄, 땅에 닿는 지점 조준.
* **조작 키 전체**: [game-manual.md](game-manual.md) 4절 + [patch-history.md](patch-history.md) 3절.
* **유닛·주문 원판 수치 대 현재 `.type`**: [game-manual.md](game-manual.md) 7절.
