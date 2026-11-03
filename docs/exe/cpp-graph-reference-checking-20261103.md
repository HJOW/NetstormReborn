# Graph 참조 감소와 SID 0 조회의 실제 진단 복귀

단위 815는 Graph 부모5개 중 `00462a50`과 `00462bc0`을 현재 제품까지 연결했다. sentinel 검사 `00462ac0`, flood `00462f10`, 병합 `004633c0`은 남는다. 전체 Graph 진단 완료로 확대하지 않는다.

참조 감소의 10.78 원문은 현재 checking이 비영이면 inUse(0x59), 다시 현재 checking이 비영이고 표면 수가 양수가 아니면 numSurfaces > 0(0x5a)을 진단한다. 돌아온 뒤 같은6BYTE 행의 현재 inUse/표면 수를 읽어 양수에서만 감소하고0이 되면 앞 두 WORD를 지운다. reserved는 보존한다. 새 `RemoveGraphSurface`가 이를 복원하고 값형 Graph.Remove도 같은 몸체를 쓴다. CD `0045b530`은 진단 없이 같은 감소를 하며 마지막 빈 helper/해제는 기록 쓰기로 보존한다.

실제 소비자는 `00462d40`의 영역 통지다. 기존 RawGraph는 진단 없이 계획 사본을 만든 뒤 Commit했다. 이제 checking 비영인 실제 호출은 기존 일반 finder의 순서로 장기 행을 직접 넘기고 진단 복귀 뒤 해당 슬롯의 graph BYTE를 무효화한다. 다음 후보 SID는 원문처럼 먼저 보관하고, 이후 후보의 현재 graph 번호와 현재 행을 다시 읽는다. 콜백이 고친 기록을 오래된 계획으로 덮지 않는다. PostPop도 같은 실제 영역 통지→Add 순서다. checking0의 기존 계획 경로와 Validate/Calculate 사전 계산 본문은 그대로이며 사전 검사에서 진단을 실행하지 않는다. 기존 SID/type/좌표/발자국/순환/할당 슬롯 guard는 유지한다.

SID0 조회는 10.78에서 checking 비영이면 mySid(0x93)를 진단한 뒤0을 반환한다. 기존 throw-only 경계를 실제 서비스로 바꿨다. CD `0045b6f0`에는 이 진단이 없다. 비영 SID의 기존 TYPE/좌표/Graph 몸체는 불변이다. 실제 일반 앵커, 지형 생성용 고정 파이프라인, 매 요청 현재 앵커 상태를 복사하는 gameplay 파이프라인을 모두 같은 현재 Session 서비스에 연결했다. 첫 Session 이전 World의 참조 감소도 장기 Storage의 현재 Client 진단을 빌린다.

새 독립 원문 기계어 **760입력(참조720·SID0 40)**을 두 PE에서 관측하고 읽기 재검증760회가 바이트까지 일치했다. signed WORD 경계·inUse0/1/-1·checking0/1/상위bit/전체비트·진단 중 checking 해제/현재 행/표면 수/활성 변경을 포함한다. C++은 전체6BYTE 행, checking DWORD, 진단 식/파일/행 순서, SID0 반환을 대조했다. 기대값은 C++로 계산하지 않았다. 원문 범위/진단 UI의 외부 경계는 [Native 보고서](../../cpppj/graph-reference-checking-native-20261103.json)에 있다.

집중 **7개(새4개), 최종 실패0**이다. 현재 영역 소비자 검사는 진단 중 행 복구·checking 해제·다음 후보의 graph 번호 변경, Validate의 무효과와 선택 장치 누락의 첫 쓰기 전 중단을 확인했다. 기존 원문 영역1,536연산을 checking0과 켠 실제 경로 양쪽에서 재생해 전체 슬롯/Graph 표/스택/공간/장부를 비교했다. 켠 경로에서 진단18회가 복귀한 뒤에도 기존 기대값과 같았다. 기존 앵커는 패치/CD 각1,598행도 통과했다.

최초 새 검사3개는 준비 풀128이 원본 고정 분할보다 작아 SID 풀 크기 오류로 중단했다. 제품/기대 fixture는 바꾸지 않고 검사 입력만 기존 배치인32,768슬롯으로 교정했고 최초 실패 로그를 보존했다. Release 빌드3회(제품/집중, 마지막 라이브러리로 제품 재링크, 검사 교정 재빌드)는 모두 종료0·경고/오류0이다. C++ 최종7개는 교정 뒤 같은 집중 실행 파일로 재확인했다. 기존 혼합 줄바꿈과 Native 바이트를 보존했고 생성기/Native 보고서 LF와 새 fixture -text를 Git 속성에 명시했다.

실제 게임/GUI·포획/희생·공격/승리·공중3종 전용·MP/통신·물리/PCM·전체 CTest/71대상·34경로·ZIP은 실행하지 않았다. 전체 메뉴 상태/수명이나 동일성 완료를 뜻하지 않는다. checking 잔여는 **27 = 부모19 + 부분5 + MP3**이다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 현재 호스트와 같고 산출물은 변경하지 않았다. 기존 보호2,739개/fixture919개가 모두 불변이다. 새 fixture1개를 더한 다음 기준은2,740개/920개, `extracted/cpp-resume-815/protected-after.json`이다. EXE SHA-256은 `4570e7dcfba9a0cd58e9d4f890995393e58d200a7a60688381beb7ffcbc9803f`다. 원시 발췌/빌드/검사/정적 감사는 `extracted/cpp-resume-815/`, 커밋 근거는 [JSON 보고서](../../cpppj/graph-reference-checking-20261103.json)다. 전체 목표는 계속 진행하며 다음은 남은 Graph3부모와 실제 소비자다.
