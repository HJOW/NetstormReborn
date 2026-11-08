# 장기 Carrier 전처리와 너겟 보충 부모 연결 — 1201

M6-n의 Carrier 전처리와 너겟 보충 계산을 같은 Client 수명에 연결했다. 실제 자산 VT+14가 `00426890`인 다섯 클래스는 `RawSurfaceStorage::CarrierPre()`를 사용하고, Native 사제 바인딩도 같은 객체를 빌린다. 기존 별도 Carrier의 finder 복사본 대신 장기 Damageable과 같은 `ClientGameplayGlobals::finder`를 읽는다. 독립 typed 바인딩은 기존 지역 몸체를 유지한다.

`RawCarrierPreDestroy::EnableNativeBody()`는 실제10.78 경로에서 typed 사전 검사를 생략한다. 원문처럼 Damageable 전처리를 먼저 실행하고, 효과 뒤의 현재 boss와 내용물 BYTE/WORD 조건을 조회한 뒤 보충한다. `Validate()`가 원문보다 먼저 TYPE/상태/공간을 소비하지 않는다. 독립 typed/CD의 기존 검사 계약은 바꾸지 않았다.

너겟 보충은 `ClientConstructionGlobals::ReplenishNuggets()`가 기존 `RawNuggetReplenish`를 실행한다. 현재 boss 진단/재조회→single/fort/editor 조기 반환→같은 Bounds의 지역0/Canon 계산→현재 여백 확장→실제 배치 순서다. Bounds를 지연 조회하는 생성자를 추가해 첫 모드 관문이 반환하면 건설 본체·SP·Player를 요구하지 않는다. 기존 직접 Bounds 생성자도 같은 몸체를 사용한다.

첫 메뉴 전에 `BindBacking`으로 실제 SP/Player/옵션 owner만 보유하고 값은 초기화하지 않는다. Storage의 보충 장치는 건설 owner를 weak로 빌리며, 선택한 호출 동안만 실제 owner를 보유한다. Storage→건설 owner→Storage의 강한 순환을 만들지 않았다. Native Session의 기존 보충 Caller들도 이 장기 부모로 모았다.

실제 `0040e050`/`PlaceBattleNuggets` 배치 본문은 현재 Session에 남아 있다. 계산 뒤 배치가 선택되는 때만 weak Runtime 대여→실제 외곽 Runtime pin→함수 사본→호출로 연결했다. 진단/Canon 계산 중 현재 Runtime이 바뀌면 배치를 선택하는 위치에서 현재 대여를 읽는다. **배치 전체의 NoWorld 복원이 완료된 것은 아니다.** 단일 배치·완전한 MayPlace·오벨리스크 생성/PostPop의 장기 owner 연결은 계속 남는다.

| 범위 | 현재 근거 |
| --- | --- |
| 자산 Pre/Post 부모 분배 | 83VT 중 **52→57쌍** 연결, 전용 부모는 **31→26쌍** 잔여다. 다섯 Carrier VT의 Post는 기존 Damageable 부모를 사용한다. |
| 부모 삭제 통지 | 기존77개 빈 통지 연결/6개 전용 통지 잔여를 유지한다. |
| 같은 실제 owner | 직접 Carrier·Native 사제의 Carrier·기존 보충 Caller가 같은 장기 finder/건설 상태/모드/Bounds를 사용한다. 다른 AI/운반 Caller의 finder 복사본까지 전수 마감한 것은 아니다. |
| 원문 | Carrier`00426890`·내용물`004ac9f0/004b2270`·보충`00485e40`·범위/Canon`0046f130/004bfd70`·배치 부모`0040e050` 및 CD 대응4개, 총11함수의 C/전체 정밀 범위 PE를 읽었다. 가상 표는 기존 PATCH83/CD68자료와 대조했다. |

새 일반 자산/수명 검사5개는 Native Carrier의 부모 효과 뒤 boss 재조회, 실제 Damageable/공통 Pre 뒤 보충 실패의 부분 효과, 모드 조기 반환 시 본체/Player 미요구, backing 정체성/반납, 실제 배치 대여의 현재 Bounds/여백·Runtime pin/예외 수명을 확인한다. 기존 자산 반납 검사에는 공중3종을 제외한 Carrier 한 경로를 더했다. 기존 양판본/보조판본 Carrier fixture와 너겟 보충 fixture도 그대로 사용한다. 실행 수·빌드·GUI·최초 실패/교정 여부는 [검증 보고서](../../cpppj/carrier-replenish-owner-20261109.json)가 기준이다.

허용 GUI 범위는 Tutorial4의 정상 생산·건설 요청과600프레임 대기, 같은 Client의 Edit TEST01↔메인 메뉴 왕복이다. 사제 포획/희생·특정 적 공격·승리·공중3종·Test Battle·신규MP/패킷/통신 실행 검사는 하지 않는다. 전체34경로 재진입·전체 솔루션 실행·ZIP 갱신은 이번 단위의 증거로 합산하지 않는다.

마지막 정밀 디컴파일은 Client 양판본2026-11-03·R전용2026-11-07, 모두 **HJOW-Athlon**이며 현재 호스트와 같다. 정밀 소스·보호 원본/CD·fixture·AGENTS.md는 수정하지 않았다. 다음은 [현행 인계](../../LEFT_JOBS.md)의 M6-n 잔여 전용26부모/6통지·다른 Form/Regular·큰 AI와 실제 배치 장기 연결이다. M6 전체와 후속 M7/M8/K/S/B는 미완료다.
