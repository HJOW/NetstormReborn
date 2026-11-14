# dotnetpj 0.519 — 일반/사제 배치 접두와 전체 픽셀 모양

2026-11-14, HJOW-Athlon. 기준 cpppj 소스 커밋은 `7c3efe9bd`다. 기존 디컴파일 산출물은 읽거나 변경하지 않았다.

## 변경과 범위

`o/RawCanonPlacement.h/.cpp`, `RawPriestPlacement.h/.cpp`, `RawCanonPixelShape.h/.cpp`, `RawPriestPlacementShape.h/.cpp`를 같은 이름의 C# 모듈로 옮겼다. 이미 존재하는 `CanonPlacementQuery`와 `PriestPlacementQuery`, `PriestPlainCanonType`, `SquidDisplayShape`를 재사용했다. 일반 쿼리 주석의 잘못된 타입/인자 설명도 바로잡았다. `RawPriestPlacementGeometry.h`에서는 픽셀 getter가 요구하는 고정8 타입 전역과 decoder 진단 상태만 추가했다. 기하 본체는 아직 잔여이므로 해당 행은 `port`다.

배치 진입은 타입/유한 좌표 검사→차단 관계0→현재 공유 전체DWORD 또는 독립 강제 허용→현재TYPE 즉시 허용→부호 확장한 owner BYTE와 로컬 번호 비교 저장→모양/여백→지도 경계→후반 정책 순서다. 모양 콜백이 로컬·강제 허용·TYPE를 바꿔도 저장한 비교와 원래 값 인자는 유지한다. PATCH 가로 차의 double 정밀도, 세로 float 저장, CD 두 합의 float 저장과 정수 절삭을 구별했다.

픽셀 getter는 실제 `CanonTypeDecoder`의 모든 유효 칸을 순회한다. 현재 SHP 헤더·타입 기준점·배율을 읽고 low DWORD 뺄셈, PATCH/CD의 중간 저장과 최댓값 동률 선택, 최솟값 기준점 기록을 보존했다. PATCH의 논리 프레임/해제 표식과 CD의 물리 접근 검사를 구별한다. 빈 decoder는 모양을 조회하지 않고 `(1000,1000,0,0)`과 초기 배율을 반환한다. 사제 wrapper는 현재 genus와 비패턴 계약을 추가로 확인한다.

함수 객체는 대리자를 복사하고 실제 장기 자료를 빌린다. 어댑터는 같은 SID 풀만 허용하며 나머지 함수는 보존한다. 후반 미리보기·충돌·지형·관계는 명시한 경계다. 접두 통과를 전체 배치 완료로 세지 않는다. 새 부품은 아직 HostSession의 전체 사제 월드에 연결하지 않았다.

## 독립 원본 대조와 합성

네 fixture를 cpppj 원문에 연결했다. 기대값을 C#으로 생성하지 않는다. SHA-256은 CRLF를 LF로 정규화한 값이다.

| fixture | 세 판본 입력 | SHA-256 |
| --- | ---: | --- |
| canonplacement-x86.tsv | 3360 | c35d8d23a65a4bac2a49c3081784a323fbd713dcf047731663a6c1c544cb00e2 |
| priestplacement-x86.tsv | 3024 | a12d5342510a4579335044eee9b973e8de8c081a591fe793b2489301f6db67ab |
| canonshape-x86.tsv | 5950 | 271aa7ded45fe38dbeee9c533bd7d3ca8e1bc27d7fe0172dff3e95e5a34c8a5d |
| priestshape-x86.tsv | 8064 | f81259fa6c49401cb286dbd923d24e6f56f97db78d4a2cdf040798cfb4c182d2 |

합계20398입력의 반환·현재 전역·TYPE·콜백 인자/순서·여섯 float 비트가 일치했다. 새 `PlacementShapeOracleTests`는21검사다. 두 판본 실제 패턴 getter/여백, 현재 기본 프레임·SHP·타입 기준점·배율 변경, 빈 decoder, 전체DWORD0x100, 요청/로컬 스냅샷, 함수 사본, 다른풀 거부, 비유한/잘못된 자료·프레임/해제 표식을 포함한다.

10.78 합성은 실제 픽셀 getter→배치→132후보 나선→서버 사제 공장 생성자를 연결해 첫 후보 거부/둘째 허용, SID15000·WORD0x8101·좌표(21,21), Pop/Carrier 각1회를 확인했다. 후반 정책과 Pop/Carrier 효과는 이번 합성에서 기록 경계다. 이들 실제 모듈 수명 검증은 앞 단위의 근거를 유지한다. 최초 합성 검사에서 CD의 실제 공장이171타입을 거부한1실패가 있었다. 기존 C# 공장은10.78 전용이라 CD 모양/배치 검사는 유지하고 실제 공장 합성만 PATCH로 분리했다. CD 공장 이식 완료로 주장하지 않는다.

소유자 비교를 모양 조회 뒤로 늦추는 변이를 넣자21검사 중8개가 실패했다. 원문 파일을 SHA-256 `63dedc3b3f8aa59ce9004f53057459f6713caf86a409169a1394f1d9ad36e2a1`까지 바이트 원복했다. 근거는 `extracted/dotnet-placement-shape-0519/`의 TRX·변이 스크립트/결과에 있다(로컬 산출물, Git 제외).

## 검증 마감과 잔여

원복 뒤 전체 비증분 Release 빌드가0경고/0오류로 끝났다. 최종 관련Core386/386·Client21/21 통과(실패0·건너뜀0·각종료0). 전체 Core/Client 회귀 완주로 합산하지 않는다. PORT_MAP은778행(L1 364+port414), 공유 fixture는265개다. 10차 NB 수정은 이번 네 모듈에 해당하는 새 항목이 없다.

실제 게임 실행이 없는 부품 단위다. 사제 포획/희생·특정 적 공격·시나리오 승리·MP 실제 재현을 하지 않았다. originals/originalCD·AGENTS.md·README 개요·cpppj·webpj는 변경하지 않았다. 외부 접속 코드를 추가하지 않았다. M1 전체일치0/34와 새 경로 플레이 불가 상태는 그대로다. 다음은 일반/사제 기하·미리보기·충돌/지역의 현재 자료 연결과 전체 사제 월드 조립이다.
