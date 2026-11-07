# webpj 건물 회수 명령·GetTechnology·의식 선택 입력 이식 — WB04-E35

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른다섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawSalvageCommand`(10.78)와 `client/ThemeTechnologyCommand`의 일반 입력 분배, `client/RitualChoiceInput`을 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-salvage-command.js` — `RawSalvageCommand`

- `request(sid)` [44c420]: 최초 SID 0은 정상 무동작. 반복마다 SID가 풀 범위 밖이면 `RangeError`, 상태 bit 0/1(free/dead)이면 끝, 진단(`checking`)이 켜져 있으면 제단·다이스 타입의 비용이 다를 때 `assertion(0x295)` 뒤 현재 값으로 계속(비용 `NaN`도 다르다고 봄). 슬롯이 제단 타입이면 `daisAt`의 다이스로 옮겨 같은 관문을 반복. 아니면 종류(70 미만은 +18 WORD, 이상은 타입 BYTE)를 구해 로컬 소유이고 `denySalvage`이며 너깃 종류가 아니면 `tell('DenySalvage')` 후 끝, 그 밖에는 `send(sid, 0x200000)`.
- `send(sid, flags)` [4437c0]: 8바이트 `[protocol&0x7f, message, 길이 WORD, SID 하위 WORD, flags 상위 WORD]`를 `send(9, 패킷)`로 보낸다(헤더 값은 호출마다 `state.request`에서 읽음).
- `receive(sender, packet)` [4436f0]: 8바이트 미만은 `TypeError`. SID 0·풀 밖·free면 무시, 미션이 아니면 슬롯 소유자 == sender, 서버 권한이 없으면 `assertion(0x563)` 뒤 권한을 **다시 읽어** 없으면 끝. 권한 진단 **이후의** 패킷 flags WORD를 한 번 보관(`<<16`)해 타입 ≥70이고 `flags&0x200000`이면 `refund(sid, 현재 소유자, true)` 뒤 `destroy(sid, 보관한 flags)`(dead는 추가로 거르지 않음).

`www/core/get-technology-input.js` — `dispatchGetTechnologyInput`: GetTechnology(`0x0200009f`) 뗌은 효과 없이 소비, 누름은 y 전체 DWORD를 `acquire`에 전달(부모 누락 거부). 실제 지급 몸체는 이미 옮긴 `raw-theme-technology.js`.

`www/core/ritual-choice-input.js` — 원문 004d82ed/004d8351/004d83b7의 정적 복원: `constructRitualChoicePacket`(헤더 `[1, 0x6b, 길이 10]`만 쓰고 본문 보존), `dispatchRitualChoiceInput`(뗌은 장치 비요구, 누름은 패킷 작성이 대상 진단보다 먼저. UpgradeAltar는 대상 = y 하위 WORD·종류 = `daisType`, GetNewTech/RequestStormpower는 제단이 0이면 진단 `0x82f`/`0x838` 뒤 다시 읽어 계속, 종류 = y 전체 DWORD / `nuggetType`. +4에 WORD, +6에 DWORD를 쓰고 `send(9, 패킷)`).

## 2. 검증 (`tests/salvage-ritual-technology.test.mjs`, 4건)

- `salvage-command-x86.tsv` **3458행**(요청 1971·송신 400·수신 1087): 호출 순서·8바이트 송신·현재 전역(`localOwner/denySalvage/checking/mission/boss/altarType`)·세 raw 슬롯·caller 소유 수신 버퍼를 **두 x87 정밀도 기대 모두와** 비교. 외부 변경 8종(`mutation` 1~8: 슬롯 상태·회수 금지·제단 타입·너깃 종류·로컬 소유·권한 진단 뒤 패킷/슬롯 변경·환불 중 변경)을 원문 시점에 재생.
- 계약: 생성자 풀/타입 수 거부, 짧은 패킷·범위 밖 SID 거부, GetTechnology의 뗌 비요구/전체 DWORD/부모 누락 거부, 의식 선택의 패킷 헤더·세 분기 대상/종류 폭·진단 줄 `0x82f`/`0x838`·뗌 비요구·장치 누락 거부.

최초 실패와 교정: 회수 관측 3458행은 처음부터 통과했다. 의식 선택 계약 검사의 기대 패킷 16진을 2바이트(`0000`) 길게 썼다(패킷은 10바이트 = 헤더 4 + 대상 2 + 종류 4). 기대 문자열만 고쳤고 모듈은 그대로이다.

## 3. 경계

- `RitualChoiceInput`·`ThemeTechnologyCommand`는 `PORT_MAP`에서 `partial`이다. 의식 선택은 cpppj에도 테스트/통신 관측이 없는 정적 복원(원문 `RawRitualChoice`와 같음, WB10에서 확인)이라 소스 계약 검사만 했다. `ThemeTechnologyCommand`의 `ApplyThemeTechnology`(`RawCampaignSession`에 기술 지급 연결)는 캠페인 세션 단위에서 한다.
- 실제 환불(44c2f0)·가상 삭제(+10)·다이스 조회(448bb0)·송신(4e2dc0 비동기 경계)·`DenySalvage` 안내(4cf960)는 훅으로만 주입했다. 멀티플레이 송신/수신 연결은 우선순위가 낮다. CD 10.72의 별도 회수 본문은 범위 밖이다.

## 4. 다음 시작점

`ProductionShortcuts`(`production-shortcuts-x86.tsv`, `ProductionShortcutsTests.cpp`)·`HtmlInputDialog`·`DialogCommandLifetime`·`LanguageOptions`·`RawSquidPoolReset`. 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawBridgeConnect`, Player 나머지이다.
