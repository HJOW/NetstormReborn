# 원본 Path 이동 시작 상위 몸체

작업일은 2026-10-13이며 현재 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. `AGENTS.md`와 두 `LEFT_JOBS`를 읽고, 같은 PC에서 재생성한 `extracted/refined` 및 읽기 전용 `pathstart`·`pathstarthelpers` 내보내기를 사용했다. 원본 게임·GUI·OS 코드를 실행하지 않았고 보호 PE는 읽기만 했다.

`RawPathStart`는 패치 `00489080`/CD `0047d150`의 상위 순서를 복원한다. 목적지 clamp와 사제 스냅, 현재 타입에 따른 파생 경로 선택, boss일 때 기존 같은 종류 경로의 두 대상 저장/삭제, 실제 파생 생성, flags0 Init wrapper, 명령 DWORD와 패치 목적지 필드 쓰기, 유효 경로 검사, 연속 시작 꼭짓점 보정, 실제 Prepare, 같은 Host 부착, 부모의 `+44`/`+40` 통지 순서다.

타입 선택은 기존 `RawPriestWorldBindings`의 몸체를 `client::SelectMovementProcessType` 공유 함수로 노출했다. 기존 바인딩과 PathStart의 `processType` 훅은 이 한 몸체를 재사용하며 O 계층은 client 헤더를 포함하지 않는다. 패치 `00488fe0`은 썬 워커(골렘)133와 empty187을 먼저 wormProcess40으로 처리하며, 윈드 워커137/레인 워커156은 simpleWalkerProcess38, balloon genus는 balloonProcess42, teleportEffect134/bulf135/dude70/priest158/manaBolt105는23/41/24/54/39다. CD `0047d020`은133만 wormProcess40으로 먼저 처리하며 다른 알려지지 않은 타입에서는 assert한다. 현재 mutable 번호를 받는 상태를 사용하므로 타입/genus 우선순위를 상수 switch로 대체하지 않는다. 명칭은 실제 `TypeLoadOrder`/`RiftTypeTable`과 대조했다. 초기 새 O 선택 몸체는 기존 구현의 중복이어서 제거했고 제네레이터/볼텍스/운반 표기도 교정했다.

기존 미등록 `RawPathStart` 소스에서 세 차이를 교정했다. 패치의 기존 경로 시작 좌표 보정은 `start <= destination`에서 0.5를 빼며, `replaceStart`가 쓰는 좌표는 이 보정된 start다. 실패 finder의 오른쪽·아래 경계는 거의 올림이다. `RawPathRuntime`에는 명령 DWORD 세 개/패치 전용 목적지 쓰기와 시각·방향을 보존한 꼭짓점 좌표 쓰기를 추가했다. `tailByte`라는 이름의 인자도 실제 원본은 DWORD 전체를 쓴다.

사제 `+44`는 실제 같은 풀/hash의 finder로 목적지 칸에 있는 **가이저(type122)**를 모두 찾아 마지막 일치 번호를 targetA에 기록한다. 기존 미등록 소스와 작업 지시의 `nuggetType`은 명칭 오류였다. 실제 조회 전역은 패치 `00541240`/CD `0051cb2c`이며 nugget(type154)의 `005412c0`를 조회하지 않는다. `005412cc`의 type157은 noIsland다. 주소·분기는 유지하고 상태 이름을 `geyserType`으로 교정했다. buried 후보는 실제 finder가 제외한다. `+40`은 사건 `0x25c`/`0x25d`(**604/605**)를 각각 처음부터 다시 검색하여 전부 삭제하고, 현재 타입의 defaultFrame을 같은 `SquidFrame`에 지정한다. teleport의 두 부모 통지는 두 판본의 실제 빈 ret 몸체다.

## 독립 대조와 범위

`tools/decomp_pathstart_oracle.py`는 두 보호 PE의 실제 기계어에서 **새 시작512개**와 **종류 선택 회귀718개**를 관찰한다. 시작 입력은 부모134/158, 정상·스냅·시작 표면0·목적지 clamp·void 부모·같은 좌표 목적지·겹침 정리 재귀·기존 guard의8개 장면, boss/기존 경로/꼭짓점 보정/사건 반복 유무를 교차한다. 종류 선택은 패치188/CD171개 모든 타입과 balloon genus 유무를 교차하며 CD의 의도된 assert도 기록한다. 이718개는 이전에 검증한2872개 이동 선택 입력의 정확한 부분집합이므로 신규 관찰 수는 **512개**다.

실제로 실행한 몸체는 상위 Start·종류 선택·파생 ctor·flags0 Init wrapper·실제 표면/Graph 조회·BFS/접합/경로 축약·Prepare/CRT·사제 목적지 가이저 finder·사건 취소 반복이다. SHA 감사는 두 PE, 현재 정밀 몸체, 도구, 소스, fixture를 고정한다. 원본 게임 설정인 x87 53비트 결과를 fixture에 쓰고64비트 결과 차이는 별도로 남긴다. 같은 입력 재실행과 정밀도 반복은 고유 관찰 수에 더하지 않는다.

C++ 재생은 실제 `RawPathSearch`·`RawPathRuntime`을 사용하고, 같은 `SidPool`의 기존 contained 경로 조회/Host 삭제·실제 서버 flags0 부착/`RawPathData` 공유 버퍼 전송·실제 종속 체인·사제 가이저 finder·예약 삭제/프레임 지정을 연결한다. 성공한 경로의 모든 꼭짓점/방향/double 시각과 대상/명령 DWORD, 공유 생성/정리/Init Pop/겹침 효과 인자 및 순서, 현재 부모 프레임, 남은 두 사건 개수/재귀 guard를 원본 관찰과 대조한다. 실패해 해제된 C++ 객체의 바이트는 역참조하지 않으며 실패 분기의 생성/정리 효과와 반환/현재 상태를 대조한다.

## 남은 외부 경계

PE 실행에서 힙·기존 경로 조회/삭제·사건 조회/삭제·Host 부착·프레임 갱신·profiling 시계는 명시 관찰 경계다. 기존 Path 삭제 경계에는 정적으로 확인한 Unpop→Repop의 부모 live 효과만 제공한다. C++의 같은 Host/Frame/직렬화 연결은 별도로 검사하며 이러한 native 효과 경계가 실제 게임 삭제/표시 구현의 검증이라는 뜻은 아니다.

실제 이동 생성 지원은 type23 teleport와 type54 사제다. 다른 이동 파생 종류·부모 통지는 실행 가능한 빈 구현으로 채우지 않는다. `clearBlocked` 실제 겹침 정리 하위 몸체, 이전 Path의 운반/예약/의식 cleanup, 실제 사제 도착 운반/의식, GameWorld 명령 연결, 표시/소리 및 캠페인1-1 실제 완주/GUI 대조는 필수 후속이다. DirectDraw 전체화면 오류 수정은 목표에서 제외한다.

검사 등록 파일은 `RawPathStartTests.cpp`, fixture는 `pathstart-x86.tsv`와 `pathprocesstype-x86.tsv`, 근거는 `recovery-pathstart-evidence.json`이다. 초기 허용 범위/입력 오류와 교정 로그는 `extracted/pathstart-smoke-*.log`에 보존한다.

공유 Release의 최신 증분 빌드 뒤 `rawPathStart` 집중2개가 모두 통과했다. 실제 서버 flags0/공유 버퍼·같은 종속 체인/예약/Frame 대조도 이 검사에 포함한다. 기존 `RawPriestWorldBindings`7개, `rawPathRuntime`4개, `rawPathProcess`3개 회귀도 root가 통과를 확인했다. 첫2회 실패는 native Kill 요청flags0을 실제 client Form cleanup flags0으로 기대한 검사 오류였다. 공통 destroy가0x10을 더해 cleanup에 전달한다는 실제 경로에 맞춰 기대를 교정했으며, 시작/경로 body는 바꾸지 않았다. 첫 실패 로그 두 개와 `campaign-runtime4-rawPathStart-final-tests.log`를 보존한다.

## 기존 PathRuntime 근거의 연결

새 두 메서드를 추가했으므로 이전 `recovery-pathruntime-evidence.json`의 소스 전체 SHA와 현재 파일 SHA는 다르다. `tools/analyze_pathruntime_lineage.py` 및 `recovery-pathruntime-lineage-evidence.json`은 기준 git commit을 고정하고, 새 `SetCommandFields`/`ReplacePoint` 선언·정의 및 해당 설명 주석만 제외한 **기존 전체 소스와 모든 코드 토큰**이 기준 commit과 같음을 확인한다. 기존 native 기록의 두 source SHA도 기준 git blob의 LF/CRLF 바이트와 직접 대응한다.

이는 기존 ctor/Layout/배열 접근/Prepare/Step/대상/부착 몸체를 수정하지 않았다는 근거다. 이전360개 원본 입력은 신규 관찰로 다시 합산하지 않는다. 새 상위 흐름은 현재 전체 소스 SHA와 독립512개 신규 근거로 감사하고, 기존360개 C++ 재생은 통합 검사에서 다시 확인한다.

`tools/analyze_movement_mapper_lineage.py` 및 `recovery-movement-mapper-lineage-evidence.json`은 이전2872개 근거의 실제 원본 source SHA를 git commit `a5afdaf`의 파일 바이트와 연결한다. 주석 보완 뒤 기준 commit까지 코드 토큰이 같고, 새 공유 함수는 기존 몸체의 참조 세 개를 인자로 받도록 옮긴 뒤 같은 코드 토큰을 유지한다. 기존 Impl은 같은 pool edition/types span/movement 참조를 넘겨 반환한다. 나머지 두 파일 전체 소스의 보존과718개 회귀 결과가 이전2872개 부분집합과 같은 것도 감사한다. 새 종류 선택 관찰 수는0이다.
