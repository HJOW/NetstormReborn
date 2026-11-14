# webpj 포인터 안팎 전환 앞단 원본 관측 연결 (WB04-E491, 2026-11-15)

3.8절 이월 목록의 `gump-pointer-boundary`를 원본 10.78 독립 x86 관측 **3,878행**에 대조했다. 3,878행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈·시험 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다.

## 비교한 것

`tests/gump-pointer-native-fixtures.test.mjs`가 cpppj `GumpPointerBoundaryTests`(`Native3878`)의 입력 구성을 옮긴다. 입력 열만 공급하고 관측 열은 비교에만 쓴다.

- **입력**: 언어 번호, 정적 초기화 DWORD·`wasInside`·마지막 안→밖 좌/우 기록(code/x/y), 첫 폴링(Alt)·Ctrl 폴링·좌/우 버튼 폴링 사건, 호출 중 현재 기록을 바꾸는 mutation 번호(1~6)와 단계(`P3`·`Q1`·`P1` 등), 포화를 만드는 큐 prefill 개수(실제 40칸 `InputQueue`).
- **비교**: 반환된 현재 포인터(code/x/y 전체 DWORD), 초기화 비트와 `wasInside`, 좌/우 기록 12바이트, 외부 효과 순서(`P,code,...`/`Q,...`/`M<n>`), 그리고 prefill·합성 release 를 포함한 큐 전체를 비워 나온 사건 열(release 도 버리지 않음). 언어 3 의 Ctrl 합성 release, 밖→안 재진입의 좌/우 먼저 폴링 후 눌림 기록+뗌 현재 조건, callback 뒤 현재 기록 재조회, 마지막 `wasInside` 덮어쓰기가 모두 이 행들에 걸린다.

## 변형 확인

자동 변형 39개 중 31개가 잡히고 8개가 생존했다. 생존 8개는 상태 클래스의 필드 기본값(`initializationFlags=0`·`wasInside=1`·좌/우 기록 0)으로 이 시험이 모든 필드를 원문 입력으로 설치해 가려지며, 기존 `gump-pointer-boundary` 시험이 기본값을 확인해 합치면 생존 0 이다.

## 남은 것

3.8절의 `hud-nsinfo-create`(170행)·`minimap-blit`(122행)·`menugump`(309행)이 HUD 쪽 남은 항목이다.
