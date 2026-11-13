# webpj Gump 이벤트·입력·프레임 (WB04-E413, 2026-11-14)

RawMenuControls의 같은 공통 트리에 필요한 GumpEvents(162줄)·GumpInput(132줄)·GumpFrame(137줄), **세 단위431줄**을 이식했다. 기준선 `1f00a25a`의 .cpp/.h 전체와 각 CPP 검사/독립 관측을 읽었다. 기준선 뒤 여섯 소스 변경은 없다. CD/추가판 분기는 웹10.78 범위에서 제외한다.

## 실제 현재 상태

- Events는 같은 GumpCollection의 현재 Bounds/캐시를 쓴다. 직계 선택은 void를 거르지 않고 전체 선택은 visible/void/dead/화면 밖 통과와 현재 마스크·signed 최소 깊이를 적용한다. 같은 깊이는 첫 방문이 우선이다. 전달은 자식 우선/visible만 제한하며 성공 토큰을 반환한다. Bubble은 현재 부모 링크를 읽고 처리기의 전체DWORD를 반환한다. 현재 dialog와 help 사건의 반개방 경계·키보드/마우스 기본 필터도 복원했다.
- Input은 같은 focus/dialog/override/overlay를 쓴다. 첫 캡처 유지·조건부 override 설정/그 외 해제·중복보다 앞서는 dialog 한도 로그를 보존한다. 캡처 실패를 재전달하지 않으며 성공 뒤 현재 캡처를 읽고 지운다. overlay는 현재 부모 visible·같은 실제 전체 Pick을 검사하고 별도 overlayEvent를 전달한다. 첫 대상의 처리기가0을 반환해도 다음 후보로 재시도하지 않는다.
- Frame은 루트를 제외한 비0등록 칸 수와 **현재 signed count**로 통지를 끝낸다. idle은 현재500칸을 필터 없이 호출한다. 재그리기는 최상위 next를 호출 전에 저장하고 하위 next는 재귀 뒤 현재 값을 읽는다. 상위 visible/비dead와 하위 visible 전파 필터도 구별한다.

별도 UI 선택 모형/트리를 만들지 않았다. 같은 E410~E412의 생성/트리/수명 저장소를 대여하며 처리기 누락은 선택된 때 거부한다. OS 입력·실제 파생 UI 가상 함수·RawMenuControls/Client 자동 프레임은 아직 별도 연결 대상이다.

## 검사

새 `gump-events-input-frame.test.mjs` **17/17 첫 통과**, 생성/트리/수명/수집/Draw와 함께 **55/55 통과**했다. 최초 원본 대조3건과 추가 실제 조립/계약14건 모두 제품/기대값 교정이 없다.

1. `gumpevents-x86.tsv` originals **257행**: 전체160BYTE 노드·7전역·선택/전달/전파 반환과 필터/처리기/투영의 **콜백 당시 전체 상태/순서**가 같다.
2. `gumpinput-x86.tsv` originals **232행**: 전체 노드·11전역·overlay100/dialog16 전체 표·반환·콜백 상태가 같다. 같은 실제 Events/Collection/Lifetime.RemoveDialog를 호출했다.
3. `gumpframe-x86.tsv` originals **270행**: 전체 노드·현재 count/등록1000/idle500·통지/idle/재그리기의 콜백 당시 상태가 같다. 응답은 독립 입력에 지정된 DWORD 위치/값만 적용한다.
4. 실제 Create/Tree/Lifetime과 연결한 선택·캐시·dialog 삭제 뒤 후속 선택, 전달/전파의 다른 반환·visible/void/dead 규칙·help 경계를 CPP 시험과 대조했다.
5. 실제 캡처/dialog→Push/Kill·overlay/override, 현재 캡처 변경과 재전달 금지, 부모 visible과 전체 Pick·별도 사건/전체 반환·첫0종료·dialog 한도 우선/미사용 칸을 확인했다.
6. 실제 idle 중 뒤 노드를 Kill하면 idle/통지에서 제외되고 Cleanup 뒤 저장소/등록표에서 해제된다. 마지막500번째 idle 칸·무필터 실행·현재 signed count/루트 제외와 상위 저장next/하위 현재next를 검사했다.
7. 없는 토큰/장치·부모/형제/자식 순환·음수 dialog 개수와 각 필터/입력/idle 경계 예외 앞의 쓰기 보존을 확인했다. 조기 종료한 분기에 가상 장치를 요구하지 않는다.

조립 검사의 파생 UI 가상 처리기는 CPP 부품 검사와 같은 명시 반환/관측 경계다. 실제 RawMenuControls의 모든 파생 UI를 생성했다고 주장하지 않는다. 원본 자료 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3950/3950 통과**, 실패/취소/건너뜀0(317.7초/352.5초). 대응표 오류0, port666·partial58·todo172이다.

## 다음

GumpRedraw·GumpHelp·공통 Gump 조립과 파생 생성기 의존을 확인해 같은 Renderer/입력 수명에 연결한다. StyleText/메뉴·RawMenuControls와 실제 ClientRendererFrame/Session/앱 수명은 미연결이며 미션은 아직 플레이할 수 없다.
