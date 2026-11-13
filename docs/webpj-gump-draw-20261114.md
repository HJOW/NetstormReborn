# webpj 공통 Gump Draw·현재 클립 픽셀 장치 (WB04-E405, 2026-11-14)

`RawCampaignCursorDisplay`가 실제 글자를 그릴 때 요구하는 선행 `client/GumpDraw.cpp/.h` 205줄을 `render/gump-draw.js`로 이식했다. 기준선 `1f00a25a` 뒤 변경은 없다. 같은 부품은 ClientRendererFrame의 kind2 UI 출력에도 필요하다. 실제 앱 수명 연결과 별도다.

## 구현

- `GumpDrawState`·`setGumpDrawClip`: half-open 논리 영역과 inclusive VFX 영역을 함께 쓰며 right/bottom−1의 DWORD 감김을 유지한다. 영역 객체 주소는 보존한다.
- `GumpClipScope`: Bounds 투영이 바꾼 뒤의 현재 논리 영역을 저장하고 네 좌표를 독립 제한한다. JS에서는 명시 `dispose`·`try/finally`로 정상/예외 반환을 복구한다. 한 객체의 중복 dispose는 두 번 복구하지 않는다.
- `GumpDraw.draw`: 실제 Collection에서 현재 플래그를 읽고0x400000일 때만 Bounds/임시 클립을 사용한다. 보호 없는 경로는 위치/캐시를 읽지 않으며 가상 출력의 클립 변경을 남긴다. visible/dead 필터·자식 재귀·색 복구를 추가하지 않는다.
- `GumpPixelTarget`: 현재 inclusive clip을 안전한 수로 실제 표면 안에 제한해 직교/대각선·그림·bounds 전용 선·원시 bitmap·signed16.16 배율·체크/일반 fill을 출력한다. 대각선/그림/배율은 기존 실제 VFX 커널을 사용한다.
- 원시 bitmap은 대상 clip을 먼저 계산한다. 행 경계를 제한하지 않는 평면 주소를 signed64 BigInt로 유지하고, 보이는 할당 밖 주소에만 `outside(BigInt)`를 요구한다. 정확한 Number 정수도 받으며 표현 손실이 있는 Number는 거부한다. 넘침/누락 오류 앞의 실제 출력은 남긴다.

## 검사와 교정

`tests/gump-draw.test.mjs` **10/10**, 기존 Collection 포함 **20/20 통과**:

1. 원본 `gumpdraw-x86.tsv` originals **476행**의 가상 출력 순서·두 clip·전체 노드 캐시·투영 호출이 **첫 대조에서 모두 일치**했다. CD/추가판은 읽지 않는다.
2. 보호 없는 경로의 무효 부모/자식 미조회·캐시 보존과 현재 가상 clip 유지, 보호 플래그 뒤 실제 위치 캐시 갱신을 대조했다.
3. 가상 출력 예외에서 보호 플래그가 있는 경우만 논리/VFX 영역을 복구한다. 원본 PE 예외 실행 관측이 아닌 C++ 계약 시험 대응이다.
4. Bounds 투영이 바꾼 clip을 저장하며 부착 노드 자신의 캐시를 덮지 않는다.
5. 실제 트리→수집/CRT→kind2 Draw→기존 버튼 계획의 네 선/투명 이미지까지 연결해 보호 바이트와 픽셀17/55/9/255를 대조했다. 기존 웹 버튼 계획의 평면 선 필드는 GumpLine 입력 구조로만 바꿨다.
6. 투명도·색0/255 치환·현재 inclusive clip·빈 영역·직교/대각선 끝점·극단−2³¹/2³¹−1·BYTE 색 절삭을 확인했다.
7. 누락 가상/노드/부모·표면/이미지 용량·직접 scope/중복 dispose를 검사했다.
8. 실제 픽셀의 음수 평면 주소/숨은 guard 미조회·할당 밖 공급·signed64 넘침 prefix·현재 clip의 fill/linesClipped/배율도 확인했다. 부모 미니맵·이미지 Gump 전체의 재대조는 해당 단위 몫이다.

최초 집중은 **7/10 통과**였다. 시험 이미지에서 기존 웹 필드 `opacity`를 `alpha`로 썼고 배율 시험 프레임의 오른/아래 inclusive 끝점을 half-open처럼 넣었다. 입력 구조를 교정한 뒤 **9/10**이었다. 남은 C++ 계약 검사는 손상 이미지가 예외 없이 지나가는 실제 누락을 드러냈다. 기존 낮은 `drawIndexedImage`는 크기 검사를 하지 않아 새 `GumpPixelTarget.image` 경계에 원문 Renderer의 이미지 용량 검사를 추가했다. 그 뒤10/10·주변20/20을 통과했다. 기대 픽셀·원본 fixture는 바꾸지 않았다.

게임·브라우저 실행과 원본 자료 변경은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3893/3893 통과**, 실패/취소/건너뜀0(350.7초/385.2초). 대응표 오류0, port655·partial59·todo182다.

## 다음

`GumpTextDraw`의 아직 partial인 글꼴 선택/길이/NUL/색·글리프 경계와 실제 GumpPixelTarget 연결을 마무리한다. 그 뒤 RawCampaignCursorDisplay의 파일 팔레트/OwnerColor/SP/현재 시각·실제 경고/비용 글자와 Renderer dirty 출력을 조립한다.
