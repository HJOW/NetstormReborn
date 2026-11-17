# dotnetpj 0.570 — Native 장기 Content 장치·정렬(Form) 결합

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 Content 장치·정렬(Form) 장기 결합을 이식했다. 실제 별칭 주소·네 효과·약한 현재 대여·정렬 본체·통지 분배다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

실제 장기 Content 장치는 Native 전송 결합 뒤에만 설치한다. 빈 저장소·이미 설치된 Content·종속 실행기·빌린 물리·정렬 본체가 있으면 설치 전에 거부하며 함수 값 사본 뒤 효과를 실행하지 않는다. 네 효과(커서 취소·로컬/원격 수입·위치 소리)는 필수이며 지갑/SP는 같은 Reward가 처리하므로 여기 두지 않는다.

상태는 같은 장기 boss/너겟/커서 주소를 별칭으로 빌린다. 로컬 소유자는 장치 상자, 권한은 같은 Authority boss, 너겟은 같은 GameAux 너겟 TYPE(최초 154, 0.570에서 GameAux에 추가), 커서는 장치 cursor 상자를 현재 getter로 읽는다. 수입 명령 초기값은 원문 0x020000b4를 유지한다.

정렬 본체는 PATCH에서만 같은 풀/TYPE로 한 번 구성하고 World 반납 뒤에도 유지한다. 전투/checking/TYPE/진단은 같은 장기 상태를 빌리고, 포획 안내는 현재 대여의 실제 pin과 Captured에 위임하며, Pop/Unpop은 같은 장기 공장에 연결하고 Reattach는 네 내장 Form VT만 선택한다. 통지는 빈 부모를 조용히 넘기고 파생만 현재 대여의 Notice에 전달하며 pin 소유자를 호출 동안 보유한다.

ContentLifetime/Reward 연결과 ProcessFormDispatch의 Content 결합·파생 전용 삭제 몸체·HostSession 별도 풀 전환·피해/해방·삭제/배치 상위 결선은 잔여다.

## 검증

최종 **Core 신규6/6** 통과다. 전송 없는 설치 거부·중복 거부·빌린 물리 거부·필수 누락 거부·독립 TYPE fallback·장치 값 반영·상태 별칭 공유(로컬/너겟/커서/권한)·수입 명령 초기값·정렬 본체 재사용·CD 거부·빈 통지 통과·파생 미연결 실패·대여 전달·중복 대여 거부·정렬 Pop/Unpop 공유를 확인한다. 원본 fixture 기대값은 바꾸지 않았다.

Assets366/366 통과·Release Core 빌드 0경고·0오류다. Core 전체는 이전 기준선(0.568: 2717/2718, 1건 NETSTORM_DATA 필요)을 유지하며 이번 단위에서 전체 회귀를 다시 돌리지 않았다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치0/34·데모dn3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP974행(L1 445·port 524), 대응cpp418단위·fixture319개다. 과정 장치 2행을 추가하고 큰 파일 전체 기준/floor/reflected는 올리지 않았다. 미대응819단위 중 R4는133단위/13,564줄로 유지, 다음은 Reward·ContentLifetime 결합과 파생 전용 삭제 몸체·HostSession 별도 풀 전환이다.
