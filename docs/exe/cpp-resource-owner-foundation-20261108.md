# C++ 공통 자원 소유권 기반·현재 Runtime·기본 전송 부분 접합

2026-11-08 / 단위1175 / HJOW-Athlon / 제품11.01. 사용자 `/goal`의 M6-l/n 선행 의존을 구현했다. M6 전체·장기 수신 전범위·지연 큐 활성화는 미완료다. 새 멀티플레이·단위·패킷·통신 검사는 실행하지 않았다.

## 구현 범위

- 같은 RawSurfaceStorage가 Factory/Display/Graph/공간·Owner/공통 Pop·Unpop·Frame/장부·삭제 상태/최근5개 간헐천과 방향9DWORD를 장기로 소유한다. 현재 간격은 실제 battleOptions[6]을 읽는다. 기존 Template의 같은 loadingDepth를 첫 World 전부터 연결하며 대여/반납 때 새 물리나 게임 상태를 만들지 않는다.
- 현재 Session의 파생 binding은 약하게 대여한다. 실제 선택 호출에서 기존 PinCurrentRuntime을 통해 전체 World/Session을 보유한다. binding 자체만 보유해 raw WorldImpl이 먼저 소멸할 수 있던 틈을 닫았다. 최종 파괴 진입에서 getter를 먼저 반납해 Drain의 약한 참조 재획득을 막는다.
- Client의 AI 통지·기억 선택 재확인은 원문 자연 반환 조건을 통과한 실제 하위에서만 같은 전체 Runtime을 보유한다. Pool/SID 조회 자체를 Session 선행 관문으로 바꾸지 않는다. ClearForState는 callee의 지역 호출 pin이 해제된 뒤 caller의 Retire/ReturnForState가 실행되어 자기 호출이 정상 정리를 막지 않는다.
- 공통 RawSquidTransmission과 같은2048BYTE mutable 예약·현재 TYPE/HP/진단을 장기화했다. 방송은 Player→현재 연결NULL→현재 local 제외→현재 실제 Source 보유→같은 버퍼의 직접 가상 송신이다. 지정 송신은 기존48f4b0 관문 뒤 같은 현재 Registry/Source를 선택하며 backend에서 observer를 한 번만 호출한다.
- 공통 감시·내용물·삭제 Form, sharedRegular 전체 writer와 Dais/Construction의 기본 writer를 연결했다. defaultWriter는 실제 Host 첫 생성 시 결합하여 BindDevices 앞에 독립 Kernel을 조기 생성하지 않는다. 기존 typed 관측/const 분기는 유지한다.

## 원문·사본과 실제 적용

클라이언트 양판본의 마지막 정밀 디컴파일은2026-11-03/HJOW-Athlon, R은2026-11-07/같은 호스트다. 생성 원문·원본/CD·기대자료는 수정하지 않았다. foundation은58함수, 전송층은16함수의 C/PE 근거를 보존했다.

| 층 | 동결 준비 경로 | 변경 규모 |
| --- | --- | --- |
| n1~n7 기반 | `extracted/cpp-m6-resource-owner-prep/` |29파일/434추가136삭제/58CPE |
| 실제 전체 Runtime pin | `extracted/cpp-m6-resource-binding-pin-prep/` |14파일/50추가21삭제 |
| n8a 기본 전송 | `extracted/cpp-m6-resource-transmission-prep/` |10파일/88추가22삭제/16CPE |

겹친 파일을 포함한 실제 고유 제품 변경은37파일이다. 각 최소 hunk 적용 전후 사본과 hash는 `extracted/cpp-resume-1175/{before,after}-*/`·`applied-*.json`에 있다. 검토한 세 원본 patch의 바이트는 보존했다.

## 빌드와 허용 비멀티 확인

NetstormCpp/NetstormRoot의 Release/x64를 빌드했다. 첫 API 시도는 주석 뒤 Factory 선언이 한 줄 주석에 들어가 오류18/종료1이었다. 종료 뒤 선언을 분리했다. 다음 시도는 C++20 std::span에 없는 at() 한 곳에서 오류5/종료1이었다. 기존 대상1..8 관문 뒤 [] 접근으로 한 줄만 교정했다. 두 실패 원시와 실패 당시 소스 hash를 보존했으며 최종 시도는 종료0·경고/오류0이다.

| 실제 미션 | 확인 범위 | 결과 |
| --- | --- | --- |
| tutorial1 | 정상 UI 다리 건설·실제 서버 객체/요청 증가·90frame 대기 | 통과/정상 종료0 |
| tutorial4 | 정상 UI 윈드 제네레이터 건설·실제 서버 객체/요청 증가·600frame 대기 | 통과/정상 종료0 |
| thewarbegins | 캠페인1-1 정상 진입·90frame 대기·게임 시각 증가 | 통과/정상 종료0 |

각 실행은 격리1920×1080/영어/무음 설치·사용자 자료를 사용하고 실제 MCP 보고/PNG를 관측해 정상 입력만 보냈다. 포획·희생·특정 적 공격·시나리오 승리·공중3종 전용 검증은 하지 않았다. 설치 사본/실행물/보호2800파일이 불변임을 확인했다. 이 세 사례를 전체34미션 회귀나 전체 동일성으로 확대하지 않는다.

전체 시도 원시는 `extracted/cpp-resume-1175/live-*/`의 summary/mcp.jsonl/TSV/PNG에 있으며 실행 실패·재시도도 같은 보고서의 liveAttempts에 모두 남는다. 첫 Tutorial1은 안내 뒤 이미 선택된 다리를 다시 생산 클릭하여 해제한 뒤 건설 확인0/정상종료0으로 끝났다. 동일 EXE의 재실행에서 실제 선택을 관측해 배치했고 제품 교정은 하지 않았다. 빌드 로그는 build.log/build-final.log/build-corrected.log로 구별한다.

## 현행 실행물

| 실행물 | 바이트 | SHA-256 |
| --- | ---: | --- |
| Netstorm.exe | 13,426,176 | `7ee1089e73cdf0df4a1df1db5ebef3601eace211adbdc622a86b094ca406b5a8` |
| R.exe | 677,376 | `c75609282a517daabef4cda51227f35818f711002fd00688160de4102c1c364e` |

## 남은 범위

canonical11개는 태그 수다. 기본 전역 등록·장기 observer·동일 F 대여/반납·Editor 기본표·실제 선택 하위가 모두 닫힌 개수와 구별한다. 기존2/1d/66/5의 기본 scope 보완과 무태그77F, 모든 실제 Source/직접 송신/Fail/큐 Node 수명이 남는다. 이 단위는 지연 큐를 활성화하지 않았다.

n8b의 나머지 ProcessForm writer와 실제 mutable 송신 caller, 기본 Host 실행/Regular·자원 파생·실제 NoWorld frame caller, JoinTestBattle·debugBurst·전수 마감이 남는다. 특히 기존 Native Session ProcessForm.Attach의 지역/const 전송은 이번 n8a 완료 범위가 아니다. M7 다섯 항목과 M8을 마친 뒤 한국어 K1~K5·시나리오 UI S1~S4를 계속한다.

현행 근거는 [보고서](../../cpppj/resource-owner-foundation-20261108.json)·[M6 감사](cpp-lan-remaining-audit-20261108.md)·[LEFT_JOBS](../../LEFT_JOBS.md)다. 전체 목표는 진행 중이다.
