# 사제·캐리어 권한의 현재 Client 주소 연결 — 단위445

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 수행일은 **2026-10-23**이다. 시작 제품은 `76e3c453`의 443·444 통합판이다. 기존 10.78·10.72 정밀 C와 저장된 독립 Native fixture를 사용했다. 새 디컴파일·원본 게임·Native 실행은 하지 않았다.

## 문제와 변경

`RawCampaignSession`은 사제 실행기를 만들 때 `SetAuthority(core.processState.Boss())`로 네 상태에 권한 값을 한 번 복사했다. Client의 실제 boss가 나중에 바뀌면 HP·낙하·캐리어 후처리·내용물 삭제가 이전 값으로 동작할 수 있었다.

`PriestHitPointMode`, `PriestFallState`, `CarrierPostPopState`, `ContainedFinderState`에 현재 boss를 빌리는 포인터와 조회 함수를 추가했다. `RawPriestWorldBindings::BindAuthority`는 같은 `RawAuthorityState::boss` 주소를 네 소비자에 연결한다. 실제 Session은 수명이 보장된 `gameplayGlobals->Authority()`를 빌린다. server와 boss를 혼동하거나, bool 반환값의 임시 객체 주소를 빌리지 않는다.

공통 Carrier PostPop과 생산·Damageable·AI 초기 갱신이 사용하는 `contentFinder`도 같은 Client boss 주소에 연결했다. 사제 prefix와 Damageable 하위가 서로 다른 권한 값을 읽지 않도록 실제 생성 부모에서 함께 조립한다.

HP·중립 회복·낙하·Carrier PostPop/PreDestroy·내용물 dead 진단과 같은 상태를 공유하는 Damageable PreDestroy가 원문 조회 위치에서 현재값을 읽는다. 하위 효과 뒤의 권한 조회를 진입 시점으로 앞당기지 않는다. 독립 fixture의 기존 값형 입력은 포인터가 없을 때만 사용한다. 이미 실제 주소를 연결한 실행기의 값형 setter와 다른 owner로의 재바인딩은 쓰기 전에 거부하며, 같은 owner 재바인딩은 현재값을 초기화하지 않는다.

## 원본 근거

| 몸체 | 10.78 | 10.72 |
| --- | --- | --- |
| HP setter | 004942b0 | 0040ca40 |
| 낙하 시작 | 004941f0 | 0040c880 |
| Carrier PostPop | 00427720 | 004e6360 |
| Carrier PreDestroy | 00426890 | 004e43b0 |
| Contained Begin | 004b2270 | 004eb8d0 |
| Damageable PreDestroy | 0044b4b0 | 004615e0 |

위 열두 원문을 기존 정밀 C에서 별도 추출해 SHA를 기록했다(`extracted/cpp-priest-authority-445-static/`). 10.78의 여섯 몸체와 10.72의 다섯 몸체는 각각 현재 `DAT_00540bc4`/`DAT_00540a2c`를 읽는다. **10.72 Contained Begin에는 패치판의 dead 부모 권한 진단이 없다.** 기존 판본 단락 평가를 유지하고 두 판본의 차이도 신규 검사에 포함했다. Native fixture 여섯 파일의 SHA를 별도로 보존했다.

## 검사와 경계

신규 검사 여섯 개는 복사한 HP 모드의 현재 권한, 실제 프로세스 예약 효과 뒤의 낙하 권한, 연속 Carrier Pop과 최초 flag, Damageable 효과 뒤의 Carrier 삭제 및 dead 조회, 실제 네 소비자의 같은 주소·외부 owner 거부, 실제 Client/Session에서 Prepare 없이 바뀌는 HP 관문을 확인한다. 기존 Native fixture의 기대값은 수정하지 않았다. 두 판본을 다루는 다섯 검사는 각각 두 판본을 실행하고, 실제 Client/Session 검사는 목표 10.78이다.

Source1의 2,027파일을 동결했다. 첫 빌드는1,021.175초·종료0·경고/오류0이었다. 정적 검토에서 찾은 공통 Carrier/생산 finder 연결과 한국어 복원을 포함한 Source2 빌드는120.499초·종료0·경고/오류0이었다.

첫 집중 검사는285개 중 실제 Session 신규 검사1개가 실패했다. 원본 자원을 쓰는 Session에도 합성 장면의 최대HP200을 가정해 비권한 경계를100으로 기대한 검사 오류다. **원본 TAFF의 `\d\priest.type`은 `maxHitPoints=100`**이며 원문004942b0의 비권한 절반 경계는50이다. 아카이브·엔트리 SHA와 원시 타입을 별도 보존했다(엔트리 SHA`67e3566b…`). 신규 실제 Session 사례만50으로 교정하고 원본 타입/초기HP100 단언을 추가했다. 합성 최대HP200 사례의100 기대값, 기존 Native fixture와 제품 코드는 바꾸지 않았다. Source3 빌드는51.434초·종료0·경고/오류0이다. 최초 실패는`extracted/cpp-priest-authority-445-focused1/`에 보존했다.

PowerShell 기본 파이프 인코딩 때문에 새 검사 주석과 이전444 fixture 두 주석, 443·444 보고서의 설명 일부가 물음표로 저장된 문제도 발견했다. UTF-8 입력을 확인하고 한국어를 복원했다. 이전 보고서는 설명 필드만 교정하고 실행 결과·이전 소스/실행 파일 SHA·원시 로그·증거 SHA를 유지했다. 최종 C++ 소스와 주석 복원은 Source3에 함께 동결했다.

최종 Source3의 **집중515고유/684관측·실패0**, 신규6개 전부가 통과했다. 전체 Campaign 계열399개도95.2592초·종료0으로 통과했다. 실제 등록 이름과 출력 집합을 대조해 필터별 누락·미등록 이름이 없음을 확인했다. 기존9파일의265사례를 모두 포함한다. 보호1,821파일·동결2,027소스·실행 파일 네 개의 SHA와 Native fixture6개의 SHA를 확인했다.

변경된 검사6파일의 기존242사례는 주석 복원을 제외한 코드·기대값이 모두 보존됐다. 비교 원시 결과는`extracted/cpp-priest-authority-445-existing-tests-audit.json`에 있다.

최종 게임 SHA는 **`db646aed858bbd9f935fd86651f6b8bfceef67643492c42c07c323d10e99ba90`**이며 [실행 보고서](../../cpppj/priest-authority-runtime-20261026.json)에 기록했다. 이번 단위에서 게임·GUI·전체 CTest를 다시 실행하지 않았다. 마지막 전체 CTest3,027개 및 메뉴 GUI·자연Early2→Tutorial3 증거는 이전443·444 통합 제품`423d0911…`의 기록이다. 최신 단위의 집중 통과와 구별한다.

이번은 사제 네 상태와 공통 Carrier/생산 finder의 현재 권한 연결이다. `priestDamageState`, `damageablePostState`, 건설·내용물 수명·빌더·간헐천 처리의 Prepare 값형 권한 사본은 후속이다. 회복 허용 Player 필드의 값형 준비도 이번 변경과 구별한다. 전체 Native State1 cleanup/prefix·반납 coordinator·Edit 제품 진입과 전체 미션 자연 완주·원본 완전 동일성은 계속 미완료다. 전체 CTest와 긴 GUI 회귀는 사용자 지침대로 후속 3~5개 단위 또는 기능 묶음의 같은 최종 제품에서 통합한다.
