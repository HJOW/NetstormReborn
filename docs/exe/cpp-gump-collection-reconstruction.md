# UI/Gump 화면 좌표·트리 표시 수집 복원

2026-10-11, `HJOW-Athlon`. `GumpCollection`으로 원본 UI 트리의 화면 위치/경계와 kind 2 표시 항목 수집을 복원하고 `BindGumpCollection`으로 실제 공통 병합·정렬·분배 경로에 연결했다. 실제 GUI의 Gump 생성/그리기와 자동 프레임 연결은 후속이다.

AGENTS.md와 두 LEFT_JOBS를 먼저 확인했으며 착수 시 작업 트리는 깨끗했다. 현재 PC가 마지막 디컴파일 PC와 같았다. `gumpcollection` **6/7/7개**를 읽기 전용 Ghidra로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창은 실행하지 않았다.

## 원본 대응과 좌표

| 역할 | 10.78 | CD/추가판 |
| --- | --- | --- |
| 최상위 수집 | `00466070` | `00492750` |
| 재귀 노드 수집 | `00464b30` | `004938c0` |
| 화면 위치 | `00464740` | `004934c0` |
| 화면 경계 | `004648e0` | `00493670` |
| 엄격한 정수 교차 | `00463f80` | 노드 수집에 인라인 |
| CRT 64비트 절삭 | `004e49c0` | `004f161c` |
| 부착 부모 int→float/float 덧셈 | 위치 몸체에 인라인 | `004950f0`/`00495110` |

일반 노드는 자신의 로컬 float 위치에서 시작해 모든 부모 위치를 누적한다. 부모의 visible/dead나 이전 캐시는 사용하지 않는다. **부모마다 float32에 저장**하고 마지막에 CRT 방식으로 0쪽 절삭한다. 원본 CRT는 64비트 정수의 하위 DWORD를 쓰므로 큰 값의 32비트 넘침과 비유한/64비트 범위 밖 입력의 하위 0도 보존한다. 일반 노드의 `+0x6c/+0x70` 캐시를 갱신한다.

`flags & 0x2000`인 노드는 root 직계 자식이어야 한다. 객체 SID의 월드→화면 투영 정수점에 **각 로컬 float을 따로 절삭한 정수**를 DWORD 덧셈한다. 이 노드 자신의 위치 조회는 root 위치를 더하지 않고 **캐시도 갱신하지 않는다**. 일반 자식이 부착 부모를 만나면 그 정수점을 더한 뒤 root까지 계속 누적한다.

여기에는 판본 차이가 있다. 10.78은 부착 부모의 정수를 x87로 정확히 올려 자식 float과 더한다. CD는 정수를 **먼저 float32로 저장한 뒤** 자식 위치와 더한다. 예컨대 부모 정수 `16,777,217`과 자식 `-16,777,216`의 합은 root 위치가 0일 때 10.78은 `1`, CD는 `0`이다. 판본은 `GumpCollectionState::edition`으로 선택한다.

경계의 left/top은 위 정수 위치이며 right/bottom은 정수 위치와 `+0x44/+0x48`의 배율 적용 크기를 더해 절삭한다. **합을 float32로 먼저 저장하지 않는다.** 위치 디컴파일의 일부 x87 누적/크기 연산이 C 의사 코드에서 누락되므로 실제 기계어로 확인했다.

월드 투영과 SID 해석은 `project(object)` 응답 경계다. 이번 구현은 그 내부를 복원했다고 주장하지 않는다.

## 트리 수집

최상위는 `root.child`에서 시작하며 임시 머리 전역이 0이 아니면 그 토큰으로 대체한다. root의 visible/dead는 검사하지 않는다. 각 최상위 노드는 visible bit 1이 켜지고 dead bit `0x10000`이 꺼졌을 때만 재귀 수집한다. **next는 재귀 호출 전에 저장**한다.

원본 최상위 함수는 모든 노드에 **같은 입력 limit**을 준다. 앞선 노드의 반환 수를 limit에서 빼지 않는다. 따라서 최상위 두 노드에 limit 1을 주어도 전체 반환은 2일 수 있다. 원본 동작을 `Collect`에 보존하되 실제 호스트 출력 span을 넘기는 쓰기는 예외로 거부한다. 공통 병합에 연결할 때는 그 경로가 공급한 실제 남은 span이 저장 용량이다.

재귀 수집은 다음 순서다.

1. limit < 1이면 위치 조회 없이 0을 반환한다.
2. 현재 화면 경계를 계산하고 역전 사각형/맞닿은 가장자리를 제외해 교차를 검사한다. 화면 내부의 점/선 크기는 원본처럼 허용한다.
3. 교차하면 자신의 **로컬 x/y**, 토큰, depth DWORD의 signed 하위 WORD, **kind 2와 tail 0**을 쓴다. 절대 화면 좌표를 정렬 키에 쓰지 않는다.
4. 교차하지 않을 때는 `flags & 0x20`이 켜져야 자식을 계속 수집한다. 교차한 노드는 이 비트 없이도 자식을 수집한다.
5. 자식에는 **visible만 검사**한다. dead 자식도 visible이 켜졌으면 포함된다. 각 자식 호출 뒤 반환 수만큼 출력 위치를 옮기고 잔여 limit을 줄인다. **next는 호출 뒤 현재 값을 읽는다.**

직접 `CollectNode`를 호출하면 자신의 visible/dead는 검사하지 않는다. 재귀 경로/형제/부모 순환·누락 토큰·원본 MAX_GUMPS 1,000 초과·부착 부모 계약 위반·없는 투영은 호스트에서 거부한다. 서로 다른 경로에 반복 등장하는 토큰은 일괄 중복 제거하지 않는다. 예외 전에 이미 쓴 항목/캐시는 남을 수 있다.

## 연결과 독립 근거

`BindGumpCollection`은 `DrawCollectionHooks.ui`에 실제 수집기를 연결한다. 새 통합 검사는 실제 UI 트리를 공통 수집→판본별 CRT→kind 2 분배로 통과시키며 다른 종류의 항목과 함께 정렬한다. UI 전용 dirty에서는 뒤 수집기를 호출하지 않는다. 다른 종류의 수집/그림 콜백은 이 검사의 명시 응답 경계다. 실제 Gump 생성/가상 Draw·Client/UberGump 자동 프레임을 연결한 검사는 아니다.

`tools/decomp_gumpcollection_oracle.py`는 세 PE 각 **389개**, 새 **1,167개**, 두 x87 정밀도에서 **2,334회 루트 정상 반환**을 관찰했다. 독립 원본 누적은 **506,253개**다. 위치/경계/정수 교차/최상위/재귀 수집/CRT와 CD 보조 함수는 실제 전체 몸체를 실행한다. **월드 투영만 완성 정수점으로 응답**하며 링크 재조회 검사의 next 변경도 그 명시 응답에 포함한다.

입력은 최상위/자식 visible·dead·화면 밖 통과, limit -1/0/1/2/4, 임시 머리, 맞닿음/점/선/역전, 분수/음수/큰 정수/NaN/무한대/비정규수/음수 0, 무작위 다단계 부모, 부착 자신/자식과 DWORD 넘침, 투영 경계가 바꾸는 최상위/자식 next를 포함한다. 기대 위치나 목록은 계산하지 않는다.

관찰은 완성 128바이트 노드 입력·투영 응답, signed 위치/경계 또는 반환 개수, **32칸 전체 표시 바이트**, 모든 노드의 캐시, 투영 순서, 실제 함수 진입, 보호 바이트를 저장한다. 비영 보존 레지스터/스택/x87/SEH·허용 실행/쓰기·전체 노드의 허용 외 변경·미사용 출력/반환 영역·보호 바이트를 검사한다. 저장 감사는 PE/도구/내보내기/fixture SHA·입력 계획·폭·진입/반환/투영 집계를 확인한다. 게임/OS/assert는 0이다.

## 검증

새 C++ **6개**, Release 경고/오류 **0**·CTest 내부 **717개·실패 0**(219.13초, 전체 219.17초), 근거 감사 **109종**·새 변이 **9개 모두 통과**(`--focused`, 변이 전 사본 전체 검사 통과). 집중 검사 **6개·실패 0**. CTest는 변이 사본 빌드와 병행했다. 새 검사는 세 PE 재생, 루트별 limit/출력 용량, 손상 링크/부착 계약, 실제 UI 트리의 공통 병합/정렬/종류 분배다.

새 변이 9종(맞닿은 경계·자식 dead 필터·최상위 잔여 limit·tail 보존·CD 부착 부모 반올림 생략·경계 합의 float 반올림·부착 캐시 변경·최상위 현재 next·공통 UI 수집 생략)은 모두 검출했다. 별도 `extracted/mutation-work` 사본에서 적용하고 원래 바이트로 복구했다. 미검출/건너뜀 **0개**다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name gumpcollection
python -X utf8 tools/decomp_gumpcollection_oracle.py --smoke
python -X utf8 tools/decomp_gumpcollection_oracle.py
python -X utf8 tools/decomp_gumpcollection_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter GumpCollection
ctest --test-dir cpppj/build -C Release --output-on-failure
```

로그는 `extracted/gumpcollection-{export,smoke,oracle,build,focused,ctest,audits,mutations}.log`다. AGENTS.md·보호 원본·dotnetpj/인계는 변경하지 않았고 커밋/푸시하지 않았다.

## 다음 작업

1. 묶음 `00433410`, 일반 자산/효과 `00433660`, 미리보기 `00446060`의 수집기 내부와 월드 투영/SID 자료 연결.
2. 실제 Gump 생성/가상 Draw·구름/배경/dirty 순회/장치·Client/GameWorld 자동 프레임과 클릭 선택.
3. 실제 건설 SHP 캡처/공유 팔레트/현재 색 표·타입 준비 소리(+0x90)·부모 공간/사제 contained·하위 경로/자원·집합/지연 분배·건설 GUI. **게임 내 건설/미션 완주는 아직 불가능하다.**
