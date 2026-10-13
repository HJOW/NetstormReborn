# cpppj 건설 운반 도착 판정

2026-10-13, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. 두 PE의 `00444080`/CD `004d2c80`과 좌표 정수화·일반 finder·타입 플래그 helper를 읽기 전용으로 내보냈다. 목록 `tools/ghidra/constructionarrival-functions.json`, 출력 `extracted/constructionarrival/`, 로그 `extracted/constructionarrival-export.log`다. 게임/GUI/보호 파일 변경·커밋/푸시는 하지 않았다.

## 복원한 몸체와 연결 계약

`RawConstructionArrival::Arrive(builder)`는 teleportEffect(타입134)의 가상 **+0x3c** 도착 몸체다. 부모 raw의 현재 좌표를 `__ftol → float 저장 → __ftol`로 정수 칸에 만든 뒤, 같은 pool/hash/types의 실제 `RawSquidFinder`를 그 한 칸에서 끝까지 순회한다. 0.5 반올림이나 0.9999 거의 올림을 더하지 않는다.

현재 후보의 abstract BYTE+40/CD+35 bit0(mask1)이 켜져 있고 현재 flags1에 needsBuilder `0x8000`이 있을 때만 대상으로 센다. 소유자/일반 state/free/dead 필터를 이 몸체에 추가하지 않는다. 일반 finder의 자체 조건은 그대로 실행된다. SID가 client 예측 범위5..14999/CD5..5999 밖이면 현재 duration 전역으로 Start를 호출한다. 여러 서버 대상이 있으면 모두 원본 순서로 시작한다. callback 뒤 후행 후보의 좌표/flags/abstract/해시와 duration을 다시 읽으며 finder가 저장한 next를 유지한다.

- 서버 대상에 하나라도 시작했거나 적합한 대상이 없으면, 도착 운반 객체를 가상 삭제(flags0)하고 **1001 `pathALREADY_DEAD`**를 반환한다. 삭제 뒤 this를 읽지 않는다.
- 적합한 예측 대상만 있고 서버 대상은 없으면 **1002 `pathLINGER`**를 반환한다. checking/debug가 꺼져 있으면 자기 첫 streakSystemProcessType(12)을 `removeContained(sid,type,0,1)`로 정리한다. 이 몸체는 대기 중 자기/경로를 끝내지 않는다.

`ConstructionArrivalState`는 checking, streakProcessType, duration을 현재 전역 참조로 받는다. 기본 duration **0.6f**는 실제 `005325c0`/CD `0053fc38` 비트값이다. `MakeConstructionArrivalHooks`는 Start를 실제 `RawConstructionProcess::Start`로 연결하며 동일 pool을 확인한다. 나머지 contained/가상 삭제는 실제 월드 Form 분배와 연결한다.

## 실제 PathProcess의 필수 후속

원본 타입 전역과 가상 표를 직접 읽었다. 사제 경로(type54)를 건설 운반 경로(type23)로 대체하거나 반대로 연결하면 안 된다.

| 경로 | 실제 원본 주소/자료 | 실행 계약 |
| --- | --- | --- |
| 이동 요청 | `00489080`, 타입별 조회 `00488fe0` | 현재 기존 경로의 사건/추가 자료를 보존하며 새 타입별 PathProcess 생성·경로 Init·속도/시간 계산·실제 ProcessForm 부착 |
| teleportEffect134 → type23 | 전역 `005410cc=23`, 생성자 `0048be40`, vtable `0050e284` | +18 `0048a950` 진행, +24 `0048b340` 공통 도착, +2c `0048b680`가 실제 `0041bd20(type23)` 부착 |
| manaBolt105 → type39 | 전역 `0054110c=39` | 원본 타입별 생성자/진행/도착 후속 필요. 건설 공급원 이동은 화면용 가짜 조각만으로 대체할 수 없음 |
| 사제158 → type54 | 전역 `00541148=54`, 생성자 `0048bf60`, vtable `0050e458` | +18 `0048bb40` 보행/방향 프레임, +24 `0048bd90` 멈춤 프레임→공통 도착, +2c `0048bd30` 사제 타입 확인 후 실제 `0041bd20(type54)` 부착. 마지막 호출은 현 refined C에서 빠졌지만 실제 기계어로 확인 |
| 경로 위치/시각 | `0048a330`, 속도 `0048a200`→`00488e20` | 현재 path vertex/각도/도착 double 시각을 같은 게임 now와 비교하며 보간/단계/발소리를 갱신 |
| 공통 도착 | `0048b340` | 현재 부모를 실제 Unpop/Pop하고 ProcessForm의 robust를 재조회. Parent 가상+3c에 보존한 두 사건 인자를 전달하며 self deletion을 추적 |
| 1002 재시도 | 동일 `0048b340` | deadFlag를 다시0으로 두고 실제 살아 있는 PathProcess를 다음 프레임에 남긴다. 별도 0.6초/10초 timer로 확정 대기를 우회하지 않는다 |
| 1000/1001 종료 | 동일 `0048b340` | 경로를 실제 Kill한다. 부모 삭제 중 경로도 삭제됐으면 이미 해제된 자료에 재진입하지 않도록 같은 Form/Kernel 수명 분배가 필요 |
| 사제 도착/후처리 | `0048bd90`→`0048b340`, 경로 삭제 `0048aa00` | 기존 `RawPathAnimation::Stop`은 멈춤 프레임 접두만 복원했다. 부모 사건·건설 target·워커/너겟/데이스·의식 후처리와 전체 경로 삭제는 별도 실제 몸체 |

이 단계는 `00444080` 도착 끝점을 복원했으며 **전체 PathProcess를 구현했다고 주장하지 않는다**. 같은 월드 공간/Kernel에 이 몸체를 등록하고 이동/도착/확정 대기/삭제를 원본처럼 실행하는 일이 남아 있다. 더 큰 실행 경로는 [건설 경로 감사](cpp-construction-runtime-audit.md)에 있다.

### 다음 최소 종단 복원 목록

1. **이동 시작/기존 경로 교체**: `00488fe0`의 실제 타입 selector→`00489080`의 현재 contained 조회/기존 사건 보존·Kill→`0048b2c0` 생성 필드→타입별 생성자23/54. 기존 함수의 Ghidra 서명이 실제 8인자 호출을 3인자로 보이는 구간이 있으므로 ret/스택 인자·비정렬 SID/실수 저장 폭을 기계어로 확정한다.
2. **실제 경로 생성**: 두 타입의 +20 `0048bb10`→`0048a660`→`00488af0`→`00488370`의 실제 탐색 결과를 쓴다. 현재 Graph 성분/spot&6·모서리 통과·출발/목표 거의 올림·vertex 방향/순서를 같은 표면 지도에서 읽는다. 직선이나 기존 `GameWorld` 값형 이동의 보간을 경로 결과로 대신할 수 없다. `00489080`의 실패 시 발자국 후보 밀어내기·한 번 재시도도 필요하다.
3. **속도/각도/도착 double 시각**: +30 `0048a200`→`00488e20`을 타입 speed/turningSpeed와 같은 현재 게임 now에 연결한다. 원본은 회전 방향 차이와 구간 길이를 합산해 vertex 도착 시각을 만든다. Builder.Move의 요청 시각 조정도 이 경로 저장소를 사용해야 한다.
4. **실제 부착 및 매 프레임 진행**: 타입23 `0048b680`, 타입54 `0048bd30`의 `0041bd20`를 실제 공유 ProcessHost/Kernel/FormDispatch에 연결한다. +28 `00489ff0`은 parent +38 halt 판정/포획·운반 대상을 현재 raw에서 검사하고 자기 삭제를 추적한다. +18의23 `0048a950`/54 `0048bb40`은 `0048a330`의 현재 위치/각도/시각과 실제 Pop/Unpop·방향 프레임·발소리를 실행한다.
5. **최종 도착/linger/stop**: +24 `0048b340` 또는 사제 `0048bd90`→`0048b340`를 복원한다. teleportEffect parent+3c는 이번 RawConstructionArrival, 사제 parent+3c는 사제 자체의 실제 사건/의식/운반 몸체다. parent가 먼저 지워지면 path+0xc `0048aa00`의 deadFlag/부모 재등록·목표 건설 Regular·너겟/데이스 및 다른 후처리를 현재 공유 수명에 전달한다.
6. **최소 실제 미션 입력**: 같은 초기 미션 pool의 건물/사제/기술·SP를 등록하고 클릭 이동 한 번, 건설용23이 예측 대상만 보는1002→서버 확정 뒤1001·0.6초 Start, 사제54의 보행/stop과 대상 삭제를 실제 프레임에서 확인한다. 그 뒤 동일 SHP·소리·프레임/좌표·현재 hash/spot/Graph/contained/PID를 원본 관찰과 비교한다. GunProcess/공격/포획/의식/승패가 남아 있는 동안 첫 건설 성공을 캠페인 완주로 해석하지 않는다.

공급 manaBolt의 `RawConstructionBuilder`→Regular 실제 adapter는 [건설 경로 감사](cpp-construction-runtime-audit.md)에 기록한 **event0/payload0.05f 반전 오류를 이번 작업에서 교정했다**. constructor raw 인자 기록만 일치하는 기존 fixture와 실제 생성자/Host 필드를 대조한 새192개 입력을 구별한다. 별도의 실제 Host/Kernel 재예약 검사는 부모 사건 몸체를 payload 반환으로 공급한 명시 경계이며 manaBolt event0 본체까지 대조한 검사가 아니다.

## 독립 근거와 검사 범위

`tools/decomp_constructionarrival_oracle.py`는 도착/정수화/actual finder/타입 플래그의 실제 기계어만 허용한다. 하위 건설 시작·첫 contained 삭제·가상 자기 삭제 세 효과만 기록 대체한다. 실제 명령의 쓰기는 로컬 스택에만 허용하며 pool/type/hash는 읽기 전용이다. 대체 callback의 명시 입력 변형은 별도로 공급한다.

두 PE **153개씩, 306개** 입력은 예측/서버 경계 포함·모든 일반 state·abstract/buried·needsBuilder 필터, 네 해시 단계·좌표/발자국·무대상·여러 서버/예측 후보·debug 대기, callback 뒤 후행 abstract/flags/좌표·저장된 next/현재 duration 변경을 다룬다. 두 x87 제어 워드의 결과/ret8/스택/top을 확인하며 정밀도 재실행은 새 독립 입력으로 더하지 않는다.

fixture `constructionarrival-x86.tsv`, 근거 `constructionarrival.json`, 로그 `extracted/constructionarrival-oracle-initial.log`·`constructionarrival-verify.log`다. C++ 검사는 원본/디컴파일/Python 없이 fixture의 전체 반환/효과 순서를 실제 finder를 통해 재생한다. 원본 관찰에는 Begin 정수 범위도 저장되어 있으며 C++ 종단 비교는 반환/효과를 비교한다. 실제 하위 Start/공통 자기 삭제 및 캠페인 진행은 월드 통합 검사 범위다.
