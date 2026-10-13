# 실제 캠페인 사제 경로 연결부

마지막 디컴파일 PC/날짜는 **HJOW-Athlon, 2026-10-14**다. `AGENTS.md`와 두 인수인계를 확인했으며 기존 단위14 생산/검사/원본 보고서를 동결한 상태에서 새 독립 client 파일만 준비했다.

## 공개 API와 소유권

새 `RawCampaignPathBindings.h/.cpp`는 `RawSurfaceWorld::Core`와 실제 사제 연결부·Halt·Arrival·cleanup·ContainedLookup을 빌린다. root는 이 모듈과 mutable 상태를 장기 소유하며 연결부는 기존 `RawPathSearch`와 `RawPathStart`만 소유한다. 만들어진 실제54 `RawPathProcess`는 같은 `core.processes`/Kernel이 소유한다.

| API | 실행하는 실제 경계 |
| --- | --- |
| `StartPriest(request)` | 실제158/사제VT/선택54 확인→기존 Start→Init→같은 BFS→명령 DWORD→Prepare→같은 Host 부착→사제 통지 |
| `CurrentPath(parent)` | 현재 종속 체인 lookup54→같은 Host `FromForm`→실제 Path/parent/Form/Pool 검사 |
| `CurrentRuntime(parent)` | 현재 Path의 원래 Runtime 참조를 반환하며 사본을 만들지 않는다. |
| `SetTargets(parent,first,second)` | 현재 Runtime의 기존 두 target DWORD 쓰기만 실행한다. 큰 재탐색 완료를 뜻하지 않는다. |
| `RetargetCurrent(parent)` | 실제 caller의 큰 운반 재탐색 callback을 요청한다. 자기 삭제 뒤 포인터를 다시 읽지 않는다. |
| `Transmission(header,buffer,transport,other)` | 기존 `MakePathFormTransmission`으로 같은 raw Path 이미지→같은 buffer→실제 전송을 연결하고 다른 프로세스 writer fallback을 보존한다. |
| `Pool()` | 다른 월드 모듈과 동일 풀인지 확인한다. |

`RawPathStartState&`는 root의 현재 전역 묶음을 직접 참조한다. 기존 `RawPathStart`가 내부에 보관하는 상태도 원래 각 필드의 참조를 유지한다. `RawPathProcessState::clock`과 Start 시계는 모두 Core의 실제 `processState`와 같아야 한다.

현재 명령 guard의 raw `+11 &9`는 free/contained를 거부한다. dead2/void4는 이 guard만으로 거부하지 않으며, 실제 runtime 명령 caller가 초기 Pop 후 살아 있는 사제를 공급하는 전제다. 기존 Init의 void4 부모 Pop 접두는 변경하지 않는다. 죽은 부모의 명령 완료 근거로 확대하지 않는다.

별도 RunFrame이나 걷기 loop는 없다. 월드의 기존 `RunFrame`이 같은 Kernel을 실행한다. 공간 효과는 월드의 실제 Pop/Unpop/Repop이고 Init/Search/보간/프레임/도착/자기삭제 관찰은 기존 원본 복원 모듈이 실행한다. 같은 `hash.Entries(0)`, spot, type, frame, SHP를 빌리며 좌표 캐시/ground 사본/직선 경로 대체가 없다.

## 생성 순환과 수명

root가 사제·Halt·Arrival·cleanup·lookup·processState/searchState를 먼저 구성한다. CarrierArrival의 currentPath callback은 후설치될 연결부 포인터를 조회하게 한다. 새 연결부를 만든 뒤 첫 저장 객체 Pop와 명령을 허용하면 같은 실제 CurrentRuntime을 사용할 수 있다. 미설치 callback 선택은 명시 오류로 처리해야 한다.

경로 callback이 이 연결부를 참조하므로 종료/교체 전에 `RawSurfaceWorld::ClearProcesses()`로 같은 Kernel을 비우고 그 뒤 경로 연결부·도착/사제 모듈을 해제한다. 생성 객체의 예외가 기존 부모/경로를 이미 변경했을 수 있으므로 caller가 예외를 성공으로 바꾸면 안 된다.

기존 Start는 실패 후 한 번 재귀가 성공해도 바깥 호출에서 nullptr를 반환할 수 있다. 이 원래 계약을 유지하며 명령 caller는 반환값과 별개로 `CurrentPath`를 현재 Host에서 다시 조회할 수 있다.

## 필수 caller 효과와 실제 자산

`RawCampaignPathEffects`는 현재 speed/turningSpeed/defaultFrame, stepSound 조회/요청, Start 실패 겹침 정리, 도착 겹침 정리, 큰 운반 재탐색을 명시적으로 받는다. 필수 callback 누락은 생성 전에 거부한다. CD의 별도 관찰시각은 기존 Process가 미연결 시 사용하는0 계약을 유지한다.

소리 번호와 실제 메타데이터는 원래 타입 자산에서 조회해야 한다. 낮은 HP/중립 귀환/포획/운반/경제/RPC/큰 회피는 이 클래스가 복제하지 않으며 실제 Halt/Arrival/cleanup의 caller hook을 통해 연결한다. 아직 복원되지 않은 분기가 선택되면 예외를 유지한다. 사제 전체 첫 Pop/회복/보호막 상태 요구는 별도 `cpp-campaign-priest-runtime-audit-20261014.md`에 기록했다.

Transmission의 `other`는 다른 프로세스의 WriteData fallback이다. 송신 자체는 이 연결부에 준 동일 transport를 사용하므로 Gun/Regular 등의 writer도 같은 packetbuffer에 써야 한다. 다른 버퍼에만 쓴 뒤 Path 전송 버퍼를 송신하는 연결은 지원 계약에 포함하지 않는다. fallback 누락이 실제 선택되면 기존 writer가 명시 오류를 보고한다.

지원 부모는 실제 사제158/양 판본 실제 VT다. 다른 부모/type23·골렘40·Dude24의 전체 명령 완료를 주장하지 않는다. 내장 프로세스 이름 표의 개수53/52와 사제 프로세스 번호54는 서로 다른 수치다. 이전 원본 selector/Start 관찰과 같은 번호54를 사용한다.

## 근거와 현재 상태

이 연결부는 **새 원본 입력0**, **새 C++ 검사0**인 준비 소스다. 기존 Start512/Helper718/BFS/Process/Cleanup/Bridge640/Halt864/Carrier406 관찰을 재합산하지 않는다. 기존 생산 몸체 수정0이다. 새 class에 대한 공유 C++ 컴파일 및 실제172 설치/같은 Host 명령 소비자 검사는 root의 다음 단위에 연결될 예정이다.

`tools/analyze_campaign_path_bindings.py`는 새 파일과 현재 실제 소비 모듈의 전체 SHA를 기록하며 이전 Carrier/사제 문법 successor를 감사한다. 이 정적 연결 감사만으로 원본 몸체 대조나 캠페인 완주를 완료로 표시하지 않는다. 원본 전체 PathStart 거의올림 bias 경계 차이는 별도 후속 과제다.

## 중단 시 인수인계

사용자의 현재 검사 마감 뒤 인수인계·커밋·중단 지시에 따라 이 준비 소스는 **미등록/미빌드/새CPP0** 상태로 동결한다. h/cpp는 준비 보고서를 만들던 전체 SHA와 같다. 단위14 Host 검사의 Factory 입력과 cleanup 연결이 이후 교정됐으므로 이전 준비 보고서의 문법 successor 참조는 당시 Host 검사 view에서 재현한다. 직접 `analyze_campaign_path_bindings.py --verify`를 현재 Host 입력으로 실행하면 이전 문법 검사 SHA가 달라 거부하는 것이 맞다. 보고서를 덮지 말고 재개 시 최신 cleanup successor와 root의 실제 공유API 변경을 별도 새 successor로 연결한다.

당시 준비 근거는 `priest-arrival-before-factory-input/cpppj/tests/PriestPathArrivalHostTests.cpp`의 전체 바이트를 current Test read view로 공급하면 원래 `expected()`와 보고서가 다시 일치한다. 이 역사 view 감사는 성공했으며 준비 source h/cpp 변경0이다. 현재 단위14의 최신 검증 CLI는 `analyze_priest_arrival_cleanup_binding_lineage.py --verify`다.

재개 시 root의 `RawCampaignWorldRuntime`이 장기 사제/Halt/Arrival/cleanup/lookup/StartState/ProcessState/SearchState를 소유한다. 표면 월드의 사제 가상 소유자 callback, 로딩 깊이/현재 Player 구분, 실제 metadata/stepSound/겹침 정리/큰 retarget, 같은 패킷 버퍼의 Gun·Regular·Path writer를 공급해야 한다. 중립 귀환/포획/큰 운반과 PathStart bias 경계는 아직 별도 미완료다. 같은172 설치와 실제 사제 명령 소비자 검사가 끝나기 전 전체 캠페인 연결 완료로 기록하지 않는다.
