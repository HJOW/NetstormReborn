# dotnetpj 0.510 — 공통 프레임 진행·지정기의 현재 자산 계약

2026-11-14, HJOW-Athlon. 기준은 `73e3b8ba3`의 `o/RawFrameAdvance.h/.cpp`, `SquidFrame.h/.cpp`, `RawPathAnimation::Side/Direction`과 `client/RawSurfaceStorage::Frame`의 현재 TYPE 연결이다. 낙하 이벤트가 공유할 진행·지정기를 먼저 옮겼다.

## 구현과 범위

`RawFrameAdvance`는 현재 타입의 코드표에서 signed 방향 글자를 읽고 같은 글자의 첫 번호·전체 개수를 사용한다. 현재 번호+signed 증분을 DWORD로 계산하고 signed 비교에 따라 길이를 한 번만 빼거나 더한다. 여러 바퀴를 modulo로 정규화하지 않는다. 비연속 글자에서도 원본 표와 같은 결과가 나오며, 증분0도 실제 지정기를 호출한다. 현재 코드표를 빌리므로 배열 원소 교체를 다음 호출에 반영한다.

`SquidFrame`의 기존 TYPE·함수 객체 참조를 원문처럼 생성 시 깊은 사본·대리자 값 복사로 바꿨다. 장기 실행기는 같은 물리 길이의 TYPE를 `BindCurrentTypes`로 명시 대여하며 null·빈 표로 생성 시 사본으로 복귀한다. 기존 서피스 월드의 장기 지정기도 같은 표를 명시 대여해 현재 조회를 유지한다. 원문 구성은 TYPE 개수를 제한하지 않으므로 오래된188개 사전 관문을 제거하고 실제 현재 번호 조회의 물리 범위 검사로 유지했다.

PATCH의 프레임은+36 signed DWORD·레벨은+33 BYTE이고 CD는+34 BYTE·+31 BYTE다. CD는 목표 DWORD와 현재 BYTE를 비교한 뒤 하위 BYTE만 쓰며 이웃 extra를 섞지 않는다. 기존 Set의 현재 프레임 기준 레벨 계산→old/new 갱신 또는 Unpop→쓰기→Pop(flags|0x50) 순서는 유지한다.

## 원본 관측과 실제 공간

`frameadvance-x86.tsv`의 세 PE 각7680행, 총23040행을 실제 방향·구간 계산·진행·지정기로 재생한다. 입력은 연속/비연속 코드표, 네 타입, 다섯 저장 단계, 여덟 현재 프레임, INT_MIN/INT_MAX·±257·0 등을 포함한 signed 증분, 두flags다. 정수 반환·효과 호출 당시 프레임/인자·전체 슬롯을 저장된 원본 관측과 전수 대조한다.

기존 `setframe-x86.tsv` 대조도 PATCH만 읽던 검사에서 세 판본 각1920행, 총5760개 실제 Set 호출로 확장했다. 기존 Thunk12행의 고정 관측 확인은 유지하며 새 함수 실행 검증으로 세지 않는다. 두 fixture에서 실제 함수에 대조한 관측은28800개다. CD 관측 재생은 제품 대상 확대를 뜻하지 않는다.

별도 합성은 실제 PATCH 생성자·공장·Pop/Unpop·해시에서 작은 프레임→큰 프레임의 다음 진행에 단계1→3, 감싼 다음 진행에3→1 이동을 확인한다. 표시 갱신은 호출 관측이고 SHP 크기는 현행 C# Pop의 명시 입력 경계로 공급한다. 최신 일반 Pop의 현재 SHP/진단/부분 실패 순서와 실제 표시 어댑터 전체를 검증한 것으로 세지 않는다. 현행 공장이 PATCH 전용이므로 CD 공장·공간 합성을 제품 범위에 추가하지 않았다.

현재 코드 교체·signed BYTE 방향·잘못된 풀/타입/프레임/방향/flags, TYPE 깊은 사본/현재 대여/원소 교체/빈 표 복귀, 외부 대리자 객체 교체 뒤 내부 값 사본도 별도로 확인한다.

## 추적과 잔여

PORT_MAP은3행 추가로717행(L1 346·port371)이며 기존 SquidFrame CPP행의 기준을 갱신했다. 프레임 진행 fixture 한 개 추가로248개이고 기존247개 해시는 불변이다. 새 SHA256은 `74f24374bf31a47be3a48024052102ae5f8edfcedd3ccc940e1ecfdacaac75f1`이며 원본 파일을 Link로 공유한다. RawPathAnimation은 이번에 옮긴 두 정적 helper 외 다른 몸체의 기준을 올리지 않았다.

새 게임 실행·포획/희생/특정 공격/승리 재현·MP·신규NB·외부 접속은 없다. 보호 파일·cpppj·LEFT_JOBS.md·병행 webpj를 보존했다. M1은0/34다. 산출물은 `extracted/dotnet-frame-advance-0510/`에 둔다.

다음은 최신 `RawSquidPopParent`·`RawSquidActivate`와 현재 TYPE/SHP·표시 연결을 일반 Pop에 반영하고 `SquidFrameBinding`을 실제 경로에 연결하는 것이다. 이어 사제 낙하·포스필드 수명·소리 소비자→실제 요새 Pop·AI 생성/초기 갱신 순서다.

## 최종 검증

집중20건 통과 뒤 한 번 빼기 보정을 modulo로 바꾸는 변이를 시험했다. PATCH2040·CD1952·별도10.37 1952관측, 총5944개 불일치를 세 재생 검사 모두에서 검출했다. 원본 소스를 복원한 사실을 바이트 비교로 확인한 뒤 전체 비증분 Release 빌드를 다시 실행해 경고0·오류0으로 마쳤다.

그 빌드의 관련 Core404건과 Client 전체1172건이 실패0·건너뜀0·각종료0으로 끝났다. 결과는 `core-related.trx`·`client.trx`이며 변이 결과는 `mutation.trx`다. 직전0.509 전체 회귀3532건은 그대로 별도 기준선으로 유지하며 이번 관련 검사 합계를 새 전체 회귀 수로 세지 않는다.
