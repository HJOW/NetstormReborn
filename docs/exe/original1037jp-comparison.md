# 10.37 일본 유통 CD(original1037JP) 디컴파일과 CD/10.37 비교

2026-10-09, 호스트 `HJOW-Athlon`. 사용자가 제공한 `original1037JP`(일본에서 유통된 10.37 CD)를 [10.37 추가 자료](original1037-comparison.md)와 같은 방식으로 읽기 전용 분석했다. [비교 근거](../../cpppj/recovery-original1037jp-evidence.json)와 [재현 도구](../../tools/compare_original1037jp.py)에 SHA, 파일 목록, PE 섹션, 디컴파일 산출물 SHA를 기록했다.

## 판본 성격 (사용자 판단, 2026-10-09)

폴더 이름은 `JP`이지만 **일본에서 유통만 했을 뿐 내용은 영문판일 가능성이 높다**고 본다. 근거는 아래와 같다.

- 일본어 리소스가 없다. 리소스 DLL은 `nsenglishres.dll`(`originalCD`와 동일)과 `nsgermanres.dll`뿐이고 `d/lang.english`가 있다.
- `NetStorm.exe`가 `originalCD/NETSTORM.EXE`와 바이트 단위로 같다. 따라서 코드는 일본어 전용 판이 아니다.
- 일본 배포 흔적(`movie/*.ang`, `dsetup*j.dll`)은 이 폴더가 아니라 `originalCD` 쪽에 있다.

이 문서에서 "일본판"은 **폴더/유통 이름**이며 언어 판본이라는 뜻이 아니다. 다국어(영어·한국어) 작업에서 일본어 텍스트/리소스 자료로 쓰지 않는다. 확정하려면 게임을 실행해 언어를 확인해야 하지만 실행하지 않았다.

## 보관 정책 (중요)

`original1037JP/`는 **Git에 커밋하지 않는다** (`.gitignore`의 `original1037JP/**`). 파일은 **`HJOW-Athlon`과 `HJOW-X3D` 두 PC에만** 보관한다. 다른 PC에는 폴더가 없으므로 다음을 따른다.

- `tools/compare_original1037jp.py`는 폴더가 없으면 "건너뜀"을 출력하고 정상 종료한다 (`--verify` 포함).
- 이 문서와 근거 JSON, 스크립트(`-Edition original1037JP`)만 Git에 있다. 근거 JSON에는 파일 이름·크기·SHA만 있고 자료 자체는 들어 있지 않다.
- `extracted/original1037JP/`(Ghidra 프로젝트·디컴파일 결과·보호 해시)도 Git 제외다. 다른 PC에서 내용이 필요하면 자료를 두 PC 중 하나에서 옮겨 와 아래 절차로 다시 만든다.

## 재현

```powershell
& tools/ghidra/run_decomp.ps1 -Edition original1037JP     # 약 4분, Ghidra 12.1.4
python -X utf8 tools/compare_original1037jp.py             # 근거 JSON 생성
python -X utf8 tools/compare_original1037jp.py --verify    # 재검증
```

`run_script.ps1 -Edition original1037JP`로 이후 함수 단위 읽기 전용 내보내기를 할 수 있다 (프로젝트 `extracted/original1037JP/ghidra/NetStorm.gpr`). 전체 결과는 `extracted/original1037JP/decomp/NetStorm.c`·`functions.tsv`이다.

## 결과

**함수 3,711개, 내보내기 실패 0.** 내장 GIF 분석 경고(`Invalid GIF data at 005409d4/005409dc`)는 CD·10.37과 같다.

### 실행 파일

| 비교 | 결과 |
|---|---|
| `original1037JP/NetStorm.exe` ↔ `originalCD/NETSTORM.EXE` | **바이트 단위로 완전히 같다** (SHA-256 `613500a3…c851233`, 1,647,616바이트) |
| `original1037JP/NetStorm.exe` ↔ `original1037/netstorm.exe` | 파일 `0x3314c` 한 바이트(`75` ↔ `eb`)만 다르다 (기존 [CD↔10.37 차이](original1037-comparison.md)와 같은 위치) |

그래서 `NetStorm.c`는 `extracted/originalCD/decomp/NETSTORM.c`와 **SHA-256까지 같다** (`f6fecc58…250f50acbc`). `netstorm.ver`는 `10.37`이다. 새로운 코드 배치는 없으므로 C++ 복원의 CD/10.37 비교 자료 해석(주소·함수 대응)을 그대로 쓸 수 있고, 이 폴더를 별개 규칙으로 세지 않는다. 기준은 계속 10.78이다.

### 자료 (3,402개 파일, 약 696MB)

- `originalCD`와 이름이 같은 362개 중 **361개가 내용까지 같고**, 다른 것은 `autorun.inf` 하나다. `netstorm.tarc`는 CD·10.37·일본판이 모두 같다 (SHA `e7e88685…`).
- `original1037`과 이름이 같은 242개 중 234개가 같고, 다른 8개는 `netstorm.exe`와 도움말 파일(`help/*.cnt|gid|hlp`)이다.
- 번들: `directx/` 3,036개(약 142MB), `demos/`의 AVI 3개(Heavy Gear, Zork Grand Inquisitor, Dark Reign), `spart/` 설치 화면 BMP 44개(Interstate 76, MechWarrior, Heavy Gear, Zork GI 등 다른 게임 로고 포함), 음악 9개(약 126MB), 소리 230개, 인트로 `movie/englishintro2x.smk`·`germanintro2x.smk`.
- `originalCD`에만 있는 자료는 `movie/*.ang` 30여 개, `dsetup*j.dll`, `netrun.exe`, `setup.bin/dat` 등이다. 일본어로 보이는 파일명(`koeilogo.ang`, `jikkou.ang` 등)은 일본판 폴더가 아니라 **`originalCD` 쪽**에 있다.
- 이 폴더의 리소스 DLL은 `nsenglishres.dll`(CD와 동일)과 `nsgermanres.dll`뿐이고 `d/lang.english`가 있다. **일본어 리소스는 확인하지 못했다.** 위 "판본 성격"대로 영문판일 가능성이 높다.

## 보호 확인

작업 전 폴더 전체 3,402개 파일의 SHA를 `extracted/original1037JP/protected-before.json`에 저장했고, 작업 후 모두 같음을 확인했다. 원본 게임/설치 프로그램/복사본/클론 창은 실행하지 않았다 (정적 분석만).
