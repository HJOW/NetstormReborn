# 캠페인 Motion·Path 실제 연결 (WB04-E568)

기준선 `225553df`의 `client/RawCampaignMotionBindings.cpp/.h` 137줄과
`RawCampaignPathBindings.cpp/.h` 281줄을 같은 작업 단위에서 옮겼다. Motion이 실제 도착/정리
몸체를 소유하고 Path가 이들을 호출하며 Carrier 도착이 다시 현재 Path를 조회하므로 함께 조립했다.
기준선 뒤 cpppj 소스 변경은 없다. cpppj·보호 원본·공통 인계는 수정하지 않았다.

## 실제 연결

- 같은 Pool/장부/spot/현재 TYPE·프레임·Host·Kernel·clock과 실제 사제/HP/Carrier/섬/Graph/내용물
  모듈을 검증한다. C++ 참조는 같은 객체/메모리 보기, 함수 묶음 값 인수는 사본으로 구분한다.
- Motion은 기존 RawCanonPlacementTerrain(RegionAt만), RawPathCleanup, RawPriestHalt,
  RawCarrierArrival, RawPriestArrival를 조립하고 마지막 Path 생성으로 순환 조회를 닫는다.
  큰 포획/의식/경제/귀환 효과는 공급된 실제 소비자를 요구한다. 빈 성공으로 대체하지 않는다.
- Path는 기존 RawPathStart·RawPathSearch·RawPathProcess/Runtime·RawPathData를 사용한다.
  실제8개 cdecl 생성자는 주소로 고정한 종류를 선택하고 현재 mutable 번호와 구분한다.
  사제/건설/운반/Dude 부모의 TYPE/VT/상태/이동 종류 검증, 현재 Form/PID/Runtime 조회,
  대상 setter와 재탐색, 일반/강제 writer·같은 버퍼·Native 장치 계약을 연결한다.
- 30h 수신은 boss 진단(복귀 가능)→실제 WORD/SID→free 생성자/decoder/Prepare/Attach→현재
  첫점 부모 재배치 순서다. live Form은 현재 TYPE 이름의 상세/진단 뒤 갱신하지 않는다.
  수신 trace는 현재 GameAux.PathTraceLevel을 매번 읽고 첫5105 Regular만 해제한다.
- RawSurfaceStorage에 같은 RawPathSearchStorage 거리표/큐/initialized를 장기 보관한다.
  일반/준비 대여 때 초기화하지 않는다. GameAux에는 보호 PE00540c68의 signed 초기99를 추가했다.
- BFS의 기존 독립 입력은 코드 배열이고 실제 World는 FrameCodeTable이다. 현재 프레임 getter에서
  `.codes` 또는 기존 배열을 직접 읽도록 연결했다. 거리/경로/지면/프레임 코드 사본을 만들지 않는다.

## 검증과 범위

`tests/campaign-motion-support.mjs`는 E567의 실제 보호 TYPE/SHP/사제/자산 월드에 같은
Graph 기준점·내용물 생성·Path/clock/음향/송신을 더한다. `CampaignMotionTests.cpp`의 실제 모듈과
큰 외부 효과 경계를 따른다. 경로 생성·공간·Host/Kernel·도착/cleanup은 실제 모듈이며
소리/전송은 관찰 장치, 미공급 큰 하위는 명시 거부다. 게임 실행/원본 관측을 새로 했다는 뜻이 아니다.

새 조립/프로토콜 검사15건과 장기 검색 저장소 수명1건을 추가했다.

1. cpppj CheckCampaignMovement의 경로 루프를 보호 사제 owner1/2에서 실행한다.
   실제 BFS·Start·같은 clock의 Path Frame·도착/cleanup/Form 반납을 확인했다(각330프레임).
2. 같은 Pool/장부/clock/현재 Core·필수 장치, 부모 TYPE/VT/상태·이동 매트릭스,
   8고정 생성자와 mutable 번호, 후보 BFS/장기 거리표·큐 공유.
3. 실제 기존 경로 교체와 현재 PID/Runtime, 두 DWORD 대상, 삭제 가능한 retarget과 함수 사본.
4. 일반/강제 writer·같은 버퍼/현재 헤더·외부 송신·다른 프로세스 fallback, Native 장치 계약.
5. 실제 송신 경로의 free 수신/재배치·현재 trace·첫5105만 삭제,
   live Form 무갱신·boss 진단 복귀·NUL/255BYTE 상세, 물리 WORD/SID/생성자/payload 오류.
6. 일반 acquire/반납·준비 acquire 뒤에도 같은 거리표/큐/initialized 유지.
   추가한 PathTraceLevel을 포함한 GameAux57칸은 보호 PE 초기값과 대조한다.

최초 CLI 조립에서 실제 FrameCodeTable을 독립 코드 배열처럼 읽던 BFS 연결 오류가 드러나
현재 코드 getter의 두 입력 표현을 연결했다. 최초 집중17건 중1건은 시험이 목적지30.5를 그대로
최종 위치로 기대해서 실패했다. 준비된 원문 경로는 정수 칸 `[24,24]→[31,31]`이었다.
첫 꼭짓점을 끝점으로 본 후속 기대도 바로잡아 실제 마지막 꼭짓점 도착을 검증했다.
이 교정은 시험 기대와 입력 표현 연결이며 원문의 BFS/이동/도착 알고리즘 변경은 없다.

최종 집중 검사(새 조립15건·Storage12건·GameAux5건) **32건 통과**.
집중 coverage는 Motion 줄100%/분기95.45%/함수58.33%, Path 줄99.09%/분기87.06%/함수95.45%다.
운반·건설·Dude의 큰 외부 Halt/Arrival/공간 효과를 모두 실행했다고 세지 않는다.
해당 기본 경로/부모 몸체의 기존 원본 fixture 대조는 전체 회귀에서 계속 수행한다.

manifest는 기존801항목과 실제 `cpppjBasis`를 유지하고 웹 모듈 변경 이력만 갱신했다.
`pathsearch`는 기존645행(원본10.78/CD 혼합, 웹은 해당10.78 행만 대조), 정규화 SHA-256
`26c697920661be2020f8e43aafb07fecd6099bd67962c94f1d16a367314b9f42`다.
`pathinit`는 기존65행(두 판본 중10.78 선택), 정규화 SHA-256
`3425807811a9e34d34dd2dc69896ced9c133cf4bb4887b23786477153a6ca2ee`다.
새 원본 관측으로 기록하지 않았다. 집중 로그는 `extracted/wb-e568-focus1.log`~`wb-e568-focus4.log`에 있다(Git 제외).

Node v22.23.3·v26.13.0 전체 검사는 각각 **5,372건 통과, 실패·취소·건너뜀 0**이다.
기존 원본 fixture 대조를 포함한다. 로그는 `extracted/wb-e568-node22.log`·
`extracted/wb-e568-node26.log`에 있다(Git 제외). `port-map.mjs`는 1,160행 오류0,
port836·partial33·todo135·제외156, session/oracle0이다.

## 남은 실제 통합

현재는 부품 `port`이며 실제 Session/앱에 설치하지 않았다.
cpppj CampaignMotion의 `thewarbegins`·`masterofwhirligigs` 저장 사제/자유 목적지 선택 및 전체
초기화 시험은 D/E Session 설치 뒤 L3에서 재대조해야 한다. 단위 시험의 실제 보호 사제/정사각
지면을 저장 미션 전체 설치와 같다고 세지 않는다. 공중 유닛 전용 플레이도 수행하지 않았다.
Native 장기 장치·그 밖의 운반/의식/경제 소비자 설치는 기존 D/E·6·7 단계 몫이다.

다음은 D 구획 `RawCampaignSession`의 구성/Load가 아직 요구하는 바인딩을 소스로 다시 판정하고,
필요한 선행만 앞당긴 뒤 실제 저장 미션 설치로 시험 장면을 넓히는 작업이다.
