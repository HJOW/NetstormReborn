# 골렘의 실제 대상 접근·귀환 명령 — 2026-10-17~18

기준은 `92b7526`, 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 일치한다. AGENTS와 양 LEFT를 재독했다. 골렘은 사용자 표시 이름이며 내부 이름은 `sunWalker`, TYPE133이다. 보호 원본 수정, 새 디컴파일, 원본 엔트리/게임 초기화/게임 창 실행, DirectDraw 수정은 없다. 별도 Win32 CPU 관측기는 기하 함수만 격리 실행했다.

`RawBoardTarget`은 일반 접근점41d530과 사각형 변 중간점41d400을 복원한다. 건설 사제의41db70 Graph 접근점과 구별한다. 중심 두 float의 저장, 기존41ce90 방향, flags2의 북쪽 특례, flags4의 사각형 확장 순서를 보존한다. 확장한 오른쪽만 x87에 남아 넓은 값을 유지하고 왼쪽/위/아래는 float 저장을 거친다. 지도 밖 좌표를 임의로 clamp하지 않는다. flags1이 없으면262 진단 뒤 현재545370/374 실패점을 읽고, 잘못된 변 방향은1f9 진단 뒤flags의float 비트를 반환한다.4af170의 초기화 후 실패점은-10000,-10000이다.

`RawCarrierTarget`은4894e0/4276a0/427600/428fd0이다.

- 일반 대상 이동4894e0은 발자국 효과 뒤 현재 부모 좌표를 읽고 가까운 변을 먼저 시도한다. 실패하면 북0/동2/남4/서6을 순서대로 시도한다. 재시도 중에는 최초 사각형과replaceStart/helper를 유지한다. 실제 시작의 반환 Path 주소를 호스트 포인터 폭으로 보존하며 새 게임 토큰으로 만들거나 직렬화하지 않는다.
- 직접 귀환4276a0은 현재40f100의 AI 존재 검사 뒤 최초 접근점 한 번만 시도한다. 간헐천 이동427600도 같으며 추가로 대상free 비트와현재TYPE.flags2의10000000만 검사한다. dead/owner/특정 번호 필터를 추가하지 않는다.
- 두 단일 명령은 발자국 효과 전에 부모 좌표를 보관한다. 일반 대상 이동과 조회 시점이 다르다. AI 존재는 owner0을 먼저 처리하고 현재Player+b0 포인터를 읽으며 복제 bool표로 대체하지 않는다.
- 귀환 wrapper428fd0은 현재boss와428ea0의 후보를 검사한 뒤4894e0(parent,target,0,0)이다. 선택 효과 뒤boss를 재검사하지 않는다. 후보 선택 본문은 여전히 명시 경계다.

같은 Session은 기존 RawSquidNeighbors.Footprint, 현재9Player, 기존 실체23/39/40/54 PathStart/BFS/Prepare/Host/부모 통지/직렬화에 연결했다.427600/4276a0/428fd0을 외부의 빈 효과로 처리하지 않는다.38/41/42 경로와 미등록187의 그래픽은 지원한 것으로 바꾸지 않는다. 실제 통합은 초기 미션의 골렘/사제를 사용해 두 번의 접근 시작과 기존40 경로 교체/동일 종속 체인/패킷을 확인한다. 사제를 간헐천으로 요청하면 현재 경로와 패킷 수가 유지된다. 경로 시작 검사이며 자연 채취·운반·경제·전체Kernel·완주로 확대하지 않는다.

## 에뮬레이터와 실제 x87의 차이를 따로 보존한다

보호 PE 독립 에뮬레이터 관측은 **2,245입력·4,490정상 반환·474,608명령·53/64차이0**다. 위 본문과41ce90/40f100/4ac200/49a840/4af170을 실행했다. 발자국/경로 시작/귀환 선택/진단과초기화 하위는 명시 경계다. GP/retN/SEH/canary/제어어·TOP·tag/명령·읽기·쓰기 범위를 검사했다.

첫 C++ 집중 검사에서 실제 Session과 AI 경계 검사는 성공했으나 잘못된 방향에 signaling NaN flags를 반환하는18행만 에뮬레이터 원문과 달랐다. 별도 x64 FLD/FSTP 관측6회에서 실제 CPU의quiet 변환/invalid 상태를 확인한 뒤, **원본41d400/41d530/41ce90을 수정 없이 실행하는 별도 Win32 관측기**로 기하 전체를 다시 대조했다. 결과는 **1,020입력·2,040정상 반환**이고, 이번 환경의Unicorn2.1.4와의 차이는 같은18입력/두정밀도36회뿐이다. 실제 x87는7f800001을7fc00001로 바꾼다. 구현의quiet 변환은 실제 CPU와 일치하므로 유지한다.

원본 파일은 읽기 전용이다. 관측기는 자신의 큰 전용 섹션에 원본 이미지와 상수를 복사하며 진단만 별도callback으로 연결한다. 원본 엔트리/import/게임 초기화는 실행하지 않는다. 반환 포인터, 입력 사각형, 출력 양쪽canary, 제어어/TOP0, 복사 이미지 전체의 허용 실패점2DWORD 외 불변을 검사한다. 이 하드웨어 관측에서는tag/모든 메모리 접근 트랩을 관측했다고 주장하지 않는다. 앞의 독립 에뮬레이터 통제 범위와 구별한다.

이전 에뮬레이터 원문은 변경하지 않았다. 실제 CPU 자료는 `carrier-target-hardware.tsv`와별도보고서로 보존한다. C++는 기하1,020행을 실제 CPU53비트 결과로 대조하고64비트 관측도 같은 결과임을 확인한다. 나머지 상위 명령은 원래 에뮬레이터 원문을 재생한다.4af170 행은 실패점 기본값만 대조하며 C++ 세계 초기화 전체를 가장하지 않는다. 하드웨어 원문은 재관측 뒤 바이트도 같았다.

재현 명령:

```powershell
cmake -S tools/carrier-geometry-hardware -B extracted/carrier-geometry-hardware-cmake -A Win32
cmake --build extracted/carrier-geometry-hardware-cmake --config Release
python tools/decomp_carrier_target_hardware.py --helper extracted/carrier-geometry-hardware-cmake/Release/carrier_geometry_hardware.exe
python tools/decomp_carrier_target.py --verify
python tools/decomp_carrier_target_hardware.py --verify
```

초기 보조 관측기의 인라인 어셈블러 문법/정렬 빌드 오류,400000 주소 예약 충돌,PowerShell UIntPtr 생성 오류는 ignored 로그에 남겼다. 최종 독립 CMake/Win32 빌드와2040회 관측은 성공했다. 제품 Release build1/2는 모두 성공했고 최초 집중의18개 차이를 실제 CPU 근거로 구별한 뒤 최종 집중3개가 모두 통과했다. 직전 전달3개/낙하 시작5개 회귀도 통과했다. 전체 CTest는 **1,606개·실패0·종료0·197.50초**(CTest197.54초)다. 감사는 **71종·60성공·기존11실패·새실패0**, 기존46보고서126현재핀만 갱신했으며 역치환하면 과거 JSON 전체가 일치한다. 원래 CMake 접두/기존 fixture/AGENTS/원본/DN 인계·보호SHA3를 보존하고 새 파일은 BOM 없는 UTF-8/LF다.

클론 진입5회가 모두 성공했고 originals_unchanged=true다. 범위는1-1/1-2briefing→world/tutorial1~3briefing이며 자연 운반·완주로 확대하지 않는다. 모든 빌드/검사/관측/감사 핸들은 종료했고 대기 작업은 없다. 인수인계 뒤 **`20261018-01 CP 골렘의 실제 대상 접근과 간헐천 및 귀환 이동 복원`**으로 로컬 커밋한다. push하지 않고 전체 goal은 active다.

아직428ea0/429420/4280a0의 실제 귀환·간헐천 후보 선택,38/41/42 이동,429010 회피, 실제 Client 명령 소비자, 생산·경제·AI·11스테이지 자연 완주·승패·저장·다음미션/전체 동일성은 미완료다. 전체 goal은 active다. 로그 접두는 `extracted/goal-carrier-target-20261017-`이다.

상위 RawCarrierArrival::Controlled는 아직state.controlledOwners표를 읽는다. 이번 작은40f100 helper의 현재Player+b0 조회와 별도이므로 후속에서 상위도 같은 현재 저장소로 연결해야 한다. 이는 이번 단위의 자연 운반/AI 동일성을 주장하지 않는 이유 중 하나다. 다음 실제 후보 선택4280a0은 기존word+8 거리 기록/원래 ring목록/qsort 동률/BFS·Prepare를 보존해야 한다.
