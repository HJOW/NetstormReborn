# HJOW-Athlon 정밀 디컴파일 재생성 기록

작업일: **2026-10-13**. 호스트는 **HJOW-Athlon**(192.168.0.94), 도구는 Ghidra 12.1.4, OpenJDK Zulu 25, Python 3.13.1이다. `LEFT_JOBS.md`의 최신 기록에 적힌 마지막 디컴파일 PC는 `VM-W11-CODEX`였으므로 `AGENTS.md` 절차에 따라 현재 PC에서 패치판 10.78과 CD판 10.72를 다시 분석했다.

## 결과

- 패치판 입력 `originals/Netstorm.exe`: SHA-256 `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`.
- CD판 입력 `originalCD/NETSTORM.EXE`: SHA-256 `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`.
- 최종 패치판 디컴파일: **5,549개 성공, 실패 0개**. 첫 단계에서는 5,548개였고, 힌트 적용 뒤 최종 내보내기는 5,549개였다.
- 최종 CD판 디컴파일: **5,822개 성공, 실패 1개**. `FUN_0049f99f`에서 Ghidra가 `Low-level Error: Cannot properly adjust input varnodes`를 반환했다. 실패 함수도 `raw.c`와 `functions.tsv`의 사실 기록에 남아 있다.
- 판본 간 대응 **2,496쌍**: strong 1,738, medium 707, weak 51. 호출 규약 일치 2,313/2,379 (97.2%), `ret N` 일치 2,339/2,389 (97.9%). 지역 순서 유지 2,068/2,495 (82.9%).
- 함수 신뢰도 등급: 패치판 A 2,326·B 3,152·C 40·D 31, CD판 A 2,315·B 2,939·C 484·D 85.
- 결과는 `extracted/refined/`에 생성했다. 기본 디컴파일 `extracted/decomp/Netstorm.c`와 소스 코드는 이 작업에서 수정하지 않았다.

## 실행 기록과 복구

```powershell
$env:PYTHONUTF8 = '1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1 -SkipStage1
```

- 최초 `refine_all.ps1` 실행은 두 판본의 1단계 분석과 내보내기를 마쳤지만, Windows 기본 CP949로 판본 대응 요약의 em dash(`—`)를 출력하다 `UnicodeEncodeError`가 발생했다.
- `PYTHONUTF8=1`을 설정하고 이미 끝난 1단계를 건너뛰어 힌트 적용과 최종 내보내기를 진행했다. 최종 PowerShell 실행이 종료 코드 1을 반환했으므로 `python tools/decomp_refine.py`를 별도로 재실행했다. 그 최종 산출물 재계산은 **종료 코드 0**이었고 위 대응·등급 통계를 다시 출력했다.
- 전체 단계별 로그는 Git 제외 경로 `extracted/refine-hjow-20261013-rerun.log`와 `extracted/refine-hjow-20261013-continuation.log`에 있다.

원본 게임을 실행하지 않았다. 원본 EXE는 읽기만 했으며 재확인한 SHA-256은 위와 같다. 게임/GUI 실행, 빌드, 테스트, cpppj 코드 변경, 커밋·푸시는 하지 않았다.
