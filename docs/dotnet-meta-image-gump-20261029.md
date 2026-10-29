# dotnetpj R3j-2 — 동적 MetaImage의 생성/기하/가상 출력

2026-10-29, HJOW-Athlon. 이미지UI 단위를 `8eece422`로 커밋한 뒤 cpppj MetaImageGump.h/.cpp·MetaImageGumpTests.cpp·RawMenuMetaImageTests.inc와 기존 RawImage를 읽었다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. 게임/창/PE 에뮬레이터·금지된 실제 플레이 재현/멀티플레이 검사는 실행하지 않았다.

## 1. 구현

같은 기본/파생 저장소를 쓰는 MetaImageGump를 만들었다. 원본+a0는 목록/창이 소유하는 실제 RawImage를 빌리며 노드 소멸이 반납하지 않는다. 생성은 기본0크기 생성→가상 표0050d290/종류21→주소/배율1/색상0을 쓰고 아직 Image를 측정하지 않는다.

Resize는 현재Image의 int폭/높이+1 감김 뒤 double을 만든다. 기본 크기는float32로 저장하지만 출력 크기에는 먼저 좁히기 전 double을 사용한다. Attach는 실제 기본 부착 뒤 현재Image와 현재 기본 크기를 조회한다. 두 비율을 각각float32로 저장하고 reciprocal은 최종 크기 곱셈까지double로 유지한다. 저장된float배율로 최종크기를 다시 계산하지 않는다.

Draw는 현재shape가 비었으면 위치 getter를 부르기 전 반환한다. 정확한 배율1 비트일 때만 colorMode로 고정00556038 표를 설치하며, 다른배율은colorMode와무관한plain16.16 변환이다. 위치/색표callback 뒤 현재Image/색상모드를 다시 읽는다. 원문 정수 감김/CRT 하위DWORD 절삭/연산 순서를 보존한다.

BindMetaImagePixels는 현재RawImage.Shape를 매출력 다시 읽어 실제SHP 판독/기존 VFX/공유클립·256색표에 연결한다. 이전 shape의 캐시로 현재 내용을 고정하지 않는다. 현재 색표 배열의 내용만 복사해 다른Gump의 같은 참조를 유지한다. 실제 공통 가상Attach/Draw를 연결하고 기본 수명은 비소유Image를 보존한다. MetaImageList/창 상위소유·전체메뉴/호스트 조립은 남는다.

## 2. 독립 원본 대조

읽기 전용 `meta-image-x86.tsv` **326행**을 Link했다. 앞12열만 입력으로 사용하고 뒤 기대 기하/종류·가상표/플래그·등록 수/일련번호·효과 순서를 비교한다. 실제 RawImage.Convert와 공통 GumpCreate를 사용한다. 부착 중 Image 교체 입력도 그대로 실행한다. 기하의float32 비트/등록/효과는 두 제어모드의 원본 관찰과 전부 일치한다. 이 자료는 전체 미작성노드/소멸자 관찰이 아니며 해당 범위의 L1로만 센다. 원문 명령 수는 자료 메타 검사이고 관리 코드의 ABI/실제 명령 수 일치가 아니다. 원본 기대값은 수정/재생성하지 않았다.

fixture105개·새 원문327행/SHA8f0707a8c3aa이며 reflected-commit은 `f95a8db2...` 유지다. 포트226행(L1 109/port117)이며 새 본체2행의 기준은 `8eece422`다.

## 3. 실제 연결 검사

집중 **6개 통과**다. 원본 전수1개 외5개는 실제동적SHP·일반/비활성색/현재색표 공유·클립·내용교체, 배율plain/0배율과padding, 기본부착/위치/색표콜백 뒤 현재Image 재조회, 빈shape의 장치 미도달·없는Image/CD/잘못된색표 거부, 실제공통 생성/가상Attach·Draw/지연Kill과 빌린Image 보존을 확인한다.

최초 검사 코드의 var 다중선언과 nullable 경고를 고쳤다. 이어 실제픽셀 검사1건에서 RawImage의 원문 기준점 때문에 출력 첫 점이로컬위치+1인 것을 반영해 검사 좌표를 고쳤고, 공통 연결 검사 표면도0x5a로 초기화해 실제 출력으로 검증했다. 제품/원본 기대값은 바꾸지 않았다. 최종 Client Release 컴파일은 경고/오류0이다.

전체 **1,908개(Assets363+Core1030+Client515) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. Git 제외로그는 `extracted/dotnet-meta-image-gump-focused-initial-20261029.log`, `dotnet-meta-image-gump-focused-failed-20261029.log`, `dotnet-meta-image-gump-focused-20261029.log`, `dotnet-meta-image-gump-tests-20261029.log`, `dotnet-meta-image-gump-build-20261029.log`다.

## 4. 다음

MetaImageList의21칸 실제 목록/자식소유·기하/선택/적중/삭제와원본514행을 연결한다. 이후 이미지/GIF·HTML/대화창·실제메뉴상위 소유/분배·전체자산/ConfigInterface/음향/호스트를 조립한다. R4 이후 전체미션/세션/월드/HUD/Edit·후순위멀티플레이 사전구현은 남으며 전체 목표는 계속 진행 중이다.

실제전체메뉴/게임캡처0·기존코드/검사은퇴0이다. cpppj·원본/CD·AGENTS.md·공통인계·외부TODO.md를 수정하지 않았다.
