# dotnetpj 0.541 — 장기 ProcessHost 기본 장치와 현재 호출 수명

2026-11-15, HJOW-Athlon. 직전0.540은58818d657다. 기준은cpppj o/SquidProcess.h/.cpp의417f22d2967decbf2ad3a587c2190b9efb93f772다. 현재 선택 DWORD를 ProcessHost로 전달하려면 원문 defaultAssets_ 기본 장치 자체가 필요해 해당 모듈 묶음을 먼저 이식했다. 원본 바이너리·디컴파일 산출물은 변경하지 않았다.

## 장기 기본 장치와 현재 선택

BindDefaultLifecycle은 같은 저장소의 실제 공통 삭제·Regular·자산 getter를 최초 한 번 연결한다. BindDefaultFormFactory는 실제 공장/Pop 깊이 getter를, BindDefaultFormTransmission은 완전한 writer/전체writer/패킷/송신과 진단을 값 복사한다. 연결할 때 생성·Pool 조회·Kernel 실행·World 대여를 만들지 않는다. 현재 World가 있으면 그 장치를 고르고, World 자체가 없을 때만 장기 기본 장치를 사용한다. 현재 World의 누락 필드를 기본 장치로 대체하지 않는다. 직렬화는 원문과 같이 현재 묶음 전체가 비었을 때 기본 묶음을 고른다.

Create/Take·Attach·Unpop·SharedRegular 생성의 실제 소비 위치가 같은 공장·현재 진단·현재 깊이를 선택한다. 공장은 같은 Pool인지 조회할 때 확인한다. 기존 직접 Host의 독립 경로와 선검사 계약은 유지한다. 기본장치가 결합된 Host의 Native Kill은 권한 관문을 먼저 지나고 최초 Form을 저장한 뒤 유효/죽음 단언의 정상 복귀를 이어가며, 현재 실제 가상 분배기로 삭제를 보낸다. 가상 분배기의 자산/다리 재정의를 root 공통 삭제로 우회하지 않는다.

전체 선택 getter는 defaultAssets_가 연결된 경우에만 만든다. 호출 시 같은 현재 대여를 잡고 현재 자산의 전체 DWORD 함수를 읽으며 누락을 0으로 바꾸지 않는다. 이미 만든 훅도 다음 현재 World와 다음 기본 선택을 읽는다.

파생 전체 writer/packet은 한 호출의 함수 사본을 유지한다. writer 콜백이 현재 packet을 재연결해도 선택한 이전 packet 함수로 같은 결과를 복사한다. 부모 SID 진단은 최초 호출 대여의 장치가 비었으면 실제 장기 진단을 사용한다.

## 실제 호출 소유자와 잔류

ProcessHostRuntimeBindings.Pin은 실제 외곽 World/EditorRuntime owner를 반환하는 getter다. RuntimeCallGuard는 pin을 먼저 실행하고 ActiveCalls를 증가시켜, 정상/예외 복귀까지 owner의 강한 참조를 유지한다. 반납은 참조만 놓고 소유자의 Dispose를 대신 호출하지 않는다. Pin 자체가 실패하면 ActiveCalls를 올리지 않는다. Shutdown drain도 같은 현재 호출 대여를 잡는다.

잔류 프로세스의 RuntimeLink는 원문 std::weak_ptr처럼 약한 참조로 바꿨다. 프로세스가 이전 World 대여와 캡처를 계속 소유하지 않는다. 현재 호출 동안의 실제 소유자 보유와 호출 밖 잔류의 약한 대여를 구분했다.

## 검증

새12검사는 장치 최초/lazy 연결·현재 World 부분 연결 우선·공장 Pool/실제 생성·NoWorld 실제 Form 부착/Regular/Native 삭제/FIFO 이력·권한/복귀 Kill 진단/최초Form·현재 전체 선택/Pin·직렬화 값 복사/부분 묶음·writer 중 재연결·장기 Shared 깊이/복귀 진단·pin 순서/예외/중복 반납·실제 owner 보유·이전 대여의 약한 잔류를 검사한다. 기존 관련74건과 합쳐 최초86/86 통과했다.

장면은 실제 Pool/Factory/Kernel을 사용한다. TYPE2의 ProcessForm 생성자 주소를 원문 OriginalConstructors에서 가져와야 한다. 비용/규칙 없는 빈 레코드의 생성자0은 별도 기본 Squid이므로 초기 입력의 잘못된 생성자 주소를 고쳤다. 삭제 이력은 고정20칸 물리여서 활성 한 건과 나머지19칸을 따로 검사한다. 기대값/오라클을 제품 결과로 재생성하지 않았다. 새 PE 관측 fixture는 없고, 합성 현재 계약을 독립 PE 관측행으로 세지 않는다.

실제 owner 보유 생략 변이는 현재12검사 중1개, 전체 선택16비트 축소는2개, writer/packet 사본 제거는1개 실패로 검출했다. 제품 두 파일을 매번 원래 바이트/SHA-256으로 복원했다. 이후 전체 비증분 Release 빌드 경고0·오류0, 관련 Core494/494·전체 Client 일반1232/1232가 실패0·각 종료0으로 통과했다. Client 목록1233 중 실장치 오디오 Explicit1은 미실행이며 실제 오디오 재생 통과로 세지 않는다. 자료는 extracted/dotnet-process-devices-0541-initial/·-mutations/·-final/이며 Git 제외다.

## 다음 연결

제품 Host에는 이번 기본 Native 장치 모듈을 아직 활성화하지 않았다. 공통 Form 해제의 현재 VT/물리·복귀 진단(FormUnpopHooks.Assertion 포함)이 C#에 아직 없고 실제 공장 Unpop 소비자도 그 원문 부모를 함께 이식해야 한다. 표면 connector/Native 다리, Depend/Content/Gump 현행 몸체 차이, damageable/해방·사제 상위 배치/삭제가 후속이다. 분리 기본 장치 파일의 header/몸체 두 PORT 쌍만 추가했고 기존 Host/프로세스 전체 base는 미이식 부분 때문에 올리지 않았다.

PORT_MAP865행(L1 418+port447), fixture294개 유지·전체floor/reflected 불변이다. 새 게임창/금지 실제 플레이/MP 재현 없음. 보호 원본/CD·AGENTS·README개요·cpppj·다른 프로젝트는 변경하지 않았다. M1 전체0/34·목표active다.
