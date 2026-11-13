# webpj 실제 커서 표시 조립 (WB04-E407, 2026-11-14)

5-A ClientRendererFrame의 선행 `client/RawCampaignCursorDisplay.cpp/.h` 161줄을 `render/raw-campaign-cursor-display.js`로 이식했다. 기준선 `1f00a25a` 뒤 변경은 없다. E398~E406의 실제 커서 전역·수집/Draw·Gump 픽셀/글꼴을 파일 자산/팔레트·SP·시각·Renderer에 연결한다.

## 현재 저장소와 출력

- RawWorldCore의 pool/types/spots/processState, Source의 같은 Drop/Lifecycle/Placement/표면 TYPE/Canon 격자와 장기 GameAssets/SP·현재 PlayerKnown 상자를 빌린다. 피드백·표시 옵션·독립 색표만 소유한다.
- 현재 SP offset/추적 offset/첫 배열/첫 순열과 게임 시각을 매 원문 접근에서 읽고 같은 실제 수집기로 kind4 본체/보조를 한 번 제출한다. Renderer용 항목은 값으로 보관하고 실제 그리기는 해당 dirty 표면에서 한다.
- 전체 월드 viewport는 마지막 작은 dirty와 별도다. 취소/잠금·목록 변경 뒤 캐시 항목은 같은 raw를 다시 실행하지 않는다. 현재 목록은 전체 DWORD로 비교한다.
- 파일 팔레트 변경 때 현재 명명색과 세 색표를 갱신한다. 고정 ColorSort 입력은 lookup 계산에만 사용한다. 외부9표 중 세 표는 C++ 값 복사와 같게 복사하며 실제 Client OwnerColor 토큰/표는 같은 주소로 빌린다. 공유 표가 있으면 지역 전용 표를 다시 만들지 않는다.
- `setPalette`는 기존 Screen.palette의 BGRX1024BYTE와 원문 ScreenColor256객체를 같은 RGBX로 연결한다. 예약BYTE는 보존한다. 기본 일반12종/전용5종·문자11개 프레임 표·조건부 확장 색표 경계를 유지한다.
- 실제 경고/비용 문맥은 번역 전에 저장하고 다음 출력에서 현재 FontSet/번역 장치를 읽는다. 경고는 원본 다섯 키, 비용은 현재 문자열이며 NUL 종료·실제 GumpTextDraw/GumpPixelTarget/dirty clip을 사용한다.

## 검사와 범위

새 `tests/raw-campaign-cursor-display.test.mjs` **6/6 첫 통과**, PreviewCollection/Draw·GumpGlyph 포함 **38/38 통과**다. 기존 CPP Session 표시 시험의 표시기 부분/기대 픽셀 조건을 옮겼으며 **실제 GameWorld/RawCampaignSession을 생성한 시험은 아니다**. 표시기가 읽는 RawWorldCore 네 참조만 구성했다. 풀·자산 TYPE·Factory·SP 저장소·시각 상태·FontSet·OwnerColor·Renderer·글리프/픽셀 몸체는 모두 실제 웹 부품이다. 별도 게임 규칙 모델이나 성공 stub은 없다.

1. 실제 Factory가 만든 TYPE75의 SHP·명시 네 외곽선으로 전체640×480 픽셀을 대조했다. 카메라100,50·kind4 두 깊이·UI 전용 dirty4 제외를 확인했다. SP8001/시각12.5에서 count20→19·강조 float 비트, 작은 dirty 뒤 count18·동일 전체 스냅샷, 전체 잠금/목록 정리 뒤 추가 수집0을 대조했다. 정리는 이번 최소 장면의 명시 입력이며 실제 Cursor.Cancel 통합 증거로 세지 않는다.
2. 실제 원본 글꼴로 경고 치수를 측정하고 에너지 경고 키/비용 문자열·비용 SHP를 같은 표면에 출력했다. 모든 dirty 바깥 픽셀이 보존된다.
3. 실제 공통 OwnerColor/ColorSort primary를 빌려 Player1의 현재 색2로 다리 SHP 전체 픽셀을 대조했다. 새 팔레트의 흰색17/노랑19를 읽으면서 최초 공통 표 주소/바이트·현재 토큰 주소는 보존한다. 무효 색 번호/미준비 소유자는 거부한다.
4. 실제9표에서 복사한 불법 표는 외부 표를 이후7로 채워도 영향을 받지 않는다. textColors 값 사본과 settings/feedback의 실제 주소도 확인했다.
5. 선택된 팔레트/부모 깊이/확장 TYPE 색표/글꼴/비용 이미지 누락·다른 풀을 거부한다. 잠금/목록에서 빠진 옛 항목은 잘못된 출력 표면도 읽지 않는다. 현재 부모 깊이 교체와 직접 CollectDrawing/DrawEntry도 확인했다.
6. BGRX와 ScreenColor 객체 변환이 예약BYTE0x5a까지 같고 잘못된 팔레트 길이를 거부한다.

원본 자료 변경·게임/브라우저 실행·새 fixture는 없다. 전체 Node v26.13.0·v22.23.3 각각 **3905/3905 통과**, 실패/취소/건너뜀0(342.7초/379.3초). 대응표 오류0, port657·partial58·todo181이다.

## 남은 경계와 다음

CPP `CampaignSession_CursorDisplayUsesActualShapesCameraAndDirtyCollection`·`CampaignSession_CursorDisplayIllegalAndActualWarningText`의 실제 GameWorld/Session/Take/Cancel 부모 호출은 4-D 조립에서 재대조한다. 이번 완료는 표시기 본체와 현재 실제 출력 부품의 조립이며 PORT_MAP session0이다.

다음은 ClientRendererFrame의 남은 표시 선행 RawCampaignFactoryDisplay다. 기준선 뒤 변경(+10/−6 본문, +3/−1 헤더)은 해당 단위에서3.6절 판정한다. 확인된 선행 RawObjectHit(원본608행)을 먼저 이식하고83VT 표는 FactoryDisplay와 함께 옮긴다.
