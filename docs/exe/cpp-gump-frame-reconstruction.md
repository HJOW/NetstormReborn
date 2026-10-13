# cpppj Gump 등록표 통지·idle·재그리기 요청 복원

작업 PC: **VM-W11-CODEX**, 기록일: **2026-10-12**. AGENTS.md와 두 LEFT_JOBS를 먼저 확인했고 현재 PC와 마지막 디컴파일 PC가 같았다. `gumpframe`을 세 판본에서 **4/4/4개** 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-12.**

`GumpFrame`은 기존 `GumpCreateState`와 `GumpLifetimeState`를 공유한다. 등록표 통지, idle 등록표 실행, 단일 노드/최상위 트리의 재그리기 요청을 복원했다. 가상 재그리기 요청은 실제 픽셀 Draw와 별개이며 파생 처리기와 Client 자동 프레임은 후속이다.

## 원본 근거

| 동작 | 패치판 10.78 | CD·추가 10.37 |
| --- | --- | --- |
| 등록표 통지, 가상 `+0x2c` | `00464100` | `00492940` |
| idle 등록표, 가상 `+0x30` | `00464170` | `00492990` |
| 단일 노드/자식 재그리기 요청 | `00464c20` | `004939e0` |
| 최상위 트리 재그리기 요청 | `004660f0` | `004927c0` |

idle 표는 패치판 `005634d0`의 500칸, CD·추가판 `0056b5b8`의 80칸이다. 등록표는 각각 `00563e38`/`0056b748`의 1,000칸이며 개수 전역은 `00565dc8`/`00535e80`이다. 실제 WinMain의 UI 처리 부분은 실행 중 삭제 정리→idle, 정지 중 idle만 호출한다. 전체 메인 루프와 파생 idle 처리기 복원 완료를 뜻하지 않는다.

## 복원한 동작

- **등록표 통지:** 루트인 0번 칸을 제외하고 1~999번 칸을 방문한다. 방문한 비영 칸 수가 현재 signed 개수에 도달하면 다음 슬롯을 읽기 전에 종료한다. 빈 칸은 개수에 포함하지 않으며 visible/통지/dead 필터에 실패한 비영 칸은 포함한다. visible·`0x100`·비 dead인 객체에만 전체 DWORD 인자 세 개를 전달한다. 콜백이 바꾼 개수와 미래 등록 칸은 다음 방문에 반영한다.
- **idle:** 현재 표의 토큰을 슬롯 순서로 호출하며 플래그·부모 visible·등록 개수는 검사하지 않는다. 중복 토큰은 중복 호출한다. 콜백이 미래 슬롯을 비우거나 채우면 그 현재 값에 따른다. 이미 방문한 칸으로 되돌아가지 않는다. CD는 호스트 배열 뒤 420칸을 실행하거나 변경하지 않는다.
- **단일 노드 요청:** 현재 `0x42000`이 없을 때 가상 `+8(false)`를 호출한다. 자신 visible/dead/void는 필터가 아니다. 콜백 이후 현재 `0x20`을 읽고, 현재 visible 자식만 재귀 처리한다. 하위 자식의 dead/void도 여기서는 제외 조건이 아니다. 각 자식의 next는 재귀 호출 이후 현재 값을 읽는다.
- **최상위 요청:** 실제 등록표 0번의 자식부터 시작한다. 루트 자신의 visible과 수집 override는 사용하지 않는다. 최상위 visible·비 dead 필터를 적용하며 다음 최상위 토큰은 콜백 전에 보관한다. 자식 전파 비트와 하위 링크는 위 단일 노드 함수와 같은 시점에 읽는다.

호스트는 유효한 노드 저장소와 도달한 가상 콜백을 요구한다. 처리 도중 노드 주소는 안정되어야 하며 실제 해제는 지연 삭제 단계에서 수행한다. 잘못된 토큰·순환 링크·누락 경계는 호스트 예외로 거부하고, 콜백이 쓴 뒤 예외를 던지면 이전 쓰기를 보존한다. 손상 입력의 원본 assert 및 PE의 실제 예외 전파는 독립 관찰 범위에서 제외했다.

## 독립 관찰과 연결

`tools/decomp_gumpframe_oracle.py`는 네 함수와 실제 하위 재귀 전체를 세 PE에서 실행한다. **가상 `+0x2c`/`+0x30`/`+8`만 완성 입력 응답 경계**다. C++ 구현이나 예상 호출 순서를 계산하는 함수를 호출하지 않는다.

판본마다 **270개**, 새 독립 원본 **810개**·cpppj 누적 **520,014개**, 두 x87 모드 최상위 정상 반환 **1,626회**다. 각 행의 전체 여섯 0xa0바이트 노드·등록/idle 표·현재 개수·콜백 직전 상태와 인자·진입/반환을 저장했다. 가상 표 불변, 노드 전후 보호 바이트, 허용 실행/쓰기, 스택/보존 레지스터/x87/SEH, 입력 순서·파일 SHA·집계를 감사한다. 게임/GUI/OS/assert 호출은 0이다.

같은 저장소를 사용하는 C++ 검사는 실제 기본 생성·트리 Attach→idle 등록→idle 중 다른 노드 Kill→실제 트리 Push/후속 호출 제외→기본/scalar 소멸자 Cleanup/저장소 해제→다음 idle을 연결한다. 등록 통지의 signed 개수·루트 제외와 재그리기의 최상위/하위 링크 조회 차이, CD 표 끝, 호스트 예외 계약도 검사한다.

자료는 `cpppj/gumpframe.json`, `cpppj/tests/fixtures/gumpframe-x86.tsv`, `extracted/gumpframe/{originals,originalCD,original1037}/`, `extracted/gumpframe-{export,smoke,oracle,build,build-final,focused,ctest}.log`에 있다.

## 검증 결과

새 C++ 검사 **9개**, 최종 Release 빌드 경고/오류 **0**, 전체 CTest 내부 **819개·실패 0**(실행 87.09초, 전체 87.12초)이다. 새 원본 근거 SHA/입력/폭/진입·반환·경계 감사도 통과했다. 첫 테스트 컴파일의 공통 도구 `Scene` 이름 충돌은 `FrameScene`으로 수정했으며 최종 빌드와 검사는 통과했다.

원본/복사본 게임·analyzeManager·클론 창은 실행하지 않았다. 원본 바이너리는 읽기만 했고 보호 디렉터리와 dotnetpj/인계는 수정하지 않았다. 작업 중 사용자가 변경한 AGENTS.md는 보존했고 제가 수정한 부분은 없다. 커밋/푸시를 수행하지 않았다. 새 코드·fixture·문서·기록은 UTF-8이며 주석은 한국어다.

## 재현

```powershell
powershell -File tools/ghidra/export_functions.ps1 -Name gumpframe
python -X utf8 tools/decomp_gumpframe_oracle.py
python -X utf8 tools/decomp_gumpframe_oracle.py --verify
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter GumpFrame_
ctest --test-dir cpppj/build -C Release --output-on-failure
```

## 남은 작업

등록표 통지/idle/재그리기 요청의 정상 몸체는 복원했다. 다음은 파생 Gump 생성·소멸자·실제 Form/버튼/메뉴/idle 처리기·배경/텍스트 Draw, OS 사건 큐와 overlay 갱신·전체 UI 프레임 연결이다. 기존 게임 조회/색 표/Kernel 공급·게임 커서 입력/선택·SP lazy·구름/장치·Client/GameWorld 자동 프레임·실제 건설/자원/전투도 후속이며 **게임 내 건설/미션 완주는 아직 불가능하다.**
