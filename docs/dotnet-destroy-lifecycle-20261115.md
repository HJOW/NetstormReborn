# dotnetpj 0.540 — 현재 공통 삭제 장부와 Form 선택 전달

2026-11-15, HJOW-Athlon. 직전 단위 0.539는01862ed1a다. 공통 수명의 기준은cpppj SquidDestroyLifecycle.h/.cpp의7ba0130b6b862e393a28f9e9ea7deaf29aaed3ff, Form 몸체는417f22d2967decbf2ad3a587c2190b9efb93f772, Form header는541624a91439623c5faeaf8fcc4b6283e93bbb0a다. 원본 바이너리와 디컴파일 산출물은 변경하지 않았다.

## 현재 저장소와 순서

RawSquidDestroyLifecycle은 명시 PATCH 진단 연결 때 실제 root의 현재 TYPE를 읽고 선행 raw/목록/보상 Plan 검사를 생략한다. form 단언의 정상 복귀 뒤에도 현재 타입을 다시 읽는다. 독립 188TYPE 구성과 typed 선검사는 유지한다. CD 복원은 이번 범위에 포함하지 않았다.

현재 전체 선택 DWORD와 실제 UI 기억점을 연결했다. abstract/buried 선택은 NaN·부호 있는0을 포함한 X/Y 원시 비트와 TYPE를 기억점으로 전달한 뒤 선택을 해제한다. 실제 선택 getter가 있으면 독립 Selected 사본을 강제로 지우지 않는다. 후행 root 선택 조회도 같은 현재 getter를 다시 읽는다.

장부는 같은 TotalMade 배열과 현재 비용·억제 저장소를 사용한다. IntBox/BoolBox 대여와 독립 TotalCost/Suppressed 사본을 구별하며 비용 Pop 소비자도 같은 현재 비용을 읽고 쓴다. 독립 Pop 선검사와 Native Pop 전체 이식은 기존 범위를 유지한다. 손실 좌표는 실제 FloatBox X/Y를 따로 빌리고 독립 LostX/Y를 덮어쓰지 않는다. 현재 로딩 깊이·fort 모드·silent TYPE·H 치환/대체/9칸 역할 DWORD를 각 분기에서 읽는다. post 비용은 현재 TYPE의 비용 대신 최초 로딩 비용표를 선택하며 미연결이면 생성 당시 비용 사본을 쓴다.

삭제 순서는 선택→Graph→owner 공장→실제 보상→현재 로컬/전역 수 감소→소리/좌표→provider 압축→현재 AI 삭제 통지→현재 TYPE의 global 공장 압축→깊이 감소다. 보상을 공급하면 같은 풀/같은 장부인지 확인하고 실제 RawSquidReward.Refund를 한 번 실행한다. Native 목록은 signed count가 양수일 때만 물리를 조회하며 매 반복에 읽은 같은 배열에 압축한다. 용량 선검사·음수 count 정상화·inactive 꼬리 지우기는 하지 않는다.

사제/표면/Depend/Content/Gump 훅 복제는 SelectedRaw 함수도 전달한다. Form 분배는 Raw-only 선택을 허용하고 이미 만든 훅도 다음 현재 부모의 전체 선택을 읽는다. 현재83 자산 VT/네 Form VT로 분배하며 Process VT는 같은 호스트에 명시 등록한다. Native SetAssets는 최초 World의 가상+10 부모를 유지하고 현재 Pre/Post 효과 경계만 교체한다. 미지/미등록 VT는 선택 위치에서 실패한다.

## 검증

기존 destroylifecycle PATCH576행(Pair384+Destroy192), destroygraph PATCH576행(Pre/Post/Destroy 각192), island-graph-frame PATCH24행, 합계1176 고유 입력을 typed/Native 두 경로에서 재생한다. 합계2352 실행의 원본 기대값은 변경하지 않았다. Native 그래프 입력은 역할 DWORD를 빌리고 독립 BYTE 사본을 다르게 두어 실제 공유 역할 사용도 확인한다. 외부 진단/trace shim만 연결했으며 이것은 새 PE 관측으로 세지 않는다.

현재 계약15경우는 lazy 검사/복귀 단언, 전체 선택과 원시 기억점, 선택 효과 뒤 현재 억제, signed count0/음수2개, 반복당 한 포인터/꼬리, provider→AI→현재factory 순서, 환불 효과 뒤 로컬 owner/손실 억제/좌표, 높은 owner 비트, silent 타입의 좌표 유지, 최초 비용 캐시와 현재 비용/독립 사본, 실제 SP/샘 풀 보상 한 번과 같은 장부, 현재 Form 부모 교체와 최초 Native+10 보존, VT/TYPE 별칭 분배를 검사했다. 이 합성 계약은 별도 PE 관측행으로 세지 않는다.

최초 집중31/31 통과 후 선택16비트 축소·반복 중 두번째 배열 포인터 재조회·최초 비용 캐시 무시 변이는 각각 현재15검사 중1개 실패로 검출했다. 각 변이 뒤 제품 바이트/SHA-256을 복원했다. 자료는 extracted/dotnet-destroy-0540-initial/·-mutations/·-final/에 둔다(Git 제외). 최종 전체 비증분 Release 빌드는 경고0·오류0, 관련 Core427/427·Client45/45는 실패0·건너뜀0·각 종료0으로 통과했다. 연결 fixture294개 SHA 동기화와 PORT_MAP 계약도 통과했다. 직전 전체 Client1232건/실제31 단순 미션 진입은0.538 결과이며 이번 집중 결과와 구별한다.

## 남은 연결과 범위

제품 Host에서 Native root를 활성화하지 않았다. ProcessHost의 전체 DWORD 선택 전달은 cpp defaultAssets_의 장기 기본 장치가 C#에 아직 없어 그 소유/기본장치 자체를 먼저 이식해야 한다. RawSurfaceLifecycle의 현재 connector TYPE/Native Bridge 분기, Depend/Content/Gump의 남은 현재 VT/Native 몸체 차이, damageable 전처리/해방·사제 상위 배치/삭제도 후속이다. 부분 훅 전달만 반영한 파일의 전체 PORT_MAP base는 올리지 않았다. 이 부품 검증을 제품 삭제·플레이 완성으로 세지 않는다.

PORT_MAP863행(L1 418+port445), fixture294개 유지다. 공통 수명/Form 몸체 두 기존 행의 기준을 올리고 header/FloatBox 세 쌍을 추가했다. 전체 floor/reflected는 바꾸지 않았다. M1 전체0/34·목표active. 이번 게임창/금지 실제 플레이/MP 재현 없음. 다른 프로젝트와 보호 파일은 변경하지 않았다.
