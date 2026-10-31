# 비로컬 의식 보상 연결과 누락된 5000 SP 복원 — 단위642

## 수정 범위

실제 Session의 `normalizeForeignSp`와 `grantForeignType`을 원본 PATCH10.78의 보상 경로에 연결했다. AI 등 비로컬 owner의 너겟 획득은 현재 잔액에5000을 더해 정수화한 뒤 정책 setter로 지정한다. 일반 기술 획득은 Client가 소유한 별도9×16BYTE Core 장부의 기술 비트를 켠다. Player contained 기술 목록이나 AI 지갑을 대신 수정하지 않는다.

기존 `MakeDaisRegularHooks`의 직접 분산 SP 쓰기는5000 가산과 known/Editor/표시 정책이 빠져 있었다. 이 대체 연결을 제거하고 실제 getter/setter를 호출자가 공급하도록 했다. 실제 Session은 같은 분산 SP와 현재 known/owner/Editor를 조회한다. `noIsland`의 전체 랭크 상승과 MP 상태/사망 안내는 잔여로 유지한다.

## 디컴파일 누락과 기계어 근거

00448260의 비로컬 너겟 분기는004482b1 getter→004482b6 `fadd dword ptr [00507548]`→004482bc `_ftol`→004482c3 setter다.00507548의 실제4BYTE는 `00 40 9c 45`, float5000이다. Ghidra의 해당 C 출력은 이 덧셈을 생략했다.599 문서와 기존 직접 쓰기 helper의 ‘절삭만’ 설명을 이 관측으로 정정했다.

원본 CRT004e3e1b `__fpmath`는004e81da `__setdefaultprecision`을 호출하며 `__controlfp(0x10000,0x30000)`으로53비트 정밀도를 선택한다. 제품은 double 덧셈 뒤 원문 masked `_ftol`의 signed64 절삭과 EAX 하위DWORD를 보존한다. signed32로 먼저 범위를 제한하지 않으며 NaN/무한대/64비트 범위 밖의 불확정 변환은 하위DWORD0이다.

0048f380은 먼저0049a840으로 TYPE을 읽고+1ec가-1이면 반환한다. 그 밖에는00595360+owner×16+4를 this로0042b280(bit,1)을 호출한다. 기술 DWORD는 이 Core+6, 즉 기록+10이다. 비트 범위 진단15a가 정상 복귀하면 원문대로 `bit&31`을 사용한다. owner의32비트 주소 감김을 보존하고, 명시 물리뷰 밖의 단일 DWORD 접근만 host 계약 오류로 거부한다.

마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이며 현재 PC와 같다. 이번에는 디컴파일 산출물을 변경하지 않았다.

## 독립 관측과 집중 검사

제품 helper를 작성하기 전에 `tools/decomp_foreign_ritual_reward.py`로 보호 PE를 격리 CPU 메모리에 읽었다. 실제 부모/정수변환/Core 쓰기를 실행하고 TYPE getter·분산 SP Get/Set·표시·진단의 효과 경계만 관측했다. 게임/Windows 창/포획/희생 장면이나 통신은 실행하지 않았다.

- 독립 입력252개×x87 두 모드=504관측을 최초 TSV로 고정했다.
- Core144관측은 전체 인접256BYTE와 TYPE/진단 효과를 제품과 비교한다. 정상/비정상 비트, 진단 복귀, 기록 padding, owner32비트 감김을 포함한다.
- 기본53비트 SP180관측은 실제 분산 저장소와 getter/setter 정책을 사용해 모든9명의 최종 원시 SP를 비교한다. 분수/음수/signed32·64 경계/NaN·무한대/known/Editor를 포함한다.
- 확장64비트 SP180관측은 원시 감사로 별도 보존한다. 기본 정밀도와 출력이 다른 입력12개가 있으므로504개 모두를 제품 통과로 세지 않는다.
- 별도 집중 검사는 비활성·잘못된 owner의 lazy 미접근, 활성 로컬의 두 번 현재 owner 읽기, 진단 중 장부 변경, Client 실제 장부 주소, TYPE 비트-1의 생략, 짧은 물리의 store 거부를 확인한다.

최초 빌드는 새 검사 소스를 target 생성보다 앞에서 추가한 CMake 오류로 중단했다. 등록을 기존 target 생성 뒤로 이동했으며 최초 실패 로그도 로컬에 보존한다. 최종 Release 빌드와 집중 고유23개가 통과했고 컴파일/최종 증분 빌드 모두 경고0·오류0이다. 부분문자열 필터 중복으로 Session3개를 한 번 더 실행했으므로 실행 횟수는26회이고 고유 검사는23개다. 중복을 확인한 뒤 추가 재실행은 하지 않았다.

등록 검사 수는3559개이며 전체 실행 통과로 세지 않는다. 기존 보호2629개/fixture770개와 최초 Native 입력/기대값 해시가 모두 일치했다. 새 fixture1개를 더한 다음 보호 기준은2630개/fixture771개다. 현재 EXE SHA-256은 `6b8fa3fe7bbf62d9cada5a184f03f7babcff2bc4a8ca4eb6ced0d365d54f23b9`다. ZIP은637의 기존 묶음을 유지하며642 새 배포 묶음은 기능 통합 뒤 생성한다.

수치와 검사별 이름은 [현재 보고서](../../cpppj/foreign-ritual-reward-20261031.json)에 있고, 로컬 원시는 `extracted/cpp-foreign-ritual-reward-642-20261031/`, 최초 CPU 보고서는 `extracted/cpp-foreign-ritual-reward-20261031-native.json`, 빌드 로그는 `extracted/cpp-642-build-v1.log`·`v2.log`·`v3.log`에 보존한다.

## 검증 범위와 다음 작업

실제 게임의 사제 포획/희생이나 시나리오 승리 재현은 사용자 지시대로 생략한다. 단위641의 전체34 진입은637 제품의 근거이며 이번642 제품의 전체 실행 통과로 확대하지 않는다. 변경 없는 전체34를 다시 실행하지 않고 기능 묶음 뒤 최종 제품에서 통합한다. 신규 MP 단위/실행/통신 검사도 하지 않았다.

남은 비미션 랭크 상승은0044fa00의 Core rank·기술 선택·내용물 삭제·NewRank/Tell·저장·화면 수명을 전체 부모로 연결해야 한다. 단순 랭크 가산으로 대체하지 않는다.
