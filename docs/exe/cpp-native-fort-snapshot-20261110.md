# Native Fort 저장과 진입 준비 연결 — 작업 1266

2026-11-10 / HJOW-Athlon. 기존 일시중지 인계의 [Snapshot 후보](handoffs/cpp-pause-20261110/snapshot/HANDOFF.md) 네 파일을 현재 제품에 편입했다. 마지막 Client 디컴파일은 2026-11-03, R 전용은 2026-11-07이며 둘 다 HJOW-Athlon에서 수행했다. 이번에 새 디컴파일이나 원본 실행은 하지 않았다.

Fort와 Battle의 서로 다른 원문 저장 부모는 같은 Client의 장기 Template, Pool, Player, Deck, 난수와 사용자 작업 파일 장치를 사용한다. `SnapshotNativeCurrent`는 manager 부재 시 먼저 돌아가는 004bfb50과 독립된 두 모드를 확인하는 004bfac0을 구분한다. 현재 기술은 Player +9c를 사용하며 상세 근거와 51개 원문 함수·진단 장부는 위 후보 인계를 따른다. 준비 후보의 구문 검사와 이번 제품 링크·실제 검증은 구별한다.

일반 입력의 SaveGoMain과 Test Battle 저장, 현재 Fort 이름 변경, 편집 복귀 및 SetFortType의 State/Notify, State 정리의 Fort/Battle 저장을 Native 장치에 연결했다. JoinTestBattle은 signed 지역 인자를 같은 장기 자료로 준비하고 저장한 뒤 기존 1bh 본문에 넘긴다. 기존 typed/CD 경로는 유지하며 Notify만 요청한 곳에서 전체 저장을 추가하지 않는다.

Release x64 `NetstormCpp`·`NetstormRoot` 빌드와 관련 집중 검사 대상 빌드가 성공했다. 새 라이브러리에 연결한 비MP 집중 검사 6건(일반 저장·읽기 전용·파일 공유 실패·TEST01 편집 체크포인트·Fort 이름 2건)이 통과했다. 최종 클론의 격리된 데이터 사본에서 일반 Edit → TEST01 → 저장하고 메인 메뉴 복귀를 두 번 수행했다. 정상 종료 코드 0, 입력 데이터 사본 불변, 사용자 폴더의 `CustomMaps/d/title950/TEST01.fort` 생성 및 재진입을 확인했다. 저장 파일 SHA-256은 `fbaabe8a9590fb7ef8fec5f435681e196ad27acb24d4d038bb36a66bb0f2b12e`다. 관측 원시는 `extracted/cpp-resume-1266/live-editor-save-final/`에 있다. AGENTS·원본/CD·fixture의 기존 보호 기준 2,808파일도 불변이다.

이 검사는 Session이 있는 일반 편집기의 저장 경로를 확인한다. Session이 없는 전체 저장·Battle 저장·Join/Test Battle 및 MP/패킷/통신은 실행하지 않았다. 읽기 전용 설치의 실제 GUI 저장은 이번에 확인하지 않았다. `FortEditEffects`의 섬 개수 조회와 `RequestFortEditState`의 Battle 판정, 일반 Descend 등 다른 호출자·수명은 여전히 감사 대상이다. 최초 후처리 나머지·AI·전체 process/caller와 큰 NoWorld, M6 및 전체 목표도 미완료다.
