# dotnetpj R3h-3a — 대괄호·다중 줄 파서와 버퍼 소유 수명

2026-10-29, HJOW-Athlon. 기준 cpppj는 `f7e1748e8c9a`의 `StyleTextMarkup.h/.cpp`, `ScrollingTextGumpMarkup.h/.cpp`, `ScrollingTextGumpWidgets.h/.cpp` 중 StyleTextBufferStore다. 원본 게임이나 별도 창을 실행하지 않고 같은 조각·그림·글꼴·버퍼·픽셀 저장소에 연결했다.

## 1. 구현

`StyleTextMarkup`은 C++에 복원된 여섯 제한 정규식의 부분 검색·greedy 역추적·우선순위를 그대로 옮겼다. ASCII space만 반복 공백으로 보고, 원본의 이스케이프되지 않은 점은 CR/LF/space도 포함하는 임의 한BYTE로 소비한다. 캡처80바이트와 프레임 지역 배열의 최대79바이트를 구별한다. 입력/이름은 Latin1 일대일 변환으로 원본 BYTE를 유지하며 숫자는 기존 CRT ParseLong의 공백/부호/DWORD 감김을 공유한다.

한 줄의 I→S→T→C→B→E, CD 한 줄의 I→T, CD 다중 줄의 I→W→T를 구분한다. 처음 일치한 형식은 그림이 없거나 옵션이 꺼져 있어도 소비한다. 그림의 현재 gump/default/글자 시작 필드와 판본별 재조회, count까지의 클램프, 한 줄 */#/&와 다중 줄 접두 뒤 suffix#의 서로 다른 맞춤 규칙을 유지했다. 다중 줄 숫자 타입 상한과 현재 제한 필드, signed SHP 높이의 패치16/CD12 문턱·99999×12 맞춤도 보존했다. E는 한 축만-4~4 범위여도 두 축을 같이 적용하고 현재 검정색을 조회한다. CD 다중 줄 W는 zone 소리 허용과 무관하게 첫 캡처만 요청한다.

`ScrollingTextGumpMarkup`은 입력과 기본 문맥을 먼저 복사하고 실제 버퍼 추가→dirty→재그리기를 구간마다 호출한다. 원본처럼 끝 빈 구간도 통지하며 리터럴~~·미인식 명령·미닫힌 대괄호·첫NUL을 구분한다. 태그는 실제 노드의 참조 가능한 필드로 보관해 호출 사이에 유지한다. 소리/추가/타입 콜백 뒤 현재 태그/제한 필드를 다시 읽고 ~. 복귀에는 현재 기본 문맥을 사용한다. 한 줄 StyleText의 대괄호 훅도 실제 같은 처리기로 연결했다.

`StyleTextBufferStore`는 Gump 등록과 별개인 소유 버퍼/외부 전역 상태의 실제 주소를 보관한다. 생성 시 경계 함수 객체를 값 복사하고 같은 상태/처리기를 표시 조회에 연결한다. 버퍼 소멸→호스트 free→레코드 제거 순서이며 free 콜백에서는 이미 소멸한 상태를 아직 조회할 수 있다. 원본의 미완료 끝 칸 누락과 free 예외 뒤 부분 상태도 유지한다. 중복 토큰/같은 외부 상태의 중복 등록/실패한 생성의 유령 레코드는 거부한다.

## 2. 검증과 범위

- 집중 **50개 통과**다. 명령 우선순위/임의 구분BYTE/탭/greedy 되돌림/비ASCII 이름/80바이트 경계, 현재 타입 재조회·숫자 감김·글자/특수 프레임/count 클램프, 한 줄·다중 줄 맞춤 차이와 제한 문턱, 소리/색/그림자/기본 옵션 유지, 파서 구간·콜백·태그/문맥·버퍼 수명을 고정 입력으로 검사했다.
- 두 판본에서 실제 대괄호 파서→실제 버퍼→글꼴/SHP→같은8비트 표면을 연결했다. 태그0/9/9의 표시 복제, 실제 글자171·그림1~4 픽셀과 양끝 guard를 확인했다. 별도 소유 버퍼의 실제 Draw도 확인했다.
- 최초 집중 검사 컴파일은 검사 코드의 `StyleTextState`에 잘못 직접 인덱싱한1건으로 실패했다. 조각 표 경로만 고쳤고 제품 코드나 기대값을 바꾸지 않았다. 최초 로그를 보존했다.
- 독립 원본 실행 fixture는 추가하지 않았으며 정적 복원 보고서의 계약/통합 검사다. **L1로 세지 않는다.** fixture99개/reflected-commit은 유지하고 포트 맵만194행(L1 97·port97)으로 갱신했다. 이전 코드/검사 은퇴0, 실제 게임/메뉴 캡처 비교0이다.
- 전체 **1,745개 통과(Assets363·Core1030·Client352), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 변경12파일의 UTF-8·한국어 주석/문서와 보호 경로 변경0을 확인했다. 로그는 Git 제외 `extracted/dotnet-stylemarkup-compile-20261029.log`, `dotnet-stylemarkup-focused-initial-20261029.log`, `dotnet-stylemarkup-focused-20261029.log`, `dotnet-stylemarkup-tests-20261029.log`, `dotnet-stylemarkup-build-20261029.log`다.

## 3. 다음 연결

실제 다중 줄 Widget 생성·입력은 미이식 공통 `ScrollGump`/`ScrollGumpWidgets`에 의존한다. 다음 R3h-3b는 공통 Slider 값/부착/손잡이/화살표 생성·입력·Draw·수명을 먼저 옮긴 뒤 `ScrollingTextGumpWidgets`에 실제 버퍼 저장소를 연결한다. 이후 MenuGump 전체와 실제 메뉴/전체 자산/GIF/호스트 조립을 진행한다. 전체 목표는 계속 진행 중이다.

GameAssets 전용 타입 이름/선택 필드 바인딩과 실제 SoundPlayer 장치는 아직 조립하지 않았다. 현재 파서는 원본과 같은 현재 조회 경계를 받고 실제 SHP 검사는 기존 MarkupAssets 연결을 사용한다. 옵션은 현재 문자열 조회/CRT 파서, 소리는 원본 PlayByName의 추가5인자0 요청까지 연결했다. 임의 파일 로딩·유사 소리 재생으로 대신하지 않았다. 실제 자산/ConfigInterface/음향 계층의 전체 연결은 해당 조립 단위에서 마친다.

cpppj·원본/CD·AGENTS.md·공통 인계를 수정하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에 포함하지 않는다.
