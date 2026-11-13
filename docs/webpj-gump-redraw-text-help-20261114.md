# webpj Gump 갱신·측정·도움말 (WB04-E414, 2026-11-14)

RawMenuControls 선행 GumpRedraw(79줄)·GumpHelp(108줄)와 기존 partial GumpText(126줄), **세 단위313줄**을 완료했다. 기준선 `1f00a25a` .cpp/.h 전체와 GumpRedrawTests/GumpTextTests를 읽었다. 기준선 뒤 여섯 소스 변경은 없다. CD/추가판 분기는 웹10.78 범위 밖이다.

## 같은 현재 경계

- GumpRedraw는 자기/조상 visible만 검사한다. Whole은 같은 Position 뒤 현재 폭/높이를 각각 절삭하여 원문 DWORD XYWH 끝점과 flags0을 전달한다. Map은 Bounds 뒤 현재 지도 영역을 읽어 각 축을 원문 max(lower,min(value,upper)) 순서로 제한한다. 역전 지도·signed 차이/감김도 보존하며 숨은 조상에서는 위치/캐시/Renderer를 호출하지 않는다.
- GumpText의 폭 합산·길이/출력 순서를 core/gump-text-measure.js로 분리하고 render/gump-text.js에서 같은 실제 FontSet의 첫 폭 표·요청 슬롯 논리 높이·세 캐시 메트릭을 연결했다. -1은 범위 내 NUL이 필수이고 다른 음수는0글자다. 명시 길이는 NUL도 측정한다. 폭/높이가 같은 저장소를 가리켜도 쓰기 및 그림자 이후의 현재 출력 재조회 순서를 유지한다.
- 불법 스타일의 폭은 슬롯0 normal이며 높이는 요청 슬롯을 먼저 검사한다. Metrics는 불법 스타일을 슬롯0으로 바꾼 뒤 해당 슬롯만 검사하므로 두 경로의 진단 순서가 다르다. bindStoneButtonText는 같은 기본8DWORD 문맥·64BYTE 라벨/명시 길이 경계를 측정기에 연결한다.
- GumpHelp는 오른쪽 누름/현재 alternateTextPointer가 있을 때만 주제 NUL 절단+Help→메뉴 시작→현재 제목→현재 본문/사건/주제→항목 뒤 현재 루트 부착→opengump.wav 순서다. bindGumpHelp는 ButtonGump의 baseEvent fallback에 실제 몸체를 설치한다. 선택되지 않은 메뉴/소리 장치는 요구하지 않는다.

한국어는 이미 확정한 E269/E406 정책의 UTF16/UnicodeFontProvider·D2Coding 경계를 유지한다. 원문4BYTE 확장 부호화를 새로 도입하지 않는다. 기존 FontSet의 UI 편의 API도 보존하며 정확한 signed 길이/alias/세 메트릭 경로는 새 GumpText를 사용한다.

## 검사

새 검사 **8/8 첫 통과**, 주변 생성/트리/수명/이벤트/입력/프레임/수집/Draw/글리프와 함께 **69/69 통과**했다. 원본 대조/제품의 실패 교정은 없다. 호스트 문맥의 font/style은 C++의 DWORD 비트로 정규화했다.

1. `gump-redraw-x86.tsv` **1474행(Whole728/Map728/Clamp18)**의 요청 인수 전체·모든 위치 캐시·투영 순서를 원문 관측과 대조했다.128BYTE 입력의 읽지 않는 뒤 문맥32BYTE만0으로 채워 공통 디코더를 썼다.
2. `gumptext-x86.tsv` originals **305행**의 원문 첫 폭 표/메트릭으로 메모리 .chfnt 파일을 만들고 실제 BitmapFont parser/FontSet을 거쳤다. 전 슬롯/스타일·명시/음수 길이·DWORD 감김·출력 alias의 폭/높이가 관측과 같다. cacheHeight31과 logicalHeight14도 구별했다.
3. 현재 투영 콜백이 바꾼 크기와 지도는 Position 뒤에 읽으며 숨은 조상/순환/없는 노드/장치 거부를 검사했다.
4. 실제 GumpCreate→GumpTree 가상 Whole→Renderer.invalidate→실제 dirty Draw에서 같은 위치/캐시와2,3..6,7 영역을 확인했다.
5. 잘못된 문자열 범위/슬롯에서 출력 불변, NUL의 명시 측정, 다른 음수0글자·그림자·Metrics의 다른 스타일 fallback을 확인했다.
6. 실제 원본 FontSet과 기본 문맥 바인딩을 연결하고 기존 편의 측정과 일치했다. UTF16 경계는 실제 UnicodeFontProvider에 명시 래스터라이저 응답을 주어 원문 바이트 글꼴과 분리/코드/폭을 검사했다. 실제 브라우저 D2Coding 렌더링을 새로 실행한 시험은 아니다.
7. CPP 도움말 현재 콜백 시험을10.78 인자로 옮겨 NUL 주제·메뉴 뒤 제목/본문/주제·항목 뒤 루트·signed 좌표·소리 순서를 확인했다.
8. 미선택 사건/포인터는 장치 없이 반환하고 선택 장치 부재는 정확한 위치에서 실패하며 이미 쓴 주제/메뉴 효과를 보존한다.

원본 자료 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3958/3958 통과**, 실패/취소/건너뜀0(334.5초/365.5초). 대응표 오류0, port669·partial57·todo170이다.

## 다음과 부모 경계

ButtonGump·StoneButtonGump와 배경 생성/출력을 같은 실제 저장소에 이식해야 한다. GumpTextTests의 실제 StoneButton 부모 FitToText/라벨/배경/픽셀·해제 조립 시험은 그 단위에서 수행한다. 이번 바인딩 검사는 실제 측정/도움말과 훅 경계까지이며 파생 버튼을 생성했다고 주장하지 않는다. StyleText/메뉴·RawMenuControls와 ClientRendererFrame/Session/앱 수명도 미연결이며 미션은 아직 플레이할 수 없다.
