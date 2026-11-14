# webpj 생산 내용물 정보 메뉴 원본 관측 연결 (WB04-E490, 2026-11-15)

3.8절 이월 목록의 `inventory-about*`를 원본 10.78 독립 x86 관측 **3,888행**(이전 `inventory-about` 1,296행 + 새 클래스 `inventory-about-class` 2,592행)에 대조했다. 3,888행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈·시험 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다.

## 비교한 것

`tests/inventory-about-native-fixtures.test.mjs`가 cpppj `InventoryAboutMenuTests`의 입력 구성을 옮긴다.

- **입력(앞 열)**: 새 클래스 fixture 는 내용물 클래스(`+10`, 6 과 회수 제외 122 등), 공통으로 종류 WORD(`+18`)·플래그 WORD(`+20`)·편집 여부·정수 비용·SP(float32 비트)·Help 종류(0 없음/1 빈 문자열/2 `Help`). SID 50 의 내용물 슬롯에 설치하고 기존 4af010(`affordable`)이 읽는 비용/SP 장치만 공급한다. 새 About 계산의 기대값은 주입하지 않는다.
- **비교**: 호출 순서 문자열(`Begin`·`Idle`·`Object`·`Describe`·`Translate`·`Header`·`Money`·`Entry`(라벨 없음 `-`과 빈 라벨 구별)·`Attach`·`Sound`)이 효과 열과 정확히 같고, 호출 뒤 내용물 슬롯과 항목 노드가 변하지 않는다. 회수 비용의 float 저장·넓은 합/quarter 분기, 편집 관문, 부족 머리, Help 3종류가 모두 이 행들에 걸린다.

## 변형 확인

자동 변형 35개 중 28개가 잡히고 7개가 생존했다. 기존 `inventory-about-menu` 시험과 합치면 생존 0 이다(생존 7개는 생성자 검증의 `||` 연결·WORD 상위 바이트 이동·종류 위치·회수 비트 등 단일 값 경계로, 관측 입력의 종류가 한 바이트 범위라 가려지고 기존 손 계산 시험이 잡는다).

## 남은 것

3.8절의 `hud-nsinfo-create`(170행)·`minimap-blit`(122행)·`gump-pointer-boundary`(3,879행)·`menugump`(309행)이 남아 있다.
