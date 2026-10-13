# 실제 PathProcess 저장소·프레임·도착

마지막 정밀 디컴파일 PC는 **VM-W11-CODEX, 2026-10-13**이다. AGENTS.md 및 두 LEFT_JOBS를 먼저 읽었으며 보호 PE/게임/GUI를 실행하거나 변경하지 않았다. 기존 사제 연결·SP 회전·건설 도착·7개 사제 집중검사·이동 selector 2,872개·전체 CTest904개 근거는 그대로 둔다.

새 구현은 `cpppj/src/o/RawPathRuntime.{h,cpp}`와 `RawPathProcess.{h,cpp}`다. `RawPathAnimation`의 정지 프레임 접두는 재사용하지만 그 모듈을 전체 이동으로 해석하지 않는다.

## 저장소와 실제 계산

| 항목 | 10.78 | CD10.72 |
|---|---|---|
| type23 생성자/가상 표 | `0048be40 / 0050e284` | `0047f310 / 00503d60` |
| type54 생성자/가상 표 | `0048bf60 / 0050e458` | `0047fcf0 / 00503eb0` |
| 공통 생성자/크기 | `0048b2c0 / 0x186f0` | `0047d4d0 / 0x2f28` |
| 경로 배열 capacity | 5,000 | 600 |
| 시간표 시작 | `+0x9c58` | `+0x12d8` |
| 방향 배열 시작 | `+0x13898` | `+0x2598` |
| current/angle/finished | `+0x186c0/c4/c8` | `+0x2f00/04/08` |
| 시각/Step | `00488e20 / 0048a330` | `00402590 / 0047e820` |

원본 생성자는 시간 배열 전체와 지정 끝 필드만 초기화한다. 꼭짓점·방향·미지정 빈 필드의 heap 입력은 보존한다. CD의 꼭짓점 기본 생성자 `0041f9d0`도 쓰기가 없고 `00401590`은 시간 QWORD만 0으로 만든다. 패치에는 CD에 없는 두 DWORD가 끝에 더 있다.

각도는 **45도/360도**다. `0041cd60/CD0043f460`은 방향 차이가 4이면 +180도를 고른다. `now-startTime > 2.0`일 때만 주어진 시작 시간을 유지하고 그 외에는 현재 시각으로 둔다. 패치는 속도/회전 속도의 reciprocal을 float으로 저장한 뒤 거리·회전·직전 double 시각을 합한다. CD는 회전 시간만 float으로 저장하고 거리/speed를 x87에서 나눈다. 이 차이를 공통 float/double 식으로 뭉개지 않았다.

Step은 호출당 **최대 한 꼭짓점**만 전진한다. 앞으로 건너뛴 큰 `now`도 한 번에 모든 꼭짓점을 소진하지 않는다. targetTime 이전에는 float scale을 저장하고 위치와 각도를 보간한다. 도착 가지는 반환 각도를 바꾸지만 `currentAngle` 저장 필드를 새로 쓰지 않는다. 아직 끝이 아니며 새 current가 짝수일 때 현재 raw 부모 위치의 타입 stepSound를 요청한다. 마지막 current==nVert는 최종 꼭짓점과 방향을 반환한다.

## 실제 Kernel/공간/종단 연결

`RawPathProcess::Attach`는 기존 `SquidProcessHost::Attach`에 소유권을 넘긴다. 다른 SID 풀은 거부하며 type54는 현재 `priestType`과 부모 BYTE를 검사한다. Form 생성·Kernel 등록·부모 종속 체인·삭제는 기존 실제 복원 Host가 담당한다. 새 모듈은 부모 raw 위치를 직접 읽고 같은 풀의 virtual Pop/Unpop/Repop을 호출한다.

type23은 finished/Halt→Step→도착 또는 달라진 좌표의 Unpop0/Pop0x40 순서다. type54는 genus0x10000이면 실제 부모의 **Regular0x5105**부터 찾는다. HP 이동 불가 조회 `RawPriestState::Immobile`로 이 몸체를 대체하지 않는다. 이후 Step→지면검사→SHP cellHotspot→`fmod(angle,45)==0`→발자국 거리 비교→글자 구간의 다음 프레임 계산→프레임이 달라진 경우에만 Unpop0/프레임 쓰기/Pop0x40이다.

사제 도착은 기존 실제 정지 프레임 접두를 거친다. 공통 도착은 Step 최종 좌표→Unpop0/Pop0→현재 Form/부모/Kernel 생존→spot8과 일반 finder의 blocked 부류→Halt→finished1→부모 virtual+0x3c다. 실제 blocked 전역은 패치 `005325b4`, CD `0053fc28`의 **0x10444200**이며 `00489080` 실패 재시도의 다른 mask0x8c044200과 구별한다.

1000은 프로세스를 삭제한다. 1001은 부모 callback이 이미 삭제했음을 허용하고, 1002는 finished를 다시 0으로 만들어 같은 Form/Kernel에서 재시도한다. `shared_ptr<bool>` watch는 원본 destructor가 stack deadFlag를 1로 쓰는 수명 관찰을 보존한다. 부모 callback의 자기 삭제 뒤 `this`/runtime 멤버를 읽지 않는다. Host Kill의 실제 Form preDestroy는 공간 재등록 접두와 외부 정리 경계를 거쳐 같은 Kernel/SID 풀에서 삭제한다.

## 명시적으로 남은 실제 하위 몸체

이 단위는 캠페인1-1 실제 경로 시작 또는 전체 이동 월드 완료를 주장하지 않는다. 다음 경계의 실제 본문이 연결되어야 한다.

- **BFS/경로 시작:** `00488af0/CD00402100 → 00488370/CD00402750`, 현재 Graph 번호/표면 alias/hash0/spots, bridge 연결 `00441f20/CD004d2ef0` 및 타입별 시작/명령 `00489080/004894e0`. `InitializePath`는 경로 탐색 callback이 없으면 명시 예외다. `SetPath`는 이미 계산된 경로 입력을 받을 뿐 직선 경로를 생성하지 않는다.
- **Init 표면 접두 교정:** 후속 BFS 분석에서 `0040eaf0`는 좌표 범위 검사가 아니라 실제 거의 올림 hash0 표면 SID 조회임을 확인했다. 현재 `InitializePath`의 float 범위 검사만으로 search를 호출하는 접두는 전체 Init 본문 복원으로 세지 않는다. 원본은 실제 시작 표면0이면 Pop/search/current reset을 모두 건너뛴다. 새 `RawPathSearch::SurfaceAt` 연결 뒤 이 접두와 집중 입력을 교정해야 하며, 360개 fixture는 Init/BFS를 포함하지 않는다.
- **운반 대상 retarget:** `00489ff0`의 권한/genus0x30000/두 대상 조건이 참일 때 실제 contained nugget과 대상 사망 후 `00428ea0/00429420/004276a0/00427600`을 연결해야 한다. 누락이면 그 분기를 예외로 진단한다.
- **삭제 정리:** `0048aa00/CD0047f0a0`의 helper 주변 예약 SharedRegular, 같은 Graph의 운반 결정 삭제, 받침의 사제 의식 `00447e40`, 남은 abstract 대상의 통계/가상 삭제는 필수 cleanup callback이다. 새 모듈의 자동 공간 접두와 전체 하위 본문을 동일시하지 않는다.
- **부모 가상 효과:** 현재 원본 타입의 virtual Halt/Arrival, 소리 번호와 실제 음향 요청, blocked 정리 `00443800/CD004d2450`가 필요하다. `teleportEffect`의 부모 Arrival은 기존 `RawConstructionArrival::Arrive`와 동일 풀에서 연결 가능하다. 사제의 실제 Arrival/명령 상태는 별도로 연결해야 한다.
- **전송/수신:** 실제 Host 부착과 수명은 사용하지만 Path 자체 `004895b0` 직렬화/명령 수신·변환은 이 단위에 포함하지 않는다. `RestoreStep`은 명시 raw 입력 복사 API이며 원본 네트워크 수신 본문 복원으로 세지 않는다.

## 독립 근거와 검사

고유 `pathruntime`, `pathruntimehelpers`, `pathmath` manifest를 순차 실행했다. 첫 원본30/CD25, getter/SHP/좌표 helper 원본13/CD11, CRT fmod 정상/zero dispatch 원본5/CD5가 모두 readOnly Exit0이다. 원본 CRT의 실제 dispatch와 fprem/zero 반환을 실행한다. fmod를 호스트 계산으로 대체하지 않는다.

`tools/decomp_pathruntime_oracle.py`의 독립 native 입력은 **360개**다: ctor8, 시간표48, Step160, Frame120, Arrival24. 보호 PE 및 제한 body가 결과를 계산하고 fixture에는 관찰된 bit·원본 호출 순서만 저장한다. heap malloc/음향 요청/진단 시계/부모 virtual Halt·Arrival/공간 요청/삭제·Regular freeze 조회는 명시 하위 경계다. Frame/Arrival 검사에서 실제 Step/Halt/프레임/정지/도착 분배와 CRT를 실행하며, 하위 경계를 원본 전체 실행으로 세지 않는다.

`cpppj/tests/RawPathRuntimeTests.cpp`의 새 집중검사 **7개/실패0**을 root가 최종 공유 Release 빌드 **경고0/오류0** 뒤 확인했다. primitive의 whole-image hash 및 모든 QWORD/float bit를 대조한다. Process 검사에서는 부모 가상 입력은 native script지만 Pop/Unpop과 Kernel/Form/종속 수명은 실제 C++ 모듈이다. 합성 base 부모를 사용하며 이 검사를 실제 사제 postPop/의식 전체 통합으로 세지 않는다. 전체 CTest는 root가 이어서 실행 중이다. 신규 BFS source는 이 완료 수에 포함하지 않는다.

Native Frame/Arrival은 **parent5/head7/Form7**을 직접 입력하며 Form7은 두 판본에서 client 번호다. 생성자/부착 호출·서버 전송은 그 fixture 범위에 없다. 초기 C++ Scene의 server=true/flags0은 서버 Form을 만들었으므로 원본 합성 입력과 역할이 달랐고 필수 ProcessForm 전송 경계 예외가 발생했다. Scene을 client 풀(server=false), 명시 client Allocate(2)의 parent5/번호6 void 입력 및 **합성 local Attach flags0x10/Form7**로 교정했다. server=false 자체가 client 할당을 선택하지 않으므로 처음 검토의 flags0 유지 설명은 잘못된 해석이었다. 임의 flags0x50으로 미복원 서버 경로를 성공 처리하지 않는다. 캠페인 actual 서버 Path의 **flags0** 부착에는 `004895b0` 직렬화/ProcessFormTransmission 연결이 여전히 필요하며, 이 검사를 서버 부착 native proof로 세지 않는다.

초기 실패 로그도 보존했다. primitive harness의 parent head0 입력은 원본 base 계약에 맞춰 7로 고쳤다. Frame의 CRT wrapper·zero helper 미허용 범위는 정밀 export로 보완했다. CD spot 포인터 주소의 입력 오타도 `0052fe48`로 교정했다. 이 실패/반복 실행은 새 관찰 수에 더하지 않는다.
