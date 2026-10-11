# cpppj Gump 입력 캡처·dialog·overlay 전달 복원

작업 PC: **HJOW-Athlon**, 기록일: **2026-10-12**. AGENTS.md와 두 LEFT_JOBS를 먼저 읽었고 착수 시 작업 트리는 깨끗했다. 현재 PC와 마지막 디컴파일 PC가 같았다. `gumpinput` **5/6/6개**를 Ghidra 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-12.**

`GumpInput`은 같은 생성·수명 상태와 실제 `GumpEvents`/`GumpCollection`을 사용한다. 입력 캡처, 단일 수집 override 설정/해제, dialog 등록, 캡처/dialog/루트 우선 전달과 overlay 입력을 복원했다. OS 입력 큐와 실제 버튼/메뉴 처리기, 전체 Client UI 프레임 연결은 후속이다.

## 원본 근거

| 동작 | 10.78 | CD·추가판 |
| --- | --- | --- |
| 첫 입력 캡처 | `00465250` | `00494120` |
| 단일 수집 override | `004654c0` | `00494510` |
| dialog 등록 | `004654f0` | `00494540` |
| 캡처/dialog/루트 전달 | `00466200` | `004928a0` |
| overlay 표 입력 | `00466150` | `004927f0` |
| 자신/부모 visible 확인 | overlay에 인라인 | `004942c0` |

위치·경계·CRT는 `gumpcollection`, 기본 필터·재귀 전달·전체 선택은 `gumpevents`, dialog 제거·실제 CRT 겹침 복사는 `gumplifetime`의 전체 몸체를 함께 실행했다. 가상 필터는 `+0x34`, 처리기는 `+0x28`이다. 한도 로그는 실제 원본 메시지와 코드 `0x13`을 확인한 명시 경계다.

## 복원한 동작

- **캡처:** 공유 `GumpLifetimeState.focus`가 비었을 때만 토큰을 설정한다. 이미 캡처한 객체가 있으면 교체하지 않는다. 토큰 0의 등록 자체는 원본처럼 허용한다.
- **단일 override:** enabled이고 현재 값이 0일 때만 설정한다. 이미 설정됐으면 enabled여도 해제하며, disable도 기존 객체/요청 토큰의 일치 여부와 무관하게 해제한다. 기존 수집기의 `collection.overrideHead`를 공유한다.
- **dialog:** 활성 구간에서만 중복을 검사하고 새 토큰을 끝에 넣은 뒤 현재 dialog를 갱신한다. 사용하지 않는 뒤 칸의 같은 값은 등록을 막지 않는다. 16칸이 가득 찼으면 **중복 검색보다 먼저 로그를 호출**하고 실패한다. 한도 로그 콜백이 바꾼 상태는 보존한다.
- **최상위 전달:** 캡처가 있으면 필터 없이 직접 처리한다. 실패하면 루트/dialog로 재전달하지 않는다. 성공하면 **콜백 이후 현재 캡처**를 읽고 0으로 지운 뒤 그 값을 반환한다. 처리기가 캡처를 0으로 바꾸면 성공한 처리기라도 반환 토큰은 0이다. 캡처가 없으면 현재 dialog, 없으면 루트의 실제 자식 우선 전달을 호출한다. 숨은 자식 강제 전달도 하위 함수에 그대로 넘긴다.
- **overlay:** 100칸 표의 현재 순서로 null/자신 visible/dead를 검사한다. 자신부터 모든 부모가 visible이어야 하며 부모의 dead/void는 이 조회의 제외 조건이 아니다. 실제 경계 안에 있고 실제 전체 `Pick(point,0)` 결과가 그 객체와 같을 때만 처리한다. 호출자의 사건 인자를 무시하고 별도 overlay 사건 코드 전역을 전달한다. 첫 대상의 처리기 반환이 **0이어도 즉시 종료**하며 전체 DWORD 반환을 보존한다. 위치/선택 콜백이 미래 overlay 칸을 바꾸면 그 현재 값을 읽는다.

## 같은 저장소 연결과 호스트 계약

실제 기본 생성→트리 부착→루트 전달→캡처→dialog 등록/중복→기존 실제 dialog 제거→자식 우선 삭제 요청/실제 Push→캡처/dialog 해제→후속 전달/전체 선택을 검사했다. 실제 overlay 등록/해제와 단일 수집 override도 같은 표/전역을 사용한다. 이번 가상 처리기는 명시 경계이며 그 몸체와 전체 Client 연결은 후속이다.

노드는 처리 중 안정된 주소로 살아 있어야 한다. 입력 캡처와 dialog 등록은 노드 필드를 읽지 않으며 실제 처리기/overlay 조회에 도달했을 때 유효 저장소를 요구한다. 잘못된 토큰·부모 순환·음수 dialog 개수·도달한 처리기/한도 로그의 누락은 호스트 예외로 거부한다. 콜백 예외 이전의 캡처/노드 쓰기는 되돌리지 않는다. 손상 입력의 원본 assert 및 실제 PE 예외 전파는 독립 근거에서 제외했다.

## 독립 관찰

`tools/decomp_gumpinput_oracle.py`는 세 PE의 위 전체 몸체와 실제 하위 선택/전달/좌표/dialog 제거를 정상 반환까지 격리 x86에서 실행한다. **가상 필터/처리기·월드 투영·dialog 한도 로그만 완성 입력 응답 경계**다. 기본 필터를 실제 가상 슬롯에 연결한 전달도 실행한다. 예상 대상/등록/성공값을 계산하거나 C++ 구현을 호출하지 않는다. void 캡처/override 함수의 EAX는 의미 있는 반환값으로 비교하지 않는다.

판본당 **232개**, 새 독립 원본 **696개**·cpppj 누적 **519,204개**, 두 x87 최상위 정상 반환 **1,638회**다. 전체 0xa0바이트 노드 여섯 개·일곱 선택 전역과 네 입력 전역·100/16칸 두 표·반환·각 콜백 직전 전체 상태/인자를 저장한다. 캡처 유지/콜백 변경·중복/마지막 등록 칸/한도 우선순위·dialog/루트/숨은 자식 전달·부모 visible·void/dead/경계·표 끝/중복/반환 0·현재 미래 표 변경·실제 월드 좌표·유한 임의 조합을 대조한다.

ABI/보존 레지스터/스택/x87/SEH/허용 불연속 실행·쓰기/노드 전후 보호 바이트/읽기 전용 SID·가상 표/SHA/진입·최상위 반환·경계 집계를 감사한다. 두 정밀도의 전체 관찰은 같으며 게임/GUI/OS/assert 호출은 0이다.

## 검증 결과

새 C++ **9개**, Release 경고/오류 **0**·CTest 내부 **810개·실패 0**(163.59초, 전체 163.64초), 근거 감사 **121종**·새 변이 **14개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). 최종 미검출/건너뜀 **0개**, 사본 소스 바이트 복구를 확인했다. 캡처 교체/override/미사용 dialog/현재 dialog/한도·이전 캡처/실패 해제/성공 유지/dialog 무시/강제 전달 누락·부모 dead/실제 Pick 생략/호출자 사건/반환 0 이후 계속 전달의 변이를 검출했다.

자료: `cpppj/recovery-gumpinput-evidence.json`, `cpppj/tests/fixtures/gumpinput-x86.tsv`, `extracted/gumpinput/{originals,originalCD,original1037}/`, `extracted/gumpinput-{export,smoke,oracle,build,focused,ctest,audits,mutations}.log`.

```powershell
powershell -File tools/ghidra/export_functions.ps1 -Name gumpinput
python -X utf8 tools/decomp_gumpinput_oracle.py
python -X utf8 tools/decomp_gumpinput_oracle.py --verify
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter GumpInput_
ctest --test-dir cpppj/build -C Release --output-on-failure
```

## 남은 작업

기본 캡처/dialog 등록/우선 전달/overlay 입력은 완료했다. 다음은 파생 Gump 생성·소멸자·실제 Form/버튼/메뉴 처리기·배경/텍스트 Draw, OS 사건 큐/idle/overlay 갱신과 전체 UI 프레임 연결이다. 별도 overlay 사건 코드의 원본 Template 공급도 실제 클라이언트 연결 시 필요하다.

게임 조회/색 표/추가 타입 상태/현재 Kernel 공급·게임 커서 입력/선택·SP lazy·구름/배경/dirty/장치·Client/GameWorld 자동 프레임, 실제 건설 SHP/팔레트/준비 소리·부모 공간/사제 contained·자원·건설 GUI는 계속 후속이다. **게임 내 건설/미션 완주는 아직 불가능하다.**

새 코드·도구·fixture·문서는 UTF-8이고 주석은 한국어다. AGENTS.md·보호 원본·dotnetpj/인계 변경·커밋/푸시는 없다.
