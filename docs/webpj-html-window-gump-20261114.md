# webpj HTML 창 부모와 장기 상태 (WB04-E456, 2026-11-14)

기준선 `1f00a25a`의 `HtmlWindowGump.cpp/.h`를 `www/ui/html-window-gump.js`로 온전히 이식해 기존 `tell-window.js` 부분 대응을 대체했다. `ClientHtmlWindowGlobals.h`와 `RawHtmlWindowContext.h`도 각각 `www/core/client-html-window-globals.js`, `www/ui/raw-html-window-context.js`에 옮겼다. 세 단위는 기준선 뒤 변경이 없어 base를 유지했다.

## 같은 저장소와 처리 순서

HTML 부모는 `GumpDecorationNode`를 상속해 기본/배경/장식과 실제 같은 주소를 사용한다. 원문8192BYTE·20개세DWORD행·현재제어수/본문/요청크기/메뉴열/보조serial·두명령/인자와기한을 유지한다. 생성자는 HTML첫BYTE와 선택필드만 쓰며 NUL뒤나 두parameter를 다시 지우지 않는다. Client의 인스턴스/serial/벽시각, Tell256BYTE, 기존 파서 배경/P억제는 표시 부모를 닫아도 유지한다.

실제 장식 생성 뒤 초기화, 현재20행 한도·돌버튼 할당→생성→현재행→가상부착→명령/인자→Instant→현재count, 기존본문Kill→새본문→scroll/Markup→부착→보조창Kill→비void부모재부착, Enter/클릭serial/태그/닫힘/전체DWORD 기본위임, 벽시각 뒤 현재기한 재조회, 인스턴스감소→OnExit큐→보조창Kill→벽시각→기본소멸을 그대로 구현했다. 하위 장치는 선택된 위치에서 요구하고 부분 효과를 되돌리지 않는다.

`RawHtmlWindowContext`의 세 값 구조체는 callback표를 복사하고 실제 owner/Config/문자열고리/목록의 참조를 공유한다. 미연결된 장치를 PE0이나 빈 성공으로 채우지 않는다. 실제 HTML/Metadata/Factory 소비자 연결은 후속 허브에서 수행한다.

## 검증

집중 검사 **9건 첫 실행 통과**(최초8건 뒤 전체DWORD 입력1건 추가). 기존 보호 원본10.78 fixture `html-window-gump-x86.tsv` **250행**과 실제Idle 후속 `html-window-idle-v2-x86.tsv` **88행**에서 현재24필드·20행 전체·HTML8192BYTE FNV64·24자식의 실제필드·인스턴스/시각·반환/중단·외부 호출 순서가 모두 일치했다. 두 fixture와 해시는 manifest의 `html-window-gump`/`html-window-idle`로 연결했다. 원본 fixture는 읽기만 했고 다시 실행하지 않았다.

추가 독립 입력은 같은주소 거부/선택장치 자연관문, signed한도/실제20행 범위, LF없는1006BYTE와LF있는긴본문, 할당0의 앞선 효과·실제자식 오류, 전체DWORD 위임이다. 공통 조립 검사는 실제 GumpCreate/BackgroundCreate/Decoration/Collection/Tree/Lifetime에 같은 HTML 노드를 등록·부착·Kill·Cleanup하여 공유count/OnExit(같은parameter두번)/시각/기본등록해제/부모링크/실제 반납을 확인했다. 장식 자산 하위는 명시 독립 입력이며 원본 글꼴·픽셀 검사는 아니다.

전체 회귀는 Node v26.13.0·v22.23.3에서 각각 **4,728건 통과, 실패0**, 대응표984행 오류0이다. Node v24는 이 PC에 없어 실행하지 못했다. 보호 원본/디컴파일/cpppj 파일을 바꾸거나 게임/브라우저를 실행하지 않았다.

## 남은 실제 연결

기존 앱은 계속 `TellWindow`를 사용하며 이번 단위로 앱 표시가 바뀌지 않는다. HtmlWindowFit·Metadata·Factory·TellSection·HtmlTextParser의 partial을 각각 마무리하고 RawMenuControls에서 실제 stone/scroll/Style/HTML·본문/링크와 큐/Client 전역을 결선해야 한다. ClientHtmlWindowGlobals가 소유하는 Tell256BYTE와 파서 전역도 그 본체가 같은 주소로 빌려야 한다. RawMenuControls 허브에서 실제 메뉴/원본글꼴픽셀/기하·수명·입력 대조 후 기존 간소화 앱 경로를 교체한다. 실제 미션 플레이는 아직 불가다.
