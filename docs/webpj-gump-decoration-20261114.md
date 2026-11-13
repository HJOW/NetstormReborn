# webpj 장식 배경 생성·출력·갱신 (WB04-E419, 2026-11-14)

RawMenuControls 선행 **GumpDecoration 205줄(.cpp/.h, partial→port)**을 완료했다. 기준선 `1f00a25a` 전체 소스와 GumpDerivedRedrawTests의 Decoration 원본 관측·실제 자산 연결/메뉴 marker 사용처를 읽었다. 두 소스의 기준선 뒤 변경은 없다. CD는 웹10.78 범위 밖이다.

## 같은 실제 배경

- GumpBackgroundNode와 같은 주소의 cornerShape/type/frame 캐시를 추가했다. Construct는 실제 GumpBackgroundCreate 뒤 장식 VT·signed command·현재 cornerType을 쓰고 현재 자산 응답을 별도 캐시에 저장한다. SHP는 소유 자원이 아니므로 별도 free하지 않는다.
- Draw는 실제 배경 출력 뒤 현재 Position을 한 번 저장한다. 모서리 순서는 TL→TR→BR→BL이며 각 호출에서 현재 캐시 SHP/첫 프레임을 다시 읽는다. 오른쪽/아래 크기의 조회 시점과 CRT signed64 절삭 하위DWORD를 보존한다. 앞 모서리 callback이 바꾼 크기/캐시는 다음 호출에 반영되지만 저장한 Position은 유지한다.
- Redraw는 자신/모든 부모 visible만 검사하며 dead/void를 추가로 거르지 않는다. Position 뒤 현재 width/height+4를 각각 절삭하고 양수일 때(x-1,y-1) 원점의 확장 영역을 요청한다. Renderer 입력 어댑터는 flags0을 공급한다.
- 실제 GameAssets/TypeTable의 첫 I 구간을 생성에 공급하고 캐시 SHP의 **물리 프레임**을 같은 GumpPixelTarget에 원색으로 출력한다. 배경이 설치한 공통 명암 색 표를 모서리에 적용하지 않는다. 없는 I 구간은 signed-1의 DWORD를 유지하며 실제 사용 시 물리 범위 계약을 적용한다.
- Draw/Frame Redraw/기본 scalar 소멸을 실제 가상 분배에 연결하고 다른 파생 fallback은 각각 유지한다. 자식 파괴/등록 해제는 같은 GumpLifetime이 담당한다.

## 검사

새 **5/5 첫 통과**, 주변 스타일/대화상자/그림·Button/Stone/Gump/메뉴와 함께 **156/156 통과**다. 제품/원본 대조의 실패 교정은 없다.

1. `gump-derived-redraw-x86.tsv`의 **Decoration384행** 전체 요청·root/대상 위치 캐시·투영 순서가 원문과 일치했다.128BYTE 입력의 읽지 않는 뒤 문맥32BYTE만0으로 채워 기존 디코더를 사용했다. 같은 파일의 Compact/Row768행은 MenuGumpInput 단위로 이월하며 이번에 검증했다고 세지 않는다. `derived-x86.tsv`는 별도 월드 자산 생성 관측으로 이 클래스의 fixture가 아니다.
2. 실제 배경 기본 생성/등록과 type0 미작성 필드 보존, signed command/현재 타입 callback·캐시를 확인했다. 실제 배경 fill callback 뒤 Position, 각 모서리 callback 뒤 현재 width/height/shape/frame을 명시한 네 호출 인수와 비교했다.
3. **실제 fortGump 타입92·I 첫 프레임54**와 I54..57 네 모서리를 연결했다. 실제 배경 출력과 독립 SHP 해독/명시 네 기준점 합성의 전체 픽셀을 대조하고, 생성 캐시/현재 cornerType·이전 배경 shape를 구별했다. 공통 명암 표가 dark여도 모서리는 원색이며 clip/앞뒤 guard를 보존한다. 없는 I/타입·캐시 SHP/물리 프레임 범위도 확인했다. 별도의 원본 Draw 픽셀 fixture를 뜻하지 않는다.
4. 같은 실제 GumpTree→GumpFrame 가상 Redraw→Renderer.invalidate/draw에서 **4,6..17,21** 확장 dirty를 확인했다. 공통 kind2 GumpDraw가 배경 뒤 네 모서리로 분배되고 실제 Kill/Cleanup은 등록 루트만 남기며 장식 토큰을 한 번 반납한다.
5. 비가시/비양수 영역의 미선택 경계, 부모 순환/공유 주소·토큰·필수 장치와 서로 다른 가상 fallback을 검사했다.

원본 파일 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3998/3998 통과**, 실패/취소/건너뜀0(329.3초/359.8초). 대응표 오류0, port676·partial52·todo168이다.

## 다음

Markup/StyleTextMarkup·StyleTextBuffer·TextGump/ScrollingText와 파생 메뉴 처리기를 같은 실제 조각/자산/수명에 연결한다. RawMenuControls·ClientRendererFrame·GameWorld·ClientGameplayGlobals/Session·기존 앱의 메뉴 교체가 남아 실제 미션은 아직 플레이할 수 없다.
