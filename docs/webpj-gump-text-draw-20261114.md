# webpj Gump 글꼴 선택·공유 글리프 출력 (WB04-E406, 2026-11-14)

5-A RawCampaignCursorDisplay가 요구하는 `client/GumpTextDraw.cpp/.h`의 남은 부분을 이식했다. 기준선 `1f00a25a` 137줄이며 이후 변경은 없다. 기존 `render/glyph-text.js`에 공유 반복기를 두고 `render/gump-text-draw.js`가 글꼴/트리/픽셀 장치를 연결한다. partial→port다.

## 구현과 웹 적응

- `drawBitmapGlyphText`: 범위 안 NUL 또는 명시 길이를 먼저 검사한다. 명시 길이에는 코드0도 출력하며 잘못된 범위는 색 설치 전에 거부한다. 빈 문자열/빈 글꼴도 그림자→네 외곽선→본문 순서로 색을 설치한다. 글리프 포인터가 없으면 전진하지 않으며 특별 빈 프레임은 출력 경계와 셋째 표 drawAdvance를 유지한다. 첫 측정 폭·ABC A는 쓰지 않는다.
- 기존 메뉴/UI의 `drawGlyphText`도 같은 반복기로 위임한다. 기존 API는 명시 길이 bytes.length를 사용해 NUL 없는 UI 문자열·UTF-16 입력의 동작을 유지한다.
- `GumpTextDraw.draw`: 원본 슬롯 검사를 불법 스타일 처리보다 먼저 수행한다. 불법 스타일은 선택적 로그 뒤0/normal로 바꾸며 실제 FontSet의 빈 조합은 null이다. drawGump는 현재 Position 투영/캐시 뒤 기본8DWORD 문맥을 해석하고 명시 문맥이 있으면 그것을 쓴다.
- `GumpGlyphColorState`·`makeGumpGlyphPixelHooks`: 원본 색100만 바꾸고 전체256색 표를 현재 VFX 표에 설치한다. 글리프의 실제 상대 이미지 위치·투명도·현재 GumpPixelTarget clip을 사용한다. 본문 색을 이전 값으로 복구하지 않는다.
- 원문의4BYTE 한글 확장 시퀀스는 E269 `OriginalText`에서 확정한 웹 제외/적응 정책을 따른다. 웹은 기존 UTF-16·D2Coding 입력을 그대로 공유 반복기와 FontSet/UnicodeGlyphFont로 전달한다. CD10.72는 웹 범위 밖이다.

## 검사

새 `tests/gump-glyph.test.mjs` **6/6**, GumpDraw·한국어 검사 포함 집중 **34/34 통과**. 임시 격리 초안6/6 뒤 실제 경로 적용 첫 집중34/34이며 제품/기대값 교정은 없었다.

1. 원본 `gumpglyph-x86.tsv`의 Font 입력을 같은 `.chfnt` 바이너리 구조(첫 폭99/ABC A−31/독립 셋째 폭/포인터 유무/특별 공백)로 메모리 파일에 만들고 **실제 BitmapFont parser·FontSet**을 실행했다. Case originals **256행**의 색C·선택 글꼴/코드/좌표G·불법 스타일E 사건이 모두 첫 대조에서 같다. 입력은 검사 소유 메모리이며 원본 파일을 쓰지 않는다.
2. 한 픽셀 캐시로 A2·공백3·NUL1/없는B의 전진과 실제 clip·색100/다른 표 칸 보존을 손계산 픽셀과 비교했다.
3. 그림자·네 외곽선·본문 여섯 픽셀과 빈 문자열/빈 글꼴의 최종 색 설치를 대조했다.
4. 명시 길이의 중간NUL도3글리프로 출력하고 잘못된 NUL/길이·필수 경계·문맥 폭은 효과 전에 거부한다.
5. 실제 GumpCollection 월드 투영이 바꾼 현재 기본 문맥77과 명시 문맥33을 구별했다.
6. 기존 IndexedSurface UI API도 같은 반복기를 지나 A/공백/B의 본문/그림자 네 픽셀을 낸다. 기존 한국어18건도 모두 통과했다.

게임·브라우저 실행과 원본 자료 변경은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3899/3899 통과**, 실패/취소/건너뜀0(304.2초/339.1초). 대응표 오류0, port656·partial58·todo182다.

## 다음

GumpDraw·GumpTextDraw 선행이 갖춰졌으므로 RawCampaignCursorDisplay의 실제 현재 팔레트/OwnerColor/SP·시간·경고/비용 글자와 Renderer dirty 표면을 조립한다. 실제 Session 부모 시험과 ClientRendererFrame/앱 수명 연결은 여전히 별도다.
