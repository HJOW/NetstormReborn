# 스커미시 게임 진행 확인과 내용물 수명 서비스 (WB04-E1106)

E1105에서 시작할 수 있게 된 스커미시를 시간이 흐르는 동안 관찰했다. 어려움 AI 한 명과 약 5분(게임 시계 279초) 진행해도 오류 없이 AI가 계속 섬과 다리를 늘렸고, 이 관찰에서 세션이 AI의 내용물 처리에 필요한 앱 서비스를 아직 공급받지 못한 것을 하나 찾아 고쳤다.

## 찾은 문제와 수정

- **증상.** 게임 시계 약 87초에 `실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:3066`이 콘솔 오류로 올라왔다. 3066은 내용물(ContentForm) 수명 효과 `wallet`·`addSp`·`sound`의 미연결 표지다.
- **원인.** 튜토리얼에서는 AI가 너겟을 나르지 않아 닿지 않던 경로이고, cpppj 클라이언트는 `services.contentLifetime`에 `localNotice`(같은 입력 큐에 명령 푸시)와 `sound`(위치 효과음 `guy.wav`)를 공급하는데 웹 앱에는 이 서비스가 없었다. `wallet`·`addSp`는 원문에서도 클라이언트가 채우지 않으며 같은 SquidReward가 지갑/SP를 맡는다.
- **수정.** [netstorm.js](../webpj/www/netstorm.js)의 미션 서비스 조립에 `services.contentLifetime = { localNotice: 입력 큐 pushCommand(command, 0, 0), sound: 현재 재생기 playNameAt(x, y, 'guy.wav', 0) }`를 더했다.

## 관찰 결과

- 어려움 AI 1명: 게임 시계 279초 동안 오류 없이 진행했고 AI 객체 수가 2 → 1,472로 늘었다(다리 조각 약 700개·다리 연결부 31개·섬 10개를 연결). 다리 건설 시작 시점은 실행마다 다르다(첫 20초 안에 시작하기도, 약 30초 뒤에 시작하기도 함).
- 사람(소유자 1)은 플레이어와 사제만 가지고 시작한다(스커미시는 시작 건물이 없다). AI도 플레이어와 사제 하나씩으로 시작한다.

## 검증

- 새 Chromium `e2e/skirmish-play.spec.js` 1건(약 1.2분): 어려움 AI 1명 설정으로 시작 → Go! → 60초 넘게 진행하며 3번 관찰해 게임 시계가 40초 넘게 흐르고, 소유자 1·2가 각각 플레이어·사제를 하나씩 가지며, AI(소유자 2)의 객체가 100개를 넘고 다리 조각 50개 초과·다리 연결부가 있으며 콘솔·페이지 오류가 없음을 확인한다. 두 번 연속 통과했다. 위 3066 오류는 수정 전 같은 시험에서 재현됐다.

## 금지 유닛과 승패 확인 (WB04-E1107)

- **금지 유닛.** 새 Chromium `e2e/skirmish-bans.spec.js`가 설정 `skirmishBans = outpost,sunCannon,bulf`로 시작해 Guide의 미션 허용 표(`restrictions`, 256칸)에서 금지(0이 아닌 값)인 TYPE이 정확히 `126:outpost`·`128:sunCannon`·`135:bulf` 세 개뿐임을 확인한다(생성 대본의 `techAllowed="deny;…"`가 기존 미션 기술 규칙을 거쳐 표에 반영됨). 허용 표는 세션의 총 생산(`RawTotalMade`)이 읽는다.
- **승패.** 승리(`[Succeeded][BadTeamDead]`)와 패배(`[Failed][GoodTeamDead]`) 구간은 생성 대본에 있고 `script.section('BadTeamDead')`·`('Succeeded')`가 Victory 본문을 돌려준다. 이 안내는 기존 단독 미션 승패 판정(`RawMissionOutcome`, 사제 owner 상태 2)의 경로를 그대로 쓰므로 새 코드가 없다. 브라우저에서 AI 사제를 `world.destroy`로 지우는 시도는 사제 사망 상태(owner 상태 2)를 거치지 않아 판정을 일으키지 못했으므로(실제 피해로 죽는 경로가 필요) 이 판정은 자동 시험으로 확인하지 않았다. 실제 전투로 사제가 죽는 재현은 1절 사용자 확인 범위다.

## 남은 일 (12차 나머지)

- AI가 시작 지연 뒤 섬을 연결한 다음 생산하고 공격하는 단계까지(수 분 이상), 사람 쪽 건설·생산, 실제 전투로 사제가 죽을 때의 승패 안내와 `MissionAbort`, 비미션 너겟 보충과 간헐천의 사람 눈 확인이 남았다(자연 완주·실제 전투 재현은 1절 사용자 확인 범위).
