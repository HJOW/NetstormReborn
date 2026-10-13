# 사제 Halt·Arrival와 Carrier 도착의 실제 경계

현재/마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 단위13 DaisBridge640/전수C++ 자료와 이 후속 분석을 독립 범위로 분리한다. `carrierarrival-functions.json`의 패치54/CD59는 검토한 직접호출과 필수 getter만 묶은 읽기 전용 목록이며 세션93649가 종료0으로 내보냈다. 현재 양 판본 `creation.c`와 `functions.tsv`를 읽었다. 단위14 새 Halt864와 Carrier406의 C++ 전수는 각각 통과했으며 같은 Host 걷기/도착은 입력 교정 후 재실행을 기다린다.

## 실제 가상 표와 직접 호출

| 몸체 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 실제 사제 VT |0050f210|005003e0|
| 사제 +38 Halt |00491bb0|0040c950|
| 사제 +3c Arrival |00491f70|0040d200|
| 공통 CarrierArrival |00429800|004e66a0|
| CommonHalt thunk |0042ab70|004e7910|
| CommonHalt 몸체 |0042a5f0|004e45c0|

CD `0040c950`은 기존 전체 Refined 함수표에서 누락됐으나 보호PE의 실제 가상표와 기계어로 확인했다. 코드 범위는 `0040c950`~`0040ca3c`이며 직접호출은 __ftol `004f161c`, 좌표거의올림 `00440a80`, 사제HP/2 getter `0040c3b0`, fallback thunk `004e7910`다. 여러 조기ret를 첫ret 하나의 범위로 잘라내지 않는다. 패치 `00491bb0`의 대응은 __ftol `004e49c0`, 좌표거의올림 `0041d7a0`, 현재 최대HP `004adcb0`, thunk `0042ab70`다.

Halt의 특정 전역이 켜진 분기는 거의올림한 지면 spot와 HP/2를 읽고 권한이 있으면 실제 Unpop0→좌표보정→Pop0 뒤11을 반환한다. 그 밖에는 +cc의 실제 지면/낙하 검사→같은 칸의 운반체 조건과 상태10/12를 처리한다. 패치는 특정 두 운반 타입 각각 `00429ed0`/`0042a0d0`의 반환12를 검사하고 CD는 `004e7980`의 0 아닌 반환을 검사한다. 기존 `RawCarrierCheck`와 `RawPriestState`를 재사용하되 큰 fallback 몸체를 항상10으로 대체하지 않는다. 마지막 공통 기본 getter `004ac990`/`004ac460`의 실제 몸체만10을 반환한다.

사제 Arrival은 neutral/권한/로딩·억제/HP에 따라 현재 plain 표면이 속한 섬의 Temple을 조회하고 포획 Form을 붙여 자기삭제하거나, 이동속도를 임시로 바꾼 귀환/무작위 이동을 시작할 수 있다. `0046f950/CD004bd9f0`은 nearest 검색이 아니라 현재 표면 지역→현재 Island TempleDWORD getter다. 거리 `0041d840/CD00440b20`는 발자국 각 축의 signed거리 중 큰 값을1.0과 비교하며 Euclid 길이를 사용하지 않는다. 그 뒤 같은 원본 공통 CarrierArrival에 위임하는 일반 분기가 있다. 패치와 CD의 neutral 조건 helper/거리 저장폭/공통후처리 분기는 각 몸체로 확인한다.

## CarrierArrival의 필수 흐름

1. 실제 genus carrier조건과 현재 Path helper를 읽고 owner0의 조기반환을 확인한다.
2. 현재 contained 너겟/사제, 주변 지면 부류, 귀환 장소, 현재 다이스 및 목표점 허용을 실제 finder/WORD 조건/발자국으로 읽는다. 포획 사제가 있으면 일반 운반 후보를 비운다.
3. 현재 경로 깊이0과 경로 없는 경우는 포획·채굴·SP 전달·같은Graph·사제이동·명령 대상 검증을 처리한다. 깊이가 있는 실제 Bridge 경로에서는 그 부분이 원본 조건으로 생략된다.
4. 다이스 인접/권한과 동맹표에 따라 포획 Form 전달/삭제, 5초 회피 요청을 만들 수 있다. 실제 허용 점 `00448740/CD004830c0`은 다이스 각 좌표에서 **3.0**을 뺀 점과 정확 일치해야 한다. PE상수 `00501908/CD00504058`의 비트는40400000이다. 일반 운반은 원본 대상A/B를 검증하거나 새 대상/경로를 선택한다.
5. 다른 운반체/특수 건물/현재점과 아래 지면 부류가 막으면 회피 요청을 수행한다. 마지막 두 점은 현재 위치와 **y+2.0**이며 `00501444/CD00506d90`의 비트40000000을 확인했다. 정상 끝에서는 패치의 조건부 Robust 확인 뒤1000을 반환한다. 1001/1002와 부모 자기삭제의 Kernel 수명은 실제 호출자 `RawPathProcess`가 처리한다.

패치의 SP 전달 분기와 CD의 직접 SP 저장·목록·음향 분기는 같다고 가정하지 않는다. 현재 프로토콜/RPC helper, 포획/운반/채굴/회피의 큰 하위 몸체는 새 읽기 전용 내보내기 뒤 순서대로 조사한다. 호출하지 않는 선택분기는 자연 생략 조건을 기록하며 호출된 미복원 효과는 예외로 드러낸다.

## 재사용과 다음 실제 소비자

기존 `RawPathCleanup.ReturnPlaceAt/NearbyType`, `RawCarrierPreDestroy.FindContained`, 실제 `RawSquidFinder`, `RawPlayerPlacementAnchor.GraphFor`, `RawDaisRegular.MoveAwayDepth`, `RawPathStart`, `RawTempleRitual`, `RawPriestRitualActions`, `SquidProcessHost`와 현재 MaxHP/프레임/HP getter를 재사용한다. 원래 포획 Form 생성/부착은 `ContentFormHost`와 실제 `SquidFactory`의 기존 몸체를 공급한다. 기존 ConstructionArrival과 CarrierCheck를 전체 사제Arrival의 대체로 사용하지 않는다.

새 `RawPriestHalt`의 원본864 전수/현재SHA 자료는 독립 문서 `cpp-priest-halt-20261014.md`에서 범위를 기록한다. 새 `RawCarrierArrival`과 `RawPriestArrival` 부모본문 초안은 기존 위 getter들과 `RawContainedCreate`·`RawCanonPlacementTerrain.RegionAt`·`RawIslandRuntime.Temple`·`RawSquidNeighbors.Center/Footprint`를 직접 재사용한다. `GenusAt`은 기존 `RawDamageablePostDestroy`의 같은 몸체를 공유할 최소 공개API가 필요하며 단위13 커밋 전 공유파일 수정은 없다. 큰 포획/너겟전달·채굴packet/RPC·SP/owner요청·회피·중립 귀환은 선택되면 필수 실제 하위 경계로 남는다.

첫 CarrierArrival Native 준비에서 원본 main이 함수 포인터로 호출하는 `0042b1a0` point 생성자가 기존54개 목록에 없음을 확인했다. 별도 `carrierarrival-helper-functions.json` PATCH1/CD1(`0042b1a0`/`0041f9d0`)는 같은 PC의 읽기 전용 세션18841 종료0으로 내보냈다. actual 몸체는 point 저장소의 this를 반환하며 외부 no-op으로 대체하지 않는다.

새 CarrierArrival 전수는 **406 정상/assert0**, 판본당203입력/두x87 정상반환406이며 모든 결과·요청·부모 전체 raw가 같다. 현재 경로 helper0/99·깊이0/1/3·경로없음·권한/중립·Player AI 존재·일반운반/사제 genus·다른운반체/특정지면/현재점4200/아래점200/허용다이스를 교차했다. 입력 중복 한 개는 전체 JSON 기준으로 제거했다. 큰 escape의 지연/사건/반환과 현재 Path lookup은 명시 외부 경계이고 실제 contained/finder/귀환장소/다이스 좌표/깊이/Player getter/끝 검색은 원본 몸체다. 이전Halt864/Bridge640/BFS/GenusAt4412는 재합산하지 않는다. 초기10smoke 전체5파일은 `hist/carrierarriv-91d3e`에 보존한다.

`RawDamageablePostDestroy.h`의 기존 GenusAt 선언 한 줄만 private→public으로 이동해 새 도착에서 같은 몸체를 재사용한다. 기준 **860030e**의 기존17파일을 Git/work 각각 전체로 `hist/genus-at-pub-4fe2a`에 보존했고 `analyze_genus_at_public_lineage.py`가 정확 선언 역이동과 기존 Cpp/4412·Host·관련 관찰·fixture 불변을 감사한다. 기존 보고서는 당시 SHA로 유지하며 새 공개API 자체의 원본 관찰은0이다.

새 전수 C++ `CarrierArrivalTests.cpp`와 `PriestPathArrivalHostTests.cpp`의2검사/현재 전체 source는 최초 `analyze_carrierarrival_cpp_binding.py`/`carrierarrival-cpp-binding.json`과 아래 successor로 연결한다. macro는 **NETSTORM_CARRIERARRIVAL_FIXTURE**, prefix는 각각 **rawCarrierArrival_**, **rawPriestArrival_**다. 같은Host 소비자는 실제 사제VT/Factory/HP200·owner1·지면6/Pop·Unpop/전체사제PostPop/보호막조회/Path의 중간 보간→actualHalt→사제Arrival→CarrierArrival1000→Path삭제/공간접두/cleanup/Form반납을 사용한다. 경로두점/1×1/비포획/AI없음/다이스없음 합성 입력이며 전체 캠페인 자산·중립복귀·포획/운반/SP/RPC/큰회피·의식307/룬304 완료 근거로 확장하지 않는다. 현재 Carrier 전수는 **1검사/406전수/실패0**, 같은 Host 검사는 최초 실패 입력 교정 뒤 재실행 대기다.

## 단위14 빌드·소비자 입력 교정 보존

최초 컴파일에서는 `RawPriestArrival.cpp`의 로컬 `kNoIsland`가 ChunkMap의 전역과 충돌했다. 당시 전체115파일과 실패 로그를 `hist/priest-arriv-ac97b`에 보존하고 두 이름 토큰만 `kPriestNoIsland`로 바꿨다. `analyze_priest_arrival_name_lineage.py`가 전체 cpp의 정확 역변환과 최초 Carrier binding의 당시 source view를 재현한다. 값127/계산/조건/Native는 불변이다.

두 번째 컴파일의 Host 검사는 RawContainedLookup 정의 include가 없었고 initializer-list를 span에 직접 전달했다. 당시 현재 name/binding/생산/Native/fixture/검사/실패 로그 전체117파일을 `priest-arriv-35eb0`에 보존했다. include 한 줄과 동일 두 점 float/방향 int32/시각 double의 const std::array 실체화만 변경했다. `analyze_priest_arrival_test_syntax_lineage.py`가 전체 검사 정확 역변환과 syntax→name→Carrier 근거를 재현한다.

첫 CPP 실행에서는 Native용 `ArrivalScene` 초기화가 Allocate의 raw `+11` void4를0으로 지운 뒤 hash를 비웠는데, Host 소비자는 VT recipe만 쓰는 `Factory.Construct`로 초기화했다. 따라서 패치 첫 Pop이 Unchanged였고 이어진 Unpop은 해시에 없는 부모50을 찾아 실패했다. CD에서는 같은 구성의 첫 Pop이 void 전제 오류다. 생산 몸체의 결함이 아니다.

완료된 전체1081/Host 실패 로그와 third/fourth-build 로그를 포함해 당시 전체122파일을 `priest-arriv-dbe45`에 보존했다. Host 소비자의 scene0 준비만 같은 Pool Reset→실제 Allocate5..49→`Factory.Create(158,2)==Sid50`로 변경했다. HP200/owner1/좌표/프레임/경로/CHECK/Native1270/전수검사/Support/생산body는 불변이다. `analyze_priest_arrival_factory_input_lineage.py`와 `priest-arrival-factory-input-lineage.json`이 이전 전체 검사 정확 역변환과 당시 syntax→name→Carrier를 재현한다. 세 교정의 추가 원본 입력은0이며 이전 보고서/fixture 원문을 덮지 않는다.

첫 실제 의식 소비자는 Bridge의 도착→부모 Arrival→Path 삭제/공간 접두→cleanup의 Temple.Ritual(mode0)→사제604/신전307→Rune 생성/건설304→다음 Dais.Progress를 같은 풀·Host·시계에 연결해야 한다. 현재 Bridge640은 마지막 점/Prepare/깊이까지이며 아직 이 걷기·도착 사슬의 완료 근거가 아니다. 실제 캠페인 포획/해방·운반·음향/UI·승패/캠페인1-1 완주는 후속이다. 생산/shared·보호 원본·AGENTS·dotnetpj 수정0, 게임/GUI 실행0이다.
