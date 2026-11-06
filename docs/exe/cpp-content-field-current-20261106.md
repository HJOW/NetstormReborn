# Content 필드 조회와 현재 Regular 재부착 — 단위971

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 우선순위4를 계속하며 원본·정밀 산출물은 읽기만 했다.

## 원문과 실제 소비자

`ContentFormHost::Robust/Linked/Damage`는 공통 `Form()`에서 TYPE6/free/내장 Form 검사를 먼저 수행했다. Robust의 WORLD는 직접 예외로 끝냈고, Linked는 부모0을 거부하며 자체 체인을 다시 구현했다. 실제 Session Regular도 TYPE6과 VT를 함께 요구했다. 원문 몸체의 직접 읽기와 호출 순서에는 이 선행 조건이 없다.

- Robust는 PATCH004aea60/CD004afe20이다. flags의 WORLD가 켜지면 원문 식/파일/행(0xbb1/0xb68)을 전달하고 정상 복귀 뒤 현재 flags의0x34를 다시 읽는다. 장치가 자기 연결을 반납해도 선택한 callable을 복귀까지 보유한다.
- Linked는 PATCH004ae080/CD004af1c0이다. 현재 VT의 부모 getter를 한 번 호출하고 부모 head부터 전체 next를 끝까지 검사한다. 예약 부모0도 읽을 수 있으며 자기를 찾은 뒤 범위를 벗어나면 false다. 기존 `CheckRawFormLinked`를 재사용하고 알려진 현재 getter에는 `CommonContainedParent`를 연결했다. 순환은 기존 호스트의 명시 중단으로 남긴다.
- 이름이 Damage인 공개 API는 실제 Content Regular+5c, PATCH004af070/CD004b0350이다. count0/음수는 this를 읽지 않고 전달 float를 반환한다. 양수는 현재 getter로 부모를 저장→현재 가상 Unpop→현재 flags의0x100 해제(PATCH high BYTE/CD WORD)→현재 가상+50 재부착 후0을 반환한다. 실제 공격/HP 피해 함수가 아니다.

분리 콜백이 바꾼 현재 클래스에 맞는 재부착을 `BindReattach`로 대여한다. 실제 Session은 Content VT에는 정렬 부착, 나머지 세 내장 Form VT에는 같은 Factory 공통 부착/Pop 깊이를 사용한다. 독립 기존 Content 호출은 기본 정렬 입구를 유지하되 장치 없이 다른 클래스의+50을 정렬 부착으로 대체하지 않는다. 선택 실패 시 앞선 flags 쓰기/링크/void를 되돌리지 않는다. 실제 Session Regular의 TYPE6 관문도 현재 Content VT 선택으로 교정했다.

CD의 기존 제한 정렬 경로에서 타입6 전용 helper를 제거하고 previous getter/setter를 현재 VT의 공통 부모에 연결했다. CD 전체 정렬/진단/파일 이관 완료로 확대하지 않는다. Process 기본 PID0/별칭과 자산 TYPE 범위의 전체 삭제 부모는 다음 범위다.

현재 재부착의 부모 통지도 대조했다. 양판본 네 Form의 VT+24는 모두 PATCH0041c100/CD00401c20의 빈 ret8이다. 기존에는 PATCH Content 하나만 알려져 있었다. 여덟 현재 VT를 모두 연결해 TYPE/상태나 form 인자를 빈 몸체 앞에서 추가로 읽지 않는다. 정적 주소/명령 기록은 단위 보고서를 따른다.

## 수정 전 증거와 검사 준비

제품 수정 전에 새4검사를 실행했다. 부품3검사에서120단언 실패, 실제 Session1검사에서TYPE7/63 Regular가 미연결로 거부되어4단언 실패, 합계124다. 실제 Session TYPE6 경로는 이미 통과했다. 첫 검사 빌드는 필수 Put width 인수와 잘못 기억한 Reserve 이름 때문에 실패해 실제 width2/Reset API로 준비를 교정했다. 최초 제품 빌드의 공통 Previous 반환 Sid→WORD C2440은 종료 뒤 `.value`로 명시했다. 최초 로그와 메모는 `extracted/cpp-resume-971/`에 보존한다.

**최종 게임·두 집중 대상 Release와 추가 공통 체인 대상 모두 종료0·경고0·오류0**이다. 빌드 이력은 검사 준비1/교정0/첫 제품1/최종 제품0/집중 등록0으로 보존한다.

최종 **고유14검사(새6/기존8), 실패0**이다. TYPE/상태96입력, 진단 정상복귀4입력, 조기 float 반환24입력, 현재 클래스/부모 저장/부분 쓰기 재부착8입력, 빈 부모 통지64입력과 예약 부모0의2입력, 실제 Session TYPE6/7/63의 각 Regular count0→양수/프로세스 반납을 확인했다. 기존 PATCH Native 체인120행·현재 풀과 getter, 혼합4클래스 Kernel 수명·공유 Factory 깊이·dead 진단·계약과 현재 커서 preview 삭제도 통과했다.

처음 RawFormLinked 필터는 다른 대상에만 등록돼0검사/불일치1이었다. 작은 기존 검사 파일을 이 공통 체인을 재사용하는 pop 집중 대상에도 포함해3검사를 실행했다. CMake 새 실행 대상은 추가하지 않았으며 솔루션83은 유지된다.

제품971/11.01은 **11,360,768바이트**, SHA256 `07f7f27d4d8d973da1ac57b255a20355914645082e306291ec4faaee2e8a840d`이다. 보호2796개/fixture976개 불변·새fixture0이다. 다음 기준은 `extracted/cpp-resume-971/protected-final.json`이며 [단위 보고서](../../cpppj/content-field-current-20261106.json)를 따른다. checking잔여8은 불변이다. 새 Native/fixture·게임/GUI·MP/프로토콜/통신·공중3종 전용·실제 포획/희생/특정 적 공격/승리·전체CTest/34미션/ZIP은 실행하지 않는다. 마지막 GUI는968이며 전체 Factory·파일·우선순위4 완료가 아니다.
