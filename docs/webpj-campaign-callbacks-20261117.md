# 외곽 Session callback 대상 (WB04-E637)

기준선 `225553df`의 `RawCampaignSession.cpp` 5650..5687·5761..5772·
6080..6099, 합계 70줄의 typed 분기를 옮겼다. 해당 `.cpp/.h`와
`RawWorldGeyser`·`RawWorldGeyserBounds`는 기준선 뒤 변경 0이다.
`git show 225553df:...`의 함수와 `CampaignSessionTests`의 현재 기술
수명·로컬 간헐천 현재 입력/실제 AI 등록 시험을 함께 검토했다.

기술 획득은 같은 Player의 현재 기술 부모와 실제 Carrier 내용물 조회를
사용한다. 비영 SID 대신 후보 +18의 **종류 WORD**를 반환한다. 종류 0은
생성을 생략하는 값이 아니다. 새 내용물 생성 뒤 Battle/Fort 두 독립 if가
각각 현재 모드와 owner를 읽고, 이후 현재 TYPE 이름·그림·안내 서비스를
호출한다. 생성 시 `newTechnology` 함수 표를 값형으로 복사하되 함수가
참조하는 실제 대상을 복제하지 않는다.

간헐천과 너겟의 단일 배치 몸체는 같은 World/Pool·섬·청크·HP·현재
MayPlace를 빌린다. 세계 범위의 현재 모드·세계 크기·공유 여백을 살아 있는
참조로 읽고, 지역 장식 난수와 게임 난수를 합치지 않는다. 보충 너겟은
현재 `readBoss()`를 사용한다. 정적 검토 중 지역 `boss` 필드 대신 실제
공유 권한 getter를 선택하도록 교정했다. Native 장기 Client의 너겟 분기는
4D-8에 남긴다. 로그는 브라우저 console로 전달한다.

현재 Session 부모는 기준선부터 NB04/NB29 수정 스위치를 켠다. 기존 웹
부품은 이를 10단계로 미뤘으므로 현재 결선의 선행으로 계산 부분을 함께
재반영했다. `shuffleEvenCells` 생략 시 원문 1..9 순서, 참일 때는 별도
시간 난수의 Fisher-Yates 순열을 쓴다. `assumedPlayers=0`·`monotone=false`
기본값은 원문 예산을 유지한다. 명시 스커미시 인원은 비전투 모드에서
최대 8명, 수정 예산은 7/8명에서 42/48이다. 실제 스커미시 생성과 UI는
10단계에 남아 있다. 기존 원본 관측 820+354입력은 그대로 통과했다.

신규 6검사는 첫 실행에 모두 통과했다. cpppj `KnownBug_NB04`의 200시각
순열·마지막 구역 다양성과 배치 난수 불변, `KnownBug_NB29`의 기존/수정
공식·인원 상한·실제 전투 Player/숨김 우선순위를 확인한다. 기술/로컬
callback 3검사는 최소 owner 계약이며 종류 WORD·두 번째 owner 재조회·
늦은 안내 누락의 부분 효과·PrepareConstruction 이후 현재 처리기 선택을
검사한다. 나머지 1검사는 실제 owner의 서비스 사본과 Load 전/반납 뒤
거부·Pool/Player/난수 불변이다. 실제 owner의 loaded는 false를 유지한다.

보조 편집 명령 한 번은 작업 디렉터리를 webpj로 지정한 상태에서 저장소
루트 상대 경로를 사용해 서비스 표 수정에 실패했다. 루트에서 명시 경로로
다시 적용했고 실제 사본 검사가 통과했다. 이 오류로 보호 자료나 다른
파일을 변경하지 않았다.

현재 함수들은 **외곽 callback의 대상 본문**이다. 게시 생성자는 다음
4D-1d이며 실제 Load가 선행인 정상 미션/기술 수명·AI 간헐천 등록 통합
검사는 4D-2h·4D-10에서 재대조한다. 따라서 4D-1c 체크는 아직 닫지 않는다.
임시 월드나 loaded=true로 실제 Session 설치 성공을 만들지 않았다.
기존 fixture 직접 소비 시험에 관련 모듈/범위만 추가했으며 신규 fixture와
직접 소비자 등록은 0이다. 게임 창/브라우저·보호 원본·cpppj·공통 인계·
README·AGENTS.md는 변경하지 않았다.

최종 Node v22.23.3·v26.13.0 전체가 각각 **6,023건 통과**, 실패·취소·
건너뜀 0이다. v24는 사용자 지시대로 생략하고 호환을 가정했다.
대응표 1,160행 오류 0·manifest 823항목(신규 0)·`git diff --check`를
확인했다. 최종 로그는 Git 제외 루트의
`extracted-webpj-e637-node22.log`·`extracted-webpj-e637-node26.log`다.
