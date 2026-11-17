# 맨 오 워 주문 자격 (WB04-E677)

기준선 `225553df`의 `o/RawManoSpell.cpp/.h`를 `raw-mano-spell.js`로 정적 이식했다. 서버·대상·세 가상표 관문, 사용하지 않는 지역 시각 seed, 현재 주문 TYPE의 반복 재조회, 생성 객체의 프레임 DWORD·소유자·헤더·원주 좌표·Pop 순서를 옮겼다. 기존 `RawDeckDraw.wallSeed`와 `windCirclePoint`를 사용하며 선택된 실제 하위 장치가 없으면 그 위치에서 실패한다. 기준선 뒤 원본 소스 변경은 없다.

공중 3종은 새 전용 생성·생산·행동 관측이나 검사를 추가하지 않는 사용자 결정을 따라 이 주문 본체는 소스 정적 대조와 구문 확인만 수행했다. 실제 맨 오 워 소환·게임 실행·Session 연결은 수행하지 않았다. `RawCampaignDevastation` 결선과 Native 장기 장치는 후속이다.

Node v26.13.0 전체 회귀 뒤 v22.23.3 전체 회귀를 순차 실행해 각각 6,103건 통과했다(실패·취소·건너뜀 0). 로그는 Git 제외 `extracted/webpj-e677-node26.log`·`extracted/webpj-e677-node22.log`다. E669 이후 여덟 번째 완료 전체 회귀이며 HJOW-Athlon에서 두 버전은 동시에 실행하지 않았다.
