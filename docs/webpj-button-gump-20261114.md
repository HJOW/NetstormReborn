# webpj 공통 버튼 상태·입력 (WB04-E416, 2026-11-14)

RawMenuControls 선행 **ButtonGump 349줄(.cpp/.h, partial→port)**을 완료했다. 기준선 `1f00a25a` 전체 소스·ButtonGumpTests·buttongump-x86 fixture를 읽었다. 두 소스의 기준선 뒤 변경은 없다. CD/추가판은 웹10.78 범위 밖이다. StoneButtonGump 생성/부착/출력은 다음 단위이며 이번에는 상태/입력의 실제 기본/돌 버튼 가상 몸체를 완료했다.

## 같은 현재 상태

- ButtonGumpNode는 공통160BYTE와 같은 주소에 DWORD value/pressed/blink/flags·두 double 예약 시각·배경 토큰을 붙인다. ConstructBase는 기존 GumpCreate의 등록/serial·기본 초기화를 사용하고 미작성 파생 꼬리는 보존한다. 기본 색은 signed BYTE를 command로 부호 확장한다.
- 값/누름은 전체DWORD를 저장하고 같은 값도 redraw(false)를 요청한다. 돌 버튼은 쓰기 전 OR와 부모 redraw 뒤 현재 OR가 달라졌을 때 현재 배경DWORD 별칭에 Pressed를 쓰고 배경에도 redraw한다. 상태의 가상 분류는 호스트 분배 정보이며 원문 raw 필드에 새로 기록하지 않는다.
- Handle은 비활성·일반/토글/즉시·우클릭·값 명령을 원문 순서로 처리한다. 값 명령 이후에도 사건 분기를 계속하고 부모 통지의 식별자는 command 대신 현재 serial이다. Capture와 idle/Kill은 같은 실제 공통 처리기에 위임한다.
- 팝업은 현재 루트 직계의 visible/void와 무관하게 dead만 제외하고 Kill 전 저장한 next로 순회한다. Broadcast는 별도 왼쪽 폴링 release와 전달된 화면 좌표를 구별하며 현재 좌표→포함 판정 뒤 누름/시각을 갱신한다.
- 첫 반복0.4초·다음 반복0.05초는 서로 다른 벽 시계 조회를 사용한다. pulse 해제가 현재 idle 표에서 자신을 제거해도 이미 진행 중인 idle 후반의 반복 판단을 취소하지 않는다. 지연 프레임 횟수를 보충하지 않는다.
- BindButtonGump는 이벤트와 캡처의 서로 다른 기존 fallback·프레임 idle/통지를 보존하고 호출 때 현재 처리기를 선택한다. 시각/입력/소리/배경/가상 장치 부재는 도달한 위치에서 거부한다.

## 검사

새 **5/5 첫 통과**, 주변 Gump/메인 메뉴 **93/93 통과**다. 원본 대조/제품 몸체의 실패 교정은 없다. 첫 전체 검사는 두 Node 모두3972/3973이었다. fixture를 직접 읽지 않는 button-gump-support.mjs까지 manifest 소비자로 등록한 추적 메타 오류1건이 원인이며, 직접 읽는 button-gump.test.mjs만 소비자로 남겼다. 추적 검사3/3 재확인 뒤 전체 검사를 다시 수행했다.

1. 원본10.78 **252개 완성 입력**의 연속 공개 action 반환, 모든 **368BYTE 노드와 미작성 꼬리/미변경 double 비트**, 일곱 스칼라·등록1000/idle500/overlay100/kill1000/dialog16 전체 표를 대조했다. 모든 가상/벽시계/폴링 콜백 당시의 전체 상태도 원문과 일치한다. fixture의 명시 raw 응답만 같은 안정된 노드에 쓰며 기대 동작을 모형으로 계산하지 않는다.
2. 실제 GumpEvents→GumpInput→Button Handle→현재 Capture/idle→GumpFrame Broadcast→지역 좌표·현재 누름→release와 배경 Pressed·표시 입력을 한 저장소에서 연결했다. 해제 뒤 프레임 idle가 추가 호출하지 않는 것도 확인했다.
3. pulse 상태에서 실제 idle 해제 후 원문 후반 통지와1.05초 예약을 확인하고 별도 반복의3회 시계 조회를 대조했다.
4. 숨은/void 팝업의 실제 GumpLifetime.Kill과 같은 지연 큐·다음 캡처를 확인했다.
5. 루트 위치 생략·월드 부모 투영의 지역 좌표, redraw 예외 뒤 현재값 보존·손상/없는 토큰·순환·비선택 비활성·서로 다른 fallback/필수 경계를 확인했다.

원본 파일 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3973/3973 통과**, 실패/취소/건너뜀0(304.3초/337.8초). 대응표 오류0, port673·partial55·todo168이다.

## 다음

StoneButtonGump의 부분 문맥 초기화/라벨63BYTE·실제 GumpText 측정→FitToText→자식 배경 생성/부착/공통 수집→정렬/kind2→실제 배경/글리프 픽셀과 자식 우선 해제를 연결한다(E414 이월 부모 시험 포함). 기존 앱의 메뉴용 StoneButton은 아직 새 처리기로 교체하지 않았다. StyleText/메뉴·RawMenuControls·ClientRendererFrame/Session/앱 연결이 남아 실제 미션은 아직 플레이할 수 없다.
