# State HUD 생성과 최초 World 도킹 분리

- 단위: WB04-E1374, 2026-11-26, HJOW-Athlon.
- E1373에서 확인한 State7 생성 수명 복원을 위한 첫 구현 단위다. 제품 State7 부모 연결과 Session 없는 생산·메뉴 생성은 다음 단위에서 수행한다.

[ClientUberHud](../webpj/www/ui/client-uber-hud.js)에 createForState()와 dockForWorld()를 분리했다. 앞 단계는 실제 UberRootGump.replace()로 루트·자식을 생성하고 현재 루트로 공개한다. World의 후처리는 아직 요청하지 않는다. 뒤 단계는 같은 객체들에 최초 mode3 도킹과 기존 채팅 숨김 전환을 한 번 적용한다. 반복 연결 통지는 구분선·채팅 표시를 다시 전환하지 않는다. close 뒤의 새 생성물은 다시 첫 도킹을 수행할 수 있다.

기존 install() 호출은 두 단계를 동기적으로 이어 호출하므로 이 단위의 게임 실행 순서는 종전과 같다. Console 기한을25ms 보정하거나 초기 시각을 직접 쓰지 않았다. 제품 State7이 아직 이 새 생성 API를 호출하지 않으므로 실제 입력줄 생성 지연과31·1·99픽셀 차이는 계속 열린 작업이다.

새 집중 검사는 실제 ClientGumpRoot·구분선·UberRoot 본체로 생성과 World 후처리의 분리, 같은 자식 객체의 정체 유지, 반복 도킹 무부작용, close 뒤 재생성을 확인한다. 자식이 같은 값의 새 객체로 바뀌어도 통과하는 깊은 동등 비교 대신 실제 객체 참조를 확인한다. 기존 도킹·해상도 변경·반납 검사도 유지했다.

## 검증

HUD 집중5/5와 번들·대응표를 포함한 Node13/13 통과했다. 실제 Chromium1/1에서 캠페인1-1 브리핑·시작·대기1200프레임의 화면3곳0픽셀·덤프3곳허용밖0을 확인했다. 공격·포획·희생·승리는 수행하지 않았다. webpack 빌드·번들 재현/크기 검사가 통과했으며 번들은3,461,014바이트다. [브라우저 원문](webpj-e1374-evidence/browser.log) · [검증값](webpj-e1374-evidence/verification.json). 정기 전체 검사는 E1376(v26)·E1380(v26/22)에서 이어간다.

## 이어 할 복원 경계

이번 작은 단위는 루트 생성/도킹을 분리했다. 다음 작업이 커서 다음처럼 나눈다.

1. **Client 생산 문맥:** 이미 있는 ProductionControlContext의 storage/gameplay/players/options/sp/색표를 생산 조립·패널·격자·항목·선택에 직접 전달한다. 초기 슬롯·bridgeUntil은 실제 생성 단계에서 기록하고 Ready에 다시 초기화하지 않는다. 월드 효과는 실제 gameplay.requireSession()을 호출 시점에 빌린다.
2. **공유 메뉴·Chat:** writer/buffers/Chat을 State7에서 생성하고 월드 전용 About/Player/Priest/Factory/Object/Knowledge 어댑터는 Ready에서 연결한다. 늦게 연결하는 필드는 같은 실제 binding을 읽는다. 메뉴 생성 때 즉시 gumpFormHost()/spStorage()로 Session을 요구하는 경계도 분리한다.
3. **Form0과 Runtime 수명:** bindObject(0)는 실제 호스트 없이 반환하고 비0 객체만 현재 호스트를 요구한다. Ready에 실제 gumpFormHost를 연결한다. State7 ReplaceUber는 Runtime 반납보다 앞이므로 기존 _clearCampaignWorld()로 새 HUD까지 닫지 않도록 Runtime 반납과 HUD 소유권을 분리한다.
4. **State7 부모와 관문:** ClearPlayerLinks→Kill/CleanupGumps→ReplaceUber→Core/Player 해제→resetPool 순서를 연결한다. Replace 시점에 playerKnown=0이라고 가정하지 않는다. 기술 링크는 앞 단계에서0이 되며 현재 실제 Player를 읽는다. Tutorial의 HUD등록→Guide init→Start/AI 및 SID 순서와 전 미션 시작/AI7시점을 재검증한다.

Native 장기 문맥은 [ProductionControlContext.h](../cpppj/src/client/ProductionControlContext.h), [RawProductionControls.cpp](../cpppj/src/client/RawProductionControls.cpp), [UberHud.inc](../cpppj/src/client/UberHud.inc)다. 웹의 [ProductionControlContext](../webpj/www/core/production-control-context.js)와 [synchronizeProductionControlContents](../webpj/www/core/client-production-sync.js)는 이미 실제 Client 저장소를 사용하므로 재사용한다. Session 형태를 흉내 내는 중간 객체는 만들지 않는다.
