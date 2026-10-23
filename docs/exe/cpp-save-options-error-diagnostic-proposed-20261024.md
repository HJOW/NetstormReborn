# SaveOptions 열기 오류 진단 최소 제안

단위387 준비·미적용이다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다. [앞선 읽기 진단](cpp-options-open-diagnostic-20261024.md)의 실제 원인 미확정을 유지하며, 다음 실제 실패의 오류 값을 남기는 단1파일 사본 패치만 준비했다. 원본 옵션·게임·GUI·공용 build/lib·기존 소스/CMake/LEFT/git에는 쓰지 않았다.

[제안 패치](../../extracted/cpp-save-options-diagnostic-20261024-proposed.patch)는 현재 `cpppj/src/client/ClientMain.cpp`의 `<cerrno>` 추가와 SaveOptions `if (!file)`만 바꾼다. `fopen(name.c_str(),"w+b")`는 그대로 단회이며, 호출 전 오류 상태 초기화·재시도·rename·다른 파일 공유 정책을 넣지 않는다. `fwrite`/`fclose`/파일 내용/성공 경로와 원래 `throw std::runtime_error("Unable to open d/options.cfg")` 문구도 그대로다.

실패 분기의 첫 실행은 `GetLastError()` 보관이다. 다음 `errno`를 복사하고, 마지막으로 `_get_doserrno`를 호출해 반환 상태와 값을 각각 저장한다. 이 수집 전에 다른 CRT 출력/문자열 구성은 하지 않는다. stderr에는 이미 계산된 절대 경로와 `errno`, `doserrno_read_status`, `doserrno_valid`, `doserrno`, `last_error`, `last_error_may_be_stale=1`을 한 줄로 쓴 뒤 원래 문구를 throw한다. `_get_doserrno` 반환이0일 때만 저장된 DOS 값을 읽은 것이다. `doserrno_valid`는 조회 성공만 나타내며 해당 fopen이 새 OS 오류를 썼다는 보장은 아니다. Win32 LastError도 fopen 내부에서 갱신하지 않은 과거 값일 수 있으므로 독립적인 확정 원인으로 해석하지 않는다. DOS 조회 실패 때 출력되는 초기값0 역시 유효한 OS 오류가 아니다.

사본은 [before](../../extracted/cpp-save-options-diagnostic-20261024-proposed/before/cpppj/src/client/ClientMain.cpp)·[copy](../../extracted/cpp-save-options-diagnostic-20261024-proposed/copy/cpppj/src/client/ClientMain.cpp)에 CRLF 그대로 보관했다. 패치는 LF다. 대상 한 파일의 기준은 다음과 같다.

| 자료 | SHA256 |
|---|---|
| 현재386 기준 원본 | `35f54f6d913b00f1a4dd5774553073838031ae95658c8cda059ba52f306b1a62` |
| 제안 사본 | `b842f89dde96ce5ae62e2766831df3ca6038838b6317be7a8e767437d3f63af7` |
| 단1파일 패치 | `76dcd849d8ac8055d58b166b4f27138d199c0a335c05f927ca5c2d56de5e9a38` |

최초/준비 직후의 원본 SHA는 [preparation.json](../../extracted/cpp-save-options-diagnostic-20261024-proposed/preparation.json)에, 최종 문맥 재조회와 국소 역변환 확인은 [verification.json](../../extracted/cpp-save-options-diagnostic-20261024-proposed/verification.json)에 남긴다. root가 후속 적용할 때 현재 ClientMain.cpp의 문맥을 다시 확인해야 한다. 이 작업에서는 git apply나 공유 파일 변경을 하지 않았다.

## 격리 검증 범위

[CaptureProbe.cpp](../../extracted/cpp-save-options-diagnostic-20261024-proposed/CaptureProbe.cpp)는 제안에서 추출한 [실제 실패 블록](../../extracted/cpp-save-options-diagnostic-20261024-proposed/SaveOptionsFailureBlock.inc)을 그대로 include한다. 별도 구현 미러/CTest를 만들지 않았다. 새 폴더에서 CREATE_NEW로 만든 합성 파일만 shareREAD|WRITE|DELETE로 열고 readonly section/view를 보유했다. 원본 옵션은 열거나 맵핑하지 않았다.

MSVC14.51.36231/SDK10.0.26100에서 `/std:c++20 /EHsc /utf-8 /MD /W4 /WX /D_CRT_SECURE_NO_WARNINGS`의 **단일 컴파일 종료0·경고0**이다. `_CRT_SECURE_NO_WARNINGS`는 정확한 기존 fopen을 쓰기 위한 제품과 같은 C4996 전용 정의이며 다른 W4 경고는 WX로 막았다. [빌드 로그](../../extracted/cpp-save-options-diagnostic-20261024-proposed/build1.log)와 [실행 결과](../../extracted/cpp-save-options-diagnostic-20261024-proposed/probe1.json)를 보존한다. 전체 ClientMain.cpp/공용 제품을 컴파일하거나 링크한 검증은 아니다.

실제 [stderr](../../extracted/cpp-save-options-diagnostic-20261024-proposed/probe1.stderr.log)는 `errno=22 doserrno_read_status=0 doserrno_valid=1 doserrno=1224 last_error=1224 last_error_may_be_stale=1`을 한 번 출력했다. [stdout](../../extracted/cpp-save-options-diagnostic-20261024-proposed/probe1.stdout.log)에서 원래 throw 문구를 확인했고 section/view/handle 반납 뒤 같은 함수의 열기는 성공했다. probe 종료0이다. 이것은 제안 수집 순서가 새 합성 파일의1224/22를 포착한다는 검증이며 과거 GuardMyBack 실패의 원인 확정이나 실제 새 제품 실행으로 세지 않는다.
