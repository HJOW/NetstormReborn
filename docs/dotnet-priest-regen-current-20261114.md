# dotnetpj 0.526 — 현재 사제 회복·SP 추적 owner와 HP getter 순서

2026-11-14, HJOW-Athlon. cpppj 기준 `f7e81025d`의 RawPriestRegen.h/.cpp, RawPriestRegenHitPoints.h/.cpp와 SpStore.h/.cpp의 현재 추적 주소·첫 분산배열 대여를 옮겼다. 디컴파일 산출물은 읽거나 변경하지 않았다.

## 구현

회복의 현재 사제TYPE·공유 표본/진단 DWORD와 실제 SP 조회기를 연결했다. 추적 sequence는 같은 IntBox를 읽고 쓰며, 기존 독립 sequence 값은 명시적인 장기 주소가 연결되면 보존한다. 보상 getter 뒤 TYPE 번호를 추정하거나 프레임마다 값 사본을 동기화하지 않는다.

PATCH는 표본 쓰기→현재 추적 주소+3/9600순환→원문 HP getter→setter/표시/피해 후처리→SP 감사→현재 중립 조건→관찰 offset/추적 주소→측정→현재 첫 배열→독립 signed 순열WORD 순서다. 첫 배열이 null이어도 뒤 WORD를 읽는다. 표본·진단 주소와 사제TYPE는 각 원문 소비 위치에서 다시 선택한다. 중첩 SP 조회기를 포함해 함수 객체를 생성 시 값 복사한다.

HP 부모는 초기 현재HP/최대HP 뒤 필요할 때만 TYPE 장치를 요구한다. 각 증분에서 번호를 먼저 고정하고 PATCH는 현재HP→TYPE HP, CD는 TYPE HP→현재HP 순서로 읽는다. 후보 뒤 최대HP를 다시 읽으며 포화 시 최대HP를 한 번 더 읽고, 비포화 시 증분을 다시 계산한다. signed ADD는32비트 감김을 보존한다.

RawSpStore는 첫 초기화 전 같은 추적 IntBox만 빌린다. 다른 owner·늦은 연결·기존 독립 기록 콜백과의 혼합은 거부한다. 초기화는 실제 offset과 추적 주소를 함께 쓰고 회전은 실제 offset만 전진시킨다. 첫 배열은 첫 할당 정상 복귀 뒤에 같은 실제 분산배열 참조를 발행한다. 초기 순열의 C++ zero-initialization도 반영했다. 기존 독립 원본 검사의 기록 콜백 API는 유지한다.

사제 월드 효과 복사와 회복 생성에 선택 SpReads를 전달했다. 실제 월드 회복→같은 장기 추적 주소/SP 회전의 합성과 중첩 함수 사본을 검사한다. 제품 HostSession 연결은 잔여다.

## 검증

기존 회복 fixture4539관찰 중10.78 PATCH1513입력을 대조하며 기존 SP 저장소 검사를 유지한다. 새 `priest-regen-hit-points-current-x86.tsv`는 양판본864입력(432씩)의 반환·전체 getter trace·효과 뒤 HP/TYPE를 독립 원본과 대조한다. 고유 회복/HP 관찰2377개이며 SP 기존 관찰은 별도로 유지한다. 새 fixture의 LF정규화 SHA-256은 `1e10b6d2fd324225a3b8a7c3b25a7cadf5b5dae35f7c09c89f56f423649efca4`이다. golden/원본을 다시 생성하지 않았다.

현재 공유 owner·SP 비트 보존·측정 안의 지연 초기화·조회순서/중첩 함수 사본·null 배열 뒤 WORD·현재 표본/진단의 독립 조건·현재 사제TYPE/Actor 구분·가상 분배·부분 초기화의 실제 배열 주소를 검사한다. Actor TYPE가158이 아닌 회복 진입은 원문과 같이 실제 VT로 들어오되, setter가 선택되면 기존 사제 상태 wrapper의 TYPE158 검증에 도달한다.

CD 전체 회복1513입력씩의 추가 대조를 시도했으나 현재 RawSquidReward 생성자의 PATCH 전용188타입/SP 저장소 제약으로 원문171타입 부모를 만들 수 없었다. 이 대조용 임시 검사 변경은 원문 상태로 반납했다. CD 통합은 통과로 세지 않으며, 별도 부모 복원 범위로 남긴다. 양판본 순수 HP getter864입력은 유지한다.

PATCH getter 순서 전치와 실제 tracking 주소를 값 사본으로 바꾼 두 변이는 각각13검사 중1건이 검출했다. 두 소스를 바이트 원복했다(HP 부모 bf41ebcec6c2a5f433f14cf704082d1724b3fc9f3784dd96a7d79896a0fe8fd3, 회복44805eca7f8fe9fe94268f847d4f547c2cb134d782e03a1c311b9f5c80b4753e). 집중Core18건·사제 월드13건은 통과했다. 원복 뒤 전체 비증분 Release 빌드는 경고0·오류0, 관련Core568/568·Client45/45통과(실패0·건너뜀0·각종료0)다. 전체 회귀 완주로 합산하지 않는다. PORT_MAP801행(L1 385+port416), 공유fixture276개다. 이번 범위에 새10차 NB항목은 없다. 로컬 산출물은 `extracted/dotnet-priest-regen-0526/`에 둔다(Git제외).

## 인계와 일시 중지

사용자 요청(2026-11-14)에 따라 이 단위의 구현·검증·문서·로컬 커밋을 마친 뒤 목표를 일시 중지한다. 다음 단위는 시작하지 않는다. 재개 시0.527에서 제품 HostSession의 실제 사제/공간/소리/피해·해방 연결과 장기 SP/게임플레이 전역 owner 결선을 이어간다. 일반 지역 정책·AI 시작·나머지 객체 후처리·M1전체0/34도 잔여다.

실제 게임/MP를 실행하지 않았다. 사제 포획/희생·특정 적 공격·시나리오 승리의 실제 재현은 사용자 확인으로 유지한다. 보호 원본·AGENTS.md·README개요·CPP·WB를 변경하지 않았고 외부 접속을 추가하지 않았다.
