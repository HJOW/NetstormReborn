# 종속 객체 조회·삭제와 건설 진행음 정지

2026-10-11, 마지막 디컴파일 수행 PC: **HJOW-Athlon**. AGENTS.md와 두 LEFT_JOBS를 읽고 현재 PC가 마지막 디컴파일 PC와 일치함을 확인했다. 같은 PC의 Ghidra 프로젝트에서 세 판본의 함수를 읽기 전용으로 내보냈다. 원본 게임·analyzeManager·클론 창은 실행하지 않았다.

[건설 프로세스 복원](cpp-construction-process-reconstruction.md)에서 경계로 두었던 contained 삭제를 `RawContainedLookup`으로 복원했다. `MakeContainedConstructionHooks`가 건설 완료의 `removeContained(parent, soundProcessType)`를 실제 체인 조회와 가상 삭제 요청에 연결한다. 콘솔 통합 검사에서는 기존 `SquidProcessHost`·`RawSquidDestroy`·`SoundProcessSystem`을 사용해 진행음 정지, Kernel 제거, form Unpop과 SID 반납까지 처리한다.

## 원본 주소와 자료

| 역할 | originals 10.78 | originalCD / original1037 |
|---|---|---|
| 종속 타입/순번 조회·삭제 | `004b1a90` | `004eb310` |
| 서버 플래그 | `00540bc0` | `00540a28` |
| client 번호 범위 | `5..14999` | `5..5999` |
| 부모 head / 종속 next / 타입 / 상태 | `+6 WORD / +4 WORD / +10 BYTE / +11 BYTE` | 동일 |

`tools/ghidra/contained-functions.json`의 내보내기는 **1/1/1개**다. 기존 process 로더의 읽기 전용 PE 적재·SID 초기화·ABI 검사를 함께 사용한다. 타입 비교와 순번 감소, 번호별 권한, 삭제 이후의 다음 링크 읽기는 새 함수의 전체 실제 명령으로 실행한다.

## 조회와 삭제의 차이

부모의 head부터 next를 따라가며 타입 BYTE가 요청 DWORD 전체와 같고 dead 비트 2가 꺼진 후보만 순번을 소비한다. 순번은 0부터 시작한다. free·void·contained 비트는 후보를 제외하지 않는다. 따라서 타입 `303`은 하위 BYTE가 `47`이더라도 타입 47과 일치하지 않는다.

조회 모드에서 선택한 후보는 그 SID를 반환한다. 삭제 모드는 서버이거나 client 번호일 때만 그 후보의 vtable `+0x10`을 flags **0**으로 호출한다. 번호 권한 때문에 삭제하지 못해도 순번을 감소시키고 진행한다. 선택한 뒤의 순번 0은 DWORD `0xffffffff`로 감기므로 짧은 유한 체인에서 다른 후보를 연달아 삭제하지 않는다. 삭제 모드의 반환값은 체인을 끝까지 진행한 뒤 **0**이다.

다음 링크는 가상 삭제 **이후** 현재 슬롯에서 읽는다. 기존 `RawContainedFinder`는 반환 전에 다음 번호를 저장하므로 이 함수와 그대로 교체할 수 없다. 실제 form 삭제는 부모 체인에서 form을 빼고 슬롯을 반납하여 그 슬롯의 next를 0으로 만든다. 이미 읽어 둔 옛 링크를 사용하거나 부모 head부터 다시 시작하는 동작을 추가하지 않았다.

C++는 방문한 범위 밖 번호와 순환 링크를 예외로 거부한다. 삭제 경계가 next를 손상시키면 삭제 이후 그 링크를 읽는 시점에 거부한다. 이미 발생한 효과를 되돌리지 않는다. 삭제 함수 누락이나 다른 풀의 건설 어댑터는 효과 전에 거부한다. reserved/free 후보의 조회는 원본처럼 가능하지만 실제 삭제 경계가 처리할 수 없는 객체까지 지원한다고 보지는 않는다.

## 실제 건설 연결

두 판본에서 서버 번호의 abstract 부모에 다른 Regular 프로세스와 건설 프로세스를 붙인다. 8초 건설은 실제 `SoundProcessSystem`을 통해 진행음 form(타입 47)을 붙인다. 그 소리 프로세스를 한 번 실행해 반복 재생 항목을 얻은 뒤 건설 완료 프레임을 실행한다.

완료의 실제 호출 순서는 진행음 정지 → 갱신 → 완료음 요청이다. 진행음과 건설의 Kernel 슬롯은 사라지고 form은 반납되며 부모 HP가 100이 되고 abstract가 해제된다. 별도로 붙인 Regular 프로세스는 남아 부모 head를 차지한다. 그 프로세스의 마지막 Kill도 정상적으로 체인을 비운다.

이 검사의 소리 장치·완료음·최대 HP·부모 표시/공간 Unpop·재등록은 명시 응답 경계다. 진행음 프로세스의 삭제/정지 통지와 Kernel/form 수명은 실제 모듈이다. 게임 화면이나 원본 소리 장치를 실행해 들은 결과는 아니다.

## 독립 근거와 검증

`tools/decomp_contained_oracle.py`는 각 PE **805개**, 합계 **2,415개**를 두 x87 정밀도에서 실행했다. 정상 반환은 **4,830회**이고 가상 삭제 경계는 각 PE 두 정밀도 합계 **58회**다. 앞 누적 491,775개에 더해 **494,190개**가 됐다. 합성 유한 체인의 타입/상태·순번 경계·예약/client/서버/예측 번호·삭제 중 next/head/다음 상태 변화를 대조한다. 가상 삭제 내부만 응답 경계다. 실제 진입/정상 반환·ABI/보존 레지스터/x87/SEH·실행/쓰기 허용 목록·SHA/입력/폭/삭제 집계를 감사하며 원본 assert/게임/OS는 0이다.

새 C++ 검사는 **6개**다. Release 경고/오류 **0**, CTest 내부 **662개·실패 0**(146.26초, 전체 146.30초), 저장 근거 감사 **102종 모두 통과**. 별도 사본의 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과): 타입 절단·dead 제외·순번 감김·삭제 반환·첫 client 번호/서버 경계·flags·건설 완료의 조회만 수행. 변이 사본의 소스는 원래 바이트로 복구했다. 로그는 `extracted/contained-*.log`에 있다.

재검사: `python -X utf8 tools/decomp_contained_oracle.py --verify`, `cpppj/build/bin/Release/netstorm_tests.exe --filter ContainedLookup_`, `ctest --test-dir cpppj/build -C Release --output-on-failure`. 실제 함수 내보내기는 `powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name contained`다. AGENTS.md·보호 원본·dotnetpj는 변경하지 않았으며 커밋/푸시도 하지 않았다.

## 다음 순서

건설 표시 효과 `00423840`·`00423900`(CD `004eab60`·`004eac30`), 타입 준비 소리(+0x90)의 실제 타입 자료 연결, 가상 표시·부모 Unpop/재등록, 하위 경로·접근점·자원 정책/정렬·공간 Pop·소유 사제 이동/contained가 남아 있다. 새 범용 조회는 통지의 사제 contained 삭제에도 사용할 수 있지만 그 경계와 실제 파생 contained 삭제의 연결은 이번 단계에 포함하지 않았다. 집합/지연 패킷 분배·사용 중인 form 재부착·삭제/부모 상태 전파·건설 메뉴/커서 GUI도 후속이다. **게임 내 건설이나 미션 완주는 아직 불가능하다.**
