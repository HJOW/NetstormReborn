# webpj L3: 전 미션 시작 상태 대조 (WB04-E1313, 2026-11-24 완료)

## 재개 조사: 로그인 중 자원 프레임 누락 수정

2026-11-24의 새 goal로 재개해 완료했다. 아래 중단 당시 기록의 `bc7sun_bars76` 차이는 로그인 중 자원 프레임 누락을 고쳐 닫았고, 31개 미션의 시작 5·30·120프레임이 모두 C++ 오라클과 일치한다. 이후 목표 수행·완주는 각 미션 L3에서 이어 간다.

- 기준선 `225553df`의 별도 소스 사본에 임시 추적을 넣어 빌드했다. 실제 제품·보관 오라클 EXE·원본 자료는 변경하지 않았고 추적 뒤 소스 사본을 바이트 그대로 복원했다. 추적 로그는 `extracted/webpj-oracle/resource-trace-run.log`다.
- C++ `UberGump::PollBattleResources`는 `GameWorld::PreparingFort()`만 검사한다. 이는 Edit의 기존 Fort 설치 관문이며 캠페인의 `Connecting()`과 다르다. 캠페인 로그인 중에는 자원 프레임을 호출한다.
- C++에서 시작 시각 9.2초의 첫 호출은 아직 전원이 로그인하지 않은 상태다. 이 호출에서 `secondaryGeyserDeadline=39.2`를 먼저 예약하고 `AllLogged=false`로 반환한다. 다음 호출은 로그인 완료·간헐천 7개·예산 35지만 예약 시각 전이므로 생성하지 않는다.
- 웹은 `CampaignGameWorld.runBattleResources()`와 앱의 활성 미션 프레임 관문 두 곳에서 접속 중 호출을 막았다. 두 관문을 고쳐 로그인 중에도 같은 자원 프레임을 한 번 호출한다. 활성 미션의 기존 Kernel 뒤 호출 순서는 유지한다.
- 실제 지연 GameWorld 검사에 로그인 중 기한 예약과 같은 시각의 재예약 방지를 추가했다. 관련 집중 검사 5건이 통과했다.
- 수정 후 bc7의 기존 ‘알려진 차이’ 검사가 5프레임 해시 `0e39464d…`의 **일치**로 실패했다. 예외 집합과 역단언을 삭제해 이제 31개 모두 같은 일치 단언을 사용한다. 실패 기록: `webpj/extracted/webpj-e1313-bc7-before-exception-removal.log`.
- 예외 제거 후 전체 Chromium 실행은 **32/32 통과**(31개 미션 × 5·30·120프레임과 메타데이터 검사, 24.3분)다. `webpj/extracted/webpj-e1313-final-e2e.log`. 덤프의 환경 의존 `sp`·`player` 행 제외 규칙은 기존 오라클과 같고 fixture를 변경하지 않았다.
- Node 22.23.5 런타임을 Git 제외 `extracted/webpj-node22/`에 마련해 밀린 전체 회귀를 재개했다. 첫 실행 6,829건 중 4건 실패: 새 자원 프레임을 자산 적재 상태에서도 호출해 기존 음향/재진입 모형과 미션 시작 전용 검사가 실패했다. 실제 캠페인 수명인 MAIN_MENU 상태에서만 로그인 중 자원 프레임을 실행하도록 좁혔고, MAIN_MENU로 실행하는 음향 검사의 접속 중 World에는 자원 프레임 관측을 더했다. 관련 집중 9/9 통과(`webpj/extracted/webpj-e1313-regression-fixes.log`), 전체 재실행 결과는 아래에 기록한다.
- 독립 자식 프로세스에서 접속 중 호출을 다시 막는 변형은 새 로그인 기한 단언에서 검출했다(`webpj/extracted/webpj-e1313-mutation.log`). 제품 소스의 디스크 바이트는 변형하지 않았다.
- 교정 후 **Node 22.23.5 전체 6,833/6,833 통과**, 실패·취소·미실행 0, 978초(`webpj/extracted/webpj-e1313-node22-final.log`). 진행 중 완료한 E1314·E1315의 새 시각 효과 검사도 포함한다. E1316 받침 공개 래퍼는 별도 집중 검사와 후속 번들/Node 26 회귀에서 확인한다.
- webpack 번들 3,422,868바이트로 크기 한도를 통과했다(`webpj/extracted/webpj-e1313-build-final.log`). C++ trace 사본·브라우저 trace·임시 변형 로더는 모두 Git 제외 경로에 보관했다.

아래는 재개 전 인수인계 원문이며 당시의 ‘원인 미확인’·‘이어서 할 일’ 상태를 설명한다.

모든 미션(Tutorial1~6·캠페인 세 묶음 16개·Priest Training 9개 = 31개)의 **시작 상태**(미션 시작 뒤 주 루프 5·30·120프레임의 Session 덤프)를 cpppj 기준선(225553df)과 대조했다. Tutorial2 L3 에서 시작 시점의 슬롯 순서 차이(생산 패널 HUD 와 Guide 시작 단계의 순서, E1305)를 찾은 것을 모든 미션으로 넓힌 것이다. **사용자 요청으로 작업 중간에 멈췄다** — 아래에서 끝난 것과 남은 것을 구분한다.

## 끝난 것

- 일회성 대조(임시 스크립트, 저장소에 넣지 않음): 28개 미션(Tutorial4~6·캠페인 16개·Priest Training 9개) 중 **27개가 5·30·120프레임 모두 cpp 와 덤프 일치**(환경 의존 행 `sp`·`player:*` 제외)이다. Tutorial1~3 은 L3 단계표로 이미 확인했다.
- 수집 도구 `tools/l3-mission-starts-collect.mjs`: 31개 미션을 cpp 로 돌려 5·30·120프레임 덤프의 정규화 해시(LF·`sp`·`player` 행 제외 SHA-256)를 `tests/fixtures/cpppj/candidate-225553df/mission-starts/hashes.json` 에 쓴다(HJOW-Athlon 에서 수집, 31개 전부 기록됨).
- 옵트인 e2e `e2e/l3-mission-starts.spec.js`: 같은 해시 정의로 웹 덤프를 해시해 비교한다(`NETSTORM_L3_ALL=1` 이면 전체 약 25분, `NETSTORM_L3_ONLY=Tutorial2,bc7sun_bars76` 로 일부만). **부분 실행으로 확인한 것은 Tutorial2·Tutorial4·bc5walkers76·bc7sun_bars76 네 미션뿐이다(통과).** 전체 31개 실행은 중단돼 끝나지 않았다.

## 알려진 차이 — bc7sun_bars76(Priest Training 7, 원인 미확인)

웹은 시작 4프레임째(월드 진입 직후 게임이 정지된 상태, `now=9.2`)에 전투 자원 프레임(`runBattleResourceFrame`, 원문 00486bc0)의 **일반 간헐천 보충**으로 간헐천(TYPE 122)을 하나 더 만든다: SID 15028, 위치 (131,204), 웹 8개 vs cpp 7개(풀 다음 SID 15029 vs 15028, `totalmade 122` 8 vs 7, 프로세스 44·regular 44 추가). cpp 는 이 미션을 12000프레임(게임 시각 9.2 에서 멈춘 채)까지 돌려도 간헐천이 늘지 않는다. 이 미션은 헤더에 `moregeysers=1` 이라 `ordinaryGeysers = 0` 으로 보충 분기에 들어가는 유일한 미션이다(다른 미션은 `ordinaryGeysers = 1` 이라 곧바로 반환해 차이가 없다).

확인한 사실:
- 웹 첫 호출 값: `now=9.2`·`period=30`(단독 미션)·`boss=true`·`editor=0`·`battleGeyserPeriod(options)=60`·`allLogged=true`·`single=true`·`battle=true`·`mode(options[18])=2`·`aux.resourcePool=0`, `loginEnd=0`. 호출 스택은 `RawCampaignSession.runBattleResources → runCampaignBattleResources → runBattleResourceFrame → populate → RawLocalGeyser.populate → create`.
- 웹·cpp 의 `runBattleResourceFrame` 본문과 호출 게이트(cpp `UberGump::PollBattleResources`: `world_->PreparingFort()` 가 아닐 때 `Session().RunBattleResources()`; 웹 `CampaignGameWorld.runBattleResources`: `connecting` 이 아닐 때)는 같아 보인다. 전투 옵션 기본값도 같은 초기값이다(`raw-mission-battle-options.js` `INITIAL_OPTIONS`).
- 웹 `budget` 훅은 `worldGeyserBudget(..., monotone = true)`(NB29 수정)이고 cpp `WorldGeyserBudget(..., true)` 도 같다. 5명(내 소유자 + AI 4)이라 예산 `(12-5)*5=35`, 목표 `35/3=11` 이 7개보다 커서 웹은 채운다. cpp 가 채우지 않는 이유가 이 목표(`geyserCount` 훅의 `assetState.Geysers().Count()` 가 cpp 에서 더 큰 값이거나 `battle`/`players` 판정이 다름)인지, 앞선 반환 조건(`boss`·`editor`·`allLogged`·`battleGeyserPeriod`)인지, `populate` 의 100회 시도가 실패하는 것인지(MayPlace·지형 차이)를 아직 못 가렸다.
- cpp `report` 의 `mission_rules`(`settings.ordinaryGeysers` 4번째 값)는 bc7·bc6 모두 0 이라 그 값만으로는 가려지지 않는다.

다음 단서(우선순위): ① cpp 에 임시 추적을 넣어 `RunBattleResourceFrame` 의 어느 반환에서 나가는지(`hooks.geyserCount()` 값 포함) 본다(cpp 빌드 필요, 사용자에게 실행 확인 규칙 적용). ② 웹에서 `geyserCount` 훅 값(`currentGeysers().readCount()`)을 찍어 cpp 의 `Geysers().Count()` 와 시작 덤프의 간헐천 목록(7개)을 맞춰 본다. ③ `populate` 가 MayPlace 로 (131,204) 를 허용한 근거(웹)와 cpp 의 같은 질의를 비교한다.

## 이어서 할 일

1. `NETSTORM_L3_ALL=1 npx playwright test e2e/l3-mission-starts.spec.js` 를 끝까지 돌려 31개 모두(bc7 은 "다름" 단언) 통과하는지 확인한다.
2. bc7sun_bars76 의 간헐천 차이 원인을 찾아 고치면 `KNOWN_DIFFERENCES` 에서 뺀다.
3. 이 단위를 마무리한 뒤 LEFT_JOBS 0.1 진행 요약·3.1 4D-4b 항목을 갱신한다(Tutorial4~6·캠페인 미션은 시작 상태까지 같고 이후 진행은 미션별 L3 로 이어 간다).
