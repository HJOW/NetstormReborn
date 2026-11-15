# dotnetpj 0.548 — 현재 connector·다리 수명·낙하 진단 부모

2026-11-15, HJOW-Athlon. R4-1의 현재 표면 connector와 다리 낙하 사슬을 cpppj 제품1313의 `RawBridgeLifecycle`·`RawSurfaceLifecycle`·`RawBridgeFallParent` 몸체에 맞췄다. 기존 대응의 부분 차이와 R6 목록의 낙하 부모를 이번 의존 단계에서 함께 닫았다.

## 구현

`BridgeConnectState.ConnectorTypeBox`는 원래 `ConnectorType` 속성과 같은 DWORD다. `RawSurfaceLifecycle.BindCurrentConnectorType`은 이 실제 주소를 빌리며, Native 다리 전처리는 각 finder 후보에서 현재 전체 DWORD를 다시 읽는다. 앞선 연결 삭제가 번호를 바꾼 경우 다음 후보에 반영하고, 하위BYTE만 같은 다른 번호는 허용하지 않는다. 독립 typed 경로의 값 인자는 그대로 유지한다. NativeStorage의 실제 장기 소유자 연결은 아직 남아 있다.

표면 수명 생성자의 공통 훅을 C++ 값 인자처럼 함수 사본으로 보관하도록 맞췄다. 대리자가 읽는 owner의 현재 값은 계속 공유하지만, 생성 뒤 외부 훅 객체의 함수 재대입이 이미 구성한 몸체를 바꾸지 않는다. 실제 공간 해시·finder를 통과하는 표면 분배 검사와 콜백 사본/현재 owner 검사로 확인했다. 다리의 판본별 추가 비트 위치도 PATCH40·CD35로 맞췄다.

`DelayedFallPayload`의 포팅 오류를 수정했다. `(20.75,21.9)`의 포장DWORD는5396이며 원문은 이를 수치5396.0f로 변환한다. 기존 C#은 DWORD를 float 비트로 재해석해 극소수 값을 반환했고, 낙하 사건이 좌표를 잘못 복원할 수 있었다. signed DWORD의 수치 변환으로 고쳤으며 기존 합성 검사도 이 원문 계약으로 교정했다. 새 원본 fixture 기대값은 수정하지 않았다.

`RawBridgeFallParent`는 checking을 한 번 읽고 선택한 현재 genus·진단 뒤 원래 SID/좌표로 같은 실제 Regular를 예약한다. 정상 복귀 진단, checking0의 미선택 장치, 현재 타입 조회, Host/Pool 동일성을 보존한다. CD는 PATCH 진단 부모를 거치지 않고 기존 자식 예약으로 간다. World의 표면 낙하 예약을 이 부모에 연결했다.

## 검증

`bridgeeffects-x86.tsv`의 원본6,170행을 연결했다(Link882·Pre2520·Post1800·Delay726·Pack242, PATCH·CD·10.37 자료). 추가로 PATCH Native840행을 같은 몸체로 재생했다. Pre/Post는 외부 효과 순서·동적 타입/상태 변경과 풀 전체 바이트를 대조했고, 비유한 좌표·음수·float 정밀도 경계를 포함한 낙하 포장의 최종float 비트를 확인했다.

`bridge-fall-parent-x86.tsv`200행은 현재checking DWORD·genus·복귀/중단 진단·원래 좌표 비트를 대조한다. 실제 Host/Kernel에서도 예약 전 진단 실패의 무할당, 복귀 후5396.0f payload·사건0x2692·시각9·count0·등록 수를 확인했다. 이 검사는 프레임을 진행하거나 실제 다리 낙하 플레이를 재현하지 않는다. CD·10.37은 관리형 코어의 독립 입력 대조이며 전체 판본 게임 지원으로 세지 않는다.

최초 관련199/199 통과 후 실제 finder·콜백 사본 검사2건과 기존 삭제/등록 검사를 포함해 최종 **211/211** 통과했다(신규12검사). 호스트 Release는 경고0·오류0이다. 전체 회귀는0.546 결과를 유지했다. 자료는 `extracted/dotnet-bridge-current-0548-initial/`·`-final/`이다.

L2는 같은 cpp34 기준선을 바이너리 SHA 확인 후 재사용하고 dn31경로를 새로 실행했다. 신규31회 모두 종료0·수집 오류0이며 원본1,394파일의 이름·SHA가 전후 동일하다. dn31경로의 모든 객체 상태는0.547과 같고, 파일 전체 동일12경로·frame행만 다른19경로다. 차이를 지우지 않고 원자료에 남겼다. cpp와 전체 일치는 여전히0/34이며 데모dn3경로는 미연결이다. `extracted/dotnet-l2-0548/`의 실행 인자·로그·`reuse.json`·`unit-comparison.json`에 근거를 보관했다. 실제 실행은 단순 진입/대기/덤프이며 HJOW-Athlon의 실행 확인 예외를 적용했다.

## 인수인계

PORT_MAP897행(L1 425·port472), fixture303개다. 신규3쌍과 `RawBridgeLifecycle`·`RawSurfaceLifecycle`의 실제 대조를 마친 기존3행만 기준을 갱신했다. `RawBridgeConnect.h`·World 등 부분 이식 대응의 전체 기준, 다른 fixture해시·floor/reflected는 유지했다. `RawBridgeFallParent`는 R6 목록에서 R4-1 완료로 옮겼다.

다음 R4-1은 현재 Content 정렬·부착·통지·수명 몸체와 NativeStorage 장기 공장/공간/World 수명, 제품 피해·해방·삭제·배치 상위 연결이다. 이후 R4-2 순서를 유지한다. 새 경로 플레이·M1 전체 일치 완료로 세지 않는다. 금지된 플레이 재현/MP·보호 파일/다른 프로젝트 변경은 없다.
