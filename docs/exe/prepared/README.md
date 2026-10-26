# 적용 전 코드 준비본 보관

## 2026-10-27 일시중지 인계

현재 단위468 제품 수정과 별개로 아래 준비본을 원시 바이트 그대로 보관했다. **네 구현 묶음은 제품 미적용·C++ 미빌드·검사 초안 미실행**이다. Native PE 관측·재검증과 정적 patch 확인은 각 README에 적힌 범위만 완료했다. Early3는 미완주 기록이며 뒤의 입력 계획은 미실행이다.

- [NewFort 세 부모](new-fort-469-20261027/README.md): Native 1,400관측, C++ 검사 초안4개. Core10BYTE clear와 Money 저장의 기존 문서 해석도 정정했다.
- [Subscriber 저장](subscriber-write-469-20261027/README.md): Native90관측, C++ 검사 초안2개. 재등록 이후 현재 manager와 캡처한 ID/이름 수명을 보존한다.
- [공유 이름표와 빈 Image 초기화](data-manager-initialize-469-20261027/work/README.md): Native76관측, C++ 검사 초안6개. 512BYTE 최초 할당·277BYTE 작성·공유 표의 제자리 변환과 기존 ReadFile 접합을 준비했다.
- [Add Island 접합](add-island-469-20261027/README.md): 새 Drag40관측과 Population118 재검증, C++ 검사 초안6개. MetaImage·Bitmap·목록/창 수명은 필수 미완성 경계다.
- [Early3 부분 진행](tutorial3-partial-468-20261027/ATTEMPT4-NEXT-C-H.md): 실제 다리·골렘 채취/귀환 후 C까지 관찰했다. attempt4는 종료 프레임 뒤 남은 명령 때문에 return1이며 실행 통과/완주로 합산하지 않는다. `unexecuted/`의 다음 계획은 실행하지 않았다.

이 다섯 보관본은142파일/32,009,649BYTE다. `archive-files.json`과 [보관 검증](archive-verification-469-20261027.json)에 전체 바이트 SHA를 기록했다. 다른 체크아웃에서는 다음 명령으로 먼저 검증한다.

```text
python docs/exe/prepared/restore-469-20261027.py
```

`--restore`는 기존 다섯 `extracted/` 준비 디렉터리가 모두 없을 때만 원래 이름으로 복사한다. 현재 PC의 준비본을 덮어쓰거나 삭제하지 않는다. 복원 뒤에도 최신 제품과 base/patch를 대조하고, 다른 PC라면 AGENTS.md에 따라 정밀 디컴파일을 다시 수행한다. Subscriber와 빈 Image patch는 같은 Storage를 건드리므로 hunk별로 조합한다. 보관 검증은 C++ 실행·제품 채택 검증을 대신하지 않는다.

**2026-10-26 후속:** 443·444 준비본을 현재 제품에 삼방향 병합하고 Release 빌드·집중422개·전체CTest3,027개·메뉴 GUI·자연Early2→Tutorial3 검증을 완료했다. [통합 결과와 남은 경계](../cpp-cursor-retirement-runtime-20261026.md)를 본다. 아래 문장과 개별 보관본의 미적용·미검증 표시는 보관 당시 상태이며, 원시 base·patch·manifest는 그대로 유지한다.

사용자의 2026-10-25 일시정지 요청에 따라 서브 AI가 이미 수행 중이던 작업의 재개 자료를 보관한다. 이 폴더의 패치는 **제품에 미적용이며 C++ 빌드·검사를 실행하지 않았다.** 현재 제품의 통과 수에 포함하지 않는다.

각 묶음의 `archive-files.json`은 필요한 원시 입력base, patch, manifest, 정적 근거의 바이트 수와 SHA를 기록한다. 기록된 원시 바이트를 유지하며, 새 작업 디렉터리에서 base를 복원한 뒤 patch로 준비 결과를 재현한다. 실제 main은 이후 변경될 수 있으므로 현재 소스에 무조건 덮어쓰지 않고 기준판과 hunk를 대조해 병합한다. `main-before`의 C++ 파일은 재현 입력이며 제품 빌드 대상이 아니다.

기존base에CRLF가 남고 patch는LF인 경우 적용 직후 혼합 줄바꿈이 생길 수 있다. 이때 본문을 바꾸지 않고 파일별LF/CRLF 정규화 중 manifest의 준비SHA와 정확히 같은 바이트를 선택한다. 444 보관본은 이 절차로10파일의 최종SHA를 모두 재현했다. `archive-verification.json`에 최종 결과와 줄바꿈 선택을 기록했으며, 정규화 전6파일 불일치는 `archive-verification-attempt1.json`에 남겼다. 이는 패치 재현 검사이며 C++ 빌드나 실행 검사가 아니다.

- [443 커서 장기 상태 준비본](cursor-owner-443-20261025/README.md): 입력28/결과32, 실제patch30파일·신규8검사 정의다. 미리보기30DWORD, Lifecycle/Placement/snap 데이터와 실제Cancel 연결을 준비했다. 기준은815fbd67·439440 준비본·별도439 Load setup 교정본의 명시 혼합이며442나 현재제품95파일을 암묵 채택하지 않았다. Git 보관48파일/2,172,701BYTE와 추가검증JSON을 보관한다. 보관본의 `python replay.py <존재하지 않는 출력 경로>` 실행으로 입력28→결과32 정규화SHA와 세Git명령 종료0을 재확인했다. Native State1 전체 부모와 단일Drop.preview의 기존Sid16 경계는 후속이다.
- [444 AI·Player 링크 반납 준비본](state-runtime-retirement-444-20261025/README.md): 제품7/검사3파일, 신규12검사 초안이다. 실제 자식 정상 복귀의 이동전용 증빙을 준비했다. Session.h/.cpp 입력은442 준비본이며 나머지8입력은 당시815fbd67이다. 전체 Native coordinator·UI/Player 고정뷰 반납은 미접합이다. 독립 추가 검토도 미착수이며 다음 재개 때 우선 검토·병합·빌드한다.

원래 ignored 준비 폴더는 상세 원시 증거를 포함해 그대로 유지한다. 이 Git 보관본은 다른 체크아웃에서도 재현에 필요한 최소 자료를 남기기 위한 것이다.

패치의 빈 문맥 줄과 정밀C 증거에는 원시 공백이 포함되어 있다. 이를 제거하면 patch 또는 기록SHA가 달라지므로 그대로 보존한다. 새 인수인계·안내문은 일반공백검사를 적용하고, 원시보관파일은 기록SHA와 재현결과를 기준으로 검증한다.
