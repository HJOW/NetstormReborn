# Dude 발자국0의 일반 후보와 경로 실패 사각형

2026-10-21 단위245. 시작 HEAD는 `f285cfec`, 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. 기존 정밀 자료와 보호PE를 읽었으며 새 디컴파일·원본/CD·AGENTS 변경은 없다. 다른 세션의 dotnetpj 변경은 보존하고 이번 작업에 넣지 않는다. 이전 목표 턴은 단위244의 연결·기록·커밋을 완료한 진행 턴이다.

## 문제와 원본

`RawPathStart::Footprint`는 이전134/158 입력의 foot≥1을 요구했다. Dude70/foot0은 경로 Count0에서 예외가 나므로 겹침 탐색과 재시도에 이르지 못했다. 공통 `RawSquidFinder::Intersects`도 foot0을 거부해, 실제 Dude가 주변 후보에 존재하면 예외가 났다.

10.78 `4ade70→49ae80`은 부모의 현재 `(x,y)`로 사각형을 시작하고 `x−(footX−1),y−(footY−1)`를40e880/40e8f0로 min/max 확장한다. 확장 입력은41d0d0으로1..255로 자른다. 원래 점 전체가40e800의0초과/256미만 검사에 실패하면 두 끝점 모두 확장 입력으로 대체된다. 한 축이0이어도 다른 축까지 함께 처리하므로 좌표별 min/max만 추가해서는 맞지 않는다. 41def0은 왼쪽/위 절삭과 오른쪽/아래 거의 올림 후 실제 finder를 시작한다. 일반 finder 후보의0폭/0높이는 원본 OR 교차식에서 유효하다. CD10.72 finder4eae20/4eafe0에서도 이를 확인했다. 실패 상위 Start의 이번 제품 범위는10.78이다.

## 제품 전 독립 관측

`tools/decomp_dude_footprint.py`는 기존 보호 이미지/함수 범위와 입력 로더를 재사용하고, 새 원문으로 다음을 수집했다.

- 일반 탐색 **480입력**: 판본당240개,0/양수 발자국·부류 시작·확장 높이·표면/일반 단계·접촉/밖 후보를 교차했다. 두 정밀도의 Begin/Next 외부 호출 **2080정상 반환**과 전체15DWORD cursor가 일치한다. 후보와 기본 필터·버킷·CRT는 실제 원본이다.
- Dude 실패 **640입력**, 두 정밀도 **1280정상 반환·20267032명령**: 실제24 생성자/Init·사각형/minmax·일반 finder·겹침 요청·매장 변경 후 재조회·기존 재시도 비트와 재귀를 관측했다. 좌표0과255.25도 포함한다. 시작 표면 부재에서 Init가 자연 반환하는 입력이므로 **BFS 내부 실패 관측은 아니다.** new·구 경로 lookup·Dispose·겹침 효과는 명시 경계이며 GP/retN/canary/x87 제어어/TOP·코드/쓰기 범위를 검사했다. 게임/OS/창/C++제품 검사는 실행하지 않았다.
- 최초 탐색480/실패160 자료를 `extracted/dude-footprint-20261021-first-capture/`에 보존한 뒤, 제품 작성 전에 좌표 경계를 확장했다. 탐색 fixture 전체 바이트와 기존 실패160입력의 두 정밀도 출력은 그대로다. 현재640행에서 최초160행을 재구성한 전체 바이트 SHA도 기록/감사해 기대값 변경을 숨기지 않는다.

새 근거는 `cpppj/dude-footprint-native.json`, `dude-footprint-find-x86.tsv`, `dude-footprint-fail-x86.tsv`다. 기존 RawFinder1752/PathStart512 원문이나 기대값은 덮어쓰지 않는다.

## 변경과 검증 범위

`RawPathStart::Footprint`는0크기를 허용하고 원본의 점 전체 유효성·min/max 확장·정수 변환을 적용한다. `RawSquidFinder::Intersects`는0크기 후보를 같은 기존 OR 교차식에 통과시킨다. 음수 크기·확장 높이 오버플로 등 기존 손상 입력 보호는 유지한다. 탐색이나 이동 결과를 별도 모형으로 만들지 않는다.

새 검사2개를 작성·컴파일했다. `DudeFootprint_NativeFinder480`은 두 판본의 전체 cursor와 풀 읽기 전용성을, `DudeFootprint_NativeFailedStart640`은 같은 실제 타입 선택/24생성/빈 표면 Init/실패 탐색/겹침 효과·매장 변경/재귀·프레임/재시도 비트 보존을 원문과 대조하도록 작성했다. native Dispose는 명시 경계이므로 C++ 미부착 unique_ptr 소멸의 실행 대조로 세지 않는다. 실패 사각형의 native 인자도 원문에 보존하지만 제품의 private area 직접 대조를 주장하지 않으며 외부 생성/겹침/재시도 효과를 비교한다.

**최종 전체 Release 경고0·오류0·종료0**이다. 첫 빌드도 종료0이었고, 검사에서 기존 실제 타입 선택 몸체를 재사용하도록 보완한 뒤 최종 `extracted/cpp-dude-footprint-20261021-build2.log`를 남겼다. 사용자 지시대로 **C++검사/CTest/게임/전체 회귀는 실행하지 않았다.** 빌드나 원본 관측을 제품 검사 통과·자연 대피/미션 완주로 표시하지 않는다.

현재 소스 핀만 갱신하며 RawSquidFinder의 같은 시작 코드에 대한 LF/CRLF 현재 핀을 구분한다. 소스의 LF 속성을 명시하고 기존 `files`·원본·fixture·과거 실행/바이너리·이력은 보존한다. 전체 JSON 역변환을 시작 커밋과 대조한다. `cpppj/dude-footprint-session.json`, `tools/decomp_dude_footprint.py --verify`, `tools/cpp_dude_footprint_audit.py --verify`를 따른다.

다음은 Dude Regular450fa0/451190/451210/451270/451340과 목록 초기화다. 앞 단위에서 찾은4fc9b0→44fb20(capacity30)/객체55a4ec·registry472880을 독립 관측한 뒤 현재 capacity0을 복원해야 한다. 자연 생성·대피·지형 Devastation/포획/Drop/제단/희생·지정 미션·F8 Guide/키·전략AI·전체 동일성 검증은 남는다. 전체 goal은 **active/미완료**다. 인수인계를 갱신하고 로컬 커밋하며 push하지 않는다.
