# 입력 앞단의 창 안팎 기록과 재진입 release 보정

## 범위와 기준

2026-10-23, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex**다. AGENTS·공통/닷넷 인수인계를 다시 확인했고, 기존 정밀 원문 `extracted/refined/originals/Netstorm.c`와 보호10.78 PE를 재사용했다. 새 디컴파일·보호 파일 변경·게임/GUI 실행·빌드·CMake 수정·커밋은 하지 않았다. 아래 C++ 검사는 새로 작성했으나 이 에이전트가 실행하지 않았으며, 제품 연결/등록/정식 빌드는 상위 작업에서 맡는다.

`GumpPointerBoundary`의 범위는 UserInput의 **`004d6379`부터 `004d65a1` 직전**까지다. 앞의 디버그500객체 생성, 바로 뒤의 Guide 폴링 사건 호출, 카메라·Overlay·Broadcast와 일반 입력 큐 드레인은 포함하지 않는다. 원본 입력 전체가 복원됐다는 의미가 아니다.

원본은 **밖으로 나갈 때 좌우 상태를 기록하고 안으로 재진입할 때 놓친 release를 큐에 보충**한다. 밖에 있는 매 프레임에 release를 만들거나 Gump 캡처를 직접 Cancel하는 처리가 아니다. 중간 버튼 보정은 없다.

## 현재 포인터 폴링

원본 `004d6379`는 현재 언어 전역 `00542384 == 3`을 검사한다. 이 전역은 InterpretOptions의 `currentLanguage` 이름 해석 결과로 설정된다(`Netstorm.c` 43279~43283). 임의 입력 장치 모드나 마우스 버튼 수가 아니다.

| 실제 호출 | 요청 code | 결과 사용 |
|---|---|---|
| `004d6398` 또는 `004d63fd` → `00452e00` | `00120000` | Alt(VK18)의 현재 상태와 포인터 좌표12BYTE를 첫 사건으로 복사한다. 일반 언어에서는 이것이 현재 pointer다. |
| 언어3의 `004d63bb` → `00452e00` | `00110000` | Ctrl(VK17)을 추가 폴링한다. 둘 다 bit31이0이면 첫 사건 code에 `80000000`만 OR한다. Ctrl 좌표로 첫 좌표를 교체하지 않는다. |

`00452e00`은 code에서 가상 키를 해석하는 기존 OS 폴링 본체다. 반환 bit31은 현재 놓임, bit30은 지난 확인 뒤 누른 적 있음, bit26은 창 밖/비활성이다. 이 모듈은 폴링 결과의 code/x/y를 그대로 받고 자체 키·좌표 정규화는 하지 않는다. Alt의 pressedSince/outside/modifier를 지우지 않으며 언어3의 결합에서도 첫 code의 release만 더한다.

조사 전 실제 `UberGump::Input`은 `Client::Poll(kLeftButton)`으로 첫 snapshot을 얻었으므로 이 원본과 다르다. 연결 시 `PrepareGumpPointerBoundary(state, static_cast<uint32_t>(client.LanguageNumber()), poll, client.Input())`의 **반환 사건**을 현재 pointer로 사용한다. 별도로 좌버튼 snapshot을 재사용하지 않는다. CLI inspectPoll도 Alt/Ctrl 요청에 같은 현재 좌표와 실제 놓임 상태를 공급해야 하며, 임의0 사건을 성공 값으로 쓰면 안 된다.

## 실제 정적 기록과 초기화

| 필드 | 원본 주소와 초기 상태 | 역할 |
|---|---|---|
| `initializationFlags` | `005cafe8`, BSS0 | 값1/2인 두 초기화 비트만 이 앞단에서 설정한다. 뒤쪽 edge/tooltip 초기화 비트는 그대로 둔다. |
| `wasInside` | `0054354c`, **PE 초기값1** | 이전 현재 pointer의 안쪽 여부다. 초기 BSS0으로 대체하면 첫 inside 프레임에 가짜 재진입 보정을 하게 된다. |
| `leftOutside` | `005cafdc/e0/e4`, BSS0 | 마지막 inside→outside 전환에서 폴링한 좌버튼 code/x/y다. |
| `rightOutside` | `005cafd0/d4/d8`, BSS0 | 같은 전환의 우버튼 code/x/y다. |

첫 현재 pointer/언어3 Ctrl을 얻은 **뒤** `004d6419`부터 초기화 비트를 읽는다. 좌 초기화가 빠졌으면 flags에1을 더하고 좌 기록 전체를0으로 만들며, 우 초기화가 빠졌으면 flags에2를 더하고 우 기록 전체를0으로 만든다. 전체 flags를3으로 덮어쓰지 않는다. poll callback이 이 전역을 변경한 경우 초기화는 그 뒤의 현재 값을 관찰한다.

## 전환 분기와 재조회 순서

`004d646a`는 이전 `54354c`의 비제로 여부를 읽고, 현재 pointer의 bit26으로 inside 여부를 구별한다.

| 이전/현재 | 원본 효과 |
|---|---|
| inside → inside | 추가 좌/우 폴링·큐 예약 없음. |
| inside → outside | `004d648c`에서 좌 폴링 → 그 결과를 좌 기록에 즉시 저장 → `004d64b4`에서 우 폴링 → 우 기록 저장. |
| outside → outside | 추가 좌/우 폴링·기록 교체·큐 예약 없음. 마지막 전환의 기록을 유지한다. |
| outside → inside | `004d64f4`에서 좌 폴링 → `004d6503`에서 우 폴링을 **모두 먼저** 실행 → 현재 좌 기록 비교 → 필요하면 좌 사건 예약 → 현재 우 기록 재조회·비교 → 필요하면 우 사건 예약. |

재진입 예약 조건은 해당 outside 기록의 bit31이0(pressed)이면서 새 폴링 사건의 bit31이1(released)인 경우다. 새 사건의 전체 code/x/y를 `004d6541`(좌), `004d657f`(우)의 `00452f90`에 전달한다. 별도로 release bit를 만들거나, 과거 outside 좌표를 쓰거나, `PushCommand`로 code를 바꾸지 않는다. 우 기록은 `004d6549`에서 **좌 queue callback이 끝난 뒤** 다시 읽으므로 좌 예약 중 우 기록을 바꾼 효과도 반영된다. 현재 우 폴링을 queue 뒤 다시 호출하는 것은 아니다.

마지막 `004d659b`는 현재 pointer가 inside면1, outside면0을 `54354c`에 저장한다. 중간 poll/queue callback이 이 필드를 바꿨어도 마지막 저장은 현재 pointer를 기준으로 덮어쓴다. 재진입 보정은 outside 기록을 새 release로 교체하지 않는다.

실제 `452f90`은 같은40칸 큐 끝에 추가하며 최대39사건을 유지한다. 기존 사건은 앞에 남고 좌/우 순서는 그 뒤다. 38개가 이미 있으면 좌 release만 들어가고 우 release는 포화 정책에 따라 버려진다. 이 모듈은 큐를 재드레인하거나 캡처를 직접 해제하지 않으며, 일반 Gump Route가 예약된 release를 소비한다.

## 바로 뒤 Guide 폴링 호출

이 모듈이 끝난 다음 원본은 `004d65a1`에서 같은12BYTE 지역 pointer의 주소를 push하고 `004d65a7`에서 `463e40(&562ca0, &pointer)`를 호출한다. **이 반환은 검사하지 않는다.** 바로 `004d65ac`의 `5cafe8` bit4 초기화 검사로 계속 간다. 반환1을 이유로 Overlay/Broadcast/일반 큐를 건너뛰면 원본과 다르다.

`463e40`은 Guide 명령 처리 뒤 현재 mission이 있으면 그 객체의 virtual+24에 같은 사건 주소를 넘긴다. 현재 복원 `RawMissionGuide`는 단계 Run을 소유하며 이 Handle 본체를 제공하는 클래스는 아니다. 실제 Client에서는 기존 현재 tutorial/mission의 폴링 Event를 호출하는 별도 연결이 필요하다. 폴링 사건은 일반 큐에 새로 Push하지 않는다.

일반 큐의 사건은 별개다. `004d6e6b`의 같은 Guide 호출 뒤에는 `test eax,eax`/`jne 004da477`가 있어 반환이 비제로면 현재 사건의 나머지 Gump/명령 처리를 건너뛴다. 따라서 앞단 폴링의 반환 무시와 기존 `UberGump::Event`의 Guide 소비 관문을 둘 다 유지해야 한다. 가상 mission 처리기가 pointer를 변경한다면 그 뒤 단계는 같은 변경된 사건을 사용하지만 `wasInside`는 Guide 호출 전에 이미 기록한 값이다.

## 새 API와 연결 수명

새 `GumpPointerBoundaryState`는 원본 전역의 실제 기록을 보유한다. 페이지나 월드를 바꿀 때 새로 만들지 않고 Client/UI 수명으로 유지한다. `GumpPointerBoundaryHooks::poll`은 필수이며 `queue`는 실제 release 예약 분기에 도달할 때만 요구한다. 누락을 빈 성공으로 대체하지 않는다. `InputQueue&` 오버로드는 기존 `InputQueue::Push`를 그대로 사용한다.

호출자는 현재 언어를 공급하고 반환 pointer로 Guide 폴링 → 입력 준비/조건부 Overlay → raw Broadcast → 일반 큐 드레인을 이어 간다. 별도 GameClock Pause나 Gump modal 상태로 이 앞단을 건너뛰지 않는다. MovieUpdatePause 검사도 원본에서는 이 입력 단계가 모두 끝난 뒤에 있다. 전체 프레임 배치는 [Gump 프레임 순서 감사](cpp-gump-frame-order-20261023.md)에 기록했다.

## 독립 원문 관측과 검사 경계

`tools/decomp_gump_pointer_boundary.py`는 제품 C++를 읽지 않고 보호 PE의 선택 구간과 **실제 `452f90` 전체**를 Unicorn으로 실행한다. OS 폴링 `452e00`만 명시 code/x/y 결과로 공급한다. 명시 poll/queue callback의 전역 변경도 입력으로 분리했다. 결과의 기대값을 제품 모듈에서 생성하지 않았다.

- 입력 **3,878개**, 두 x87 정밀도 **7,756회** 모두 Guide 직전 동일 EIP·동일 ESP·x87 상태를 유지했다.
- 실제 본체 **549,668명령**, 정밀도 차이0. 전체 UserInput 함수의 정상 RET나 GUI 통과를 뜻하지 않는다.
- 초기 비트0/1/2/3/상위 비트, 이전 inside0/1/비정규 비제로, 현재 밖/안, 네 좌우 과거/현재 pressed 조합, 언어3 Ctrl 조합, 음수 좌표 비트, callback 재조회, 기존 큐38/39개 포화를 포함한다.
- fixture SHA-256: `eb689894225910c60d163e4ca1198e7c932497913938a87c588998399b5026e5`.
- 재생성·SHA 확인 보고서: `extracted/cpp-gump-pointer-boundary-20261023-native.json`. `--verify` 종료0을 확인했다.

새 `GumpPointerBoundaryTests.cpp`는 macro **`NETSTORM_GUMP_POINTER_BOUNDARY_FIXTURE`**를 사용한다. `Native3878`은 최종 기록·실제 InputQueue 내용·poll/queue/mutation 순서를 전행 비교한다. `ReentryUsesCurrentRawQueue`는 실제 연속 outside→outside→inside와 기존 사건 뒤의 raw release를 확인한다. `LanguageAndReachedCallbacks`는 초기 inside1·언어3의 code-only 보정·도달한 필수 콜백을 검사한다. 이 에이전트는 C++ 빌드/검사를 실행하지 않았다.

실제 Client 등록 후에는 창 밖에서 놓은 좌우 버튼의 capture가 재진입 release로 풀리는지 GUI 회귀가 필요하다. 중간 버튼은 원문 보정 범위가 아니며 이 모듈에 임의로 추가하지 않는다. 원본 창 메시지의 보통 release와 이 보충 경계는 같은 입력 큐를 사용하지만 서로 다른 사건 생산자다.
