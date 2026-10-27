# 비석의 Edit/일반 메뉴와 CHEAT 표시 관문 — 단위525

2026-10-28, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 기존 산출물을 읽기만 했다. 시작 커밋은 `912ca8ac`이다. 제품 코드·기존 검사/fixture·실행파일은524와 동일하다.

사용자는 **CHEAT 문구가 있는 항목은 게임 진행 중에 나타나지 않으며, Edit 편집에서만 나타날 수 있다**고 명확히 했다. Edit 안에서도 항목별 원본 조건을 유지한다.

## 확인한 원문과 초기 실패

원래 비석의 `462390`은 `005c85a4`가 켜져 있으면 Turn To Spell!을 추가하지만, CHEAT Dig Up의 include 인자는 별도의 **0055a480**이다. `4353ea..43541a`의 설정 해석은55a480/55a484를0으로 쓴 뒤 userCheat를 읽고 유일한 값8675309를 허용한다. userCheat와55a480은 같은 변수가 아니다. 실제 Client의 같은 해석과 원문144조건을 재생하는 기존 검사도 이를 확인한다. Session의 `objectState.special=true`를 보고 `playerState.special`/55a480도 켜졌다고 판단하면 안 된다.

첫 드라이버는 Edit 진입만으로 Dig Up이 표시된다고 가정해 실패했다. 실제 비석 SID15021을 정확히 우클릭했지만 메뉴는 Turn To Spell!/Destroy/About/CHEAT Set Owner의4항목이었고, `UI control not found: CHEAT Dig Up`으로 종료1이다. **Dig Up 명령 호출은0회**이며 제품 종료 결함이나 보상 실행 실패의 재현으로 세지 않는다. 원문 조건을 바꾸거나 special을 강제로 켜지 않았다. 최초 드라이버·로그·메뉴 캡처를 별도로 보존했다.

디컴파일의55a480 직접 참조 범위에서는0 대입만 확인했다. 이것은 모든 간접 쓰기/외부 디버그 상태가 불가능하다는 증명은 아니다. 합법적인 special 진입 경로 확인 전에는 실제 Dig Up 클릭을 완료로 기록하지 않는다. 일반 입력0200002d→4620c0의 기존 보상 백엔드 연결은517 그대로 유지한다.

## 실제 검증과 범위

- 최종1280×720·1024×768의 Edit/일반 **4실행 모두 종료0**이다. 원래 dissolvedalliance의(88,119) 비석을 방향키로 화면 안에 가져와 픽셀 선택기를 통해 우클릭했다. 새 객체·QA·프레임·owner를 주입하지 않았다.
- Edit는 실제Editor/missionPost `1/1`이며 위4항목을 표시한다. Dig Up은 숨겨져 있고 바깥 클릭 후 비석의종류/owner/좌표를보존하며메뉴를반납한다. 일반 모드`0/1`은 About만 표시하고 **CHEAT 문구 항목이 없다**. 새 사용자 기준은 저장된 원시 메뉴에도 다시 대조했다. Edit1280×720·일반1024×768 캡처를 직접 확인했다.
- 기존 `ClientScriptMenuOptions`2개·`ObjectMenuVariants`1개·`GemReward`2개, **5고유/실패0**이다. 원문 설정144조건·메뉴1024조건과 실제 Session의 보상 선택/Pop/삭제를 포함하지만 새 원문 관측이나 실제 Dig Up UI 효과로 세지 않는다. 전체3,357개/34경로/자연 완주를 반복하지 않았다.
- 빌드0회. 제품/검사/기존fixture의 모든 소스 해시와 두 실행파일이524 freeze와 동일하다. 게임SHA `ab2862d7e56fb3751d0a2aab270404f9e7b2da03d94e16abc376ed1d9e6412af`, 검사SHA `266562c4bad85c2c5fc22fe015f4956c292d5a1fcbbc46200ba4b59ebf0af57b`다. 같은 보호 기준선에 대조한 원본/CD1,819파일과AGENTS.md도불변이다. 진행중게임/빌드/검사는없다.

원시는 `extracted/cpp-obelisk-dig-up-525/`의first/final·집중검사이며, 반복 가능한 도구는 `tools/cpp_obelisk_mode_menus_smoke.py`, 요약은 `cpppj/obelisk-mode-menus-runtime-20261028.json`이다. 최초 탐색1실패와 최종4수락을 구별한다. 공중3종 생산/생성/우클릭 재현·새 전용 검사·멀티플레이/통신 시험은 하지 않았다.

다음은 일반 건물형 전체와 신전·제단·워크샾의 소유자/레벨/포획 상태별 메뉴다. 저장Spell frame0/재적재frame2의 수명도 남는다. Dig Up은 정상 Edit에서 강제로 노출하는 작업이 아니라 원래 special 진입 경로를 확인할 때의 별도 실제 효과 검증으로 남긴다.
