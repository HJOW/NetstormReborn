# 플레이어가 가만히 있을 때의 자연 승패와 Priest Training 승패 뒤 흐름 (WB04-E1161)

LEFT_JOBS 의 남은 확인 두 가지를 실제 Chromium 으로 시도했다.

## 1. Priest Training 승패 뒤 흐름 (확인함)

- 대본은 `netstorm.tarc` 의 `bc5walkers76.english` 이다. 실패 구역 `[Failed]` 의 단추는 `Training Menu`(`MissionBegin,BC1Menu76`)·`Restart`(`Tell,TryAgain`)·`Leave Missions`(`MissionAbort`)이고, 성공 구역(`[succeeded][ai2priestcaptured]`)은 여기에 `Next`(`MissionBegin,BC6Mows76`)가 더 있다.
- `e2e/mission-priest-training-flow.spec.js`(4:3·16:9 통과): Death Bridge 시작(요청한 대본이 도는지 확인) → 로컬 Player 활성 플래그를 지워 실패(E1152 와 같은 판정 입력) → 단추 세 개 확인 → `Restart` → 다시 시도 묻기(`Replay Mission`/`Leave Missions`) → `Replay Mission` → 브리핑(`Technology available`/`Lets do this!`) → 다시 실패 → `Training Menu` 가 `bc1menu76`(대소문자 무시)을 시작 → 시계가 흐름. 시작 허용 목록에서 `bc1menu76` 을 빼면 `Training Menu 브리핑` 단계에서 실패한다.
- 성공 구역은 판정 조건이 `ai2priestcaptured`(적 사제 포획 상태, 소유자 상태 1)라 판정 입력을 시험에서 만들 수 없어 확인하지 않았다(적 사제 포획은 실제 전투가 필요).

## 2. 플레이어가 가만히 있을 때 적 AI 의 자연 패배 (관찰 못 함)

- 옵트인 `e2e/mission-idle-outcome.spec.js`(`NETSTORM_IDLE_SECONDS`·`NETSTORM_IDLE_MISSIONS`)로 입력 없이 실제 시간을 지켜봤다. Vicious·Surrounded·GuardMyBack 을 각각 10분(게임 시간 약 600초, 안내 창만 닫음) 둔 결과 **세 미션 모두 승패 안내 창이 뜨지 않았다**(31분 소요).
- 해석: 이 미션들의 AI 는 가만히 있는 플레이어를 10분 안에 쓰러뜨리지 않는다(거리가 멀거나 다리·섬 확장에 시간을 쓴다). 자연 패배를 보려면 더 오래 지켜보거나 플레이어가 AI 와 접촉해야 한다. 가짜 시계 빨리 감기는 AI 가 프레임 구동이라 쓸 수 없다([빨리 감기](webpj-fast-forward-clock-20261121.md)).
- 승패 안내와 이어지는 흐름 자체는 E1152(The War Begins!)·이번 단위(Death Bridge)에서 판정 입력을 바꿔 확인했다.

## 남은 것

- 실제 전투로 적 사제를 포획·희생하는 승리, AI 가 사제를 쓰러뜨리는 패배는 확인하지 못했다(사람이 직접 하는 플레이 확인).
