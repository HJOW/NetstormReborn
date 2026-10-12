# VM-W11-CODEX 정밀 디컴파일 재생성 기록

작업일: **2026-10-12**. 호스트는 **VM-W11-CODEX**(10.0.0.17), 도구는 Ghidra 12.1.4, OpenJDK 21.0.12, Python 3.12.4다. `LEFT_JOBS.md`에 기록된 마지막 디컴파일 PC가 `HJOW-Athlon`이어서 AGENTS.md 절차에 따라 패치판 10.78과 CD판 10.72의 정밀 디컴파일을 이 PC에서 다시 만들었다.

## 결과

- 패치판 `originals/Netstorm.exe`: 최종 내보내기 함수 **5,549개 성공, 실패 0개**.
- CD판 `originalCD/NETSTORM.EXE`: 최종 내보내기 함수 **5,822개 성공, 실패 1개**. `FUN_0049f99f`에서 Ghidra가 `Low-level Error: Cannot properly adjust input varnodes`를 반환했다.
- 판본 간 대응 **2,496쌍**: strong 1,738, medium 707, weak 51. 호출 규약 일치 2,313/2,379 (97.2%), `ret N` 일치 2,339/2,389 (97.9%).
- 정밀 결과는 `extracted/refined/` 아래에 생성했다. 기존 기본 디컴파일 `extracted/decomp/Netstorm.c`는 갱신하지 않았다.
- 첫 실행에서 Ghidra의 두 판본 분석과 1차 내보내기는 마쳤지만, Python의 기본 `cp949` 콘솔 인코딩이 대응표 진행 문자의 출력에 실패했다. 저장된 Ghidra 프로젝트를 보존하고 `PYTHONUTF8=1`과 `-SkipStage1`로 이어서 호출 규약 힌트·최종 내보내기·대응표 생성을 끝냈다. 재분석은 반복하지 않았다.

실행 로그는 `extracted/refine-vmw11-20261012.log`와 `extracted/refine-vmw11-20261012-resume.log`에 있다. 전체 절차를 새로 재현할 때는 Python UTF-8 모드를 켜고 다음 명령을 실행한다.

```powershell
$env:PYTHONUTF8 = '1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1
```

## 원본 보호 확인

원본 게임을 실행하지 않았다. `originals/`와 `originalCD/`의 Git diff는 비어 있으며 두 바이너리의 SHA-256은 다음과 같다.

- `originals/Netstorm.exe`: `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`
- `originalCD/NETSTORM.EXE`: `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`

이 기록은 cpppj의 동작이나 코드 복원 범위를 변경하지 않는다. 새 정밀 출력으로 주소를 조회해 이어서 분석할 수 있다.
