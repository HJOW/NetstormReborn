# dotnetpj 0.518 — 사제 생성과 Carrier 지면 확인

2026-11-14, HJOW-Athlon. cpppj 기준 `dfdbcd655`의 `o/RawPriestSpawn.h/.cpp`, `RawCarrierCheck.h/.cpp`를 모듈/어댑터 묶음으로 옮겼다. [0.517 현재 소유자](dotnet-owner-current-20261114.md)에 이어 실제 사제 월드 조립의 선행 부품을 연결한다. 원본·디컴파일·cpppj 소스는 변경하지 않았다.

## 생성과 현재 지면

`RawPriestSpawn`은 입력 좌표에 원문 float0.99999를 double로 더해 signed32 절삭한 뒤64비트 셀로 보관한다. stage2~23에서 위/오른쪽/아래/왼쪽으로132후보를 순회한다. 지도 가장자리 밖 후보는 검사하지 않고, 빈 spot은 배치 콜백을 부르지 않는다. 점유 후보는 전체 배치 질의 뒤 **콜백 전 저장한 spot의0x10**을 검사한다. 콜백이 현재 지도를 바꾸더라도 이 판정은 앞에서 읽은 값으로 수행한다.

첫 허용 후보에서 실제 생성(flags0)→전체WORD 쓰기→현재 다리TYPE면 표면 알림→원래 소유자 지정→Pop(flags0)→현재 실제TYPE의 Carrier genus 확인 순서다. 생성/알림/Pop이 바꾼 TYPE나 전역을 각 소비 위치에서 읽는다. 마지막 확인의 비Carrier는 PATCH에서 원문 실패, CD에서는 가상 호출 없이 반환한다.132후보를 모두 실패한 뒤에만 PATCH checking 전체DWORD를 읽으며 CD는 이 진단을 읽지 않는다. 함수 객체는 생성 때 확인/복사한다.

`RawCarrierCheck`는 현재TYPE 범위→제외 genus0x320000→현재y/x를 float0.9999와 double로 합산/절삭→현재 지면 비트6→실제 Walker 낙하 순서다. 제외 genus이면 비유한 좌표도 읽지 않는다. 원문의 절삭 후 음수0은0셀이다. 같은 풀의 생성/후처리 어댑터는 필요한 함수 하나만 교체하고 다른 함수 객체를 보존한다. 이미 있는 `PriestPlacementQuery`를 재사용하며 중복 구조체를 만들지 않았다.

## 원본과 실제 합성 대조

| 자료 | 이번 대조 | SHA256 (LF) |
| --- | --- | --- |
| `priestspawn-x86.tsv` | PATCH408+CD414+10.37 414=1236 | `d303adced95eee52b4b5b8be520116e8c51abb4ced84413e12b9344282bcdd86` |
| `carriercheck-x86.tsv` | 전체2028행 중 PATCH676 | `949a692a3cbd4f8dfd9a3eae79c63fd005b9a7c40fd6eae4de385014ff9caee3` |

새1912입력에서 나선의 전체 질의·정확한 float 비트·생성/알림/소유자/Pop/Carrier 순서·전체 슬롯·현재 지도 Adler·권한/반환이 처음부터 일치했다. Carrier 자료는 실제 `RawPriestFall`을 호출하며 보호막/공유 생성/공간/소리의 외부 효과만 원본 관측 입력으로 공급했다. 기대값을 C#에서 만들거나 원본 자료를 교체하지 않았다. CD 슬롯36BYTE와 C# 나머지14BYTE의 입력 보존을 구분해 검사했다. Carrier의 CD 실제 HP/세션 전체 복원은 미완성이므로 나머지1352행을 이번 관측 수에 더하지 않았다.

실제 `RawDamageableRelease`→나선→서버 `RawSquidFactory` 사제 생성자→`RawPriestOwner`/공통 Owner→현재 Carrier 확인을 합성했다. 서버SID15000, 풀 빈 수3감소, WORD0x8181/원래 종속WORD0x8101 유지, 소유자1, 알림/Pop/Carrier 각1회를 확인했다. 빈 지도에서는 배치 검사가 선택되지 않는다. Pop은 기록 경계이며 실제 공간 표시 완료를 뜻하지 않는다. 기존 [0.514 낙하](dotnet-priest-fall-20261114.md)·[0.515 소리 수명](dotnet-sound-lifecycle-20261114.md)의 실제 공유Form/Kernel·보호막 검증은 유지한다.

생성 후 입력 함수 객체를 바꿔도 복사한 함수가 실행되며 해방 어댑터는 원본 훅을 덮어쓰지 않는다. 다른 풀의 어댑터 연결은 거부한다. 생성 뒤 현재TYPE/지도 변경과 Carrier 제외 관문,132후보 뒤 현재 checking0x100의 비영 판정/PATCH 실패/CD 미조회도 확인했다. 새8검사가 모두 통과했다. 최초 빌드의 동일 이름 구조체 중복 오류는 기존 질의 타입 재사용으로 해소했고 이후 제품/Core.Tests 빌드0경고/0오류다.

배치 콜백 뒤 spot을 다시 읽도록 바꾼 변이는8검사 중 세 판본 나선3검사에서 검출했다. 나머지5건은 통과했다. `RawPriestSpawn.cs` SHA256 `8625fe19e5bf12564fe37358952e21857dae293fbc4b85b088d03251ec283be7`로 바이트 원복했다. 원복 뒤 전체 비증분 Release 빌드0경고/0오류·최종 관련Core279/279·Client21/21 통과(실패0·건너뜀0·각종료0)다.

PORT_MAP770행(L1 356+port414), 공유fixture261개다. 기존 질의 구조체의 원문 헤더 행도 재대조했다. 증거는 `extracted/dotnet-priest-create-0518/`에 있으며 신규 배포 실행파일은 없다. 실제 게임 창/MP 실행, 사제 포획/희생·특정 적 공격·시나리오 승리 재현은 수행하지 않았다. 전체 사제 월드/HostSession 조립·배치 검사와 M1 전체0/34는 남아 있다.
