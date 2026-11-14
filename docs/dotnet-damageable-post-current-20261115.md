# dotnetpj 0.532 — 삭제 후처리 현재 역할·드롭·실제 HP/프레임

2026-11-15, HJOW-Athlon. 직전0.531은d5d944008다. 기준은 cpppj o/RawDamageablePostDestroy.h/.cpp의079f170e3a44eb7329d0d48613cf4002e1b4619d, PATCH0044b9e0·드롭0044b830이다. 근거는 docs/exe/cpp-damageable-post-destroy-20261014.md, cpp-damageable-lifetime-owner-20261109.md, cpp-damageable-current-roles-20261109.md다. 디컴파일 산출물은 변경하지 않았다.

## 구현

현재 battle/boss·섬 정리 제외TYPE·드롭 제외/특별TYPE·실제 population 목록·피해 이벤트를 장기 owner에서 소비 때 조회하도록 이식했다. 목록 개수는 전체DWORD를 signed로 읽는다. 원문대로 수량을 한 번 정한 뒤 생성 루프 중 입력이 바뀌어도 반복 수는 유지한다. 각 피해 예약의 사건 번호와 다리 붕괴 조회 뒤의 권한은 그때 다시 읽는다. 함수 묶음은 값 복사하고 캡처한 실제 월드 수명은 유지한다.

실제 RawSquidReward HP와 RiftTypeFrames를 빌리는 생성 경계를 추가했다. 기존 RawHitPoints/Assets.FrameCode 검사 API도 유지하되 같은 하위 몸체로 들어간다. 원문 프레임 표는 복사/변환하지 않고 직접 빌리며 NotifySurface 뒤 현재 배열/타입/코드를 다시 읽고 RawBridgeFrames.CrackedFrame을 사용한다. HP도 현재 실제 부모의 HasHitPoints/MaxHitPoints 메서드를 소비 위치에서 호출한다. Weaken은 실제 Dais/Altar/삭제 파생이 공유할 수 있도록 원문대로 공개했다.

판본별 슬롯 TYPE길이(PATCH188/CD171), Owner34/32·extra40/35·frame36 DWORD/34 BYTE를 구별한다. CD의 드롭 제외 타입 비교는 앞4개뿐이다. signed64 CRT 절삭과 DWORD 더하기/packed count 순서는 기존 몸체와 대조해 유지했다.

## 검사

PATCH 원본2246관찰(Post1606+Drops640)을 기존 부모와 실제 보상HP/원문 프레임 부모로 각각 재생해4492실행 일치를 확인했다. fixture 전체에는 CD2166행도 있지만 CD의 전체 HP 부모가 아직 없으므로 해당 재생을 완료로 세지 않는다. 원본/golden/fixture를 바꾸지 않았다.

현재 대여 추가 검사는 signed 개수0/20/21/FFFFFFFF/80000000, 제외/특별 역할과 루프 수 고정, 섬 정리 뒤 battle, 각 예약의 현재 피해 이벤트, 붕괴 조회 뒤 boss, Notify 뒤 실제 프레임 배열 교체를 검증한다. 최초 피해 예약 검사에서 부모 최대HP 입력을 생략해1피해가 나온 것은 검사 준비를100HP로 보완했다(제품/기대값/오라클 변경 없음).

CD 경계 검사는 HP가 선택되지 않는 실제 Drops/Weaken으로 제외4개·extra35·frame34 한BYTE 쓰기와 인접 extra/관리 패딩 불변을 확인했다. 이 검사만의 기존 HP 객체는188칸이며 같은 풀만 빌리고 HP 조회에 도달하지 않는다. CD 실제 HP/후처리 전체를 확인한 것으로 합산하지 않는다. 현재 역할/프레임과 원본 재생을 포함한 집중13검사가 통과했다.

signed 목록 개수를 양수로 보정한 대표 변이는 집중6개 중2실패, 실제 프레임 배열을 사본으로 보관한 변이는6개 중1실패로 검출했다. 각 소스 바이트/SHA를 복원한 뒤 전체 비증분 Release 빌드 경고0·오류0을 확인했다. 원복 뒤 최종 관련 Core78/78·Client23/23은 실패0·건너뜀0·각 종료0으로 통과했다. 결과는 extracted/dotnet-damageable-post-0532-final/에 보존했다. 최근 전체 회귀0.529와 구별하며 이번 집중 결과를 전수 완주로 합산하지 않는다. 로컬 자료는 extracted/dotnet-damageable-post-0532-mutations/와 최종 검사 디렉터리다(Git제외).

## 남은 실제 조립

실제 Host의 population·GameAux 역할·권한 대여와 공통 삭제/사제 해방·드롭 생성·표시/프레임 실행의 전체 결선은 후속이다. 이 단위는 실제 부모를 받을 수 있는 하위 모듈을 현행화한 것으로, 게임 플레이/삭제 전체 완료를 뜻하지 않는다. 다음은 burning/일반 피해/실제 프로세스 묶음을 현행화해 같은 사제/삭제 부모에 조립한다.

PORT_MAP813행(L1 389+port424), fixture278개 유지. 새 외부 접속/NB 수정은 없다. AGENTS.md·보호 원본/CD·README 개요·cpppj·LEFT_JOBS.md·다른 프로젝트를 변경하지 않았다. 게임 창·실제 사제 포획/희생·특정 적 공격·시나리오 승리·MP 재현은 하지 않았다. 전체 목표 active·M1전체0/34·후속 단계는 잔여다.
