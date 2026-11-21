# 미션 승패 안내와 이어지는 흐름 실제 확인 (WB04-E1152)

LEFT_JOBS 의 "실제 승패" 항목을 실제 Chromium 에서 확인했다. 앱에는 승패 판정기(`RawMissionOutcome.check`)·대본 구역 안내·미션 전환·진행 저장이 모두 연결돼 있었고, 이번에 그 전체 경로가 원본과 같은 화면·순서로 동작함을 시험으로 고정했다. 코드 결함은 찾지 못했다.

## 확인한 흐름 (The War Begins!)

| 조건 | 안내 창 | 단추 | 결과 |
| --- | --- | --- | --- |
| 로컬 사제 사망(로컬 Player 비활성) | `Failure!` ([Failed] 구역, 원본 `Lose 1` 스크린샷과 같은 배치) | `Continue` | 다시 시도 묻기(`Would you like to attempt the mission … again?`, 원본 `Lose 2`) |
| 〃 | 다시 시도 묻기 | `Replay Mission` / `Leave Missions` | Replay 는 같은 미션 브리핑(`Review Knowledge`·`Play Mission`) → 새 미션으로 다시 시작(사제 생존·시계 흐름·안내 창 재발생 없음). Leave 는 메인 메뉴로 나감 |
| 적 팀 전멸(적 Player 비활성) | `Success!` ([Succeeded][BadTeamDead] 구역, 원본 `Victory` 스크린샷과 같은 배치) | `Leave Missions` / `Next Mission` | 진행 키 `DoneTheWarBegins` 가 저장되고(실패는 저장 안 됨), Leave 는 메뉴로 나가 캠페인 목록에서 1번에 체크 표시·2번(Master of Whirligigs) 해제·3번은 잠김. Next 는 `MasterOfWhirligigs` 브리핑 → 시작 |

- 안내 중에는 게임 시계가 멈춘다(원본 Tell 의 `pauseForDialog`).
- 오류·경고 콘솔 출력은 없다.

## 승패 조건을 만드는 방법(시험 한계)

실제 사제 사망·포획은 적 AI 가 건너와 사제를 쓰러뜨리거나 포획해야 해 한 번에 수십 분이 걸린다. 그래서 시험은 판정기가 읽는 입력(Player 기록의 활성 플래그, +0x50 & 0x22)만 바꾼다. 판정기는 로컬이 비활성이면 사제 사망(Failed), 적이 모두 비활성이면 적 팀 전멸(BadTeamDead)로 본다. 그 뒤의 대본 구역 선택·안내 창·단추 명령·미션 전환·진행 저장은 실제 경로를 그대로 쓴다.

- 참고: 메뉴의 `InflictDamage` 명령은 원본처럼 HP 를 1 만 깎고(`RawObjectCommand`), 사제 HP 가 0 이 되면 사망이 아니라 `Our Priest has been Immobilized` 로 움직임만 막히는 상태가 된다. 포획·사망은 이후 적 유닛이 닿아야 생긴다. 이 때문에 이전 단계에서 "사제를 죽이는 시도가 패배로 이어지지 않던" 것은 결함이 아니라 원본 규칙이다.

## 시험

- `e2e/mission-outcome-flow.spec.js`(`NETSTORM_E2E_ASPECT` 로 화면비 선택, 4:3·16:9 통과): 위 세 흐름 3건.
- 변형 확인: 앱의 승패 구역 안내 연결(`services.missionOutcome.section`)을 끊으면 세 시험 모두 첫 안내 창 단계에서 실패한다.

## 남은 것

- 적 AI 가 실제로 사제를 쓰러뜨리거나 포획하는 자연스러운 패배, 실제 적 섬 점령으로 인한 승리 같은 장시간 진행(자연 완주)은 확인하지 못했다.
- 앱에 연결되지 않은 나머지 모듈(`move-dialog`·`hud-ns-info`·`raw-theme-technology`·`text-clipboard` 등)은 하나씩 확인한다.
