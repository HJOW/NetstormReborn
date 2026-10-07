# 10.37 추가 자료와 CD 실행 파일 비교

2026-10-07. 사용자 제공 `original1037`를 읽기 전용으로 분석했다. [비교 근거](../../cpppj/recovery-original1037-evidence.json)와 [재현 도구](../../tools/compare_original1037.py)에 원본 SHA, 전체 자료 목록, PE 섹션, 검토 함수 바이트 해시와 디컴파일 산출물 SHA를 기록했다.

## 전체 디컴파일

`tools/ghidra/run_decomp.ps1`과 `run_script.ps1`에 `-Edition original1037`를 추가했다. 기존 프로젝트와 분리한 `extracted/original1037/ghidra/netstorm.gpr`, 전체 C 결과 `extracted/original1037/decomp/netstorm.c`, 함수 사실 `functions.tsv`를 생성했다. 함수 **3,711개, 내보내기 실패 0**이다. GIF 내장 자료 분석 경고는 있었으며 자동 분석/내보내기/프로젝트 저장은 성공했다. 완전한 C++ 소스 복원이나 컴파일 가능한 디컴파일 결과라는 뜻은 아니다.

```powershell
& tools/ghidra/run_decomp.ps1 -Edition original1037
python -X utf8 tools/compare_original1037.py
python -X utf8 tools/compare_original1037.py --verify
```

전체 import/decompile은 추출 프로젝트에 저장한다. 이후 `run_script.ps1`는 기본 읽기 전용으로 필요한 함수만 내보낸다. 원본 디렉터리 파일에는 쓰지 않는다. 전체 디컴파일 파일은 기존 규칙대로 Git 제외 `extracted`에 두고 비교 결과만 저장한다.

## 실제 바이트 차이

새 실행 파일과 `originalCD/NETSTORM.EXE`는 모두 **1,647,616바이트**, image base `00400000`, entry `004f4b40`, 같은 여섯 PE 섹션 배치다. 두 디렉터리의 `netstorm.ver`는 모두 **10.37**이다. 프로젝트에서 사용해 온 CD 배포판 설명 10.72와 실제 실행 파일의 버전 표시는 구분해야 한다. `OriginalEdition::Cd1072`는 기존 구조 배치 식별자로 유지했다. 새 10.37을 별개 게임 규칙으로 추가하지 않았다.

두 파일 전체에서 차이는 다음 **한 바이트**뿐이다.

| 위치 | CD | 새 10.37 | 의미 |
|---|---|---|---|
| 파일 `0003314c` / VA `00433d4c` | `75 12` JNE | `eb 12` JMP | 모두 `00433d60`으로 이동 |

`00433bf0`의 CD 확인 실패 뒤 `InsertCD` 창을 만드는 구간을 새 사본은 무조건 건너뛴다. 앞선 확인 호출과 실패 반환은 남아 있으므로 모든 CD 검사 우회를 증명한 것은 아니다. 실행 결과를 추측하지 않고 정적 분기/문자열/호출로 해석했다. 원본 게임 프로세스와 진입점은 실행하지 않았다.

`.text`에 위 한 바이트만 다르고 나머지 데이터/리소스/재배치 섹션은 같다. 검토한 13개 영역/일반 탐색/발자국/해시/알림 helper도 주소와 전체 몸체 바이트가 같다. 새 PE의 해당 함수를 Ghidra로 다시 내보냈고 [영역 그래프 대조](cpp-regiongraph-reconstruction.md)에서는 **실제 새 PE 자체**를 별도로 격리 에뮬레이션한다. 기존 CD 메타데이터 공유 전에 한 바이트 차이를 검사하고, 실행 허용 함수에 그 주소가 포함되면 거부한다.

## 자료 비교

새 폴더 파일 261개와 CD 폴더 파일 425개 중 공통 내용이 같은 파일은 236개이며 `netstorm.tarc`도 같다. 공통 이름 중 8개가 다르고 이 중 하나가 실행 파일이다. 나머지는 도움말 문서/목차/캐시다. 새 폴더에만 있는 17개는 글꼴 캐시와 도움말 파일, 설정, 온라인 맵, trace 파일 등이다. 차이 목록과 새 폴더 전체 261개 파일의 SHA는 비교 근거에 저장했다. 작업 시작 때 저장한 전체 원본 해시와 종료 시 다시 비교하여 원본 보존을 확인한다.

10.37은 기존 CD 코드 배치를 해석하고 추출을 재현하는 자료로 유용하다. 두 구버전 사본의 동일 로직을 서로 다른 시대의 규칙 차이로 세지 않는다. 패치 10.78을 플레이 복원 기준으로 유지한다.

## 삭제 준비 후속의 이번 재검증

2026-10-07 이번 요청에서도 새 10.37을 전체 재디컴파일해 **3,711개 함수·실패 0**을 확인했다. 전체 C/함수 표 SHA는 앞선 기록과 동일하다. 삭제 준비/일반 Unpop·Graph·공통 표시/postPop helper를 새 경로에 읽기 전용 내보내기하고 세 PE의 실제 함수로 새 **1,158회**를 대조했다. [이번 독립 기록](../../cpppj/recovery-graphremove-evidence.json), [내보내기·범위·재현](cpp-graphremove-reconstruction.md).

전체 실행 파일의 유일한 한 바이트 차이와 이번 전체 산출물 SHA는 새 `decomp_graphremove_oracle.py --verify`에서 확인한다. 현재 PC에는 과거 `extracted/regiongraph/` 일부 추출물이 없어 과거 `compare_original1037.py --verify`의 전체 통과를 새로 주장하지 않는다. 과거 기록은 변경하지 않았다. 원본 세 폴더 전체 **1,394/425/261개**의 파일 SHA가 작업 시작과 동일하며 AGENTS.md·기존 원본/C# 변경과 원본/복사본/클론 창 실행은 없다. 이후 창 검사는 사용자 지시대로 다른 PC에 인계한다.
