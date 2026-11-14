# webpj Gump 입력 어댑터·포인터 경계·Form 연결 GumpControlInput/GumpPointerBoundary/GumpFormBinding (WB04-E433, 2026-11-14)

RawMenuControls 선행 **Gump.cpp(GumpControlInput) 86줄·GumpPointerBoundary 77줄·GumpFormBinding 84줄(todo→port)**을 새 모듈 [`www/ui/gump-control-input.js`](../webpj/www/ui/gump-control-input.js)·[`www/ui/gump-pointer-boundary.js`](../webpj/www/ui/gump-pointer-boundary.js)·[`www/ui/gump-form-binding.js`](../webpj/www/ui/gump-form-binding.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 좌표/호출 순서, 실제 40칸 `InputQueue`·`GumpCreateState` 연결이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72 경로는 옮기지 않았다.

## 기준선 뒤 변경 (3.6절)

`GumpFormBinding`만 `4ee69f4f8`(20261109-105)에서 바뀌었다 — `BindMenuGumpForms`가 메뉴 상태의 사본 `objectFormType`뿐 아니라 현재 Gump TYPE 조회 `currentObjectFormType`을 같은 Host getter로 공급한다(이미 설치된 조회는 보존). 같은 커밋이 [E432](webpj-menu-gump-widgets-20261114.md)의 `MenuGumpWidgetState` 조회를 추가했으므로 이 연결도 **②로 판정해 이식**하고 대응표 base를 `4ee69f4f`로 올렸다. 웹 `GumpFormHost`는 독립 typed 입력(TYPE 3)만 가지므로(`BindCurrent`는 Native 전용, 3.6③) 현재 TYPE은 호스트의 `currentType()`이 있으면 그 값, 없으면 3이다.

## 구현

- **`GumpControlInput`(Gump.cpp)**: 복원된 `GumpInput` 트리 처리기와 별개로, 화면 단위 판정 영역 목록(`GumpControl{id,rect,menu,enabled=true,instant}`)을 소유하는 사각형 입력 어댑터다. `contains`는 왼쪽/위 포함·오른쪽/아래 제외, `event`는 창 밖 비트(`0x04000000`)면 취소, 종류 마스크(`0x03ffffff`)가 왼쪽 버튼이 아니면 **위치도 갱신하지 않고** 무시, 그 밖에는 위치를 갱신한 뒤 뗌이면 눌린 번호를 반환·캡처 해제, 누름이면 앞의 활성 영역 하나만(비활성이면 아무것도 안 함) 목록 행/즉시 버튼은 번호를 돌려주고 돌 버튼은 캡처·눌림만 기록한다. `move`는 눌림/호버를 새로 계산(눌린 모양은 캡처 번호와 같은 활성 영역에서만, 호버는 목록 행만)하고 `active=false`면 캡처를 지운 뒤 변화 여부를 반환한다. `setControls`는 이전 누름 캡처를 해제한다.
- **`prepareGumpPointerBoundary`(GumpPointerBoundary)**: 첫 폴링(요청 `0x120000`) 사본, 언어 3에서만 Ctrl 폴링(`0x110000`)을 읽어 둘 다 눌림이면 첫 code에만 release 비트를 더한다. 정적 초기화 비트 1/2(`005cafe8`의 다른 비트는 보존)로 좌/우 outside 기록을 0으로 만들고, `wasInside=1`에서 창 밖 포인터가 되면 좌→우 순서로 폴링해 기록하며, `wasInside=0`에서 안으로 돌아오면 **좌·우를 먼저 모두 폴링한 뒤** 기록상 눌려 있었지만 지금 뗀 버튼마다 폴링 사건을 raw code/x/y(signed) 그대로 큐에 예약한다(좌 콜백 뒤 현재 우 기록을 다시 읽음). 마지막에 현재 포인터 값으로 `wasInside`를 덮어쓴다. 큐 변형은 실제 `InputQueue.push`에 연결한다.
- **`GumpFormBinding`**: `projectGumpObject`는 풀 슬롯의 +14/+18 비정렬 float32를 매번 읽어 현재 카메라 투영 `{x,y}`를 돌려주며 판본·SID 표현 범위(`0..0xffff`, 절단하지 않음)를 먼저 확인한다. `makeGumpFormNotice`는 알림 시점의 등록 표에서 소유 UI 토큰을 읽어(범위/없는 등록 거부) 현재 처리기에 (토큰, 객체 SID)를 전달하고, `releaseGumpObject`는 부모 SID를 쓰지 않고 `kill`만 한다. `bindMenuGumpForms`는 현재 TYPE을 메뉴 상태에 공급하고 `objectForm`(현재 TYPE과 같을 때만 `create`)·`formOwner`(DWORD 소유 ID)·`formObject`(SID 범위 확인 후 `attach`)를 같은 풀에 연결하며, `bindGumpFormLifetime`은 GumpForm이면 호스트 `destroy`로, 아니면 이전 삭제 경계로 분배한다(없으면 거부).

## 검사와 발견

새 `tests/gump-control-input.test.mjs` 7건·`tests/gump-pointer-boundary.test.mjs` 7건·`tests/gump-form-binding.test.mjs` 6건 **통과**. 입력 큐는 실제 `InputQueue`, 등록 상태는 실제 `GumpCreateState`이고 풀/Form 호스트/수명/카메라는 호출 기록 대역이다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 1건):** 원본 언어 인자는 `uint32_t`이므로 `0x100000003`은 호출 시 3으로 줄어든다(웹도 DWORD 비교)는 점을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 89개(gump-control-input.js 33·gump-pointer-boundary.js 39·gump-form-binding.js 17) 중 87개를 잡았다. 남은 2개는 `GumpControlInput`의 마지막 포인터 위치 초기값으로, 사건/이동이 항상 먼저 덮어쓰므로 동등 변형이다.

## 남은 경계

- 실제 `RawMenuControls` 허브와 Native `GumpFormHost.BindCurrent`(Native 전용). 앱의 메뉴 입력은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
