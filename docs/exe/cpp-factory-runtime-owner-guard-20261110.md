# Factory의 실제 전체 Runtime owner 필수화 — 제품1250

1249 caller/owner 감사에서 FactoryCall이 pin 함수/반환 owner가 없어도 큰 효과를 호출하던 누락을 발견했다. [확정 보고서](../../cpppj/factory-runtime-owner-guard-20261110.json)·원시 `extracted/cpp-resume-1250/`를 따른다.

- 현재 Runtime weak lock 뒤 실제 pin 함수와 반환 owner를 모두 확인하고, 같은 전체 owner를 호출 끝까지 보유한 채 선택 함수 사본을 호출한다. 실제 세 caller는 업그레이드 시작/begin, 완료/finish, 상위 플레어/flare다. 기존 RunFactoryRegular 알고리즘은 변경하지 않았다.
- 제품 수정 전 새 회귀의 **6누락 입력/8단언 실패**를 보존했다. 세 경로 각각 pin 함수 부재/반환 owner 부재를 실행했고 owner 없는 콜백 호출과 시작의 후속 프레임 변경을 검출했다. 수정 뒤 콜백0·raw 불변·명시 거부를 확인했다.
- 기존 정상 플레어 관찰 검사에 실제 관찰 counter owner를 pin했다. 레벨2는 Runtime 없이 통과하고 레벨3은2/1 플레어1회라는 기대값을 유지했다. 이 준비 교정은 집중 실행 전 정적 검토에서 반영했다.
- 최종 Release/x64 제품2·집중1대상 **47.358초/종료0·경고/오류0**, 고정2파일 불변이다. **비MP288/288** 통과이며 기존 유효 owner/함수 사본의 실제 호출 수명 검사도 포함한다.
- 최종 EXE Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기·정상 종료0/PNG를 확인했다. 건물 완공·청취·원본 전체 동일성은 단정하지 않는다. MP·특정 적 공격·포획/희생·승리/자연 완주·공중3종 전용 실행은0이다.
- 보호 원본/CD/fixture·현행 AGENTS75e4eed9e **2,808파일 불변**이다. 새 ZIP/디컴파일/원문 변경0이며 Client 양판본2026-11-03/R2026-11-07의 HJOW-Athlon 기준을 유지한다. 글꼴 외부 fonts 계약의 M8·K1/K4/K5 확인도 유지한다.

알려진 Regular87/87은 유지하며 전체 NoWorld/M6/목표는 미완료다. 같은 선택적 pin 패턴을 RawSurfaceDecoration.cpp의 가상98, RawSurfaceDudeRegular.cpp의 큰 이동, RawSurfaceResidenceRegular.cpp의 Dude 생성/작업에서도 발견했다. 이 세 곳은 이번 단위에서 미수정이며 다음 회귀/수정 대상으로 남긴다. 전수 caller/owner·큰 후단/최초 후처리·Path/AI/배치/Fort Finish·M6-l/m과 나머지 상위 계획도 계속 남는다.
