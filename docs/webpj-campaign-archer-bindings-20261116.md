# 캠페인 Archer·Missile·발사/회전 수신·전송 연결 (WB04-E567)

`client/RawCampaignArcherBindings.cpp/.h`(기준선 `225553df`, 223줄)를
`webpj/www/core/raw-campaign-archer-bindings.js`로 이식했다. 기준선 뒤 cpppj 변경은 없다.
선행 Host 공장은 E566, 실제 현재 Gun 조회는 E565, 피격/사제/자산 연결은 E561·E563·E564다.
cpppj와 보호 원본은 수정하지 않았다.

## 구현

같은 World/Core/Host·표적·게임 난수·시각을 빌리고 기존 실제 Missile·Archer·Gun 회전·
Cannon/Crossbow/Ice 발사·Disc Regular를 조립한다. 외부 함수 묶음은 C++ 값 인수처럼
복사하고 실제 World/표/상태는 같은 객체를 빌린다. 크로스보우 회전의 각도 진단 함수 묶음도
C++ 대입처럼 값으로 복사하며 함수가 읽는 현재 Client 전역은 공유한다. 선택한 미공급 장치는 명시 오류다.

- 여섯 부모 TYPE(71/128/131/140/148/129)·11단계 생성 분배. 원문 비교 순서와 첫 일치 복귀를
  유지한다. 실제 Gun/부모와 현재 장기 TYPE을 조회한다. 부착 실패나 미복원 파생을 숨기지 않는다.
- `prepareFrame`: 유한 게임 시각/경과, 음수 경과 거부 후 double 그대로 같은 파생 상태에 전달.
- Missile의 같은 Factory/owner/Pop/Unpop/중심/파괴·PRNG 연결. Archer 회전/발사와 아이스
  냉각·현재 shotContext/실제 MayTarget, Disc 예약은 기존 검증한 몸체를 재사용한다.
- `GameAuxState`: SunArcher/SunCannon/WindArcher 현재 DWORD(00541174/00541258/00541288),
  현재 발사/회전 메시지·프로토콜 BYTE(0054046e/6f·00540468/69), 같은
  `createSquidTransmissionState()` 공통2048BYTE owner를 한 번 소유한다.
  일반 reset이나 Session 교체를 위한 새 사본을 만들지 않는다. 다른 미이식 장기 칸은 그대로 남긴다.
- 3b/3c 수신: 물리 DWORD SID 검증, 새 Form의 원문 dirty 진단→현재 부모 TYPE 네 생성자→
  같은 decoder→명시 Form 부착. live Form은 진단 복귀 뒤 현재 PID를 다시 찾으며 생성/부모/시각을
  초기화하지 않는다. 원문 진단은 복귀 가능하며 앞선 쓰기와 다음 현재 조회를 유지한다.
- 일반/강제 writer: 현재 실제 파생 필드·VT·헤더로 같은 장기 버퍼에 작성. `packet`과 송신은
  같은 현재 버퍼의 보기를 사용한다. 다른 프로세스의 함수 사본 fallback과 선택 표식도 유지한다.
  Native 장기 Serialize/Broadcast 장치가 명시 공급되면 그 계약을 선택한다. 실제 장치 설치는 E 구획이다.

## 검증

전용 원본 조립 fixture는 없다. cpppj `CampaignFireTests.cpp`의 세 factory/clock loop와
`CampaignInstallSupport.h`의 실제 보호 TYPE/SHP/기하/피격
조립을 `tests/campaign-archer-support.mjs`로 확장하고 실제 World/Host/Kernel로 소스 계약을 대조했다.
크로스보우 Pop은 실제 Session의 `PatchGunGraphFromFrame`과 같은 Core.graph.setNumber를 사용한다.
초기 fenceMark가 없을 때만 cpppj 장면과 같은 invalid corner를 공급한다. 포구·치수·owner·자산 생성과
수신/일반 writer는 실제 모듈이며 소리/송신은 명시 관찰 장치다. 게임 실행이나 실제 플레이 관측이 아니다.

새 검사 17건(factory/clock의 cpppj 세 loop에 해당하는 1건 포함)과 GameAux PE/장기 수명 추가1건이다.
세 loop는 실제 void 부모·Gun·시각·파생·동일 Host Kill을 대조한다. 원문의 저장 미션/사제 선택과
`ActualInitialPositions`·`ActualWarBeginsLaunchAndFlight` 전체 Kernel/첫 이동 검사는 D/E Session 통합
선행이어서 아직 완료로 세지 않는다. 현재 launch 검사는 통제된 실제 World 자산 생성 범위다.

- Pool 대여/거부, 6종11단계, 현재 시각/TYPE·값 사본 fallback·실제 부착 실패.
- 네 수신 생성자·현재 process TYPE·패킷 시각, 현재 Sun/Wind TYPE 변경,
  live PID/선택 필드·진단 재진입과 머리/SID/선택 필드 오류.
- 현재 VT/헤더·dirty/force, 공통 버퍼 별칭과 범위, 다른 프로세스 fallback/누락,
  명시 Native 장치 계약, 실제 Disc Regular.
- Wind Archer의 실제 공유 회전/같은 PRNG·음향 함수 사본,
  Sun Disc launch의 실제 owner/포구/Pop/Missile Form,
  아이스 냉각 뒤 현재8DWORD/실제 MayTarget/Gun 완료·자기 삭제.
- 정상 SunArcher Gun의 회전 생략과 정상 경로 밖 강제 회전의 cpppj와 같은 명시 거부.
  GameAux 56칸과 두 헤더4BYTE는 보호 PE .data/BSS와 직접 대조한다.

최초 17건 중 7건이 실패했다. PE reader의 `{section,bytes}` 반환 해석, 명시 Form을
원문의 서버 SID 영역에 두어야 하는 점, 크로스보우 Pop의 미공급 각도→Graph 장치,
복귀 가능한 dirty 진단을 무조건 throw로 기대한 시험 입력/기대를 교정했다. 이후 17건 통과.
추가 통합 시험에서는 실제 SunArcher에 orientations가 없다는 원문 입력이 드러났다(아래).
공유 회전은 실제 orientations가 있는 Wind Archer로 PRNG를 대조하고 SunArcher의 오류는 별도로
고정했다. Fire 추적은 delta>0에서만 진행하고 아이스 문맥은 프레임 쓰기 전에 읽는 원문 순서로
시험 기대를 교정했다. Sun Disc에는 공통 firingSound가 없어 Archer 전용 소리와 구별했다.
기존 프레임 처리기나 보호 자료를 수정하여 검사를 통과시키지 않았다.

최종 집중 검사(새17건·GameAux5건) **22건 통과**.
집중 coverage는 본체 줄100%·분기97.97%·함수90.74%, GameAux 줄/함수100%·분기97.44%다.
이번에 실행하지 않은 Missile 비행/충돌의 외부 콜백은 Session의 전체 Kernel 시험에서 이어 검증한다.
manifest는 기존801항목을 유지하며 웹 모듈 이력만 갱신했다. 기존 `cpppjBasis`와 원본 자료는 보존했다.
`original-pe-data`는 보호 PE SHA-256
`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`,
`factory-initialization`은 기존577행/정규화 SHA-256
`77bf5a1893a8078b6b2b4113ab49315ce8e746d9105ae314a3f8a5282e74cea2`다.
초기 집중 로그는 `extracted/wb-e567-focus1.log`~`wb-e567-focus6.log`에 있다(Git 제외).

각도 진단 함수 묶음의 값 사본을 최종 대조에서 보강한 뒤 전체 검사를 다시 실행했다.
HJOW-Athlon에서 Node v22.23.3·v26.13.0 최종 전체 각각 **5,356건 통과, 실패·skip 0**.
전체에 대응표7건·manifest3건이 포함된다. 대응표 도구는 1,160행 오류0이며
port834·partial33·todo137·제외156, session/oracle0이다.
최종 로그는 `extracted/wb-e567-final-node22.log`·`wb-e567-final-node26.log`·
`wb-e567-final-focus.log`와 `wb-e567-port-map.log`에 있다(Git 제외).
초기 전체도 두 버전 각각5,356건 통과했으나 최종 판정은 값 사본 보강 뒤 재실행 결과를 사용한다.
Node v24 전체는 이번에 반복하지 않았다.

## 실제 Session 전에 남은 경계

RawCampaignArcherBindings는 부품 `port`이며 앱/Session·실제 Native 전송 장치는 아직 설치하지 않았다.
다음은 같은 생성자/Load가 요구하는 Motion·Path 등이며 D/E에서 실제 장기 소유자·L2를 조립한다.

**SunArcher 정상 회전 생략:** 보호 `.type`의 TYPE71은 orientations 속성이 없어
`ReadRawArcherTypeData`가 directions=0을 반환한다(물리35프레임). 원문 `RawGunProcess::Frame`
단계2는 group0이고 Wind Archer가 아닌 이 TYPE의 회전을 생성하지 않고 단계4로 진행한다.
추가 시험은 실제 Gun을 실행하여 이 정상 경로·프로세스 수 불변을 확인한다. API에서 정상 경로 밖
회전을 강제하면 기준선 cpppj와 웹의 `Rotate`가 같은 명시 오류로 거부한다. 최초에는 이를
미완료 플레이 경계로 해석했으나 상위 호출 조건을 확인해 그 해석을 교정했다. 임의 방향 수나
cpppj 수정을 도입할 필요가 없으며 새로운 해결 대기 항목으로 세지 않는다.
