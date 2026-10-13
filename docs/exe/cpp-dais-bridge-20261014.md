# 다이스 Bridge 명령의 실제 Path 연결

현재/마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 기존 `daisregular` 패치18/CD22와 `templeritual`·`runepostpop`·`pathcleanup`·`pathstart`·`pathruntime`·`pathmath`·`pathsearch`의 현재 읽기 전용 몸체를 재사용한다. 단위12 자료는 변경하지 않으며 이 자료는 별도 후속 단위다.

## 새 관찰 범위

다이스 **00449900/CD00481ab0**는 현재 또는 다음 단계 목표를 실제 SHP 핫스폿으로 구한다. 자격은 현재 typedAltar, 알터 방향, 사제 genus/owner, 살아 있는 부모의 포획 Form32/현재 priestType, 패치 동맹표를 실제 몸체로 읽는다. 같은 칸이면 실제 Ritual mode0으로 들어가며, 다른 칸의 boss 분기는 현재 mapper→첫 Path Form→기존 이동 깊이를 읽는다.

깊이+1<4이면 실제 **00489080/CD0047d150**의 PathStart→실제 생성자/flags0 Init→BFS→명령 DWORD→첫 Prepare→서버 부착→사제 통지/취소를 수행한다. 원래 다이스 명령은 새 경로의 마지막 꼭짓점을 목표 float로 교체하고 가상+30의 두 번째 Prepare 뒤 깊이+1을 쓴다. helper0, tailWord=다이스SID60, tailByte=목표stage이며 실제 타입54를 client 편의 경로로 바꾸지 않는다.

새 `decomp_daisbridge_oracle.py`는 stage0..3 ×현재/다음×기존경로/깊이0..3×명시/검색사제×boss×표면존재를 교차한다. 기존경로 없는 입력의 중복 깊이는0만 남겨 **640개 정상/assert0**을 관찰했다. 판본당320입력을 두x87에서 실행해640정상반환/판본을 확인했다. 새 Path 생성64/판본이며 자연반환·깊이제한·경로실패는256/판본이다. 초기화에 사용하는 기존 PathStart 호출과 이전512/668/2964 관찰은 새 입력에 합산하지 않는다. 최종 공유 **Release 경고0/오류0**, C++ 전수 **1검사/실패0**이며 좌표/방향/double시각/명령·실제 Host/Frame/packet/기존 Kill 소비자가 통과했다. 로그는 `run13-rawDaisBridge_final.log`다.

## 입력 메모리와 정밀도

- Path 배열은 기존 격리 주소를 사용하고 새 SHP는 **0x11800000**, spot 지도는 **0x11820000**에 둔다. hash1과 spot 지도는 같은 주소를 공유하지 않는다.
- 초기 smoke14는 spot 초기화가 hash1을 지워 자격 조건을 통과하지 못했다. 당시 전체 tool/report/fixture와 실패 로그는 `hist/daisbridge-i-00c54`에 보존하며 새 완료 관찰에 합산하지 않는다.
- 표면 없는 입력은 hash0 머리만 지우고 사제·알터·포획 Form과 일반 해시를 유지한다. 실제 PathStart 실패/해제를 읽으며 상위 몸체를 빈 경계로 건너뛰지 않는다.
- 표면83의 코드 포인터와 사제158의 코드 포인터는 원본 합성 입력에서 **CODES=0x11200000**를 공유한다. 사제 프레임 입력을 마지막에 쓰므로 최종 표면 코드도 **AP(41500100)**다. C++ 입력은 이 최종 표면 코드로 준비해 동일한 접합/방향변경 꼭짓점을 읽는다. 원본 관찰640/tool/fixture를 변경하지 않는다.
- Host 부착은 Native의 명시 경계다. 두 번째 Prepare가 실제 부모를 다시 읽으므로 그 경계에서 PATH의 PID777/parent5뿐 아니라 부모 head777도 공급한다. 구 경로 Kill은 현재 부모 head를0으로 갱신한다. Native 전체 Host 수명 복원으로 표시하지 않는다.
- 두 x87 정밀도에서 모든 효과/개수/명령/꼭짓점/방향/raw는 같다. 패치18입력에서 double 시각26단어가±1ULP 차이를 보였다(-1:14/+1:12). CD 차이는0이다. 정상 목표 `0x027f` 결과를 주 fixture로 기록하고 `0x037f`의 전체 결과를 별도로 보존한다. C++는 실제 모든 시각 비트를 `0x027f`와 정확 비교하며 허용오차를 사용하지 않는다. 같은 입력을 두 번 합산하지 않는다.

## C++ 연결과 남은 범위

새 `DaisBridgeTests.cpp`는 같은 풀/hash/spot/실제 BFS/mapper/RawPathStart/RawPathProcess/Host/서버 PathData/실제 Frame을 연결한다. 기존 경로가 있을 때 실제 Host.Kill과 Path 공간 접두 뒤의 `RawPathCleanup.AfterSpatial`을 실행하고 같은 실제 Temple.Ritual을 공급한다. 선택된 시작점은 현재 의식 목표와 다르므로 이 Ritual은 원본 조건에서 자연반환한다. 같은 객체 주소가 재사용될 수 있어 구/새 경로는 실제 Form SID로 구분한다.

전수 검사는 실제 살아 있는 경로의 모든 꼭짓점/방향/double시각/명령/깊이, Form 부모 연결, 서버 패킷1개, Frame 갱신2회와 Kernel 크기를 검사한다. 판본별 일반 사제 공간 VT와 표시1×1 합성 입력을 사용하며 actual 사제의 모든 최초/후처리로 확장하지 않는다. allocator 해제/Native Host/Frame 요청 경계는 실제 C++ 저장소 확인과 구분한다. 등록 macro는 **NETSTORM_DAISBRIDGE_FIXTURE**다.

첫 공유 Release 경고0/오류0 뒤 전수1검사는 경로 비트2체크와 Unpop 표시 입력 예외로3실패했다. 당시 전체106파일을 `hist/daisbridge-i-42bda`에 보존했고 `analyze_daisbridge_cpp_correction.py`가 Native 합성VT를 판본별 일반VT로 준비한 변경과 일시 진단 문장을 정확 역변환한다. 두 번째 Release 경고0/오류0 뒤 진단은 양 판본 생성64개씩 모두 C++ 직선2점/원본 방향변경4점의128체크 차이를 확인했다. 시각 허용오차 문제로 처리하지 않았다.

두 번째 전체 검사/생산/Native/첫 successor/도구/실패·빌드 로그110파일은 `hist/daisbridge-b-6acfc`에 별도로 보존했다. `analyze_daisbridge_surface_input_successor.py`와 `daisbridge-surface-input-successor.json`은 C++ 표면 입력 AA→AP와 진단 제거만 정확 역변환하며 기존 두 보고서·Native640·생산 몸체를 불변으로 감사한다. 새 입력 합산0이며 최종 공유 재대조도 실패0으로 통과했다.

새 자료는 `daisbridge-x86.tsv`, `daisbridge.json`이다. `analyze_daisbridge_cpp_binding.py`와 `daisbridge-cpp-binding.json`은 첫 검사 당시의 실제 소비한 생산 몸체 전체SHA/초기smoke 보존/정밀도 차이 수를 유지한다. 현재 검사 SHA는 두 정확 역변환 successor로 연결하며 추가 원본0이다. 전체 걷기/부모 가상 도착/의식307→룬건설304→다음 이동·전체Progress 음향·사제 해방·실제 캠페인 metadata/GameWorld/승패·UI는 후속이다. 단계4의다음목표5는 원본 미초기화 좌표 경계이며 현재 정상640에 넣지 않는다. 원본/복사본 게임·GUI 실행0이다.
