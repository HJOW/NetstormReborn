# 워크샾·신전의 소유자별 실제 메뉴 — 단위526

2026-10-28, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 현재PC와 같으며 산출물은 변경하지 않았다. 시작 커밋은 `6fd85ceb`이다. 이전 목표 턴은524/525의 실제 구현·검증·커밋을 완료한 진전이었다.

## 원본 조건과 이번 범위

일반 플레이에서 Factory `454c00`은 현재 owner가 로컬 owner와 같은 일반 워크샾에 생산/지식/업그레이드 항목을 붙인다. 공통 Damageable 부모의 회수도 같은 현재 owner를 비교하고, Editor에서는 회수 항목을 붙이지 않는다. 신전 `4dcca0`의 View Netstorm Knowledge 역시 현재 로컬 owner와 같은 신전에만 붙는다. 비영 owner의 Player 하위 메뉴와 About은 공통 Object 부모가 작성한다.

편집 우클릭은519에서 복원한 `4d5290`으로 클릭 대상의 비영 owner에 해당하는 **현재 Player Core**를 선택한다. 따라서 Edit의 owner2 워크샾/신전에도 소유 전용 메뉴가 붙는다. 이를 일반 플레이의 적 소유 메뉴와 같은 조건으로 판단하면 안 된다. Set Owner는 실제 가상74에 전달되며 신전은 `RawCampaignOwnership`→같은 `RawVortexOwner::Set`의 현재 Player/섬/지역 처리와 생산 갱신을 통과한다. 메뉴용 owner BYTE나 Player 사본을 만들지 않았다.

이번에는 원래 BreakingThrough의 윈드 워크샾·윈드 신전에서 Edit 소유자를 바꾸고, 일반 플레이에서는 원래 레인(owner1)/윈드(owner2) 두 진영의 워크샾·신전을 각각 선택했다. 일반 플레이의 owner/HP/프레임을 조작하거나 새 객체를 생성하지 않았다. CHEAT 표시 기준은525와 같은 **Edit 편집 전용/항목별 원본 관문 유지**다.

## 실제 결과

- **Edit 4실행/종료0:** 두 종류×1280×720·1024×768에서 원래 CHEAT Set Owner 대화의 Cancel·Human Player·Computer #1·No Owner를 클릭했다. 같은 SID/종류/위치와 실제 선택을 유지하고 owner `2→2→1→2→0`, 현재 Core `2→2→1→2→2`를 확인했다. 비영 owner에서는 전용 생산/지식 메뉴가 갱신되고 회수는 숨겨진다. 중립은 Destroy/About/CHEAT Set Owner의3항목으로 돌아가 Player/소유 전용 항목이 없다.
- **일반 8실행/종료0:** 네 종류×두 화면비에서 실제 로컬 owner1/Editor0/missionPost1을 확인했다. 자기 워크샾은 생산·지식·Upgrade·회수가 있고, 다른 owner의 워크샾은 About/Player만 있다. 신전의 지식/회수도 같은 소유자 조건을 따르며 일반 메뉴에는 CHEAT 문구가 없다. 실제 바깥 클릭으로 반납하고 대상의 종류/owner/위치를 보존했다.
- 일반 워크샾의 실제 레벨은 자기0(Level I), 다른 owner2(Level III)다. Upgrade 표시를 이 현재 레벨/소유자 조건과 대조했다. 자기 SP0에서 비용1000의 붉은 표시도 캡처에서 확인했다. **모든 레벨/자금·실제 Upgrade 실행을 검증한 것은 아니다.**
- 기존 독립 원문 fixture를 재생하는 ObjectOwnerCommand2·PlayerCoreData3·VortexOwner4·FactoryMenu2·ObjectMenuInfo2, **13고유/실패0**이다. 최종 실행은 각 프로세스의 종료0까지 기록했다. 앞선 같은13개 관측을 더해26개로 세지 않는다. 실제 등록은3,357개이며 전체 실행은 아니다.

제품/검사/기존fixture의 전체 소스 해시와 두 실행파일은524 freeze와 동일하고 빌드는0회다. 게임SHA `ab2862d7e56fb3751d0a2aab270404f9e7b2da03d94e16abc376ed1d9e6412af`, 검사SHA `266562c4bad85c2c5fc22fe015f4956c292d5a1fcbbc46200ba4b59ebf0af57b`다. 원본/CD1,819파일과AGENTS.md도같은보호기준선과일치한다. 진행중게임/빌드/검사는없다.

자기 워크샾1280×720·다른 소유자 워크샾1024×768·중립 변경 뒤 신전1280×720 캡처를 직접 확인했다. 섬/지역 소유권 전체의 원본 픽셀·전체 월드 대조, 포획/은신/마비 등 모든 상태나 일반 건물형 전체의 완료 증거로 확대하지 않는다. 공중 공격3종의 전용 생산/생성/우클릭 재현·검사와 네트워크/멀티플레이 시험, 자연 완주·전체34경로·전체CTest는 실행하지 않았다.

## 초기 실패와 관측 열 정정

최초 입력은 DissolvedAlliance 전용 사본이어서 BreakingThrough 파일이 없었다(종료1). 다음은 owner1만 찾는 `world:` 진단 도구로 owner2를 찾으려 했다(종료1). 원래 몸체의 칸을 실제 클릭하도록 고쳤다. 세 번째는 게임이 모든 소유자 변경을 수행하고 종료0했지만, raw 메뉴 라벨에 화면 화살표 `>`도 포함된다고 가정한 검사만 실패했다. 화살표는 렌더러 표시이며 raw 라벨은 별도다. 이 세 관측을 최종12수락에 넣지 않았다. 전체15게임은13종료0/2종료1이다.

오프라인 Upgrade 조건을 보강할 때 `raw_object`의 frame열을 flags로 읽은 혼동도 정정했다. 실제 출력기 `RawSurfaceWorld::Dump`와 Level III 캡처를 대조해 **raw_asset_fields의 첫 값이 raw[11]**임을 확인했다. 최종 `verify`는 이 상위3비트에서 레벨을 읽고 원래8개 보고서를 다시 대조한다. GUI를 반복하거나 기존 원시 보고서를 덮어쓰지 않았다.

원시·최초 실패·집중 로그는 `extracted/cpp-building-owner-menus-526/`, 요약은 `cpppj/building-owner-menus-runtime-20261028.json`이다. `tools/cpp_object_owner_smoke.py`에 두 대상을 추가하고 `tools/cpp_building_owner_menus_smoke.py`로 일반 모드/저장 보고서 검증을 제공한다. 다음은 일반 건물형 전체와 제단의 상태별 메뉴, 워크샾 생산/지식·자금 부족/업그레이드/회수의 실제 실행, 저장Spell 수명이다.
