# 실제 Dude 제거와 재등록

## 현재 범위

- 주 대상은 패치 **10.78**, 비교 대상은 CD **10.72**다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다.
- 정상 실행은 **48/assert0**이며 실제 고유 입력은 **36(각18)**다. Unpop-only의 목적지 label만 다른12반복은 PE에 공급되지 않아 새고유로 세지 않는다. 실제 Birth/생성/owner/I최초등록→실제 Unpop→선택 실제 Pop(flags50)을 실행하며 **외부대체0**이다. 앞 Spawn288/Drop308/Notice414는 새 원본 수에 다시 더하지 않는다.
- 양판본 각각 실제 Birth/Unpop24와 실제 재등록12를 실행했다. 제거시점 전체 raw를 먼저 저장하고 최종 raw/전체hash/spot/population/표시/장부/깊이를 별도로 비교한다.
- C++ 새2검사는 저장48행을 모두 같은 실제 Factory/Owner/Frame/Pop/PostPop/Unpop/보호SHP/변경표에 재생한다. **공유 빌드와 C++ 전수는 아직 미수행**이며 공유 Unpop 수정은0이다.

| 역할 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 실제 생성 | `00450a20` | `004e8b30` |
| 실제 Unpop | `004afe50` | `004ad0b0` |
| 실제 Pop | `004b02d0` | `004ad490` |

원본 격자는 최초 위치20.75/21.9 또는50.5/50.5, 목적지0.5/0.5 또는25.125/24.75, owner0/1/8, 제거만/실제재등록을 교차한다. 대상은 실제 생성된 Dude6이며 population capacity3/count0과 보호65클러스터/130물리프레임을 사용한다.

## 실제 분기와 초안 경계 교정

실제 Dude의 TYPE **flags2=02000400**, **flags1=28040010**이다. 두 PE의 building 마스크 `005424b8/CD005395cc`는 **50444200**이며 spot 마스크는 여기에400ff를 OR한 **504442ff**다. Dude의 genus 교집합과 surface800 교집합은 모두0이다. 따라서 원본과 현재 C++는 **writesSpots=false**로 해시/상태/표시 제거·재등록을 실행하며 발자국 크기 guard/spot 순회에는 들어가지 않는다.

최초 초안의 `pending_shared_foot0=true`는 그 guard를 Dude가 실행한다고 잘못 판단한 경계였다. 수정 전에 report/tool/fixture/test/support/관련 비보호출처 **50파일 전체**를 `recovery-history/dude-space-before-predicate-scope`에 보존했다. 당시48보고서/원문/SHA는 그대로 두고 `analyze_dude_space_predicate_scope.py`와 새 successor가 **`requires_shared_unpop_change=false`**인 실제 현재 경계를 기록한다. 두 PE의 실제 마스크 바이트와 현재 입력의 교집합을 직접 감사하며 새native0이다. 공유 Unpop에 추가 변경을 넣지 않는다.

초기 분기 successor와 잘못된 고유수 문서를 포함한 전체8자료도 `recovery-history/dude-space-predicate-before-effective-count`에 보존했다. 현재 schema2는 각 판본24정상 실행 중 Unpop-only6고유/재등록12고유/중복6을 입력별로 확인하며 중복행 전체 출력이 동일한지도 검사한다. fixture48/native report/tool/Source/test는 불변이고 다시 실행하거나 고유 관찰을 추가하지 않았다.

이48과 앞 Spawn/Drop 성공은 일반 **0크기 spot 분기를 실행했다는 근거가 아니다**. 공통 Pop의0크기 허용 자체는 그 분기를 실행하는 별도 입력으로 대조한다. 실제 DudePath24/연속 이동·전체 삭제 부모/Host 수명/캠페인1-1 월드는 여전히 후속이다.

이번 `repop=1`은 실제 Unpop 뒤 실제 Pop/Activate/파생 후처리 호출이다. 최종 공통 깊이는0으로 복원된다. 앞 Spawn288의 `repeat=1`은 flags50 후처리 몸체를 직접 호출해 depth0→ffffffff를 관찰한 입력이므로 이 실제 재등록과 구분한다.

## 자료와 등록

- `cpppj/tests/DudeSpaceTests.cpp` 양판본2검사와 새 전용 `DudeSpaceSupport.h`.
- fixture `dude-space-x86.tsv`, 매크로 **`NETSTORM_DUDESPACE_FIXTURE`**. 기존 Source/공유 처리기를 재사용하므로 새 production 파일은 없다.
- native `tools/decomp_dude_space_oracle.py`/`recovery-dude-space-evidence.json`: 최초48의 당시 SHA/초안 경계다.
- 현재 successor `tools/analyze_dude_space_predicate_scope.py`/`recovery-dude-space-predicate-scope-evidence.json`: 실제 분기/소비자/공유 변경0의 현재 경계다.

```text
python -X utf8 tools/decomp_dude_space_oracle.py --verify
python -X utf8 tools/analyze_dude_space_predicate_scope.py --verify
```

두 감사는 정상48·원문 보존과 실제 predicate를 확인했다. 최초 감사 출력의 pending 값은 당시 초안이며 현재 successor의 false가 실제 연결 범위다. 단위14의 현재 공유 C++ 양판본2검사는 모두 통과했다(`campaign-runtime14-DudeSpace_initial-tests.log`). 저장48행/고유36/반복12 및 실제 writesSpots=false를 유지하며 새로운 Player 공유 분배의 source SHA는 `analyze_dude_space_player_successor.py`가 당시 전체 readview로 연결한다. 단위14 전체 검사는 아직 진행 중이다.
