# webpj 한국어 지원 6단계: F1 일반 도움말 `…Type` 유닛·건물·주문 설명 주제 번역으로 도움말 번역 마무리 (WB03-S, 2026-11-01)

2026-11-01, **HJOW-Athlon**(마지막 전체 정밀 디컴파일 호스트와 같은 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 원본 아카이브의 영어 `help.english`·[5단계 기록](webpj-korean-help-multiplayer-20261101.md)·[한국어 명칭 규칙](korean-unit-names.md)·공유 명칭 JSON·기존 webpj 도움말 변환 코드다). 사용자 요청("webpj 다음 작업 진행, 텍스트 내용 검수는 내가 발견하는 대로 요청할 테니 일단 넘어가 줘")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목 **`…Type` 도움말 주제 번역**을 했다(번역 문구 검수는 개발자가 따로 요청하는 방식으로 넘겼다). 작업 시작 때 `git status` 에 다른 세션의 변경(`cpppj/`·공통 `LEFT_JOBS.md`·`tools/*`·`originals/` 실행 흔적)이 있었으므로 `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md`·`README.md`(이전 단위에서 받은 '공통 파일은 나중에 몰아서 갱신' 지시를 이어 지킴)·공유 명칭 JSON 은 수정하지 않았다. 원본 게임·analyzeManager 는 실행하지 않았고, 브라우저는 우리 웹 클론만 띄운 Edge 헤드리스다. **코드 변경은 없다**(번역 자료·자산 복사본·매니페스트·준비 도구의 설명 문구·테스트만 바뀜).

## 1. 범위와 결과

| 대상 | 상태 |
|---|---|
| 새로 번역한 본문 55개(영어 약 31KB) | **한국어** (`help.korean`) |
| 새로 더한 별칭 앵커 5개 | 영어와 같은 자리에 두어 같은 본문이 나온다 |
| 번역 전체 | 본문 104개 + 별칭 22개 = 앵커 126개. 영어 도움말 고유 앵커 128개 중 **98%**, 영어 글자 수로는 **99.7%**(109.5KB / 109.8KB) |
| 영어로 남은 주제 | `serverHelp`(영어 원문이 제작진의 `[[ DO NOT TRANSLATE YET ]]` 미완성 틀, 어디서도 링크되지 않음)와 `map`(그림 하나뿐인 주제라 번역할 글이 없음) |

새 본문 55개: 다리(`bridgeType`)·제단(`altarType`)·사제(`priestType`)·에지 팜(`edgeFarmType`)·거주 구역(`residenceType`)·스톰 간헐천(`emptyGeyserType`)·스톰 크리스탈(`nuggetType`), 주문 18개(`bombLightingZapType`·`bombLightingwaveType`·`bombTwisterType`·`bombIITwisterType`·`bombIIITwisterType`·`bombIManoType`·`bombIIManoType`·`bombIIIManoType`·`bombGravitonType`·`bombMeteorType`·`bombExplodeSmallType`·`bombSpecialOneType`·`bombExplodeLargeType`·`bombHardenerType`·`bombHealType`·`bombInvisibleType`·`bombParalyzeType`·`bombTreasonType`), 썬 유닛 8개(`sunArcherType`·`sunAviaryType`·`sunFlyerType`·`sunBlockerType`·`sunCannonType`·`sunBalloonType`·`sunFenceType`·`sunWalkerType`), 윈드 유닛 7개(`windArcherType`·`windAviaryType`·`windFlyerType`·`windBatteryType`·`windBlockerType`·`windBalloonType`·`windWalkerType`), 썬더 유닛 6개(`thunderArcherType`·`thunderBatteryType`·`thunderBlockerType`·`thunderCannonType`·`thunderFenceType`·`bulfType`), 레인 유닛 9개(`rainAviaryType`·`rainFlyerType`·`rainBatteryType`·`rainBlockerType`·`growingrainBlockerType`·`rainCannonType`·`rainBalloonType`·`rainFenceType`·`rainWalkerType`). 별칭 앵커 5개: `daisType`·`runeType`→`altarType`, `geyserType`→`emptyGeyserType`, `bombExplodeMediumType`→`bombSpecialOneType`, `thunderWalkerType`→`bulfType`.

이제 F1 도움말에서 링크로 닿는 모든 주제가 한국어로 열린다(영어 대체가 쓰이는 곳은 `serverHelp` 를 직접 여는 경우뿐이다).

## 2. 구현

| 파일 | 내용 |
|---|---|
| `translations/help.korean` | 새 앵커 60개 추가(약 36KB, UTF-8·LF·BOM 없음). 파일 맨 끝의 `battleOptionsHelp`(영어 원문처럼 닫는 태그 없는 마지막 앵커) 앞에 끼워 넣어 그 구조를 지켰다. 머리 설명 갱신. |
| `www/data/d/help.korean`, `www/data/manifest.json`, `tools/prepare-assets.js` | `prepare-assets` 로 복사본·매니페스트 갱신(크기 89146 → 125311 바이트). 매니페스트의 `use` 설명 문구를 "앵커 126개: 본문 104개와 별칭 22개" 로 고쳤다. |
| `tests/korean-help.test.mjs` | 번역 주제 목록 126개·별칭 표 22개로 갱신. 번역하지 않은 주제의 예를 `altarType` 에서 `serverHelp` 로 바꿨다. 허용 영어 낱말에 메뉴 이름 `Pray`·`Meltdown` 과 인용문 속 이름 약자 `DWMI`·`XIIIBY` 를 더했다. |

도움말 창·변환 코드(`help-window.js`·`help-topic.js`)는 3단계 그대로다.

## 3. 번역 방식과 결정

- **구조는 영어와 똑같이, 보이는 문구만 번역했다**(1~5단계와 같다). 태그·`<info>` 머리글·그림(`<!"bombHeal.1">`·`<!"mana.8">`)·링크 주소·`<i>`·`<Br>` 대소문자까지 그대로다. `<info>` 머리글(타입 그림·능력치)은 타입 표(`.type`)가 없어 여전히 그려지지 않고 본문만 나온다.
- **영어 원문의 오타·깨진 태그를 그대로 뒀다**: `altarType` 의 `<a href="#priestType>Priests</a>`(따옴표가 닫히지 않음), `bombLightingwaveType` 의 `<!"bombLightingwave.1>`, `bombIIITwisterType` 인용문의 어긋난 따옴표, 아무 이름 없이 끝나는 `-Telsen Rott, ` 출처, 없는 주제로 가는 링크 `generatorHelp`. 깨진 링크 태그는 목적지 해석이 `>` 에서 끊겨 `#priestType` 으로 동작하며 영어와 같다(테스트가 목적지를 영어와 대조해 같음을 확인). 고치려면 이 구조 대조를 바꿔야 하므로 개발자가 정할 일이다.
- **인용문(분위기 글)과 출처를 새로 번역했다**: 분류·직함은 번역하고 이름은 영어 발음으로 적었다(`General Jan Masaryk → 얀 마사리크 장군`, `Askeld Urvonnus → 아스켈드 우르보누스`, `Captain Knut Hamsun → 캡틴 크누트 함순`, `Book of Nimbus → 님버스의 서`, `Nimbian Poet → 님버스의 시인` 등). 출처의 책 제목은 `《…》` 로 적었다(`《벌룬 만들기》`·`《제6 궁수 교본》`·`《제10 썬더 포병 교본》`·`《썬더 능선의 찬가》`). 첫 줄에서 시를 줄 단위로 나눈 `<br>` 구조는 그대로다.
- **용어**: AGENTS.md 명칭 규칙·공유 표·앞 단계 용어를 그대로 따랐다. 이번에 새로 정한 말: `Shooter → 사격 유닛`, `Aerial Attacker → 공중 공격 유닛`, `Ground/Aerial Transport → 지상/공중 수송 유닛`, `damage absorber → 피해 흡수 유닛`, `Barricade post → 바리케이드 기둥`, `force field → 포스 필드`(공유 표), `Altar level → 제단 N단계`(유닛 단계와 같은 말), `Production Slot → 생산 칸`. 치료·투명화 같은 주문 이름은 본문 속에서도 주문 목록과 같은 음역(`패럴리시스`·`인비저빌리티`)으로 적어 연결이 보이게 했다.
- **게임 안 메뉴 이름은 영어 그대로 두었다**: 사제 우클릭 메뉴 "Pray", 제네레이터 메뉴 `Meltdown`(게임 안 메뉴가 아직 한국어가 아님, 앞 단계의 `"Drop ..."` 과 같은 결정).
- **원문의 수치·모순은 그대로 번역했다**: 더스트 데빌의 "20초·32초", 맨 오 워의 "1분·15초", `Note: …` 의 반복 문구 등.

## 4. 새로 알게 된 영어 도움말 파일 구조

- 별칭 묶음의 닫는 태그 수: `daisType`·`runeType`·`altarType`(3개, 별칭 사이에 빈 줄이 없음), `geyserType`·`emptyGeyserType`(2개), `bombExplodeMediumType`·`bombSpecialOneType`(2개), `thunderWalkerType`·`bulfType`(2개).
- `bridgeType` 본문의 "Meltdown" 설명은 `generatorHelp` 로 가는 죽은 링크를 담고 있다(영어도 같음, 원문 결함).
- 모든 `…Type` 주제는 본문이 짧은 한두 문단이라 한글화로 창 크기·스크롤 문제가 생기지 않았다(4개 화면 크기에서 모두 열려 화면 안).

## 5. 검증(HJOW-Athlon, Node v24.21.0, Edge 헤드리스)

- `npm --prefix webpj test` **141개 통과**(기존 141 그대로 — 새 검사는 기존 검사 안에 넣었다. 이 PC 에서는 `prepare-assets` 로 맞춘 상태 기준, 아래 환경 메모 참고).
  - 구조 대조(`~~글자`·`~[…]` 전체 토큰 포함)·별칭 대조(한국어·영어 모두, 본문·링크·머리·꼬리)·링크 목적지 영어 대조·영어 낱말 잔존(게임 안 메뉴 이름을 둔 `<c>` 는 영어 원문에 같은 문구가 있을 때만 허용)·126개 앵커가 4개 화면 크기(800×600·1024×768·1366×768·1280×800)에서 모두 열려 화면 안.
- **변이 검사** 한국어 자료 17종(링크 목적지 변경 3·별칭 앵커 삭제 3·별칭 묶음의 닫는 태그 하나 빠짐·그림 이름 변경·깨진 태그/그림 태그를 고침 2·문장·문단을 영어로 2·영어 원문에 없는 `<c>` 문구·`<br><br>` 하나 삭제·`<i>` 삭제·앵커 이름 변경·`Meltdown` 철자 변경)이 **모두 걸렸다**(처음 시도한 2종은 같은 문구가 두 곳에 있거나 공백이 달라 적용되지 않아 고유한 문구로 바꿔 다시 했다. 자료는 소스·복사본을 같이 바꿔 실행하고 끝나면 복원 확인).
- **실제 브라우저(진짜 마우스 클릭·휠·키보드, CDP):** 1024×768·1920×1080·1366×768·1280×800 × 한국어·영어, 8회 × 660개 = **5280개 모두 통과**(콘솔 오류·예외 0, 경고는 그림 미지원뿐). 같은 흐름을 매번 반복한다: 클릭하여 시작 → 메인 메뉴 → F1 → 첫 경로(링크 15개, Back 7번)와 OK 로 닫기 → 다시 F1 → 두 번째 경로를 링크 위치를 찾아 **진짜 클릭**으로 따라간다(멀티플레이어·채팅 링크 13개와 Back 12번, 이어 `sphereHelp`→`pyrosphereHelp`→`buriedType`→`bombHealType`→`bombParalyzeType`) → OK 로 닫기 → 다시 F1 → **세 번째 경로**(`unitHelp`→`sunWalkerType`→Back→`rainFlyerType`→`rainAviaryType`→Back 3번→`priestType`→`vortexHelp`→Back 2번) → OK 로 닫기. 매 단계 주제 이름·한글 유무(영어 모드는 한글 없음)·창이 화면 안인지 확인하고, 본문이 긴 주제는 휠로 링크가 보일 때까지 스크롤한다. 이어 번역한 116개 주제(별칭 포함, 앞 단계 56개 + 이번에 더한 60개)를 모두 열어 본문 줄·한글·화면 안을 확인하고, 번역하지 않은 `serverHelp` 는 한글이 없음(영어 대체)을 확인한 뒤 Escape 로 닫는다. 스크린샷으로 한국어 `sunWalkerType`(골렘)·`altarType`(제단, 깨진 링크 태그가 그대로 링크로 동작)의 글자 모양·링크색·줄바꿈을 눈으로 확인했다.
- 확인하지 않은 것: Chrome/Firefox·터치·소리 출력·기기 픽셀 비율 2 이상의 한글 모양·번역의 어색함(개발자가 발견하는 대로 요청하기로 함)·그림(아이콘) 표시(미지원 그림은 경고만 남는다).
- 환경 메모: 4단계 기록과 같다. 이 PC 는 `fonts/License.txt` 작업 트리 사본이 CRLF 라 `prepare-assets` 가 License 복사본·매니페스트 항목을 바꾸므로 그 두 곳은 되돌리고 커밋했고, 커밋한 상태로 이 PC 에서 테스트를 돌리면 assets 관련 2개가 그 한 파일 때문에 실패한다(내용 차이 아님).

## 6. 남은 일

1. **도움말 그림(아이콘) 지원**: 에너지 기호(`<!"mana.8">` 등)·간헐천·크리스탈(`geyser.*`·`nugget.*`)·건물/유닛 그림(`*vortex.*`·`*battery.*`·`*Cannon.*`·`sunFactory.*` 등)·주문 아이콘(`bomb*.1`)·`fortgump.L*`·`icon.B*`·`battleIsland.*`·`challengeIsland.*` 은 아직 그려지지 않고 콘솔에 경고 한 줄씩이 남는다(WB03-G 부터 알려진 미지원, 영어도 같음). 이제 도움말 번역이 끝났으므로 다음 후보다. `fortgump.3`(스톰 파워 아이콘)은 이미 그려진다.
2. 도움말 `<info>` 머리글(타입 그림·능력치): 타입 표(`.type`)가 생길 때.
3. 번역 문구 개발자 검수는 개발자가 발견하는 대로 요청하기로 했다(대기).
4. 맨 위 인계의 나머지(미션 스크립트 한국어·Auto-Demo·Options −40 규칙·본문 끌기 Overlay·커서 2~14), 그 뒤 정적 맵 화면(M2).
