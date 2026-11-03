# Graph sentinel의 현재값 진단과 실제 Free 연결 — 단위816

`CheckGraphSentinel`은 10.78의 `00462ac0` 순서대로 `numSurfaces`를 검사하고, 첫 진단에서 돌아온 뒤 현재 checking DWORD와 `inUse`를 다시 읽는다. sentinel을 자동 복구하지 않는다. 실제 `RawGraph::Free`는 254이면 아무것도 읽지 않고 반환하며, 정상 번호이면 sentinel 진단 복귀 후 같은 현재 대상 행의 `inUse`와 `surfaces`만 지운다. 진단 장치가 바꾼 reserved와 다른 행·슬롯·스택은 보존한다.

CD판의 실제 대응은 **`0045b560`**이며 빈 함수다. 기존 인계의 `0045b520`도 빈 함수지만 참조 감소의 다른 하위 함수다. `0045c460`의 실제 호출을 따라 대응을 교정했다. 새 원문 실행기는 처음에 이 둘을 혼동해 CD의 미허용 `0045b560`에서 중단했다. 결과 파일을 만들기 전 함수 대응만 고쳤으며, 제품/기대값을 약화하지 않았다. 실패 예외와 교정 요약은 `extracted/cpp-resume-816/native-first-failure.txt`에 보관한다.

원문은 마지막 전체 정밀 디컴파일 **HJOW-Athlon / 2026-11-03**의 양판본 `Netstorm.c`와 `functions.tsv`를 읽었다. 기존 원문/실행 파일은 수정하지 않았다. 격리 기계어 실행은 OS·게임·GUI·소켓을 호출하지 않으며 진단 UI만 명시 반환 장치로 대체했다.

새 fixture는 양판본 × Check/Free/254반환 × sentinel 각 WORD 3값 × checking 4값 × 진단 변경 5종의 **1,080행**이다. 첫 진단에서 checking 끄기, 두 번째 WORD 복구/손상, Free 대상 reserved 변경을 포함한다. 독립 PE 출력 전체 행과 실제 C++ 공유 저장소 경로를 대조한다.

이 단위의 실제 연결 범위는 **Free와 공통 sentinel 처리기**다. Flood/Add/Rebuild/Detach의 중첩 호출·현재 풀/지도/스택·진단/로그 복귀는 다음 단위에서 연결한다. 따라서 checking 잔여 27함수를 이 단위에서 줄이지 않는다. Validate/계산 경로를 진단 효과가 있는 실행으로 바꾸지 않았다. 공격·포획·희생·승리·공중3종 전용·MP·물리/PCM 검사는 실행하지 않는다.

Release 집중 대상과 제품 빌드2회가 종료0·경고/오류0으로 완료됐다. 새 sentinel2개와 기존 Graph 참조 감소4개, 합계6개가 실패0이다. 기존 원문760행과 영역1,536연산도 재대조했다. 게임/GUI는 실행하지 않았다. 기존 보호2,740개/fixture920개 불변을 확인했고 새 fixture를 포함한 다음 기준은2,741개/921개다. 제품816/11.01 EXE SHA-256은 `9262fffb715cee93d1b84081ba54a03360f5218ee111c9929f04ec9c9977e185`다. [보고서](../../cpppj/graph-sentinel-checking-20261103.json)를 따른다.
