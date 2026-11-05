# Form 공통 Unpop 현재 진단·VT·World 종속 분리 (단위908)

현재 공통 Form 해제는 진단이 정상 복귀한 뒤의 TYPE/상태와 실제 VT를 읽고, void를 쓴 뒤 부모 free 관문을 통과한 경우에만 같은 종속 분리를 실행한다. 현재 World 공장, Process/Gump/Depend 및 실제 Session의 ContentForm이 이 몸체로 이어진다. 전체 Factory/Pop 복원은 진행 중이다.

## 근거와 범위

- 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-11-03이며 현재 작업 호스트도 HJOW-Athlon이다. 정밀 산출물은 변경하지 않았다.
- PATCH `004ae140..004ae1d3`, CD `004af270..004af309`의 실제 PE 명령을 확인했다. 네 Form 생성 VT의 `+48`은 각 판본에서 같은 진입점을 가리킨다.
- CD 진입점은 현재 정밀 함수 목록에 없으므로 보호 PE에서 명령 경계와 `ret 4`까지 별도로 확인했다. 범위를 확인한 순수 함수만 격리 실행했으며 게임 진입점·OS·소켓은 실행하지 않았다.
- PATCH의 세 진단은 `isForm()/isValid()/!isVoid()`의 `a59/a5a/a5b`, CD는 `a10/a11/a12`다. CD 원문 파일 경로의 소문자 `\ns\o`도 보존한다.
- CD는 void OR 직전에 VT를 저장하며 PATCH는 직후 읽는다. 두 판본 모두 현재 VT+0의 부모 getter를 호출한 뒤 그 부모의 현재 free 비트를 확인한다.
- 부모가 free이면 void만 남기고 분리하지 않는다. 그 외에는 기존 `RawSquidContainedUnpop::Detach`를 사용한다. 부모 dead의 링크 보존, 저장 next/현재 previous, contained 해제 뒤 현재 ContentForm 번호 비교/부모 알림은 같은 공통 자식의 책임이다. Form 해제에 자신의 UI finder를 추가하지 않는다.

## 실제 연결

`SquidFactory::Take`의 네 Form VT+48은 현재 `UnpopForm`을 사용한다. 실제 World는 Session의 약한 대여가 살아 있으면 그 종속 분리를 pin하고, 반납되면 같은 World의 기본 종단을 사용한다. 실제 Gump/Process/Depend와 Session ContentForm도 같은 공장으로 연결했다. 장기 Process Host에는 World 종단을 캐시하지 않고 현재 Runtime 대여를 사용한다.

독립 `SquidForms`도 같은 원문 접두와 공통 종속 분리를 사용한다. 독립 입력의 기본 ContentForm 번호는6이고 실제 World/Session의 현재 번호를 대신하는 전역으로 주장하지 않는다. 선택된 부모 알림이 없으면 앞선 void/링크 쓰기를 보존한 채 실패한다. 물리 풀 범위 밖 메모리·알 수 없는 getter VT는 호스트 경계다.

## 검증 기록

- 새 순수 PE 관측은 두 판본 각864개, 합계1,728입력이다. SID0/5·TYPE1/2/3/6/69/70·상태6종·부모 상태4종·정상/진단 변경/getter 변경3프로필의 호출 순서와 두 슬롯 전체를 동결했다. 실제 가상 getter/종속 자식·진단 UI를 실행한 관측이 아니다.
- 새4/기존46의 고유50사례를 최종 통과했다. 최초50실행에서3사례/4단언이 실패했고 교정 뒤 같은50사례를 통과했다. 합계32명령/100사례 실행이다. 최초 실패 원시는 삭제하거나 성공으로 바꾸지 않았다.
- 새2사례의 준비가 일반 Form 부착기로 ContentForm을 붙이려 했으므로, 부착 자체를 시험하지 않는 명시적인 정상 부착 바이트 입력으로 교정했다.
- 기존 `FormTake_ContentNoticeTimingAndPayloadMutation`의 알림 부재 시 무변경 단언1개는 원문 `004ae140→004ac6b0` 및 기존 `formtake-x86.tsv`의 부모 알림 시점 바이트와 충돌했다. 알림 호출 전에 void/contained·부모 head 쓰기가 이미 끝난 원문 계약으로 교정했다. 상태 BYTE만 바뀐 **전체 슬롯 바이트와 부모 head=0**을 함께 검증하고 다음 알림/payload 사례는 독립적인 정상 부착 입력에서 시작한다. 기존 Native fixture·다른 후속 단언을 제품 출력에 맞춰 바꾸지 않았다.
- 최초 PE 접두1,728행과 기존 Form Take Native600입력 대조는 첫 검사부터 통과했고 교정 뒤에도 같다. 순수 종속 분리10·Factory Take7·Form Take4·Process 생성4·Host 저장소11·Process 수명10과 새4가 최종50사례다. 신규 의식/공중3종/MP/수신/패킷/통신 전용 검사는 실행하지 않았다.
- Release2명령 모두 종료0·경고/오류0이다. 두 번째 빌드는 검사2파일만 교정했으며 제품 소스/EXE는 첫 빌드 이후 같았다. 최종2,871소스는 두 번째 빌드 전 snapshot과 같고 각 빌드 중 소스 편집/다른 빌드는 없었다.
- 보호 기존2,782개/기존 fixture962개는 불변이다. 최초 관측 fixture1개를 추가해 현행2,783개/fixture963개이며 새 파일은 최초 관측 바이트와 같다.
- 제품908의 표시·파일·제품 버전은11.01, 크기는10,758,144바이트, SHA-256은 `264799dd73b6a9404880454970503fe712f339d31460993cd4fcdb08ddc73b52`다.
- 원시는 `extracted/cpp-resume-908/`에 있으며 Git 제외·현재 PC 전용이다. 커밋되는 [JSON](../../cpppj/form-unpop-current-20261105.json)·관측 도구·fixture가 근거다. checking 잔여8, 활성74프로젝트 및 과거 전체/GUI/34미션/ZIP 기준은 그대로다. 집중 통과를 전체 회귀 통과로 합산하지 않는다.

## 남은 범위

CD 공간 Unpop `004ad0b0`, 공통 종속 분리의 CD 진단 파일 경로 대소문자, 고정 Form 편의 Create/Take·Depend 생성, 다른 Attach/Pop 진단과 Baseprocess 부모/할당 실패 정책은 남는다. 객체 UI·Edit·State/파일·Windows·무검사 MP 및4~8차도 전체 목표에 유지한다. checking 잔여8 집계는 이 하위 해제 작업으로 완료 처리하지 않는다.
