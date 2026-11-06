# cpppj 진행분 마감 인계 — 2026-11-06

사용자가 **진행 중 작업 마감 → 문서/로컬 Git 커밋 → 목표 일시중지**를 요청해 준비·분석 범위를 마감했다. 실제 제품은 **985 / 11.01**, 제품 구현 커밋은 `1f00a25add9619291f7978a18b2c07f2de0ee242`다. 아래 코드는 **제품 미반영 후보**이며 C++ 컴파일·검사·게임 실행을 하지 않았다. 준비본 보존 완료를 한국어·시나리오 UI·멀티플레이 기능 완료로 세지 않는다.

호스트와 마지막 정밀 디컴파일 호스트는 **HJOW-Athlon**, 마지막 정밀 자료 날짜는 **2026-11-03**이다. 다른 호스트에서 원문 분석을 계속할 때는 AGENTS.md의 재디컴파일 규칙을 따른다. 사용자 재개 지시 전에는 후속 구현을 시작하지 않는다.

## 마감한 진행분

| 디렉터리 | 보존 내용 | 확인/미검증 범위 |
| --- | --- | --- |
| [korean](korean/FINAL_PLAN.md) | 985에 재접합한 코드·CMake·배포·검사 후보41파일, 한국어 자산44개, 기준 사본과 해시·정적 결과 | 패치 적용 검사·배포 PS 구문 분석 통과. 새 검사24개는 작성만 했고 컴파일/실행0 |
| [scenario](scenario/PLAN.md) | 별도 편집 UI·파일/English 기본 생성·Test Battle 연결 후보24파일(변경23), 기준18파일, CMake 목록·패치·검토 결과 | 패치 적용 성공·개행 정규화 후24/24 동일. 새 통합9개와 기존 모델10개 준비 검사는 모두 미실행 |
| [remote-fort](remote-fort/PLAN.md) | 실제 이름/manager/MarkStream/이미지 수명의 접합 설계·원문 함수표 | 정식 산출물은 설계다. 제품 구현/컴파일/동작 검증 미착수. 별도 스케치는 적용 금지 |

원래 `cpppj/src/client`와 `cpppj/tests`에 미등록 상태로 남아 있던 모델5파일은 **삭제하지 않고** `scenario/base/cpppj/...`로 이동했다. 같은 원래 경로의 후보도 `scenario/pending/cpppj/...`에 있다. 기존 제품 CMake에 등록되지 않았던 소스여서 제품985는 바뀌지 않았다. 이동한 정확한 경로와 SHA는 전체 manifest 및 단위 보고서에 기록한다.

## 바이트 기준과 검증

이 디렉터리의 `.gitattributes`는 원래 CRLF/LF 혼용과 UTF-8 BOM을 보존한다. `MANIFEST.json`이 보관된 모든 파일의 실제 바이트 SHA-256을 기록하며, 다음 읽기 전용 검증기를 저장소 루트에서 실행할 수 있다.

```powershell
python .\docs\exe\handoffs\cpp-pause-20261106\verify.py
```

**전체 후보 사본이 바이트 기준이고 패치는 검토·hunk 병합용**이다. 시나리오 패치를 다른 Git 개행 설정에서 적용하면 내용이 같아도 CRLF/LF 때문에 준비본 SHA와 달라질 수 있다. 관련 검증 JSON 두 개를 함께 보존했다. 정적 적용 검사·해시 검증을 C++ 구문/링크/동작 통과로 해석하지 않는다.

이전 `extracted/...` 경로는 작성 당시 로컬 원시의 출처다. 다른 PC에서 그 경로가 존재한다고 가정하지 않는다. 재개에 필요한 최종 후보·기준·자산·검증 문서는 이 추적 디렉터리에 있다. 한국어의 `finalize-final.py`, `final-tweaks.py`는 작성/인계 생성 기록이며, 경로와 입력이 작성 당시 환경에 연결돼 있으므로 **재개 시 적용 도구로 실행하지 않는다**.

## 재개 절차

1. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md와 Git 상태를 다시 읽는다. 실제 제품985와 이 후보의 기준을 새 HEAD와 대조하고 현재 변경 위에 사본 전체를 덮어쓰지 않는다.
2. 원래 우선순위를 유지해 원격 Fort 등 LAN 잔여를 진행한다. [원격 설계](remote-fort/PLAN.md)와 [함수표](remote-fort/FUNCTION_MAP.md)를 따른다. `remote-fort/unvalidated-draft`는 작성 도중의 미검증 스케치다. [적용 금지 안내](remote-fort/unvalidated-draft/DO_NOT_APPLY.md)를 먼저 읽고 그대로 적용하지 않는다.
3. 한국어 단계에서 `korean/FINAL_PLAN.md`를 따라 최종 코드 패치 하나만 검토한다. **`final.patch`에는 자산이 없으므로** `korean/final-pending/assets/lang/korean/`의44파일을 별도로 병합한다. 옛 패치를 중복 적용하지 않는다. 실제 빌드와 허용된 집중 검사를 수행하고 결과를 새 단위에 기록한다.
4. 시나리오 단계에서는 한국어의 DisplayTextInput·IME/클립보드·HTML 표시 보존을 먼저 통합한다. 미등록 모델5파일이 없으면 `scenario/base`에서 해당 다섯 개만 원래 경로에 복원한다. 이후 기존 소스와 후보를 hunk 단위로 병합하고 `CMake-additions.txt`의 등록·허용 검사를 수행한다. 실제 사용자 홈 저장/Editor/Test Battle 진입은 별도 검증이 필요하다.

단위별 남은 구현은 [LEFT_JOBS.md](../../../../LEFT_JOBS.md)에 유지했다. 실제 사제 포획/희생·특정 적 공격·승리, 신규 MP/소켓/통신, 사용자 직접 확인 대상은 기존 지시대로 제외한다. 다른 세션의 WB/DN 변경은 이 인계에 포함하지 않는다.
