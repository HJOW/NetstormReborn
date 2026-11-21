# 객체 명령(Detonate·Meltdown·신전 명령) 연결과 입력 큐 정체 안전장치 (WB04-E1150)

앱에 연결되지 않은 모듈을 `www` 안에서 찾는 점검(다른 `www` 모듈이 가져오지 않는 71개)으로 `detonate-input`·`vortex-command-input`·`scenario-object-input`·`rotation-key-input` 이 앱에 연결돼 있지 않음을 찾았다. 그중 객체 명령 세 분배기는 실제 플레이에서 **입력을 영구히 멈추게** 하는 결함이었다.

## 증상

- 우클릭 메뉴의 `Detonate!`(Walker), `Meltdown!`(제네레이터), 신전의 Ascend 같은 항목은 입력 큐에 객체 명령(`0x03000074` 등)을 넣는다. 웹 입력 루프가 이 코드들을 모르는 사건으로 보고 "미구현 사건 앞을 추월하지 않는다" 규칙으로 큐 머리에서 멈췄고, 이후 모든 입력(선택·단축키·클릭)이 막혔다.
- 직접 `DetonateBomb` 명령을 큐에 넣고 `P` 를 누르면 큐 크기가 계속 늘고 선택이 바뀌지 않았다.

## 원인

- 원본(cpppj `UberGump.cpp` 1660~1700행)은 일반 입력에서 신전 세 명령(WipeOut·Ascend·Descend) → 시나리오 객체 다섯 명령(MakeMine·InflictDamage·DestroyPiece·TurnToObelisk·Meltdown) → DetonateBomb 을 차례로 `Session.CommandObject`(객체 가상 6c)에 전달한다. 웹에는 분배기 모듈과 본체(신전 `RawCampaignVortex.command`, `RawMeltdown.command`, `RawObjectConversion`, `RawObjectCommand`)가 있었지만 세션의 `commandObject` 진입점과 앱 입력 루프의 분배가 없었다.

## 수정

| 위치 | 내용 |
| --- | --- |
| `www/core/raw-campaign-session.js` | `commandObject(parent, command, parameter)` 추가: 네 신전 VT(`5196f0·5197a8·519860·519918`)는 신전 명령을 먼저, 제네레이터(VT `501ea0`)의 Meltdown(`0x030000f8`)은 요청 뒤 공통 부모, 비석/주문 전환(`0x030000ce`)은 `RawObjectConversion`, 그 밖은 공통 객체 명령(소유자·피해·삭제·진단)이다. |
| `www/netstorm.js` 입력 루프 | 신전·시나리오 객체·Detonate 명령 코드를 지원 목록에 더하고 원본 순서로 세 분배기를 부른다. |
| 같은 입력 루프(안전장치) | 처리하지 못하는 사건이 3초(벽 시각) 넘게 큐 머리에 남으면 `console.error("미구현 입력 사건을 버립니다: …")` 로 기록하고 버린다. 3초 전에는 순서를 지켜 머리에 남는다. 앞으로 다른 미연결 명령이 생겨도 입력이 영구히 막히지 않는다. |

- `WipeOut`(0x0300006f)은 원본이 스스로 표시하는 개발자 진단(`Vortex.cpp:746`)을 `console.error` 로 낸다(원본 `assertDisplay` 를 그대로 옮긴 것).

## 시험

- `e2e/mission-object-commands.spec.js`(`NETSTORM_E2E_ASPECT` 로 화면비 선택):
  - 신전 세 명령·시나리오 객체 다섯 명령·DetonateBomb 을 큐에 넣어 각각 큐 머리에서 소비되고(오류 없음) 이후 `P` 로 사제가 선택된다. 분배를 빼면 첫 명령에서 실패한다.
  - 처리하지 못하는 사건(임의 코드 `0x0300ffff`)이 1.5초 뒤에는 머리에 남고 3초 넘으면 버려져 입력이 계속된다. 안전장치를 무력화하면 실패한다.
- 세션·입력·객체 명령 관련 Node 시험 348건이 통과한다.

## 남은 것

- `Detonate!` 가 Walker 를 실제로 폭파시키는 효과는 원본에서도 객체의 가상 6c(`CommandObject` 기본 `RawObjectCommand`)로 위임되며, 웹 `RawObjectCommand` 는 소유자·피해·삭제·진단 명령만 처리한다. 폭파 자체의 본체(Walker 의 자기 6c)가 웹에 있는지는 Walker 를 실제로 폭파시켜 봐야 알 수 있다(현재는 입력이 막히지 않고 미처리 0 으로 끝난다).
- 앱에 연결되지 않은 나머지 모듈(71개 중 `rotation-key-input`·`move-dialog`·`hud-ns-info`·`raw-theme-technology` 등)은 같은 방식으로 하나씩 확인한다.
