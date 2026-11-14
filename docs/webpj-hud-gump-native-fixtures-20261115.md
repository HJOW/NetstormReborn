# webpj HUD 보조 Gump 원본 관측 연결 (WB04-E486, 2026-11-15)

3.8절 이월 목록의 "이미지/메타/HUD 나머지" 중 `production-hint`(126행)와 `floating-sp`(426행)를 원본 10.78 독립 x86 관측에 대조했다. 552행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈·시험 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다.

| fixture | 행 | 대상 | 비교한 것 |
| --- | --- | --- | --- |
| `production-hint` | 126 | `ProductionHintGump` | 구성(C)·Idle(I)·Draw(D)·Delete(X)·Show(W) 입력의 노드 필드 전체(160바이트 중 관측 구간 + 파생 필드 + 64비트 만료 시각)·마지막 일련번호·호출 순서(시계/할당/복제/번역/서식/맞춤/부착/삭제) |
| `floating-sp` | 426 | `FloatingSpGump` | construct/show/delete/idle 입력의 노드 float32/double 비트(vtable/depth/x/y/속도/가속도/패딩/stamp)와 호출 순서(Redraw 안의 필드 변경 포함). 두 x87 정밀도 열이 모두 같음 |

- 입력 열만 장치에 공급하고 관측 열은 비교에만 쓴다(cpppj 시험과 같은 방식). 생산 안내의 만료 시각처럼 NaN 비트가 의미 있는 double 필드는 64비트 정수로 보관하는 접근자로 바이트를 잃지 않게 했다.
- 노드의 원문 raw 직렬화는 기존 `tests/gump-node-support.mjs`(`decodeGumpNode`/`encodeGumpNode`)를 재사용했다.

## 변형 확인

자동 변형 78개(생산 안내 48·비용 표시 30) 중 16개가 새 시험만으로 생존했고, 기존 `production-hint-gump`·`floating-sp-gump` 시험과 합치면 1개(`new ArrayBuffer(4)`의 `5`로의 변경: float32 비트 보기의 크기가 더 큰 동등 변형)만 남는다. 나머지 15개는 클래스 필드 기본값·생성자 앞 단계의 초기값(같은 생성자의 뒤 단계가 덮어씀)·기존 비트가 이미 켜진 플래그의 `|` 변경이며 기존 시험이 모두 잡았다.

## 남은 것

3.8절의 `hud-nsinfo-attach/create`(311/171행)·`minimap-*`(1,056/122/4,723행)·`gump-pointer-boundary`(3,879행)·`pointer-special-trace`(37행)·`menugump`(309행)·`inventory-about*`(1,297/2,593행)은 후속 단위다.
