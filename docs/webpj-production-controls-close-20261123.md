# RawProductionControls partial → port 판정 (WB04-E1257)

대응표 `cpppj/src/client/RawProductionControls.cpp` 행(393줄)에 남아 있던 E1224 감사의 "표시만" 차이를 하나씩 확인·판정해 행을 `partial` 에서 `port` 로 닫았다. 코드 변경은 없다(e2e 한 건 추가, partial 25→24단위, 18,259→17,866줄, port 983).

| 남은 차이 | 판정 | 근거 |
| --- | --- | --- |
| 우클릭 Help 문구/단어 미설정 | **E1236 에서 해소** | 스톰 파워 `Storm Power Available`/`moneyGump`, 생산 격자·가운데·아래 배경 `Production Window`/`teleportView` 를 `client-production-panel.js` `create` 가 설정하고 번역표 항목은 E1246 에서 더했다 |
| 루트 `SetIdle` 대신 `netstorm.js _runCampaignProductionFrame` 이 매 프레임 호출(순서 미세) | **영향 없음** | 원본은 Gump idle 패스 안에서 루트·예산 idle 을 돌리고 웹은 같은 프레임의 `gump.idle()` 바로 앞에서 `updateProductionContents()`·유지 작업을 돌린다. 두 갱신 모두 프레임마다 한 번이고 서로 독립이라 차이는 같은 프레임 안의 순서뿐이다 |
| `UpdateContents` 뒤 `common.changed` 무효화 없음 | **실제 확인(새 e2e)** | `e2e/mission-object-commands.spec.js` 새 시험(실제 Chromium, The War Begins!): 생산 다리 격자 영역을 처음 모습으로 읽어 두고 Session 의 생산 내용물 갱신이 새 다리 조각을 만들 때까지 지켜보면(최대 40초) 영역이 5% 넘게 바뀐다(세 번 반복 모두 통과). 내용물이 바뀐 뒤 격자가 다음 프레임들에 화면에서 다시 그려진다. 기술(`CheatAllTechnology`)은 생산 패널이 아니라 사제의 건설 메뉴에 나타나므로(Priest Training 7 에서 기술 지급 전후 패널 캡처로 확인) 이 시험의 대상이 아니다 |
| 격자 부모 변경 재바인딩 없음 | **단독 미션에서 도달하지 않음** | 격자는 기술(Technology) 객체에 바인딩되는데(`handleHudMode`→`bindObject`) 그 부모(현재 `templateOwner` 의 기술 링크 `+0x9c`)는 미션 중 바뀌지 않는다(E1086 의 기술 링크 관측: 비영·불변). 바뀌는 경우는 소유자를 옮기는 멀티플레이어·편집기 경로 |
| HUD 모드 이벤트 `0x0200003d` 는 설치 때 직접 호출만 | **원본과 같음** | cpppj 에서 이 코드를 보내는 곳은 `UberHud.inc:122` 의 설치(첫 월드 대여 mode 3 도킹)와 `HudDock` 내부 처리뿐이다(`RawProductionControls.cpp:224` 는 받는 쪽). 웹도 같은 두 곳(`client-uber-hud.js`·`hud-dock.js`)이 같은 상수를 쓴다 |

## 시험

- 새 e2e 1건(위 3번째 행, 처음 시도한 "기술 지급 전후 픽셀 비교"는 기술이 패널에 나타난다는 잘못된 가정이라 버리고 새 다리 조각 관찰로 바꿨다). 기존 생산 관련 Node·e2e 와 대응표·manifest 시험 통과.

## 남은 일

- 원본 실행으로 비교한 것은 아니다(웹 실제 동작·소스 판정). 순서 차이(2번째 행)는 한 프레임 안의 차이로 판단했고 프레임 단위 캡처로 대조하지는 않았다.
- 5-B 는 이 행으로 닫혔다. 단 `Cursor·RawCursor 표시` 체크는 카메라 투영·가장자리 스크롤·마나 볼트 정기 표시의 실제 화면 대조가 남아 열려 있다.
