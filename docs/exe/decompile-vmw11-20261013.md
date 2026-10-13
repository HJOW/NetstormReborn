# VM-W11-CODEX 정밀 디컴파일 재생성 기록

작업일: **2026-10-13**. 호스트는 **VM-W11-CODEX**(`10.0.0.17`), Ghidra **12.1.4**, Temurin OpenJDK **21.0.12.1**, Python **3.12.4**다. 사전 확인 당시 `LEFT_JOBS.md`의 마지막 디컴파일 PC가 `HJOW-Athlon`으로 기록되어 있어 AGENTS.md 지침에 따라 10.78 패치판과 10.72 CD판을 모두 현재 PC에서 다시 정밀 분석했다.

## 실행 및 결과

`PYTHONUTF8=1`을 설정하고 다음 절차를 실행했다. CP949 콘솔에서 판본 대조 진행 문자가 깨지지 않도록 Python UTF-8 모드를 사용했다.

```powershell
$env:PYTHONUTF8 = '1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1
```

최종 산출물은 Git 제외 경로 `extracted/refined/` 아래에 있다.

- 패치판 `originals/Netstorm.exe`: 전체 함수 5,549개, 디컴파일 성공 5,549개·실패 0개.
- CD판 `originalCD/NETSTORM.EXE`: 전체 함수 5,823개, 성공 5,822개·실패 1개. `FUN_0049f99f`에서 Ghidra가 `Low-level Error: Cannot properly adjust input varnodes`를 반환했다. 이전 정밀 디컴파일에서 확인된 동일한 한계다.
- 판본 간 대응: **2,496쌍**(strong 1,738, medium 707, weak 51). 대응 함수의 호출 규약 일치율은 2,313/2,379(97.2%), `ret N` 일치율은 2,339/2,389(97.9%)다.
- 판본별 정밀 출력은 `extracted/refined/originals/Netstorm.c`와 `extracted/refined/originalCD/NETSTORM.c`다. 함수별 원시 내보내기, 함수 표, 호출 규약·누락·switch 정보와 대응·등급표도 같은 디렉터리에 생성했다. 기준 출력 `extracted/decomp/Netstorm.c`는 갱신하지 않았다.
- 전체 Ghidra/Python 출력은 `extracted/refine-vmw11-20261013.log`에 저장했다. Ghidra가 일부 내장 GIF 자료 오류와 일반 p-code 경고를 출력했으며, 전체 작업은 종료 코드 0으로 끝났다.

## 원본 보호 확인

게임이나 별도 GUI를 실행하지 않았다. 실행 전후 보호 실행 파일의 SHA-256은 이전 기록과 동일하다.

- `originals/Netstorm.exe`: `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`
- `originalCD/NETSTORM.EXE`: `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`

이번 작업은 `extracted/refined/` 생성 산출물과 이 작업 기록 및 `LEFT_JOBS.md`만 갱신했다. 코드·테스트·dotnetpj는 수정하지 않았고, 테스트·빌드·커밋·푸시는 하지 않았다.
