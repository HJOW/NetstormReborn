# GetTechnology·ChangeMoney의 대본 연결 — 단위683

2026-11-01, **HJOW-Athlon**. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23** 유지다. 디컴파일/원본/CD 파일을 바꾸지 않았다. [보고서](../../cpppj/get-technology-input-20261101.json)를 따른다.

## 연결한 경계

원본 이름표 `543080`은 GetTechnology 문자열→전역542970→명령 **0200009f**, `542e90`은 ChangeMoney→54287c→**02000061**이다. 현재 대본 조회에는 두 이름이 없었고 GetTechnology의 일반 입력도 연결되지 않았다. 예를 들어 `TpVchoice.english`의 20000 Storm Crystal 지급 태그는 이미 있는 SP 가산 몸체까지 도달하지 못했다. 공식34경로의 모든 태그 실행 여부를 이 정적 예시로 주장하지 않는다.

GetTechnology를 공통 기술 이름 조회에 넣어 HTML/Help/일반 대본의 같은 큐를 사용하도록 했다. 현재 일반 입력은 `DispatchGetTechnologyInput`을 거쳐 **이미 복원한 Session.LearnNewTechnology→AcquireNewTechnology**에 전달한다. 원문 일반 입력의 현재 두 번째 DWORD를 그대로 사용하고, Editor/미션/소유자나 타입의 BYTE/WORD 절삭 관문을 추가하지 않는다. release에서는 하위 Session/효과를 요구하지 않고 소비한다. 호스트의 별도 UI 재생성 없이 기존 NewTech 부모의 표시 효과를 사용한다. 실제 현재 Session이 필요한 press의 누락은 명시 오류로 유지한다.

기존492a30 부모의 버리는 첫 기술 조회·현재 부모 재조회·중복 확인/조건부 생성→snapshot→현재 타입 이름→두 Config 쓰기→NewTech 안내 순서를 재사용한다. 기술 지급·저장·안내 몸체를 별도로 만들지 않았다. ChangeMoney도 기존 signed SP 가산/모드 관문을 재사용한다. 원문에 없는 GetTechnolgy 같은 오자를 별칭으로 추가하지 않았다.

## 원문과 집중 검증

제품 변경 전에 **실제 일반 입력004d6e61..004da477와494ea0 thunk**를 실행해 48입력/144관측을 동결했다. 492a30은 명시 외부 부모 경계다. 타입 DWORD의 고위 WORD/FFFFFFFF, 무관한 첫 인자, 입력 플래그/release, Editor/미션 모드를 포함한다. 두 x87 정밀도·재관측 차이0이며 fixture SHA는 `251d45e816ba20c2a02e5510be34d6910b679115b2f16a69f0228d1a9388bc51`다. 범위 밖 타입을 실제 세계에 지급한 검사가 아니다.

Release 제품/전용 집중 빌드는 종료0·경고/오류0이고 `netstorm_get_technology_tests`는 **10개(새2·기존8), 실패0**이다. 새 이름/실제 parser·큐와 원문48행을 대조했다. 기존 기술 획득 부모45사례(현재 부모·중복·부분 중단/Config), SP144행, 일반 명령610행,682의 PlaySound48행/이름4개를 기존 검사로 확인했다. 기존 fixture/기대값은 수정하지 않았다. 주 검사EXE/전체 C++는 빌드·실행하지 않았다.

**새 게임/GUI 실행0**이다. 최신 실제 음향 이름표 확인은 이전682제품의 별도2회이며, 이번683제품의 GetTechnology 지급/안내 GUI로 합산하지 않는다. 원시는 `extracted/cpp-get-technology-683/`와 `extracted/cpp-get-technology-683-native-v1/`다. 새 fixture1개를 더해 현재 fixture는784개이고 원본/AGENTS와 기존 fixture는 그대로다.

제품 SHA는 `334258ee94c9479683f8819e2586f39c3e1dbdd77ab519696454a0f9acbcc738`다. 마지막 전체34/ZIP은645의644제품으로 유지한다. 새 포획/희생·특정 적 공격·승리·공중3종 행동/생산·MP/실제 통신 검사는0이다. CD492a30의 기존 대응 후보40e300은 메타데이터(string/strong0.889) 수준이며 이번 CD 본문/호출/GUI 대조를 하지 않았다.

나머지 시나리오 태그의 AddRandom·DestroyAt·InflictDamage·StartSmoke·Lightning·SendBucks 등은 실제 원문 이름/소비자를 구분하여 계속 확인한다. 태그처럼 보이는 본문·주석·오자나 MP 분기를 일반 싱글플레이 미구현으로 단정하지 않는다. 남은 Edit/수명/호환성과 후순위3~7차는 계속 진행한다.
