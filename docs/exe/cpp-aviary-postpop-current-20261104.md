# 공중 생산 시설의 선택 후처리와 중복 진단 연결

단위855 정적/코드 연결 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전854는 `b9831cc4`이며 현재제품은855/11.01이다. 사용자 결정대로 이 세 시설/공중3종은 정적 분석·코드 작업만 하고 새 전용 실행/생성/생산/입력/행동 검사를 추가하지 않는다.

| 원문 | 최초 생성 관문 | 중복 진단(Flyer.cpp) | 확보/inner | 직접 부모 |
| --- | --- | --- | --- | --- |
| PATCH0045ec50 훨리베이스 | 현재 server·flags1 | `!findProcess( getSid(), sunAviaryProcessType, 0 )`,1e2 | 48BYTE/+0x20 |0044bf80|
| PATCH0045ed10 맨오워 풀 | 현재 server·편집 중 friendlyFire off 아님·flags1 | `!findProcess( getSid(), rainAviaryProcessType, 0 )`,257 |48BYTE/+0x20|0044bf80|
| PATCH0045edf0 데빌메이커 | flags1·편집 중 friendlyFire off 아님·현재 server | `!findProcess( getSid(), windAviaryProcessType, 0 )`,275 |80BYTE/+0x38|0044bf80|

기존 세 하위 몸체는 위 관문을 한 번 선택한 뒤 중복 조회/진단·할당·현재 server/시각을 쓰는 생성자·현재 processType의 inner 부착·직접 Damageable 순서를 유지한다. 중복 진단에서 복귀한 뒤 server/모드 관문을 새로 검사하지 않는다. 생성자/부착의 현재 전역은 장기 참조로 빌린다. 원문 NULL 할당도 NULL inner 부착을 시도하며 실제 Host의 bad_alloc은 해당 무효 원문 경계로 남긴다. 성공을 합성하지 않는다.

RawAssetPostPop의 상위 실제 가상 진입에서 이 세 시설 앞 일괄 TYPE/공통 Validate를 제거하고 도달한 전체 몸체만 요구한다. 알려진 PATCH20부류의 실제 실행 분배는 이제 전수 사전 검사를 추가하지 않는다. 명시 Validate/Activate와 CD의 기존 사전 계약은 유지한다. Session은 선택한 실제 몸체가 아직 미설치이면 명시 실패하고, 지금까지 SessionMissing이던 중복 진단 세 줄을 실제 현재 Client 서비스에 연결한다. 서비스가 정상 복귀하면 기존 하위의 생성자/부착과 직접 부모를 계속하며 현재 callable 사본을 호출 동안 유지한다. 서비스 API는 원문 식을 받으며 파일/행은 위 표와 소스 분기에 기록된다. 다른 Frame/부모 머리/상태 진단은 여전히 미복원으로 남긴다.

CD 대응0049b420/0049b680/0049b810의 중복 진단은17f/1b8/1d6이고 하위 부착은0048f590/직접 부모004621c0이다. Sun/Rain은48BYTE/inner+0x20이며 Wind는72BYTE/inner+0x38로 PATCH의80BYTE와 다르다. 이 정적 비교를 CD 시설 구현/동작 검증으로 확대하지 않는다.

첫2제품 소스와 기존 하위7소스 SHA/정밀 원문6개를 Git 제외 `extracted/cpp-resume-855/`에 보존했다. 새 Native 관측/fixture·새 전용 검사는0이다. 기존 비공중 필터161개/28묶음과 제품/두 집중 대상 빌드를 실행했다. Release 제품/두 집중 대상1회는 종료0·컴파일 경고/오류0이며 기존 비공중161개는 첫 실행 실패0이다. 핸들54815 종료/자식 해제·116.6290626초 소요와 첫2소스/기존 하위7소스 불변을 확인했다. 보호2761개/fixture941개는 모두 불변이며 fixture 입력/기대값 변경0이다. 전체 공중 행동/픽셀·동일성/전체 Pop 검증 완료가 아니며 checking12·완료부모36/부분3은 유지한다. 다른 실제 가상20/부모·TYPE/모드 전역·객체 메뉴/커서·Edit·State/파일/군도·Windows·무검사MP와4~8차 잔여는 계속 진행한다. 실제 게임·Frame/Process 틱·공격/포획/희생/승리·진단 UI·신규MP/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외한다.

현재 제품 EXE SHA-256은 `7bfa793e40d37f69c4fec39f22be9abe01fb1c9d1f02b6592cc2b3775d5ccb4f`다. [검증 JSON](../../cpppj/aviary-postpop-current-20261104.json)에 정적 조건/판본 차이·소스/원문 SHA와 기존 회귀의 범위를 보존했다. 공중 전용 동작을 검증한 것으로 합산하지 않는다. 단위 인계/로컬 커밋 뒤856으로 이어가며 목표는 active다.
