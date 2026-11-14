# dotnetpj 0.525 — 실제 사제 월드 묶음과 지연 연결

2026-11-14, HJOW-Athlon. cpppj 기준 `bcdd11383`의 `client/RawPriestWorldBindings.h/.cpp`에서 월드 조립·공개 API와 `o/RawPriestPreDestroy.h/.cpp`의 현재 Native 경계를 옮겼다. 디컴파일 산출물은 읽거나 변경하지 않았다.

## 구현

`RawPriestWorldCore`는 같은 풀의 HP·공장·소유자·후처리·프레임과 실제 타입/방향/점유 지도를 빌린다. 표면 지도는 `Hash.BorrowEntries(0)`과 같은 실제 배열이어야 한다. 장기 사제 상태·목록·현재TYPE를 `RawPriestWorldState`로 묶었으며 함수 객체와 중첩 훅은 생성 시 값 복사한다.

원문 순서로 보호막 조회→사제/소유자→보호막 생성→프레임/낙하→Carrier 확인/후처리→사제 접두/후반→Carrier/사제 전처리→보호막 후처리/정기→내용물/회복→생성을 조립한다. 공유 Native 모듈 네 종류는 같은 실제 풀만 허용한다. 모든 생성이 성공한 뒤 공통 postPop의 사제·보호막 분배를 확장한다. 늦은 실패 전에 연결한 보호막 정기의 현재TYPE 상자는 원문과 같이 남지만 가상 분배 등록은 바꾸지 않는다.

Host·Damageable·Placement의 순환 의존은 지연 연결한다. 각각 같은 풀에 한 번만 연결하고 `ValidateReady`가 실제 세 경계를 순서대로 요구한다. 아직 없는 효과를 성공 기본값으로 치환하지 않는다. 권한 네 소비자는 같은 `RawAuthorityState.ReadBoss` 대리자의 실제 owner/메서드로 확인하고 현재값을 빌린다. 알 수 없는 클로저를 같은 주소로 추정하지 않는다. 값형 권한은 공유 연결 전에만 허용한다.

Player9 전체 배열은 정확한 판본별 길이와 같은 배열 참조만 허용한다. 실패는 기존 대여를 보존한다. `Dispose`는 외부 Kernel을 먼저 비운다는 원문 수명 계약 아래 공통 postPop 등록과 HP의 Player9 대여를 반납한다. 관리 환경의 중복 Dispose는 한 번만 처리한다.

정기 분배는 이동 선택 전역과 별개로 TYPE158+실제 사제VT를 요구한다. PATCH 보호막은 TYPE를 추가 조건으로 쓰지 않고 실제VT로 선택한다. 601/602/603은 기존 실제 정기·회복·낙하 몸체, 604/605는 선택된 외부 부모, 나머지는 기존 부모/원래 payload다. Damageable 갱신은 free·현재 damageEffect0에서 자연 반환하고 나머지는 원래 burning 전체 경계를 요구한다.

사제 Native 전처리는 typed의 TYPE·상태·목록·Carrier 선검사를 생략한다. 현재 signed 목록 개수로 안정 압축하며 음수는 저장소를 조회하지 않고 그대로 둔다. 물리 범위에 도달하면 그 시점에 진단하고 앞선 압축 쓰기는 유지한다. extra9는 목록/보호막을 생략하고 Carrier만 실행한다. 실제 VT 분배는 예약1~4도 허용한다. typed 두 판본의 선검사 순서는 유지한다.

## 검증

기존 독립 원본 `priestdestroy-x86.tsv`의6552관찰을 typed 세 판본으로 대조하고 PATCH Native2184입력을 추가 재생했다(합계8736실행, 고유6552관찰). 10.37의 이 모듈 관찰은 CD와 같은 가상표/extra35/36BYTE 배치이므로 CPP 재생과 같이 CD 배치로 구성한다. 처음 PATCH로 구성한 검사 입력의 가상표 불일치를 이 근거로 교정했고 원본/golden/제품을 바꾸지 않았다. 기존 PATCH2184검사도 유지한다. LF정규화 SHA-256은 `56da3fd0581263e9b9d1ba9e39cd60ac1158d5129cbe67ea8e730c873aa6c19d`이다.

사제 집중13검사는 기존 검사·원본 재생4·새 Native 계약3을 포함한다. 월드 조립12검사는 생성 실패 후 등록 보존, 같은 실물 지도/목록, 현재 권한 owner, Player9 대여/반납, 지연/중복/다른 풀 거부, 원래 함수 사본, 현재 damageEffect/이동TYPE, Native 장기 모듈 네 종류, 실제 가상 postPop→같은 Host/Form/Kernel 회복 틱과 정리를 확인한다. 최초 합성 장면의 필수 SP 저장소 누락은 같은 난수 스트림의 실제 저장소로 보완했다.

signed count를 unsigned로 바꾼 변이는 Native 집중3건 중1건, 현재TYPE 상자 대여를 값 사본으로 바꾼 변이는 월드12건 중3건이 검출했다. 두 제품 소스를 바이트 원복했다(전처리774ef24609faebf8e1f3cf5cdc8fe82b530ac112b7e37a79e3c762f91df3bcd0, 월드c6410d2a098e253d468612bee8b86b575e5db418843bd6249d8eea93314c09e4). 전처리의 판본 설명 주석은 이후 실제 지원 범위에 맞게 정정했다. 두 번째 변이의 최초 컴파일 불가 표현은 UIntBox의 실제 Value 초기화식으로 교정 후 검사 실패를 확인했다. 원복 뒤 전체 비증분 Release 빌드는 경고0·오류0, 관련Core546/546·Client44/44통과(실패0·건너뜀0·각종료0)다. 전체 회귀 완주로 합산하지 않는다. 로컬 근거는 `extracted/dotnet-priest-world-0525/`에 둔다(Git제외). PORT_MAP797행(L1 381+port416), 공유fixture275개 유지. 새로 해당하는 10차 NB항목은 없다.

## 보호와 잔여

실제 게임·MP를 실행하지 않았다. 사제 포획/희생·특정 적 공격·시나리오 승리의 실제 재현은 사용자 확인으로 남긴다. 보호 원본·AGENTS.md·README개요·CPP·WB를 변경하지 않고 외부 접속을 추가하지 않았다.

이번 결과는 실제 모듈을 합성 월드에 조립한 것이다. 제품 HostSession의 전체 사제/가상 공간/소리/피해·해방 연결은 잔여다. 현행 Session 결선 검토에서 기존 RawPriestRegen의 spReads·공유 sample/changedSample/사제TYPE와 RawSpStore의 현재 tracking owner가 아직 과거 값형 경계인 것을 확인했다. 따라서 다음0.526은 이 회복/SP 묶음과 HP getter 순서를 현행화한 뒤 제품 HostSession을 연결한다. 이번 효과 복사는 현재 C#의 일곱 회복 훅까지이며, 아직 없는 선택 spReads를 연결했다고 세지 않는다. CD 전체 월드는 C# 사제 접두/후반·공장이 PATCH 전용이므로 아직 구성하지 못한다. Native 공유 모듈은 원문처럼 호출자가 실제 장기 본문 선택을 한 뒤 빌린다. 일반 지역 정책·AI 시작·그 밖의 객체 후처리·M1 전체일치0/34는 잔여다.
