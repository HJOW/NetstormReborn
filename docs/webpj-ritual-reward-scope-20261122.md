# 의식 보상 선택 메뉴(SacrificeMenu·기술 목록)의 웹 범위 판정 (WB04-E1180)

의식(희생) 뒤의 보상 선택 UI — Tell `SacrificeMenu`, 그 안의 `*TECHLIST*` 기술 목록 행, 명령 `UpgradeAltar`(0x020000d4)·`GetNewTech`(0x020000d5)·`RequestStormpower`(0x020000f9)와 이를 보내는 `RitualChoiceInput` — 을 단독 미션에서 앱에 연결해야 하는지 소스와 원본 대본으로 판정했다. **연결하지 않는다(웹 범위 밖).**

## 근거

1. **희생 메뉴는 비미션(전투·메타) 경로에서만 열린다.** 운반체가 제단에 사제를 내려놓는 입력(원문 `Carrier.cpp`, cpppj `RawCarrierInput.cpp:101`)은 `showSacrifice(dais)`를 부르는데, 미션 어댑터(cpppj `RawCampaignCarrierInput.inc:29`, 웹 `core/raw-campaign-session-carrier-input.js:53`)는 `if (missionPost) return`으로 **미션에서는 메뉴를 만들지 않는다**. cpppj `RawCampaignSession::ShowSacrificeMenu`(`RawCampaignSession.cpp:6171`)도 같은 관문(`globals.missionPost`)과 "비미션에서만 세 인자의 현재 Tell을 연다"는 주석을 가지며, 웹 클라이언트는 단독 미션 월드를 항상 `missionPost = true`로 만든다(`core/client-gameplay-globals.js`).
2. **미션에서의 보상은 자동이다.** 단독 미션의 희생 완료는 `RawCampaignRitual.inc`의 의식 소비자가 `Stormpower`(스톰 파워)·`Upgrade`(제단 업그레이드) 안내 Tell(`SacrificeForStormpower`·`AltarUpgrade`)을 띄우는 흐름이며 이미 앱에 연결돼 있다(`netstorm.js`의 `services.priest.shield.tell`). 선택지를 고르는 창은 없다.
3. **대본에도 없다.** 영어 대본 82개(`\D\*.english`·`*.fort`·`*.html`)를 모두 읽어 `TECHLIST`·`SacrificeMenu`·`GetNewTech`·`UpgradeAltar`·`RequestStormpower`를 찾았다. 쓰는 곳은 전역 `tell.english`의 `[SacrificeMenu]`(`$Timeout=15,RequestStormpower,0`·`$Menu=*TECHLIST*,DoNothing,{param.1}`) 한 곳뿐이며, 어떤 미션 대본도 이 섹션이나 명령을 부르지 않는다. 원소별 `*THEMETECHLIST*`는 아예 쓰이지 않는다.
4. **명령 소비자도 전투 전용이다.** 세 명령은 `RitualChoiceInput`이 원문 `6b` 요청 패킷(`RawRitualChoice` 수신)으로 서버(9)에 보내는 일이고, 이 요청은 멀티플레이 서버의 의식 선택 처리다. 단독 미션의 로컬 서버에는 그 입력을 낼 화면이 없다.

## 결론

- 인계 7단계 "의식 — RitualChoiceInput과 보상 UI·제단·World 연결"과 "공장·지식 메뉴와 기술" 항목의 남은 일인 "`TechnologyListMenu`(원문 004cb690) HTML 기술 목록 앱 연결·제단 업그레이드/`Get 5,000` 행"을 웹 범위 밖으로 닫는다. 이미 이식된 `core/ritual-choice-input.js`·`core/raw-ritual-choice.js`·`ui/technology-list-menu.js`는 소스 계약 시험으로 남기고 앱에는 연결하지 않는다.
- 멀티플레이 전투(비미션) 경로를 구현하게 되면 그때 `SacrificeMenu` Tell(`session.showSacrificeMenu` 신규 구현 — 현재 웹 Session 에는 없다)·`HtmlWindowMetadata`의 `tech`/`themeTech` 서비스·세 명령의 입력 프레임 소비를 함께 연결한다.
- 기준선 뒤 cpppj 변경 없음(`RitualChoiceInput.cpp`·`TechnologyListMenu.cpp`·`RawCampaignSession.cpp`의 해당 구간).

## 대응표 정리

`RitualChoiceInput.cpp` 행의 남음(앱 결선)이 위 판정으로 사라져 상태를 partial → port 로 닫고 비고에 근거를 더했다. `TechnologyListMenu.cpp` 행은 이미 port 이며 비고에 같은 판정을 더했다.
