# SID 할당 진단과 현재 World의 소진 회수 — 단위976

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 우선순위4의 외부 할당 caller 대조 중 발견한 PATCH10.78 SID 할당 경로를 복원한다. 원본/정밀 자료는 읽기만 했다. 사용자의 요청에 따라 이 단위의 문서 반영과 로컬 커밋을 마치면 전체 목표를 일시 중지한다.

## 실제 할당 부모

PATCH004af1d0은 할당 금지 진단, predictable의 현재 free/next 검사, 일반 client/server 머리 선택을 거쳐 실제 슬롯을 초기화한다. 기존 C++는 일반 머리와 꼬리가 같으면 즉시 예외를 냈다. 실제 원문은 현재 boss가 참이면 iterator0040eb40으로 두 번째 물리 슬롯부터 free 비트가 꺼진 후보를 순회하며, 현재 DWORD Bridge 종류와 TYPE BYTE가 같은 객체의 현재 VT+10을 flags0으로 최대50회 호출한다. 콜백 뒤 현재 종류와 물리를 다시 읽고 현재 free 머리를 선택한다. 종류 비교는 전체 DWORD이며 BYTE로 줄이지 않는다.

단언의 정상 복귀를 지원한다. 회수 뒤에도 머리와 꼬리가 같으면 `p != *freeTail` 진단 뒤 선택한 꼬리를 계속 소비하며, next0의 헤더 진단도 임의의 성공이나 원상 복구로 바꾸지 않는다. predictable의 두 플래그 동시 입력은 bit1을 우선한다. next0은 실제 pool 기반 포인터이므로 NULL 포인터 검사 `checkMe`와 같은 것으로 취급하지 않는다. 호스트 벡터의 물리 범위 밖 접근은 별도로 거부한다.

SidPool은 현재 할당 금지 DWORD, boss bool, Bridge DWORD와 진단/회수 장치를 빌린다. RawSurfaceStorage가 긴 수명의 실제 권한/연결 번호/진단을 제공하고, RawProcessHostStorage가 현재 World의 짧은 가상 삭제 대여를 호출 종료까지 유지한다. 회수는 DestroyDependent의 현재 클래스 분배를 사용하므로 TYPE 별칭이 있어도 원래 클래스의 삭제를 호출한다. 종료된 World의 주소를 보관하거나 임의의 Bridge 기본 소멸자로 대체하지 않는다. 실제 Client의 공유 Storage 경로가 연결 대상이다.

## 독립 원본 입력과 경계

새 도구는 canonical functions.tsv를 읽는 SidOracle의 선택적 경로를 사용한다. 기존 기본 경로의 동작은 유지한다. PATCH Reset004abb10/Allocate004af1d0/iterator0040eb40/Release004abf60/SetNext004aba70과 삭제 기록의 CRT 이동을 실제 x86 코드로 실행했다. 단언은 기록 후 정상 복귀하거나 명시 입력 값을 바꾸고, 가상 삭제 경계는 실제 Native Release로 진입한 뒤 명시적인 현재 종류/새 후보/예외 효과만 적용한다. 실제 게임이나 통신은 실행하지 않았다.

47입력은 정상 할당, 금지값 진단, 비free 머리, boss0의 계속 진행, 다리 후보/50회 상한, 콜백 뒤 현재 종류 변경과 새 후보, 회수 중 예외의 부분 상태, predictable의 head/candidate/next/끝 경계를 포함한다. 각 행의 반환/중단, freeCount/cursor, 두 머리/꼬리, 전체 풀과 두 삭제 기록의 Adler32, 현재 전역과 진단/삭제 순서를 동결했다. fixture SHA256은 `1527716c4194bc879f0bab5cb6352a378951f7beb049b730cf503485426fd5d8`이다. 이 경계의 가상 삭제 효과를 전체 원본 게임의 파생 소멸자 실행으로 쓰지 않는다.

처음의 원본 실행기는 명령마다144개 코드 영역을 선형 탐색해 비효율적이었다. 해당 분석 프로세스만 종료하고 같은 허용 영역을 바이트맵으로 바꿔 정상 완료했다. 원본 코드나 입력을 바꾼 시간 제한 회피는 하지 않았다. `native-run-linear.log`와 완료 실행 기록은 단위 원시에 남긴다.

## 검사와 제품

Release3명령 모두 종료0·경고0·오류0이다. 전체 재컴파일이 실제 종료된 뒤 검사를 실행했다. 최종 **고유25검사(새4/기존21), 실패0**이다. 새47행과 기존 SID3168행의 Native3215행을 대조했고 기존 HostStorage11/현재 Process생성4검사도 통과했다. 실제 World는 현재 Content 호스트의 삭제, 전체 DWORD 종류 비교, 종료된 대여 거부/다음 prepared 대여, 회수 중 이전 Clear 증빙의 반납 거부와 정상 복귀 뒤 소비를 확인했다.

첫 새3검사에서는 Native47와 선택된 미연결 경계가 통과하고 실제 World1검사가 실패했다. 검사 준비가 실제 Session의 Content 호스트 후설치를 빠뜨린 원인이었으며 같은 분배기에 실제 ContentFormHost/ContentFormLifetime을 연결해 교정했다. 제품에 임의의 빈 Form 처리기를 추가하지 않았다. 최초 실패와 최종 로그는 원시에 보존했다. 이 단위에는 수정 전 제품의 비교 실행 결과가 없으므로 새로운 수정 전 실패 수를 만들지 않는다.

제품976/표시·파일·제품11.01은 **11,379,712바이트**, SHA256 `01b9366f6efb12a0cd4b89073ad31adb3ccef49b842eb8c888f1350c4e5d01a7`이다. 기존 보호2798개/fixture978개 불변·새fixture1개로 다음 기준은 **2799개/fixture979개**, `extracted/cpp-resume-976/protected-final.json`이다. 분석 도구 두 파일의 Python 문법 검사도 통과했다. [단위 보고서](../../cpppj/sid-allocation-recovery-20261106.json)를 따른다. 원시는 `extracted/cpp-resume-976/`이다.

## 남은 범위와 재개

CD004aae40의 실패 경로는 객체 dump0041f3e0/상세004875b0, 별도 진단 줄과 boss 회수 루프, BridgesKilled004c3dc0→004c3a20을 사용한다. PATCH 경로로 덮어쓰지 않고 기존 CD 성공 경로를 유지한다. 이 CD 실패 경로와 실제 malloc 실패의 소유권/외부 heap 반환, 미등록 생성자/가상 callee, 다른 파일/비멀티 수명은 잔여다. 유효한 호스트 pool에서 재현할 수 없는 NULL pool 포인터와 원본의 범위 밖 메모리 접근을 가짜 관측으로 만들지 않는다. legacy 외부 raw Kernel World의 독립 소유 경로까지 복원했다고 주장하지 않는다.

Factory 공통 부모 대조975와 이번 PATCH SID 할당을 전체 Factory/우선순위4 완료로 확대하지 않는다. checking잔여8/솔루션83은 불변이다. 새 GUI·진단UI/MP·패킷·통신/공중3종 전용/실제 포획·희생·특정 적 공격·승리/전체CTest·34미션·ZIP 반복은 하지 않는다. 마지막 GUI는968이다. 다음 단위977은 사용자 재개 후 우선순위4의 나머지를 계속하며 순서는4→7→8이다.
