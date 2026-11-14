# dotnetpj 0.520 — 일반/사제 배치 기하와 로컬 미리보기

2026-11-14, HJOW-Athlon. 기준 cpppj 커밋 `1b009866d`의 `RawCanonPlacementGeometry`, `RawPriestPlacementGeometry`, `RawCanonPlacementPreview`, `RawPriestPlacementPreview` 헤더/본체를 이식했다. 기존 디컴파일 산출물은 읽거나 변경하지 않았다.

## 구현 범위

기존 `CanonPlacementCell`, `PriestPlainCanonType`, 현재 기하 상태를 재사용했다. 일반 기하는 실제 `CanonTypeDecoder`의 최초 범위와 모든 칸을 순회한다. 생성 때 첫 프레임/패턴을 저장하며 지역 초기화 뒤 모양마다 현재 발자국/코드를 다시 읽는다. 각 칸은 준비→scan→지형 종료→Advance 순서이며, scan/종료 거부 뒤에는 후속 칸·최종 관계를 호출하지 않는다. 빈 decoder도 초기화/최종 관계를 호출한다. 일반 타입의0발자국과 기본 프레임 개수 검사 생략(코드가 없으면side0)을 유지한다.

사제 기하는 현재 genus/비패턴 계약과 양수 발자국을 확인한다. 원래 여섯 값 인자를 보존해 공통 칸 순회와 사제 지형 경계를 연결한다. 실제 미리보기 범위/충돌 모양 어댑터도 포함한다. 충돌 헤더에서는 두 함수 객체 형식만 먼저 옮겼다. 후보 충돌 본체는 아직 없으므로 해당 행은`port`다.

일반/사제 미리보기는 같은 `CanonPlacementPreviewState`를 사용한다(C++ alias를 별도 사본 형식으로 늘리지 않았다). 비로컬은 배열 초기화/범위 조회 없이 후반 정책에 전달한다. 로컬은144BYTE를 지운 뒤 실제 범위를 읽고, 오른쪽 아래부터 y/x를 역순으로 순회한다. 배열의 x가 바깥 차원이다. PATCH의y254/지도 끝 조기 거부는 이미 작성한 부분 배열을 보존한다. CD는linear spot을 먼저 읽으며 좌표 검사는 표면 조회 시점에 한다.

표면 조회는 현재 raw 타입/genus/소유자를 읽고 free/dead/void 추가 필터를 넣지 않는다. 편집기→동일 소유자→활성 관계 순서이며 관계 인덱스는unsigned 표면owner×9+signed 요청자BYTE다. 요청자-1도 실제 유효 인덱스8을 만들 수 있다. 현재 지도/타입/관계는 대여하고 구성 때 함수 객체는 복사한다. 어댑터는 같은 실제 풀만 허용하고 다른 함수는 보존한다.

## 검증

네 fixture는 cpppj 원문에 읽기 전용으로 연결했다. SHA-256은 LF 정규화 기준이다.

| fixture | 입력 | SHA-256 |
| --- | ---: | --- |
| canongeometry-x86.tsv | 5967 | a842473cbc5b493f75c3a6dfd262576f12af0acb9f75d498e03d07a90dcc3ae8 |
| priestgeometry-x86.tsv | 4352 | b49b1ce9cb4145afdd45806ba43404c49ab5523ca05e1a259ce83fa273c8e12a |
| canonpreview-x86.tsv | 4860 | f85f5e24f83f2fa9c1cd7ed00981dae471d43363b53543c398d0303f4de17aa9 |
| priestpreview-x86.tsv | 4860 | e16e2df64f4dbd06afc689e1c8d4da264f67628407518af0d0b22b143f865a40 |

합계20039입력을 세판본에 대조했다. 일반은 모든 패턴/별칭·회전·각 칸·현재 발자국 변화, 사제는 직접 decoder 생성/진행·첫/끝 범위/float 비트/충돌 사각형을 비교한다. 미리보기는 접두→미리보기 실제 모듈을 연결해144BYTE·raw 표면·관계 전역·TYPE·두 지도Adler32·콜백 순서와 반환을 대조한다. CD raw36BYTE와 C# 나머지14BYTE 입력 보존을 별도로 검사한다.

신규24검사는 원본 재생과 함께 실제 패턴→공간 해시/finder의 첫 거부/지형 생략, 현재 사제 발자국·초기 프레임 캡처·값 인자/함수 사본·조기 거부·빈 decoder, 실제 일반 기하→미리보기→접두, 실제 사제 픽셀 getter→접두→기하→미리보기→기존 사제 지형의 공유 상태를 포함한다. 사제 합성의 scan은 후보 결과를 공급하는 기록 경계다. 성공 때begin/prepare/scan/end/finish와지면 미완성·차단16칸을 확인했고 거부 때begin/prepare/scan만 확인했다. 실제 게임 플레이가 아니다.

초기 검사 프로젝트 빌드에서 새 preview 검사에 `SidAllocation` 네임스페이스using이 빠져1오류가 났으며 이를 보완했다. 제품 첫 빌드는0경고/0오류였고, 이후 새21검사와 실제 finder/계약 추가 후24검사가 모두 통과했다. 미리보기 x/y 배열 전치 변이는24검사 중 세판본 원본 재생6개가 검출했다. 제품 소스를 SHA-256 `8c182dcbdf5c39b6f372aca3efee144d1cbe6ae41daa24f51529e331b6cf783c`까지 바이트 원복한 뒤 전체 비증분 Release빌드0경고/0오류·최종 관련Core410/410·Client21/21통과(실패0·건너뜀0·각종료0)를 확인했다. 전체 회귀 완주로 합산하지 않는다. 근거는 로컬 `extracted/dotnet-placement-preview-0520/`의 TRX·변이 스크립트/결과다(Git 제외).

PORT_MAP은785행(L1 372+port413), 공유fixture269개다. 기하 두 헤더의 기존 부분 매핑을 본체로 바꾸고 실제 미리보기/기하 본체·충돌 함수 형식 행을 추가했다. 사제 기하 상태만 담은 기존 부분 행은 남긴다. 이번 이식에 해당하는 새10차 NB 항목은 없다.

## 보호와 잔여

실제 게임/MP를 실행하지 않았다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현은 생략했다. originals/originalCD·AGENTS.md·README 개요·cpppj·webpj는 변경하지 않았다. 외부 접속 코드를 추가하지 않았다. M1 전체일치0/34와 새 경로 플레이 불가 상태는 그대로다. 후보충돌 본체/일반 지역 정책, 전체 사제 월드·HostSession과 다른 파생별 후처리 연결을 계속해야 한다.
