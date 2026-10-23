# 전체 Objects와 실제 동적 Temp 저장 통합 — 단위377

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 보호 원본/정밀 원문과 AGENTS는 변경하지 않았다. 이전 단위376은 `229a2c10`이며 이번에는 기존 Storage를 동적 노드 수명으로 확장하고 전체 Objects 저장 부모를 실제 Session에 연결했다.

## 실제 변경과 원본 경계

- [전체 Objects 부모](cpp-fort-objects-parent-20261023.md)의004be430을 `RawFortObjectsSave`로 등록했다. 현재 manager 재조회, version2 쓰기, Terr00~19마다 Find/필요한 Create/새 Temp/version BYTE/영역 저장/Select Temp/조건부 Rename, 마지막 현재 fort 값에 따른 Chaff 생성·쓰기를 보존한다.
- [Terr/Chaff 순회](cpp-fort-region-save-20261023.md)의004be230/004be350을 `RawFortRegionSave`로 등록했다. 최초 height→scope1/2, 양수 height에서만 최초 width, 각 셀 뒤 width 재조회와 행 사이 캐시, signed BYTE 지역 판정 및 Chaff의 실제 지도 조회 효과를 보존한다.
- [CampaignTemplateStorage](cpp-template-dynamic-sections-20261023.md)는 같은 실제60BYTE 노드·generation·버퍼를 Create/Find/Select/Rename에 연결한다. Temp→TerrNN은 원본 Rename을 실행한 뒤 같은 벡터 할당을 move하며 source handle/generation/data 토큰/실제 주소/pos/end/capacity를 유지한다. target의 옛 참조는 거부한다. 표준35개 파일 위치를 노드 handle로 해석하던 기존 검사도 이름 기반 조회로 교정했다.
- `RawCampaignSession::SnapshotFortObjects`는 같은 Player의 templateOwner, 실제 Storage manager/stream, 현재 청크 지도·Island Matches, 기존 `WriteFortObjectChunk`와 실제 Factory/Pool/가상 객체 writer를 사용한다. 부모/영역/단일 객체 writer는 같은 `objectState.version/scope`를 공유한다. 마지막 Chaff 선택은 현재 `core.ownerMode.fort` getter다.

`TemplateWriteStream.section`은 생성 수명 안에서만 의미가 있는 불투명 노드 번호다. 파일의 표준 섹션 위치나 `section-1`로 변환하지 않는다. 다른 섹션을 선택하거나 이름을 바꿔도 같은 source 참조는 유효하며, 같은 이름의 노드를 재생성하거나 manager를 반납하면 옛 참조는 무효다.

## 원문 자료와 제품 검사

전체 Objects 부모의 최초 원문 관측은 **357입력×두 정밀도=714회**(정상666/명시중단48), 1,038,830명령이다. Terr/Chaff 순회는 **930입력×두 정밀도=1,860회**(정상1,820/명시중단40),133,422명령이다. 두 생성 도구의 읽기 전용 `--verify`가 종료0으로 최초 SHA를 확인했다. 부모의714관측 및 순회의930개 저장 행은 새 제품 검사를 통과했다.

Release 제품 빌드와 검사 빌드는 모두 종료0이며 관측 컴파일/링크 경고·오류0이다. 통합 검사 소스의 최종 수정 시각23:29:20보다 실제 obj 컴파일 시각23:31:02가 늦어, 동결된 최신 검사가 빌드에 포함됐음을 확인했다.

- 제품 빌드: `extracted/cpp-fort-objects-20261023-build-game1.log`
- 검사 빌드: `extracted/cpp-fort-objects-20261023-build-tests1.log`
- 제품 SHA256: `4e9d28dd308411fd40ec149203ea060fcb84636dd6d40455751e2ca34c9b3786`
- 검사 SHA256: `c1cff1f9877044388cff8d29e060cae2735bd97d997ac7f6dd4896d67d712f47`
- 관련9필터 **63검사/실패check0**: FortObjectsSave2, FortRegionSave3, CampaignFortObjectsSnapshot1, CampaignTemplates18, DataManager20, FortSnapshotBasics4, FortTerritorySave5, FortObjectSave5, EditorFiles5. 원시는 같은 접두사의 `focused-<필터>.log`다.

전체 CTest는 **종료0/322.57초**(전체322.61초), 내부 **2,442검사/실패check0**를 통과했다. 원시는 `extracted/cpp-fort-objects-20261023-ctest.log`와 `...-ctest-internal.log`다. 빌드 뒤 C++ 변경은 신규 부모/검사 파일의 옛 ‘준비·미등록’ 표기 삭제 주석뿐이며 실행문/인터페이스는 변경하지 않았다.

같은 제품 SHA로 실제 Early6/훈련9/캠페인16 **31/31진입**과 데모 **3/3진입**을 모두 통과했다. 각 원시 `result.json`의 실행SHA가 위 제품과 같고, 두 실행군 모두 `complete=true`/`originalsUnchanged=true`다. 일반 미션의 실제 안내 버튼과 데모의 자연15초 안내 뒤 플레이 phase4/비정지·Guide4/실제 월드·로딩반납을 확인했고, 데모는 실제 데모플래그와 localAI도 확인했다. Fury 훈련의 최종 BMP에서 실제 월드·덱·미니맵도 읽었다.

- `extracted/cpp-fort-objects-20261023-solo-all/results.json` 및 같은 경로의31개 미션 원시/최종BMP
- `extracted/cpp-fort-objects-20261023-demos/results.json` 및 같은 경로의3개 데모 원시/최종BMP

실행 중 허용설정은 영구 `settings-backup`에 보존하고 각 실행군 종료뒤복원했다. 최종 git 비교에도 AGENTS/기존원본·CD/다른프로젝트변경이없다. 이34개는 **안내/플레이 진입 회귀**이며 사용자완주/승패·전체외관/음향/동일성 검사가아니다. 이전 단위의 GUI 결과를 새 빌드 결과로 대신하지 않았다.

## 실제 두 Terr와 Chaff 파일 왕복

`CampaignFortObjectsSnapshot_TwoTerritoriesAndChaffRealRoundtrip`은 보호 `thewarbegins.fort`를 읽어 실제 Session을 로드한다. 원본 두 섬의 신전 소유자는 각각1/2임을 실제 PlayerRecords와 Pool에서 확인한다. 같은 실제 Create/Owner/Pop 경로로 서로 다른 Terr00/Terr01 및 섬이 없는 Chaff에 윈드 배터리3기를 만든다.

같은3기의 현재 raw owner BYTE만 명시 입력으로 `6/7/8`, 다시 `3/4/5`로 바꾼 두 pass를 실행한다. 저장 섹션이나 직렬화 결과를 입력으로 주입하지 않는다. 실제 TypeNames/전체 Objects를 저장한 뒤 `EditorFiles`가 보호 원본/CD 밖의 `cpppj/build/test-data/snapshot-full-objects-*` 작업본에 실제 SaveFile을 수행한다. 파일을 다시 파싱하여 각 영역의 전체 청크 개수, 정확한 청크/위치/타입/현재 owner가 한 번씩 기록됐는지 확인한다. 각 Terr와 Chaff의 원시 섹션은 두 pass 사이 실제로 달라야 한다.

저장 바이트를 별도의 실제 `RawCampaignSession.Load`에 전달하여 Pool의 살아 있는 같은 좌표/타입/owner도 대조한다. SID 재배치 자체를 오류로 보지 않는다. 장부/삭제 깊이0과 보호 원본 읽기 결과 불변도 확인한다.

현재 일반 캠페인 설치의 `core.ownerMode.fort`는false다. 이 검사는 Chaff 저장 분기를 명시적으로 선택하는 RAII 입력으로 해당 bool만 잠시true로 바꾸고 원래 값으로 되돌린다. 실제 generic Editor 설치나 사용자의 Edit 입력을 완료했다고 주장하지 않는다. Territory/Money/Mission/Badges/Deck/State는 원시 섹션이 원본과 같음을 별도로 확인하여 이번 부분 저장의 범위를 보존한다.

## 남은 실제 Editor 연결

Storage DataManager 생성자가 초기화하는005c89c0(`managerGlobal_`)와 Session의 객체 판본(`objectState.version`)은 아직 전체 Client 수명이 물리적으로 통합되지 않았다. 이번 CreateSection은 DataManager 생성자를 호출하지 않으며 Objects 부모/단일 writer는 같은 Session 값을 사용한다. generic Editor/NewFort를 연결하기 전에 manager 생성자의 같은 전역 쓰기까지 통합해야 한다.

전체 snapshot의 Money/Mission/Badges/Notify, 실제 generic Editor 초기 설치, 사용자 작업본 선택/저장·불러오기, 메인 Edit/NewFort/SaveAs/TestBattle/Return 및 언어별 시나리오 왕복은 계속 미완료다. [잔여 감사](cpp-editor-gap-audit-20261023.md)를 따른다. 이번 통합은 전체 Objects와 저장소 수명 기반이며 Early/캠페인 전체 사용자 완주·모든 UI/외관/음향 동일성 또는 전체 목표 완료 판정이 아니다.
