# Session의 Flyer Process·Frame·Target 결선 (WB04-E611)

기준선 `225553df` 생성자1669의 `ConnectFlyer`2196..2260과 내부
`ConnectFlyerFrame`2275..2351·`ConnectFlyerTarget`2353..2367을 연결했다.
현재 SID 진단2262..2272와 방송2369..2373도 같은 현재 owner를 사용한다.
Session.cpp·RawFlyerProcess/Frame/Target·RawSunFlyerFrame·RawWindFlyer의
기준선 뒤 변경은0이다. Native 장기 Wind/자산 통지는 실제 Client 결선에 남았다.

Process의 가상 Frame·통지/소실·실제 Host/raw 이미지와 부착 소유권을
기존 몸체에 연결했다. 기본 생성/부착 뒤 같은 객체의 전용 표를 쓴다.
후처리는 같은 미부착 Flyer/Sound/Regular 예약과 현재 시계·공간·그림자·
프레임/소리 TYPE 메타를 사용한다. JS 신규 메모리의0 초기값은 플랫폼
할당 특성이며 CRT 미정 힙 바이트를 재현하거나 별도 난수로 합성하지 않는다.

Frame은 같은 현재 Gun/HP·입자·일반 finder·의존 watcher·현재 홈의
Host 이미지를 사용한다. Sun과 Wind 전용 몸체는 기존 구현에 연결했다.
Target은 단일 현재 finder 커서와 실제188TYPE 사거리·같은 Host 부모를
빌린다. 원문 Sun 폭탄 구간의 `explosionEnd`에는 B.count를 그대로 보낸다.
이름을 보고 first+count로 재해석하지 않는다.

`captureCampaignFlag`는 RawCheckingFlag 값형 복사 때 선택된 포인터와
지역 DWORD를 구별한다. 복사 뒤 원래 struct의 지역 값 대입은 기존
복사본을 재연결하지 않으며 같은 전역 DWORD의 현재 값은 계속 읽는다.
SID 상세 진단은 실제 Pool의 상대 바이트 주소를 사용한다. 실제 호스트
포인터를 합성하지 않으며 TYPE 선행 읽기와 같은 장기 진단 상태를 유지한다.
86h writer의 미작성+8/+16도 JS 할당 바이트 그대로 두고 별도 의미를 만들지 않는다.

신규5건은 Flyer 조립/현재 참조/불변 메타3건과 공통 플래그 복사2건이다.
최초 집중17건 중16건 통과·1건 실패는 AiDebug의 signed 값과 동일한
DWORD 비트를 unsigned 수치로 비교하던 시험 오류였다. `>>>0`으로 같은
비트를 확인하도록 고쳤으며 제품 코드는 바꾸지 않았다. 정적 재대조에서
movedCell의 원문 Flyer 관문을 세 TYPE/VT 조합 모두로 맞췄다.
기존 관련 부품을 포함한 집중110건이 통과했다. 신규 공중3종 전용
생산/생성/우클릭/행동·프레임 실행 검사는 추가하지 않았다.

기존 원본 fixture를 기존 부품 시험으로 재생했으며 새 fixture는0이다.

| fixture | 행 | SHA-256 |
| --- | ---: | --- |
| flyer-postpop | 1,421 | `111beec5675ad2fcd8edc4fd2fb7b5dbe2c129d8b3c796f58b68ec3a7c8d456f` |
| sun-flyer-postpop | 391 | `c7693afd860973b51efad4321894bbfddec6eb058dea041dac0e56307ca22bda` |
| flyer-frame | 744 | `627270d186495d6929cac8a1d4e74c29e92c69294558e020b7d774854b0dfc85` |
| flyer-target | 1,302 | `6a8865b9ab3046b748e1776256e822a50dcfc0938acdad3a17764da9e25fc2d2` |
| sun-flyer-frame | 410 | `aa3a4fd3f4e69b528f63dd070bdcd20e009ac77404a6f28d66c7d9bd8820c5f4` |
| sun-flyer-clear | 30 | `fdd6b87b20e60b765830b3d2114a3c4a1a002914f58ade557019e87ff42ca307` |
| wind-flyer | 334 | `f405cf9f4d89da2daab5f3746838393fb2d90d4208e4e3d749dcda8accfb1aaa` |

Node v22.23.3·v26.13.0 전체는 각각5,876건 통과했고 실패·취소·건너뜀은0이다.
두 실행 exit0·대응표 오류0·UTF-8/구문/diff 검사를 확인했다.
웹 JS132,283줄/검사781파일·manifest806항목이며 port846/partial39/todo122·session0을 유지한다.
메모리 경쟁을 줄이기 위해 두 전체 실행에 `--test-concurrency=4`를 주었으며
검사 파일·검사 범위는 기존 전체 명령과 같다. Node24는 기존 사용자 지시로 생략한다.

실제 World 게시1825·Regular 분배/수신/Load·Native 장기 대여와
공중3종의 실제 생산/비행 검증은 남았다. 이번 조립 검사로 실제 비행을
검증했다고 기록하지 않는다. 다음은1670 `ConnectFence`1911..1965다.
보호 원본/cpppj 변경·디컴파일·게임/브라우저 실행·실제 교전/포획/희생은
하지 않았다. 검사 로그 접두는 `extracted/wb-e611-`이다.
