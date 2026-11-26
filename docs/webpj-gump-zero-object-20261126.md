# World 없는 HUD의 객체0 연결

- 단위: WB04-E1375, 2026-11-26, HJOW-Athlon.

[ClientGumpRoot.bindObject](../webpj/www/ui/client-gump-root.js)는 유효한 노드의 객체0 연결에서 Form 호스트를 요구하지 않고 그대로 반환한다. 실제 비0 객체는 현재 호스트·기존 연결 플래그·SID 범위를 확인한 뒤 기존 실제 Form을 만든다. 노드가 없거나 이미 반납된 경우의 오류 처리는 유지했다.

Native [RawProductionControls.cpp Bind](../cpppj/src/client/RawProductionControls.cpp)는0 부모일 때 바로 반환하고 이전 Form도 지우지 않는다. State7이 실제 Player 기술 링크를0으로 만든 후 HUD를 교체하므로, World/Session이 아직 없어도 빈 생산 격자 생성이 이 계약을 사용한다. 웹은 객체0보다 호스트 검사가 먼저여서 이 생성 단계를 막고 있었다.

새 검사는 실제 ClientGumpRoot에서 호스트 없는 객체0 연결이 동일 노드·flags·object·form·등록 개수를 유지하는지, 비0 객체·잘못된 토큰·제거된 노드·반납된 루트가 기존 오류 경계를 유지하는지 확인한다. 이것은 HUD/Runtime 분리의 선행 수정이며 State7 부모 자체를 연결하지는 않았다.

## 검증

Node26 집중22/22와 번들 재현1/1 통과했다. 실제 Chromium1/1에서1-1 브리핑·시작·대기1200프레임 화면3곳0픽셀·덤프3곳허용밖0을 확인했다. 공격·포획·희생·승리는 수행하지 않았다. webpack 빌드·크기 검사도 통과했고 번들은3,461,020바이트다. [브라우저 원문](webpj-e1375-evidence/browser.log) · [검증값](webpj-e1375-evidence/verification.json).

다음은 공유 메뉴 생성의 eager gumpFormHost 조회를 실제 객체 Form 생성 시점으로 옮기고, 생산 UI가 실제 ProductionControlContext를 사용하게 연결하는 일이다. E1374 문서의 Client 저장소·메뉴/Chat·Runtime 반납·State7 순서 분할을 따른다. Console의 기한을 직접 쓰거나 고정25ms를 보정하지 않는다.
