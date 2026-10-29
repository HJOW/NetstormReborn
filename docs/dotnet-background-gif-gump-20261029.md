# dotnetpj R3k-1 — 배경 GIF 창과 실제 원시 버퍼

2026-10-29, HJOW-Athlon. AGENTS.md·현행/공통 인계를 다시 읽었다. 시작 HEAD는 `a1680825`이며 마지막 완료된 전체 정밀 디컴파일 PC도 HJOW-Athlon(2026-10-23)이다. cpppj BackgroundGifGump.h/.cpp·GifImage.h/.cpp·RawBackgroundGifControls.h/.cpp와 BackgroundGifGumpTests.cpp·BackgroundGifClientTests.cpp를 읽었다. 보호 파일은 읽기만 했고 게임/창·PE 에뮬레이터·금지된 플레이 재현·멀티플레이 검사는 실행하지 않았다.

## 1. 구현과 경계

`BackgroundGifGump`는 동일 기본/파생 노드에 생성→현재 종류→비트맵0→Refresh→현재 종료 사건/인자0을 쓴다. Refresh는 이전 버퍼 해제→현재 비트맵0→경로 치환/존재 조회→실제 적재→현재 비트맵 저장→base 치수/배율 치수 순서를 유지한다. 적재 콜백의 반환 뒤 현재 노드를 조회하며 원문 FILD 중간 정수와 float32 저장을 구별한다.

SetEvent는 사건→둘째 인자→첫 인자→flags600 순서다. 수정자를 제외한 좌/우 누름과 ESC만 종료 사건을 원시 세 DWORD 그대로 보낸다. Release·Command·다른 키는 기본 처리기에 위임한다. 큐 콜백이 현재 창을 즉시 삭제해도 이후 노드를 다시 읽지 않는다. Draw는 현재 위치 효과 뒤 base 폭/높이와 비트맵을 다시 읽고 원시 복사한다. 표시 폭/높이·투명 마스크·파일 팔레트·리샘플링을 추가하지 않는다. 소멸자는 버퍼를 반납한 뒤 기본 소멸자에 맡기며 비트맵0을 임의로 쓰지 않는다.

`Display.GifImage`의 일반 판독기는 기존 Assets GIF 판독기를 재사용하고 bool 불투명 표를 원문0/255 BYTE로 바꾼다. 일반 판독기의 LZW 사전/초기화·확장/투명·네 인터레이스 패스를 cpppj와 재대조했다. 배경 전용 판독기는 전역 팔레트 바로 뒤의 첫 이미지 descriptor를 요구한다. 메모리 사본의 논리 크기를 descriptor 크기로 바꾸고 오프셋을0으로 만든 뒤 일반 판독기를 호출한다. 실제 titleMenu는639×480이며 논리 화면 크기·큰 오프셋을 바꿔도 배경 픽셀은 같아야 한다. 일반 GIF와 배경 GIF의 계약을 구별한다.

`BackgroundGifStorage`는 RawBackgroundGifControls.cpp 중 파일/독립 비트맵 소유·반납/원시 복사 부분만 옮겼다. 현재 토큰·원시 stride/평면 주소·클립/행 padding과 보이는 할당 밖 BYTE 공급 경계를 유지한다. 같은 실제 GumpCreate/Collection/Lifetime 및 사건/직접 입력/Draw/scalar 분배를 연결하는 바인딩도 만들었다. **전체 RawBackgroundGifControls의 노드 할당/등록 소유자·Close/페이지 전환·일시 Draw 문맥/재진입 제한과 실제 ConfigInterface 시작/메뉴/호스트 조립은 남는다.** 부분 저장소를 전체 소유자 이식으로 세지 않는다. 검사 조립의 기본 입력은 명시 외부 경계이며 전체 기본 Help 처리기를 복원했다는 뜻이 아니다.

## 2. 독립 원본 대조

읽기 전용 fixture3개를 Link했다. 기대값을 수정/재생성하지 않았다.

| 자료 | 실제 관찰 | 검증 |
| --- | --- | --- |
| background-gif-gump-x86.tsv | 308행, 명시 중단16행 포함 | 입력13열만 실행에 사용하고 뒤6열의14필드/효과순서/반환/완료를 비교 |
| background-gif-assets-x86.tsv | 보호 GIF34개 | 실제 VFS/저장소 적재 후 소스 SHA-256·치수·전체 색 번호 SHA-256/FNV-1a64 일치 |
| background-gif-variants-x86.tsv | 논리 화면/오프셋·작은 화면/큰 오프셋·인터레이스·로컬 팔레트4개 | 같은 명시 입력 머리와 독립 소스/픽셀 SHA·FNV 일치 |

원본 명령 수/블록 수/행 수는 관찰 메타데이터다. 정상 복귀 행은 RET 스택 복귀량도 확인하지만 중단 행의 ESP는 원문 함수 중간값이므로 복귀량과 비교하지 않는다. 최초 검사의 이 범위 오류를 바로잡았으며 제품·원본 기대값은 바꾸지 않았다. 관리 코드의 실제 ABI/명령 수가 같다는 판정은 아니다.

fixture는109개이며 새 파일의 목록 행 수는 머리 포함309/35/5, SHA 접두부는 각각 `93844c8848a1`/`6153a1ec5a7c`/`e3084fcfc65d`다. reflected-commit은 `f95a8db2...`를 유지한다. 포트는233행(L1 115/port118)이다. 새 본체/판독기4행은 L1, 부분 저장소1행은 port다. 기존 Assets/GifImage.cs의 cpppj 대응1행만 재대조 기준을 `a1680825`로 올렸으며 Assets 제품 코드는 수정하지 않았다.

## 3. 실제 장치 검사

집중8개가 통과했다. 위 원본 전수/자산/변형3개 외에 일반 GIF 투명 확장과 배경의 즉시 descriptor 요구, 실제 VFS 동시 소유/교체/없는 파일/부분 생성 예외 및 지연 수명, 실제 공통 위치/Draw/원시 base 치수/0 opacity 무시·전체647×480 표면/클립/7바이트 padding, 실제 사건/직접 입력/Route·원시 큐 인자/수정자·콜백 중 현재 노드 삭제, 변경된 stride/보이는 할당 밖 주소 공급을 확인했다.

최초 실제 조립 검사에서 명시 설정 입력을 FromPairs로 만들면 중괄호가 이스케이프되어 경로 치환이 일어나지 않았다. 검사 입력을 원문 ConfigText로 바로잡고 실제 ExpandSpec 결과도 확인했다. 제품에 경로 대체값을 추가하지 않았다.

전체 **1,924개(Assets363+Core1030+Client531) 통과**, 실패/건너뜀0이다. 전체 Release 빌드도 경고/오류0·종료0이다. Git 제외 로그는 `extracted/dotnet-background-gif-native-initial-20261029.log`/`dotnet-background-gif-native-20261029.log`와 `dotnet-background-gif-focused-20261029.log`/`dotnet-background-gif-focused-fixed-20261029.log`, `dotnet-background-gif-tests-final-20261029.log`·`dotnet-background-gif-build-20261029.log`다. 실제 전체 메뉴/게임 캡처0, 기존 코드/검사 은퇴0이다. 이 결과는 모듈 L1·실제 장치 조립 근거이며 D1 전체 화면 일치가 아니다.

## 4. 다음

HTML/대화창과 실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/호스트를 잇는다. R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 전체 목표는 계속 진행 중이다. cpppj·원본/CD·AGENTS.md·공통 인계·외부 TODO.md는 수정하지 않았다.
