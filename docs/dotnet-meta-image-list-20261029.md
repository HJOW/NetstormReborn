# dotnetpj R3j-3 — 실제 MetaImage 목록의 배치/선택/삭제

2026-10-29, HJOW-Athlon. AGENTS.md·현행/공통 인계를 다시 읽었다. 직전 목표 턴은 ImageGump/MetaImageGump2단위의 구현·검증·커밋을 완료한 진행이며 시작 HEAD는 `27075f9b`다. 현재/마지막 완료된 전체 정밀 디컴파일 PC는 HJOW-Athlon(2026-10-23)이다. cpppj MetaImageList.h/.cpp·MetaImageListTests.cpp·RawMenuImageListTests.inc와 기존 실제 MetaImage/Gump/VFX를 사용했다. 게임/창/PE 에뮬레이터·금지된 실제 플레이 재현·멀티플레이 검사는 실행하지 않았다.

## 1. 구현

실제 기본 Gump 뒤의21칸 자식/별도ID·count/선택/열 수·두float 셀 크기를 옮겼다. 생성은 임의의 생성 전count도 초기화하지만 ID 배열과 두 셀 크기는 쓰지 않는다. Add는ID 쓰기→실제 MetaImage 생성→현재count 칸에 자식 설치/증가→자식 부착→목록Push/현재위치 재부착 순서를 유지한다. 한도21이면 쓰기/생성을 생략한다. 생성 실패 앞의ID 쓰기는 되돌리지 않는다.

가로 배치는 전체 폭을count로 나눈 뒤 셀 높이를 목록 높이와 비교한다. 격자는 열 수와 실제 마지막 부분행을 사용한다. 단계/누적 좌표는 각각 원문float32 저장 지점에서만 좁힌다. 자식Push/Attach 뒤 현재count·자식 토큰/셀 크기를 다시 읽고 실제 MetaImage.Attach가 원본 reciprocal로 축소한다.

적중은 오른쪽/아래를 제외하며 표시scale을 계산에 추가하지 않는다. 가로 경로의count*dx는32비트 정수로 먼저 감기고 격자는index==count도 원문처럼 반환한다. 좌누름은 선택/해제를 토글하고 화면 밖 클릭도 현재선택과 비교해 부모에 전체DWORD 인자를 전달한다. 체크무늬의 오른쪽/아래는 절삭된 왼쪽/위쪽에 셀 크기를 더한 뒤 다시 절삭한다.

Remove는 지연Kill 뒤count를 줄이고 두 배열의 유효 접두부만 압축한다. 선택/마지막 중복 포인터·ID는 남긴다. 목록/MetaImage는 공통 자식 우선 수명으로 반납하지만 RawImage는 외부 소유이므로 해제하지 않는다. 실제 트리/기본 수명/경계/부모사건·체크무늬 픽셀 및 현재 목록의 가상Attach/입력/Draw/scalar 분배도 연결했다. 자식 할당/현재 녹색과 전체 창 소유자는 상위에서 공급한다.

## 2. 독립 원본 대조

읽기 전용 `meta-image-list-x86.tsv` **514행**을 Link했다. 앞15열만 실제 입력/명시 장치 효과에 사용한다. 뒤11열의 결과는 비교에만 사용한다. 실제 RawImage.Convert·MetaImageGump 생성/부착·GumpCreate/기본 소멸자를 사용한다. 기본부착에서 목록 크기가 바뀌거나 첫 자식Push에서 현재count/셀 크기가 바뀌는 입력도 실행한다.

원본 두 제어모드의 전체0xbc tail·22개 실제자식의7개float 기하·기본등록/일련번호/선택·효과순서/반환이 최초부터 일치했다. 자식 주소는 기존 C++ 대조기와 같은 논리 순번으로만 정규화한다. 원문 명령 수는 자료 메타 검사이며 관리 코드의 실제 명령/ABI 일치가 아니다. 원본 기대값은 수정/재생성하지 않았다. 전체 기본노드의 미작성 바이트 관찰이나 전체 화면 판정으로 확대하지 않는다.

fixture106개·새 원문515행/SHA fb7a7b8465a6이며 reflected-commit은 `f95a8db2...` 유지다. 포트228행(L1 111/port117)이고 새 본체2행의 기준은 `27075f9b`다.

## 3. 실제 공통 연결 검사

집중 **8개 통과**다. 원본 전수1개 외7개는 실제21개 자식의 별도ID/같은Image·공통등록/부모/기하/한도, 열0/-3의 가로배치2개, 실제입력/부모선택 사건과 해제·화면밖 인자, 현재전체DWORD 녹색/체크무늬·공유클립/guard, 실제삭제의 접두부압축·끝슬롯/선택 보존과 지연Cleanup·자식우선반납/빌린Image 생존, 생성의 미작성 필드/실패한Add 선행쓰기·범위/판본 거부를 확인한다. 수명 검사는 반납한 노드를 실제 저장소에서 제거한다.

Client Release 컴파일은 경고/오류0이며 집중검사는 수정 없이 통과했다. 전체 **1,916개(Assets363+Core1030+Client523) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. Git 제외로그는 `extracted/dotnet-meta-image-list-native-initial-20261029.log`, `dotnet-meta-image-list-focused-initial-20261029.log`, `dotnet-meta-image-list-tests-20261029.log`, `dotnet-meta-image-list-build-20261029.log`다.

## 4. 다음

BackgroundGifGump의 원시 GIF 버퍼/소유·현재 치수/입력·실제복사와 GIF 판독기/자산 연결을 진행한다. 이어HTML/대화창·실제메뉴 상위 소유/분배·전체자산/ConfigInterface/음향/호스트와R4 이후 전체미션/세션/월드/HUD/Edit·후순위멀티플레이 사전구현을 잇는다. 전체 목표는 계속 진행 중이다.

실제 전체메뉴/게임캡처0·기존코드/검사은퇴0이다. cpppj·원본/CD·AGENTS.md·공통인계·외부TODO.md는 수정하지 않았다.
