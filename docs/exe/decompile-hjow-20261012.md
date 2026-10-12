# HJOW-Athlon 정밀 디컴파일 재생성 기록

작업일: **2026-10-12**. 호스트는 **HJOW-Athlon**(192.168.0.94), 도구는 Ghidra 12.1.4, OpenJDK Zulu 25, Python 3.13.1이다. `LEFT_JOBS.md`의 마지막 디컴파일 PC가 `VM-W11-CODEX`였으므로 AGENTS.md 절차에 따라 현재 PC에서 패치판 10.78과 CD판 10.72를 다시 분석했다.

## 결과

- 패치판 `originals/Netstorm.exe`: 최종 내보내기 **5,549개 성공, 실패 0개**.
- CD판 `originalCD/NETSTORM.EXE`: 최종 내보내기 **5,822개 성공, 실패 1개**. `FUN_0049f99f`에서 Ghidra가 `Low-level Error: Cannot properly adjust input varnodes`를 반환했다.
- 판본 간 대응 **2,496쌍**: strong 1,738, medium 707, weak 51. 호출 규약 일치 2,313/2,379 (97.2%), `ret N` 일치 2,339/2,389 (97.9%).
- 결과는 `extracted/refined/`에 생성했다. 원본 기본 내보내기 `extracted/decomp/Netstorm.c`는 건드리지 않았다.
- Python 출력에 CP949에서 표현할 수 없는 기호가 있으므로 실행 때 UTF-8 모드를 켠다.

재현 명령:

```powershell
$env:PYTHONUTF8 = '1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1
```

최종 대응표·등급·C 출력 생성 단계는 UTF-8 모드에서 별도로 재실행해 종료 코드 0과 동일한 2,496쌍 및 등급 통계를 확인했다.

## 원본 보호 확인

원본 게임을 실행하지 않았다. 실행 전후 바이너리 SHA-256이 일치한다.

- `originals/Netstorm.exe`: `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`
- `originalCD/NETSTORM.EXE`: `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`

실행 로그는 Git 제외 경로 `extracted/refine-hjow-20261012.log`에 있다.
