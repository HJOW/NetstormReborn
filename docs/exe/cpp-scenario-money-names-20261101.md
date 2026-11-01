# 대본 송금 이름과 남은 명령 목록 — 단위687

2026-11-01, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-10-23 자료다. AGENTS의 구 온라인 검증 제외·외부 기능 문서화·11.01/Credits3항목을 유지한다. 제품687/11.01이다.

`SendMoneyScriptCode`에 빠져 있던 원본 이름 다섯 개를 연결했다. 기존 `SendAllBucks`와 함께 여섯 이름을 ASCII 대소문자 무시로 조회한다. 실제 일반 입력/메뉴/금액 차감·AI 지갑·패킷 처리는 변경하지 않았다.

| 이름 | 원본 표 항목 | 명령 전역 / 코드 | 기존 효과 |
| --- | --- | --- | --- |
| SendBucks | 005432e8 | 00542aa4 / 020000ec | 1000 |
| SendxBucks | 00543388 | 00542af4 / 02000100 | 숫자 입력 창 |
| Send700Bucks | 005433c8 | 00542b14 / 02000108 | 700 |
| SendAllBucks | 00543400 | 00542b30 / 02000118 | 현재 SP 조회/실행 |
| Send300Bucks | 00543430 | 00542b48 / 02000120 | 300 |
| SendAllBucks2 | 00543488 | 00542b74 / 0200012b | 전체 송금 확인 |

제품 변경 전에 읽기 전용 도구 `tools/cpp_scenario_script_inventory.py`로 보호 PE 이름표와 `originals/d/*.english`499개를 조사했다. 지정 범위의 이름 항목288개 중 명시 `<$태그>`에 쓰인 이름은 대소문자를 합쳐22개다. `SendBucks`는5개 대본에서 발견됐다. 이는 커스텀 자료를 포함한 파일 목록이며 공식34경로의 실행 횟수/사용 명령 전체가 아니다.

후속 일반 입력/부모 대조 후보는 `AddRandom`, `CreateRandomGuns`, `DestroyAt`, `Lightning`, `StartSmoke`다. 이름이 코드 문자열에 없다는 이유만으로 누락을 단정하지 않는다. 예를 들어 `CreateRandomGuns`는 원문 일반 입력의 명시 무동작 후보이므로 실제 처리 구간을 확인해야 한다. 표에 없는 `Menu`/`Timeout` 같은 HTML 메타와 `MisionBegin`/`MoveDialogBox` 오자를 구별하며 임의 별칭을 추가하지 않았다. TARC·Config 확장·동적 명령·Menu 둘째 인자는 이 목록의 범위 밖이다.

Release 제품/집중 대상을 한 번 빌드해 경고·오류0이다. 새 단위 검사/fixture는 추가하지 않았다. 기존 송금 검사에 여섯 이름의 실제 parser/정수 큐·뗌 확인을 확장했고 `netstorm_scenario_money_tests` 기존8개가 모두 통과했다. 기존 원문 송금888·대본262·객체 입력480의 재생을 포함한다. 원문 PE를 새로 실행한 것이 아니며 게임·통신·MP 검사·전체34·전체C++·새ZIP은0이다.

기존 보호2604파일과686의 추가 fixture1개를 재해시해 같은 내용을 확인했다. 현재 fixture785개다. 제품/검사/해시는 `cpppj/scenario-money-names-20261101.json`, 원문 목록은 `cpppj/scenario-script-inventory-20261101.json`, 작업 PC 빌드/실행 기록은 `extracted/cpp-scenario-money-687/`에 있다. 사제 포획/희생·특정 적 공격·승리 재현과 사용자 자연 완주 검사는 하지 않았다. 남은 시나리오/Edit·수명·호환성 뒤MP 무시험 사전 구현과4~7차를 계속 진행한다.
