# 건설 프로세스의 현재 Form 연결 진단

단위800은004428c0의 현재 checking·getForm().isRobust() 진단을 실제 ConstructionProcess/Host에 연결했다. checking은 같은 Client DWORD 뷰이고 독립 기본값은0이다. checking이 비영일 때만 현재 Form을 읽고 전체 연결을 검사한다. 실패 진단 Construction.cpp1c4가 복귀하면 checking을 다시 검사하지 않고 기존 HP 진행을 계속한다. 실제 Host의 첫 부모 조회도 진단 뒤에 수행한다. 기존 독립 parent 값형 입력과 CD의 무진단 경로는 유지한다.

004ae080 연결 검사는 가상 부모를 한 번 읽은 뒤 부모의 head WORD부터 next WORD를 끝까지 따라간다. 자기를 찾았어도 이후 번호가 범위 밖이면 false다. 타입/free/dead/void 필터는 추가하지 않는다. 순환은 원문이 정상 반환하지 않는 입력이므로 호스트에서 명시 예외로 중단한다. 낮은 층은 명시 물리 span/stride/논리 capacity를 받아 소형 원문 입력과 대조하고, 실제 풀 wrapper는 부모 callback이 끝난 뒤 현재 span을 새로 빌린다. callback 중 풀 재확보는 C++ 수명 검사로 구별했다.

첫 빌드는 성공했다. 이어 정적 검토에서 기존 SquidForms::Parent가 free/TYPE 관문을 추가하는 것을 확인해 진단 전용 ReadRawProcessFormParent를 분리했다. 실제 ProcessForm 가상 표의0번은 패치512098→4ad010의5BYTE `0fb74110c3`, CD505990→4b04a0의7BYTE `33c0668b4110c3`다. 둘 다 상태/TYPE를 읽지 않고+0x10 WORD만 unsigned로 반환한다. 정밀 C에 별도 함수 표제가 없어 PE 기계어를 직접 디코딩했으며 추출물은 수정하지 않았다. 진단 전용 getter는 알려진 두 가상 표만 연결하고 다른 가상 표를 임의 부모로 대체하지 않는다.

원문 도구는 ae080 전체의120입력/240관측과4428c0 진단 앞단의64입력/128관측을 만들었다. 총184입력/368관측이며 재관측368회가 최초 두 fixture와 일치했다. Form은 부모/자기/비영 체인과 자기 뒤의 범위 실패를, 건설 앞단은 checking0/1/상위비트/FFFFFFFF·현재 Form·연결 반환·진단 복귀/중단과 getter 중 현재값 변경을 포함한다. 건설 앞단의 getter/연결 반환/진단은 명시 장치이며 본문의 첫 부모 조회에서 중단한다. 전체 Form 쪽은 가상 부모 반환만 명시 장치다.

최종 게임/기존 파일 집중 대상 Release 빌드는 경고·오류0이다. 새5개와 기존13개, 총18개가 실패0이다. 새 검사는 원문120체인·64진단 접두, 현재 풀 재대여/순환, 양판본 getter의 전체 WORD/상태·TYPE 무관성, 실제 Kernel/ProcessForm 진단 복귀를 확인했다. 기존 세 판본의 각956행/총2868행과 건설 시작·진행·완료·Kill 수명 검사도 유지했다. 기존 검사의 Completion은 건물 한 개의 완성이다. HP/시각/표시/Kill 몸체는 소스에서도 불변 대조했다. 두 빌드는 성공했고 검사/Native 관측의 실패는 없다.

checking 잔여를43에서42(부모33·부분6·MP3)로 갱신했다. 새 게임/원본 GUI·특정 적 공격·포획/희생·시나리오 승리·공중 전용·MP/통신·물리/PCM·전체34/전체 C++/전체71대상·ZIP은0이다. 마지막 GUI는794의793 제품이다. 임의 virtual getter·32비트 ABI/해제 주소·원문 비반환 고리·전체 SEH/진단 UI는 별도 한계다. 기존 Kill 뒤의 둘째 Kill 회피와 Host 등록 검사도 유지했다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-10-23이며 현재 호스트가 같고 추출물 변경은 없다. 원시 관측/첫·최종 빌드/집중 검사/정적 leaf/동일 몸체 확인은 `extracted/cpp-resume-800/`에 있다. 최종 제품 SHA·보호 기준과 fixture 식별값은 [JSON 보고서](../../cpppj/construction-form-parent-20261103.json)에 기록한다.

최종 EXE SHA256은 `d153850b55dd0f48d34d6ec2f203890157aaa32f825e10936b31fe5df0b3a0bb`이다. 기존 보호2728개 불변·새fixture2 포함2730개/fixture910개이며 다음 기준은 `extracted/cpp-resume-800/protected-after.json`이다.
