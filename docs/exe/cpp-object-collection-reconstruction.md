# cpppj 일반 자산·그림자 표시 수집 복원

2026-10-11, `HJOW-Athlon`. AGENTS.md와 두 인계 문서를 확인했고 현재 PC가 마지막 디컴파일 PC와 같았다. `objectcollection` 목록으로 패치 7개·CD/추가판 각 4개를 읽기 전용 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.**

`ObjectCollection`은 실제 `SquidHash` 1~3단계와 `SidPool` raw 슬롯, 타입/물리 프레임 표에서 본체와 그림자를 수집한다. `BindObjectCollection`은 공통 일반 자산/효과 수집 슬롯에 연결한다. SHP의 월드 hotspot을 기존 로더와 `SquidRenderer::Shapes` 캐시에 추가했다. 실제 sprite 그리기와 Client 자동 프레임은 후속이다.

## 원본 근거와 방문 순서

| 내용 | 10.78 | CD/추가판 |
|---|---|---|
| 일반 표시 수집 | `00433660` | `004887a0` |
| 셀 좌표/CRT 절삭 | `00432580`→`0043fa50`→`004e49c0` | 인라인→`0048ee10`→`004f161c` |
| 현재 해시 칸 주소 | `004b29a0` | `00479be0` |
| 타입 조회/프레임 검사 | `0049a840` / `00419850` | 수집기에 인라인 |
| raw stride / 깊이 / 프레임 | 50 / signed BYTE `+35` / signed DWORD `+36` | 36 / signed BYTE `+33` / BYTE `+34` |

해시 0단계의 지면/다리 묶음은 앞 단계 수집기가 담당한다. 일반 수집기는 1→2→3단계, 각 단계의 y 바깥·x 안쪽 순서로 칸의 머리 SID와 raw `+4` WORD next 체인을 방문한다. +0.5/CRT 64비트 절삭 하위 DWORD를 signed로 읽고 현재 scale 2/4/16으로 나눈다. 단계별 side-1로 먼저 제한한 뒤 왼쪽/위 -1·오른쪽/아래 +1을 DWORD로 확장하고 양 끝을 독립적으로 0~side-1에 클램프한다.

같은 SID가 다른 칸/단계에 나타나면 그대로 중복 수집한다. type은 0이 아니어야 하며 state/dead/extra에 따른 별도 제외 필터는 없다. 정상 반환은 이전 선택과 관계없이 해시를 **2단계**로 만든다. C++는 같은 next 경로 안의 순환·풀 밖 SID·누락 타입을 예외로 거부한다.

## SHP 경계·반올림·레이어

실제 물리 SHP 프레임은 VFX 앞 36바이트에서 float 칸 폭/높이, 앞 28/24바이트에서 float 월드 hotspot을 읽는다. 픽셀 hotspot signed short와 별도다. `SquidFrameMetrics`→`SquidDisplayFrame`으로 네 float의 비트를 보존하며 앞선 프레임/dirty 계산의 필드 순서는 유지한다.

월드 교차의 하단 기준은 `(view.left-width)-1`, `(view.top-height)-1`이고 상단은 view.right/bottom이다. 객체 위치에서 월드 hotspot을 뺀 값이 양쪽에 **포함**되면 수집한다. 반올림 차이는 기계어로 확인했다.

| 계산 | 10.78 | CD/추가판 |
|---|---|---|
| x-hotspotX | x87에 유지 | float 저장 |
| y-hotspotY | float 저장 | float 저장 |
| top-height-1 | x87에 유지 | float 저장 |
| NaN 비교 | 네 조건 모두 ordered | x 하단/상단·y 상단은 unordered 통과, y 하단은 거부 |

타입 `flags1`의 shadow `0x40000`은 현재 프레임+논리 frameCount를 그림자로 **별도 판정**한다. 본체가 화면 밖이어도 그림자가 들어오면 수집한다. 저장한 원래 XY로 두 경계를 검사하고 제출 때 현재 raw XY/깊이를 다시 읽는다. 본체는 walker(`flags2 & 0x10000`)일 때 y에 0.5를 더한다. 그림자는 walker와 관계없이 y에서 **float 0.1**을 빼고 float으로 저장한다.

항목은 raw XY·풀 토큰·signed BYTE→WORD 깊이·kind 0이다. 그림자 tail은 4, 본체 tail은 abstract(`flags1 & 0x400000`)이면서 signed 현재 프레임이 논리 count 이상이면 4, 나머지는 0이다. 패치 프레임 보조는 shadow/abstract에 따른 논리 count 2배·null shape·해제 HP 흔적을 검사한다. CD는 물리 포인터를 직접 읽으므로 C++도 CD에 패치 논리 제한을 추가하지 않는다. 물리 배열 밖/없는 shape는 두 판본 모두 안전하게 거부한다.

## 제한과 실제 연결

원본은 항목 기록 뒤 **누적 count < limit**를 검사한다. 본체/그림자가 각각 검사 대상이며 limit에 도달한 기록과 현재 해시 단계는 실패 후에도 남는다. 실제 C++ span 크기는 쓰기 전에 검사한다. 비어 있는 창은 limit 0/음수여도 정상 0개를 반환한다.

호스트 주소를 원본 포인터로 해석하지 않는다. `tokenBase + SID*stride`를 사용하고 생성 때 전체 32비트 범위, `Object`에서 기준·정렬·유효 SID를 검사한다. pool/hash/타입/shape는 수집기보다 오래 살아야 하며 반환 목록은 호출자가 넘긴 span에 저장한다. shared hash를 바꾸므로 같은 수집기의 동시 호출은 지원하지 않는다.

연결 검사는 실제 UI 트리·0단계 지면 SID·3단계 자산 SID와 그림자를 공통 목록에서 합친 뒤 실제 CRT 정렬과 kind 분배를 호출한다. 높은 깊이 UI→같은 깊이의 그림자→walker 본체→지면 내부 순서를 확인한다. 묶음 숨김은 일반 자산을 유지하며 UI 전용 dirty는 일반/지면 수집을 건너뛴다. 장치 sprite 경계와 GUI 자동 프레임은 아직 연결하지 않았다.

## 독립 원본 대조와 검증

`tools/decomp_objectcollection_oracle.py`는 세 PE의 수집기/프레임/타입/좌표/해시/CRT 전체 실제 몸체를 격리 실행한다. **응답 대체 0**이다. raw 슬롯 전체와 희소 해시, 실제 타입 배치·SHP 참조 규칙의 경계 입력만 만들고 기대 교차나 목록을 계산하지 않는다. fixture는 전체 64개 작업 칸·정상 종료 상태·진입 횟수·보호 바이트를 저장한다.

각 판본 **477개**, 새 독립 원본 **1,431개**·누적 **508,743개**, 두 x87 정상 반환 **2,862회**다. main/shadow/abstract/walker·signed 깊이·정확한 양 끝과 인접 float·단일 레이어 교차·판본별 저장 경계·NaN/무한대·순서/중복/next 체인·extra/state 무시·inverted/clamped 창·strict 유효 상한·고정 시드 불규칙 장면을 관찰한다. 0단계 입력은 호스트에서 생략 여부를 추가 검사하며 원본 일반 수집기가 읽는 격리 그리드는 1~3단계다.

전체 출력/미사용/가드, 타입/SHP/해시/슬롯 입력 보존, 비영 보존 레지스터·cdecl 스택·x87 제어/깊이·SEH, 불연속 허용 실행·허용 쓰기·SHA·진입/정상 반환 집계를 감사한다. OS/assert/실제 게임 호출은 0이다. 원본 assert를 넘는 손상 입력은 C++ 계약 검사에서 확인한다. 비유한/큰 입력은 저장한 관찰 범위이며 실제 화면/정렬의 정상 지원을 뜻하지 않는다.

새 C++ **7개**, Release 경고/오류 **0**·CTest 내부 **730개·실패 0**(199.98초, 전체 200.05초), 근거 감사 **111종**·새 변이 **10개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). 집중 검사 **7개·실패 0**. 세 PE 재생, strict 누적 제한/실제 용량, 손상 체인/타입/판본별 프레임/토큰, 실제 UI·지면·자산 공통 정렬/분배, SHP 월드 hotspot 분리를 확인한다.

창 없는 실제 자산 검사도 통과했다. 10.78 **116타입/3,783물리 프레임**, 추가판 **101타입/3,167물리 프레임**의 월드 hotspot/칸 크기/픽셀 메타데이터가 로더와 캐시에 비트 단위로 일치한다. 실제 표시/Pop/frame 변경 경로의 invalidation 4회·부분 Draw 1개도 유지한다. 이는 실제 sprite 픽셀/GUI 실행 검사가 아니다.

새 변이 10종(1단계 생략·깊이 unsigned·walker/그림자 보정 생략·abstract tail 경계 변경·그림자 본체 종속·패치 x float 저장·CD top 저장 생략·strict 완화·공통 수집 생략)을 모두 검출했다. 별도 `extracted/mutation-work` 사본에서 적용하고 원래 바이트로 복구했다. 미검출/건너뜀 **0개**다.

기록은 `extracted/objectcollection-{export,smoke,oracle,build,focused,ctest,audits,mutations,assets}.log`, 내보내기는 `extracted/objectcollection/{originals,originalCD,original1037}/`다. AGENTS.md와 보호 원본은 수정하지 않았다. 원본/복사본 게임·analyzeManager·클론 창 실행·커밋·푸시는 없다.

## 남은 범위

미리보기 `00446060` 수집, 실제 SID 월드 투영/각 종류 Draw·Gump 생성, 배경/구름·dirty 순회·장치·Client/GameWorld 자동 프레임과 클릭 선택을 이어간다. 건설 SHP·팔레트/현재 색 표·준비 소리·부모 공간/사제 contained·건설 GUI도 남았다. **게임 내 건설/미션 완주는 아직 불가능하다.**
