# webpj 공통 Gump 트리 (WB04-E411, 2026-11-14)

5-A RawMenuControls의 선행 `client/GumpTree.cpp/.h`207줄을 이식했다. 기준선 `1f00a25a`의 전체 몸체·GumpTreeTests/GumpAttachDiagnosticTests와 독립 관측 두 파일을 읽었다. 기준선 뒤 이 두 소스 변경은 없다. CD/추가판 분기는 웹10.78 범위에서 제외한다.

## 현재 저장소와 순서

`ui/gump-tree.js`는 E410의 같은 GumpCreateState/Node를 빌린다. 대체 노드/등록표를 만들지 않는다. 원본 현재 checking DWORD는 기존 웹 정책의 getter로 대여한다. signed GT/상한 진단은 첫 쓰기 전에 상세 writer를 선택하며, 복귀 뒤 현재 flags/부모/GT를 읽어 실제 부착을 계속한다.

부착은 원문처럼 현재 부모 검사→void 해제/깊이 상속→동종 교체 요청(호출 전 next 저장)→머리 삽입→월드 투영/화면 제한→정렬→가상 redraw→현재 자식 재배치다. Push는 자신의 parent/next/previous를 남기고 외부 형제/부모 링크를 먼저 분리한다. 패치 진단3/4의 의도적 링크 변경도 유지한다. Align은 기본 크기·배율·지도 중심의 signed 덧셈 감김과 float32 저장 순서를 유지하고 위치는 반환만 한다. 깊이는 자식 우선이며 재귀 뒤 현재 next를 읽는다. Move와 MoveRelative는 Push 전/후에 읽는 기준점·부모·크기를 구별한다.

float의 직접 복사 비트와 산술 결과를 구별한다. E410의 원문 floatBits를 사용하고 산술 쓰기에서는 이전 복사 비트를 버린다. 입력/가상 부착의 선택적 xBits/yBits와 정렬 반환의 비열거 비트는 신호NaN 같은 직접 복사를 보존한다. 일반 숫자 장치의 인자 계약은 그대로다. 부분 실패를 트랜잭션으로 되돌리지 않는다.

## 검사와 첫 실패

새 검사 **6/6**, 생성/수집/Draw와 함께 **31/31 통과**했다.

- `gumptree-x86.tsv`의 originals **574행**: 전체160BYTE 노드·현재8전역·정렬 반환 비트·가상 Redraw/Push/Attach/Kill/Project 호출 순서와 **각 콜백 당시 모든 노드/전역**이 첫 대조에서 일치했다. 응답은 fixture의 token/offset/DWORD만 안정된 현재 노드에 쓴다. 기대 부착/정렬 결과는 계산하지 않는다.
- `gump-attach-diagnostic-x86.tsv` **432행**: signed GT/상한·checking의 전체DWORD·writer 변경·상세 ASCII·전체 노드와 첫 몸체 도달 표기가 첫 대조에서 일치했다.
- 실제 GumpCreate→GumpTree 부착/Push/재부착→GumpCollection→정렬→GumpDraw/GumpPixelTarget의 같은 캐시/깊이/clip·픽셀을 CPP 조립 시험과 대조했다.
- 가상 Push/Attach를 같은 실제 트리로 다시 연결해 자식 재배치가 링크/위치/깊이에 도달함을 확인했다. unsigned 깊이 감김이 수집의 signed WORD 키−16으로 전달된다.
- 실제 진단 writer가 첫 쓰기 전에 GT/flags/부모/checking을 바꾸면 그 현재 필드로 후속 부착한다. 미선택 writer 생략·없는 토큰/장치·void·지도 정렬/순환·월드 투영 누락·redraw 예외 후 이미 쓴 링크/위치 보존을 검사했다.

첫 초안5/6 통과, 한 실패는 별도 writer 조립 장면의 초기 입력/기대 플래그 오류였다. 공통 create가 추가한 DrawClip 비트를 빠뜨려 비교했다. CPP의 해당 진단 시험과 같이 flags9를 명시하여0x200001 기대값으로 대조했다. 원본574/432행 및 제품 몸체의 교정은 없었다.

원본 자료 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3926/3926 통과**, 실패/취소/건너뜀0(334.2초/372.3초). 대응표 오류0, port662·partial58·todo176이다.

## 다음

GumpLifetime의 지연 삭제·기본 소멸자·idle/overlay/dialog 표를 같은 생성/트리 저장소에 연결한다. 이어서 이벤트/입력과 파생 메뉴 처리기를 갖춰 RawMenuControls를 조립한다. 실제 ClientRendererFrame/Session/앱 수명은 미연결이며 미션은 아직 플레이할 수 없다.
