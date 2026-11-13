# webpj Gump 기본 생성·등록·루트 (WB04-E410, 2026-11-14)

RawMenuControls의 단일 실제 트리에 필요한 `client/GumpCreate.cpp/.h`를 이식했다. 기준선 `1f00a25a` 전체와 `GumpCreateTests.cpp`, 전체 필드 직렬화 `GumpRawSupport.h`, 독립 `gumpcreate-x86.tsv`를 읽었다. 기준선 뒤 이 두 파일 변경은 없다. CD10.72/추가판 분기는 웹10.78 범위에서 제외한다.

## 같은 저장소

`ui/gump-create.js`의 GumpCreateNode/State/Create가 기본160BYTE 의미 필드와1000DWORD 등록표를 소유/대여한다. 기본 노드·현재 캐시·배열은 그대로 유지하며 파생 고유 필드도 보존한다. 기본 생성자가 쓰지 않는 form(+0x28)/untouched(+0x74)는 보존한다. 등록 생략 비트가 있더라도 serial은 감기며 증가한다. 일반 등록은 첫 빈 칸만 쓰고, 가득 찬 표의 원문 정상 id0/개수 증가도 유지한다. signed 개수 한도 초과는 변경 전에 거부한다.

루트 초기화는 표/루트만 비운 뒤 실제 new(160) 장치를 호출한다. 할당이 현재 화면 크기·카운터를 바꿀 수 있으므로 복귀 뒤의 값을 읽는다. 실패0이면 카운터·serial·overrideHead·이전 객체를 남긴다. collectionHooks는 이미 이식한 GumpCollection/Draw에 같은 노드 주소를 공급한다. 자식 부착은 후속 GumpTree 단위이며 임시 별도 트리로 성공시키지 않는다.

JS 숫자로는 signaling NaN의 직접 복사 비트를 보존할 수 없다. 기존 xBits/yBits 이식 정책처럼 construct의 선택적 `{widthBits,heightBits}`와 노드의 floatBits를 함께 제공한다. 일반 숫자 입력은 float32로 저장한다. 원문 DWORD 입력이 있으면 width/baseWidth·height/baseHeight의 비트를 직접 보존하고, gumpNodeFloatBits는 현재 숫자가 달라지면 현재 숫자의 비트를 기록한다. 의미 없는 NaN을 유효한 게임 크기로 바꾸지 않는다.

## 검사와 첫 실패

새 검사 **5/5**, 주변 Gump 수집·Draw와 함께 **25/25 통과**했다.

1. 원본 originals **315행**의 연속 Construct/Root 입력을 같은 상태에 공급했다. 모든 노드의160BYTE·등록표1000DWORD RLE·카운터/serial/화면 크기·반환 토큰·할당 요청/순서/현재 전역이 기록과 같다. 기대 초기화/등록 결과는 관측 열에서 읽고 계산하지 않는다.
2. 루트0 실패 시 기존 객체·등록표 배열·수집 상태 주소·override/카운터 보존, 할당 뒤 현재 전역·float32 반올림·serial 감김, 같은 기본 캐시/배열과 파생 고유 필드 보존을 검사했다.
3. 첫 구멍/미등록/가득 찬 표/개수 DWORD 감김을 구별했다.
4. 실제 생성기→공통 GumpCollection→수집/정렬→GumpDraw/GumpPixelTarget 사슬에서 같은 캐시·등록 깊이−40·kind2·visible와 clip 복구를 확인했다. 트리 링크는 CPP의 같은 부품 시험처럼 명시 입력이다.
5. 없는 노드/토큰·할당 장치와 개수 한도 거부, 할당 뒤 대상 누락 실패의 빈 등록표 보존을 확인했다.

첫 초안3/5 통과,2개 실패였다. 원본262행의 signaling NaN 크기를 JS 숫자/Math.fround로 다시 저장하면서 quiet 비트가 달라지는 실제 구현 차이를 발견했다. 원래 크기 DWORD 전달/보존 경계와 현재 숫자 비트 접근자를 추가해315행 전부 통과했다. 다른 실패는 조립 시험이 공통 제출기에 잘못된 요청 필드명을 전달한 입력 오류로, 기존 screen/dirtyFlags/groupRowLimit 계약으로 수정했다. 공통 제출기 구현은 바꾸지 않았다.

원본 자료 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3920/3920 통과**, 실패/취소/건너뜀0(334.5초/365.7초). 대응표 오류0, port661·partial58·todo177이다.

## 다음

같은 GumpCreateState/Node를 사용하는 GumpTree의 부착·정렬·깊이·이동을 이식한다. 이후 같은 상태에 수명/입력·파생 메뉴와 RawMenuControls를 연결해야 한다. 실제 ClientRendererFrame/Session/앱 수명은 아직 미연결이며 미션을 플레이할 수 없다.
