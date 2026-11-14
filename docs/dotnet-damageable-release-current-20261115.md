# dotnetpj 0.531 — 현재 종속 해방의 Native·전역·판본 경계

2026-11-15, HJOW-Athlon. 기준은 cpppj o/RawDamageableRelease.h/.cpp의230528e31fc1c239b5d1251a729b99041bf45d9e, 원문 PATCH0044b626~0044b80c/CD0046174e~0046195a다. 근거는 docs/exe/cpp-damageable-release-reconstruction.md, cpp-damageable-lifetime-owner-20261109.md, cpp-damageable-current-roles-20261109.md다. 디컴파일 산출물은 사용/변경하지 않았다.

## 구현

기존 typed Release의 부모 자격 검사를 유지하고 Native는 같은 물리 슬롯/현재TYPE로 직접 진입한다. Native의 좌표는 double 덧셈→signed64 CRT 절삭→하위DWORD를signed로 해석→float 저장이며, 축별0~255 검사를 앞에 추가하지 않는다. 공간BYTE는 두 좌표를 다시 편향/절삭한 뒤 DWORD y*256+x의 감김으로 선택하고 실제 flatBYTE 범위만 검사한다. typed의 발자국/좌표 사전 계약과 구별한다.

Native battle/BridgeTYPE은 호출자가 제공한 현재 조회자를 후보/생성 뒤 원문 위치에서 읽는다. 현재 allowedGenus는 같은 UIntBox owner를 후보마다 다시 읽고, 없으면 독립 상태를 읽는다. TYPE 표도 원문 소비 위치에서 조회한다. 객체 생성 뒤 후보 WORD와 생성된 타입/현재 BridgeTYPE을 다시 읽는다. 생성자에 전달한 함수 묶음은 값 복사하며 캡처한 실제 월드는 같은 수명으로 빌린다. MakeDamageableReleaseHooks도 원문 값 인자처럼 입력을 수정하지 않는 새 함수 묶음을 반환하도록 교정하고 별도로 검증했다.

자료 길이는 PATCH188/CD171 TYPE로 구별한다. 원본 자료4512행(PATCH/CD/10.37 각1504)을 모두 재생하고, 같은 PATCH1504행을 실제 Native Damageable/Carrier 부모와 Native 해방으로 추가 재생해6016회 실행을 대조했다. 원본/golden 변경과 새 fixture는 없다. CD 관찰은36BYTE, 관리 슬롯은50BYTE이므로 하네스는36BYTE만 준비/대조하고 뒤14BYTE 패딩 불변도 별도로 검사한다.

## 검증 진행

최초 집중15건으로 위 원본6016회·Native 경계/현재 전역/함수 사본·기존 거부 경로를 통과했다. 실제 나선→서버 공장→사제 Owner/Carrier 연결을 typed/Native로 함께 실행한 관련24검사도 통과했다. 첫 하네스 컴파일의 Carrier 메서드명은 실제 EnableNativeBody 뒤 PreDestroy API로 교정했다. 게임 창 없이 수행한 합성/단위 검사이며 사용자 제외 조건인 실제 포획/희생/공격/승리 재현이 아니다.

현재 공유 마스크 대신 독립 값을 읽는 대표 변이는 당시7검사 중6실패, Native battle 대신 독립 상태를 읽는 변이는7중1실패로 검출했다. 각 변이 뒤 소스 바이트/SHA를 복원했다. 추가 어댑터의 입력 불변/함수 사본 검사도 넣었으며 최종 전체 비증분 Release 빌드는 경고0·오류0이다. 원복/어댑터 교정 뒤 관련 Core66/66·Client23/23이 실패0·건너뜀0·각 종료0으로 통과했다. Core는 기존 피해/삭제·Native/Carrier·원본 재생과 실제 사제 공장/Owner, Client는 사제 월드/제품 등록·메타데이터를 포함한다. 최종 결과는 extracted/dotnet-damageable-release-0531-final/에 보존했다. 전체 회귀 완주로 합산하지 않는다(최근 전체는0.529). 로컬 자료는 extracted/dotnet-damageable-release-0531-mutations/와 최종 검사 디렉터리에 둔다(Git제외).

## 남은 실제 조립

이 단위는 해방 하위 몸체 현행화다. 현재 FortKeepGenus 주소는 cpppj ClientGameplayGlobals가 Editor prefix/객체 보존/해방에 함께 제공하지만 C# 실제 상위 owner/Host 연결은 아직 없다. 실제 사제 Damageable 삭제/해방·일반 지역 정책/배치·postDestroy/드롭·burning/피격 전체 결선은 후속이다. Native 경계를 조립했다고 제품 전체 삭제/배치 준비 완료나 실제 플레이 완료로 세지 않는다. M1전체0/34와 R5~R8·AI Host 등은 계속 잔여다.

AGENTS.md·보호 원본/CD·README 개요·cpppj·LEFT_JOBS.md·다른 프로젝트는 변경하지 않는다. PORT_MAP812행(L1 388+port424), fixture278개 유지. 새 외부 접속/NB 수정은 없고 목표는 active다.
