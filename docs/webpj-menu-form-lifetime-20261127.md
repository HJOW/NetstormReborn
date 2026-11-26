# 공유 메뉴의 지연 Form 호스트 조회

- 단위: WB04-E1376, 2026-11-27, HJOW-Athlon.

[ClientMenuGump](../webpj/www/ui/client-menu-gump.js)는 생성 중 Session의 gumpFormHost()를 읽지 않는다. parts.formHost()를 제공하면 실제 객체 Form을 생성·소유 등록·부착하는 단계에서 현재 호스트를 얻는다. 기존 호출자는 parts.session.gumpFormHost()를 같은 단계에서 사용한다. 상태 전환 중 공유 writer·Chat을 먼저 만들 수 있도록 좁은 실제 장치 경계를 추가한 것이다.

객체0 메뉴 부착은 기존 MenuGumpWidgets.reference의 원래 분기로 Form 장치를 요구하지 않는다. 실제 객체는 TYPE3·mode2 Form을 만든 뒤 같은 메뉴 등록 ID와 SID를 연결한다. Root의 공통 소멸이 이 Form을 반납한다. 이 단위는 전체 생산 조립기의 Session 의존과 State7 부모를 아직 연결하지 않았다.

## 검증

새 검사는 실제 ClientGumpRoot·MenuGump·레이아웃·수명 본체를 사용한다. 생성과 객체0 부착 때 호스트 조회가0회인지, 생성 뒤 Ready가 된 장치에서 실제 Form 생성→소유자→객체 연결과 공통 반납이 이어지는지 확인한다. 글꼴·그리기와 Form 장치 경계는 검사 장치이며 Session 대체 객체를 만들지 않는다.

Node 집중20/20과 번들 재현 검사가 통과했다. 실제 Chromium1/1에서1-1의 아군 신전·적 배터리·적 신전 객체 메뉴가 정상 생성·닫힘을 확인했다. 객체의 HP·SP·승패를 바꾸거나 공격·포획·희생을 수행하지 않았다. 번들은3,461,117바이트다. Node26.13.0 전체6,864/6,864가742.474초에 통과했고 실패·취소·미실행은0이다. [브라우저 원문](webpj-e1376-evidence/browser.log) · [검증값](webpj-e1376-evidence/verification.json).

## 이어 할 일

기존 ProductionControlContext의 실제 Client 저장소를 생산 패널·격자·항목·선택에 전달하고 초기 슬롯/기한을 생성 단계에서 보존한다. 공유 writer·buffers·Chat 생성과 World 전용 메뉴 어댑터 연결을 나누고, Runtime 반납이 State7에서 새로 만든 HUD를 닫지 않도록 수명을 분리한다. 그 뒤 State7 공통 부모와 실제 시작·AI7시점 화면을 재검증한다. E1374 문서의 세부 순서와 E1373 관측값을 따른다.
