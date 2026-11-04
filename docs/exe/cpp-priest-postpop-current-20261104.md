# 사제 postPop의 현재 접두와 실제 가상 진입

단위862 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 원본·정밀 산출물을 변경하지 않았다. 직전 CP 커밋은 `0c06fa66 / 20261104-46 CP`이고 그 뒤 공통 문서 정리 `4fa7e89c / 20261104-47 CM`과 다른 세션의 webpj 커밋 48·49가 있다. 현재 제품은 862/11.01이다. 사용자의 재개 지시(LEFT_JOBS 3절 4번)로 일시 정지를 해제했다.

## 원문 순서

PATCH 사제 VT `0050f210`의 postPop `004950f0`은 사전 검사 없이 `[esi+0x28]&9`를 바로 읽는다(기계어 `00495110..15`). 최초 flags1이면 `00414450(&005954c4,sid)`로 등록하고 `004afb40(this,0x25a)`로 기존 회복을 찾는다. 없으면 `operator new(0x28)`이 성공한 뒤에야 `[0x5412d0]`의 현재 사제 TYPE DWORD를 읽어 `0049a840`→`0049fce0`으로 TYPE HP를 얻는다(`00495188`). `q=HP/6`을 signed 0방향으로 자르고 `fild q; fmul [0050f20c]=50.0; fstp dword`로 분자를 float로 저장한 다음 `004adcb0` 객체 최대 HP를 읽어 `fdivr; fstp dword`로 payload를 만든다. 원래 소유자 BYTE+0x0c의 하위7비트와 현재 소유자 BYTE+0x22가 같고 1..8이면 `00595428[owner]=0`이다. 그 뒤 Carrier `00427720`, 현재 extra 재조회, `00427030`, `test bh,8`(0x800)·지면·방향 조건부 `004941f0`, `00493d30`, 이동 가능 시 `00491980` 순서다.

`00414450`은 처음 읽은 signed count가 signed 용량보다 작을 때만 매 반복 현재 count와 비교하며 중복을 찾고, 저장 위치는 처음 count를 쓴 뒤 현재 count를 한 번 증가시킨다. `0049fce0`의 1/4 분기는 TYPE 번호를 현재 `005412d0`과 비교한다. CD `0040c110`도 `0051cbbc`의 현재 DWORD로 분자 HP를 읽고 `00444720`도 같은 DWORD와 비교한다. 원문에는 `005412d0` writer가 없으며(TYPE 표 쓰기만 있음) 실행 중 값 변화는 검사 입력으로만 확인했다.

## 연결한 범위

- **PATCH 접두:** `RawPriestPostPop::Prefix`의 일괄 `Validate`·`AllocatedBytes`를 제거하고 물리 슬롯을 읽는다. 목록은 현재 `Count/Capacity/Storage/SetCount` 접근자로 `00414450`의 signed 관문을 따른다. 음수 저장 위치와 짧은 배열은 실제 역참조 위치의 호스트 물리 경계로 거부하며 앞선 효과는 되돌리지 않는다. 검색·확보·생성 장치는 도달 시점에만 요구하고 선택한 callable 사본으로 호출한다.
- **현재 사제 TYPE:** 회복 분자는 고정 TYPE158 대신 생성 때 빌린 `005412d0`/CD `0051cbbc` DWORD를 호출 시점에 읽는다. 실제 Session은 `RawPriestWorldState::movement.priest`를 넘긴다. `SquidRewardState::sharedPriestType`을 추가해 `TypeHitPoints`의 1/4 비교도 같은 DWORD를 읽게 하고 Session에서 연결했다. 독립 fixture는 기존 `types.priest` 값을 쓴다.
- **PATCH 후반:** `RawPriestPostPopTail`의 낙하/보호막 확보/보호막 조회 훅을 도달 시점에만 요구한다. 풀·프레임 표·spot 크기 검사는 두 판본 모두 효과 전에 유지한다.
- **실제 가상 진입:** PATCH `SquidPostPop::HandlesPriest`는 실제 VT `0050f210`만으로 사제 wrapper에 분배하고 현재 TYPE를 추가 관문으로 쓰지 않는다. 실제 PATCH Pop/Activate 부모는 이미 사전 검사 없이 `PostPopVirtual`을 부르므로 이제 접두 안에서도 사전 거부가 남지 않는다.
- **유지한 계약:** 명시 `Validate`/`SquidPostPop::Validate`의 TYPE158·VT·목록 count 무효과 사전 검사와 CD의 구성/접두 사전 계약(Validate·AllocatedBytes·훅 요구)은 그대로다. `SquidReward`의 TYPE 표 사본과 debug assert throw, `RawPriestState::Raw`의 free/contained/TYPE 범위 guard는 이번 범위 밖이며 정상 Pop 뒤에는 도달하지 않는 호스트 경계로 남긴다.

## 검증

Release 첫 빌드(제품+두 집중 대상, `SquidReward.h` 변경으로 1176.5초)는 종료0·경고/오류0이다. 새 검사는 집중6개+Session1개다. 첫 실행에서 Session1개는 통과했고 집중6개 중5개(10단언)는 실패했다. 원인은 검사 장면에 주석을 덧붙이면서 `raw[10]=158`·소유자·원래 소유자 쓰기가 줄 주석 안에 들어간 **검사 입력 오류**였다. 같은 실행의 PowerShell 5.1 오류 처리로 나머지 필터가 실행되지 않은 것도 기록한다. 임시 진단 사례로 슬롯을 확인한 뒤 진단 사례를 제거하고 검사 파일 한 줄만 교정했으며 제품 수정은 없다. 이후 집중 대상만 다시 빌드(18.7초, 종료0)하고 14묶음을 실행했다.

14묶음 중 13개는 첫 실행에 통과했다. `PriestPop_RequiresExplicitWholeHandlerBeforeMutation`은 2단언이 실패했다. 첫 단언은 사제 후처리를 등록하기 전의 PATCH Pop 사전 거부를 기대한다. 이 지점에서는 `priestPostPop_`이 비어 있어 862 변경이 닿지 않는다. 842 이후 PATCH Pop이 현재 원문 부모(공간 쓰기 뒤 가상 후처리)로 바뀌면서 생긴 기존 guard의 노후 실패이며, 이 파일은 549 전체 실행 이후 집중 실행된 적이 없다. 두 번째 단언은 862 원문 계약상 목록 초과가 등록 생략 후 계속 진행하므로 더 이상 거부가 아니다. 무변경 사전 거부를 CD Pop과 PATCH 명시 Validate 위치에서 검사하도록 교정했고, PATCH 실제 Pop이 초과 목록을 그대로 둔 채 공간 등록을 마치는지 확인을 더했다. 다시 빌드(19.9초, 종료0)한 뒤 5개 모두 통과했다.

최종 결과는 고유90개/14묶음, 실패0이다. 새7개에 더해 기존 원문 재생(접두 7,176행, 후반 6,144행, Pop 3,840행)과 기존 소비자를 실행했다. 기존 소비자는 소유자 칸2·Pop/Activate 재생·경로 도착/정리·장기 목록9·조회8·preDestroy5·회복10, 그리고 같은 공통 부모의 PostPopCurrent17·CarrierParentCurrent12다. fixture 입력/기대값은 바꾸지 않았다. 수동 guard 3개(접두·후반·Pop)만 판본별 계약 위치로 옮겼다.

- **새 집중 검사:** 할당/free·TYPE 무관 물리 접두와 명시 Validate 유지, signed 초과/음수/중복/현재 count 재조회·처음 count 저장, 현재 사제 DWORD의 분자 변화와 같은 DWORD의 1/4 비교(양판본), 도달 장치만 요구(PATCH)와 CD 구성 거부, VT만으로의 PATCH 가상 분배와 CD 미분배, 후반의 도달 효과·부분 효과 보존을 확인한다.
- **실제 Session:** `thewarbegins`의 실제 Factory 사제에 대해 `PostPopVirtual`로 장기16칸 목록 등록, 실제 ProcessHost의 0x25a Regular payload/부모, 공유 소유자 칸 0 쓰기, 이동 가능 지면의 보호막 조회가 빈 결과로 삭제 없이 끝나는 것과 공통 깊이 복귀를 확인했다. 변경한 장기 목록/소유자 칸은 정리 전에 복구했다. 틱/행동/포획/희생/공격/승리는 실행하지 않았다.

실제 게임/GUI, 포획/희생/특정 적 공격/승리, 공중3종 전용, 신규 MP/통신, 물리/PCM/CD, 전체71/CTest/34미션/ZIP 반복은 제외했다. checking12=부모6+부분3+MP3, 완료부모36/부분3은 변하지 않았다(이 단위는 공통 Pop 현재 파생의 부속 연결이다).

기존 보호2761개/fixture941개는 불변이다. 현재 EXE SHA-256은 `9cd5b26346f6cc7b073304713b9dd35762e7c99b37103928e7de6cf5698ba726`(10,580,992바이트)다. 첫 빌드 입력·교정 스냅샷·원문/기계어 발췌·로그는 Git 제외 `extracted/cpp-resume-862/`에 있고, SHA와 실행 결과는 [검증 JSON](../../cpppj/priest-postpop-current-20261104.json)에 기록했다.

## 남은 일

Teleport의 다른 유효 생성 함수 분배, 다른 현재 가상20/접두와 공유 TYPE/모드 전역(예: `SquidReward`·`RawPriestState`의 TYPE 표 사본, altar/dais 등 다른 TYPE 번호 사본), 보호막 Regular의 기존 별도 진단/수명, 전체 Pop 완료 판정은 LEFT_JOBS 3절 4번에 남는다.
