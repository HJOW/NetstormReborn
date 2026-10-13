# 건설용 객체 생성·이동과 공급원 소비 복원

2026-10-13 교정, **마지막 디컴파일 수행 PC VM-W11-CODEX**: 기존 Regular 경계의 인자 의미를 반대로 해석한 오류를 발견해 수정했다. 실제 `00496e80`/CD `0048efd0`은 `(event,parent,payload)`를 각각 +0x18/+8/+0x1c에 저장한다. Builder의 `(this,0,carrier,00532750)`는 **event0/payload0.05f**이며 `ConstructionBuilderState.timerPayload`의 현재 float를 전달한다. 기존의 timerEvent/출력 T의 `(payload,parent,event)` 해석은 잘못이었다. 이전 fixture는 이 생성자 몸체를 대체해 세 raw 인자 순서만 관찰했으므로 이 의미 오류를 검출하지 못했다.

이번에는 실제 Builder 전체→실제 Regular 생성자→event/payload/parent/현재 double 시각/count 필드를 두 PE **96개씩·새192개** 입력으로 확인했다. ProcessForm/Kernel 부착만 native의 명시 경계이며 C++의 실제 Host/Kernel 필드를 같은 고유 fixture와 대조한다. 기존 세 PE **1,110개** 입력은 재실행·재라벨했고 새 관찰 수에 더하지 않는다. 실제 Host event0/.05 재예약 검사도 추가했다. native 근거 `constructiontimer.json`, fixture `constructiontimer-x86.tsv`, 도구 `decomp_constructiontimer_oracle.py`, 기록 `extracted/constructiontimer-*.log`다. 아래 수치/PC/검증은 최초 복원 당시 이력이다.

첫 focused 실행에서 실제 factory/Host 검사의 명시 Pop 경계가 void를 지우지 않아 Regular 실행이 생략되었고, 새 ctor 검사는 native factory 경계의 임의 바이트 패턴을 종속 머리에도 공급해 미할당 SID 연결에서 거부되었다. `extracted/construction-unit-builder-focused.log`에 실패를 보존했다. C++ 통합 검사의 명시 Pop 경계에는 정상 void 해제 결과, 합성 새 객체에는 빈 종속 머리를 공급하도록 검사 장면을 교정했다. 원본 fixture/빌더/Host 몸체는 바꾸지 않았다. 추가한 실행 검사는 부모 event 몸체를 payload 반환으로 둔 **Regular의 재예약 검사**이며 manaBolt의 실제 event0 몸체까지 복원·대조했다는 뜻은 아니다. 현재 Builder 검사는 9개다.

교정 뒤 공유 Release 증분 빌드 경고/오류 0에서 Builder **9개·실패 0**을 재실행했다(`extracted/construction-unit-builder-focused-final.log`). 기존 1,110개와 새 실제 Regular 생성자 192개의 필드 대조를 포함한다. SP 회전4개·건설 도착3개도 같은 최신 실행 파일에서 실패0이며 고유 `construction-unit-sp-focused-final.log`/`construction-unit-arrival-focused-final.log`에 보존했다. 네 native 근거 감사는 각61/306/1,110/192 입력에서 모두 통과했다. 전체 CTest 결과는 상위 공유 작업 기록을 따른다.

2026-10-11, **HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS를 읽고 [기준점/소유 사제 조회](cpp-construction-selection-reconstruction.md)의 다음 생성·이동 항목을 진행했다. 시작 트리는 깨끗했고 현재/마지막 디컴파일 PC가 같았다. `constructionbuilder` **23/20/20개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창 실행은 없었다.

## 주소와 복원 범위

| 흐름/자료 | 10.78 | CD/10.37 |
|---|---|---|
| 생성·이동 전체 | `00443000` | `004d1120` |
| 공유 공급원 소비 전체 | `00473570` | `0048aaf0` |
| 워크샾/그래프 기준점 | `0048fdb0` | `00406f80` |
| 생성 / 경로 시작 | `004af530` / `00489080` | `004ab390` / `0047d150` |
| 공급원 선택 / 정렬 | `004734d0` / `00474970` | `0048aa40` / `0048ab40` |
| 기준점 발자국 접근점 | `0041db70` | `00440400` |
| 공급원 중심 / 단어 쓰기 | `004adea0` / `004ac220` | `004aef30` / `004abb00` |
| 건설용 / 운반용 타입 전역 | `00541270` / `005411fc` | `0051cb5c` / `0051cae8` |
| 접근 mask / 운반 Regular payload | `005424b8` / `00532750` | `005395cc` / `0052da30` |
| 공급원 배열 / 개수 | `00592f4c` / `00592f54` | `00567658` / `00567660` |
| 경로 현재 단계 / 시각 배열 | +`0x10` / +`0x9c50` | +`0x10` / +`0x12d0` |

[`RawConstructionBuilder`](../../cpppj/src/o/RawConstructionBuilder.h)는 첫 함수 전체의 흐름과 공급원 소비를 복원한다. 경로·자원 정책·가상 공간 동작은 명시 훅으로 연결한다. `ConstructionBuilderType`은 이 흐름이 읽는 타입 +`0xe0` 속도, +`0x98` 프레임 표 인덱스, +`0x130`의 첫 프레임 16개를 별도로 공급한다. +`0x98`의 더 넓은 게임 의미는 아직 단정하지 않는다.

건설용/운반용 타입은 호출자가 **실제 타입 전역 값**을 넣어야 한다. 원본 전역 주소의 차이를 나누어 로드 순서상의 타입 번호라고 추정하지 않는다. fixture의 타입 134/105/106은 합성 입력이며 실제 건설용 객체의 타입 이름을 확정한 근거가 아니다.

## 원본 순서

1. 요청 player DWORD·현재 목표 타입 BYTE/좌표로 워크샾 기준점을 조회한다. 0이면 **5.0**을 반환한다.
2. 현재 건설용 타입을 플래그 **2**로 생성하고 가상 소유자에 요청 DWORD 전체를 전달한다. 현재 목표 발자국의 **위쪽 가운데**를 `floor(좌표+0.5)`로 반올림해 목적지를 보관한다.
3. 기준점 genus와 현재 접근 mask가 겹치면 `mask|1`, 아니면 1이다. 기준점의 현재 발자국 사각형으로 접근점을 구하고 최초 객체를 **Pop 0x10**으로 놓는다. 경로 시작 실패면 5.0이며 앞선 생성/소유자/Pop을 되돌리지 않는다.
4. 요청 시각이 0/-0이면 속도 조정을 건너뛴다. 나머지는 현재 경로 단계의 도착 시각·최초 객체 현재 타입 속도·현재 게임 시각을 읽어 속도를 계산하고 경로 가상 함수에 `(속도,0)`을 전달한다. 패치는 `((요청-현재)*속도)/(도착-현재)`, CD는 `((요청-현재)/(도착-현재))*속도` 순서다.
5. 현재 목표 좌표/타입과 **기준점 현재 소유자 BYTE**로 자원 정책을 실행한다. 선택 경계 뒤 목표 좌표를 다시 읽어 정렬한다. 공유 목록은 매번 처음부터 스캔해 첫 0이 아닌 DWORD를 돌려주며 해당 칸만 0으로 바꾼다. **개수를 줄이지 않고 구멍/중복을 보존**한다. signed 개수 0 이하는 읽지 않는다.
6. 공급원마다 현재 운반용 타입을 플래그 2로 생성한다. 순번의 낮은 WORD를 raw +12에 쓰고 현재 다리 타입이면 표면 알림을 부른다. 기준점의 현재 소유자로 지정하고, 공급원 현재 타입의 +`0x98` 인덱스로 새 객체 **현재 타입**의 첫 프레임 표를 읽는다. 프레임은 raw에 직접 쓰며 패치는 DWORD(+36), CD는 **BYTE(+34)**다. `SquidFrame::Set`의 표시 효과를 호출하지 않는다.
7. 공급원 중심은 `x-폭/2, y-높이/2`다. 이 위치에 Pop 0으로 놓고 new(`0x28`) 성공 뒤 **event0과 할당 경계 이후의 현재 float payload**로 Regular를 만든다. 정상 payload는0.05f다. **할당 실패해도 목적지 이동은 계속**하며 운반 경로 반환값은 버린다. 모든 공급원 처리가 끝난 뒤 최초 경로의 **현재 단계 도착 시각을 다시 읽어 반환**한다.

원본 경로 인자는 패치 8개, CD 7개이며 SID/목적지 뒤는 모두 0이다. C++ 이동 훅은 이 고정 0들을 생략해 SID/목적지만 받는다. 최초 경로 객체는 전체 Move 호출이 끝날 때까지 살아 있어야 한다. 접근 훅의 사각형 참조를 보관하려면 복사해야 한다.

## 연결과 호스트 계약

`MakeConstructionBuilderHooks`는 같은 풀의 실제 `RawPlayerPlacementAnchor`, `SquidFactory`, 일반 `SquidOwner`를 연결하고 선택적으로 실제 `SquidProcessHost::AddRegular`까지 연결한다. 파생 소유자 재정의는 호출자가 해당 가상 타입에 맞게 분배해야 한다. `MakeConstructionBuilderLocalHooks`와 `MakeConstructionBuilderNoticeHooks`는 기존 로컬/서버 통지의 `moveBuilder` 경계에 전체 Move를 연결하며 원본처럼 double 결과는 버린다. 어댑터가 캡처한 객체들은 훅보다 오래 살아야 한다.

C++은 실제 읽을 DWORD SID가 풀 밖인 경우 WORD로 자르기 전에 거부한다. 양수 공급원 개수의 저장소 부족·잘못된 현재 타입/발자국/프레임 인덱스/경로 단계·잘못된 factory 반환·미연결 필수 효과도 예외로 거부한다. 원본의 assert/손상 메모리 실행을 복제하는 검증은 아니다. 공급원 0은 빈 항목이며 기준점 0은 실패다.

[`ConstructionBuilderTests.cpp`](../../cpppj/tests/ConstructionBuilderTests.cpp)의 새 검사 **8개**는 세 PE 관찰 재생, 조기 실패/기존 효과 유지, 타이머 실패 후 이동, 손상 자료 거부, 로컬/통지 어댑터 호출, 다른 풀 거부를 확인한다. 실제 워크샾 조회→factory→일반 소유자 지정→공급원 소비→Regular의 Form 부착/Kernel 등록도 두 판본에서 실행한다. 이 통합 검사의 접근점·공간 Pop·자원 정책·경로 응답은 명시 경계다. 어댑터 검사는 두 이동 훅 자체를 실행하며 전체 GUI에서 건설한 관찰이 아니다.

## 독립 관찰과 검증

[`decomp_constructionbuilder_oracle.py`](../../tools/decomp_constructionbuilder_oracle.py)는 세 실제 PE의 생성·이동 전체, 발자국/위 가운데/중심·반올림/CRT·genus/타입 getter·단어 쓰기·공급원 소비를 제한 x86으로 정상 반환까지 실행한다. 기준점/factory·접근점·가상 소유자/Pop·경로/속도 적용·자원 선택/정렬·할당/Regular·표면 알림의 **몸체는 실행하지 않고 실제 인자만 관찰**한다.

유효/지도 밖 좌표, 여러 발자국, 요청 player DWORD 폭, genus/mask, 0/-0·앞/뒤 시각·속도, 빈/단일/구멍/중복 공급원, 프레임 폭, 할당 실패를 교차했다. 경계 중 목표 좌표/기준점 소유자/운반 타입/공급원 순서·좌표/이벤트/최초 경로 단계가 바뀌는 입력도 포함한다. 각 PE **370개**(Move 342, 소비 직접 호출 28), 새 저장 **1,110개**, 두 x87 정밀도의 최상위 정상 반환 **2,220회**, 누적 저장 **480,462개**(479,352 + 1,110)다. 루트 내부의 소비/helper 호출 수는 최상위 반환 수와 구분한다.

[fixture](../../cpppj/tests/fixtures/constructionbuilder-x86.tsv)와 [근거 JSON](../../cpppj/constructionbuilder.json)의 SHA·입력/출력 폭·진입/반환/경계 수를 감사한다. 두 x87 제어값 639/895에서 관찰이 같았고 SEH 머리·스택·보존 레지스터·x87 균형·허용 실행/쓰기를 확인했다. assert·게임·OS 호출은 0이다. 합성 유한 시각/좌표 범위의 대조이며 모든 IEEE 특수값/극단 시각의 x87 정밀도 동등성을 증명하지 않는다. CD의 Pentium FDIV 우회 전역은 0으로 공급했다.

Release 경고/오류 **0**·CTest 내부 **622개·실패 0**(139.41초, 전체 139.53초), 근거 감사 **97종 모두 통과**. 새 변이 10개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

## 다음 순서와 재실행

2026-10-11 후속: [건설 끝 시각 재설정](cpp-construction-time-reconstruction.md)과 실제 form→Kernel 연결을 완료했다. 현재 전체 641개·근거 감사 99종·새/기존 변이 22개 검출이다. 다음은 전송용 상태와 표시/contained/공간 경계의 실제 연결이다. 아래 순서는 이 절 작성 당시 기록이다.

2026-10-11 후속: [건설 프로세스 시작·진행·완료](cpp-construction-process-reconstruction.md)에서 건설 시작/생성자·진행·완료·form 삭제 통지를 완료했다. 아래는 이 절 작성 시점의 다음 순서다. 현재는 건설 프로세스의 남은 몸체(끝 시각 재설정·전송용 상태)와 경계의 실제 연결, 하위 경로·접근점·자원 정책·공간 효과를 이어서 진행한다.

다음은 **건설 시작 `00443fe0`/CD `004d06a0`와 생성자 `00443c40`/CD `004d0720`, 건설 진행/완료**다. 이번 전체 생성·이동의 하위 경로/접근점·자원 선택/정렬·가상 Pop, 소유 사제의 실제 이동/contained, 타입별 공간/파생 삭제/추락/알림·Form 부착/최종 삭제/표시, 메시지 전파/수신·법적 위치/미리보기·건설 GUI도 남는다. **게임에서 건물을 지을 수는 아직 없다.**

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionbuilder
python -X utf8 tools/decomp_constructionbuilder_oracle.py --verify
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionBuilder_
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_mutation_check.py --focused --only builder-cd-frame-width
```

로그는 Git 제외 `extracted/constructionbuilder-*.log`다. 변이는 저장소 밖 사본에서만 실행한다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시는 없다.
