# 군도 생성 상위와 현재 모드·플레이어 연결

단위894는 패치 `004c1510`과 CD `0049e620`의 실제 생성 상위를 `RawNormalTerrainWorldBinding::GenerateOriginal`로 복원하고, `RawCampaignWorldRuntime`과 `MakeFortWorldLoadBinding`의 실제 Session/Fort/편집 호출을 연결한다. **코드/정적 연결은 완료했으며 군도 MP 분기의 실행 검증은 사용자 지시로 제외했다. 전체 cpppj 목표는 미완료**다. 분석 호스트와 마지막 전체 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-11-03**으로 같다. 보호 원본/전체 정밀 산출물은 읽기만 했다.

후보 조건은 context constructor 전에 현재 활성 DWORD·전투·미션 월드·섬 개수1로 캐시한다. 패치는 constructor 뒤 현재 layout을 읽고, 군도일 때 크기 getter를 두 번 따로 호출해 첫 확장량과 두 번째 공유 span을 구한다. CD는 후보가 참이면 layout/크기 getter를 거치지 않고 현재 기존 span을 사용한다. 확장 SetBounds와 같은 context의 흩뿌리기→두 번째 첫 raw+5 시드×443→밀도 두 추출/강제값→전체 성장→현재 Player1~8의 subscriber/실제 배치 제거→한 번 빈틈/성긴 칸 제거를 원문 순서로 연결했다.

반열린 난수 범위, signed DWORD 곱셈 감김/0방향 나눗셈, 성장 전1d 백분율 로그, record의 정확한6BYTE `{7,1,0,0,0,0}`를 보존한다. subscriber는 각 반복에서 `ClientCoreDataGlobals::Players()`의 현재 base를 읽으며 실제 배치는 같은 `construction->PlayerBounds().Placement`다. 등록된 플레이어에서만 배치 장치를 요구한다. 군도 결과bb0은 도장/정규화 뒤1을 쓰고 일반 분기에서 임의0으로 지우지 않는다. context 해제 뒤 같은 실제 단일 배치/군집을 실행한다. 독립 일반 전용 Generate/PostprocessFarms의 거부 계약도 유지한다.

`RawGameAuxState`가 활성1·강제 밀도0·최소15/최대35를 장기 소유한다. 양판본 보호 PE의 초기 DWORD를 직접 읽어 확인했다. 군집과 너겟 보충은 간헐천과 같은00542654의 실제 참조를 읽고, 로그인 metadata의bb4 강제 밀도와 생성 결과bb0은 분리한다. Zone 보정도 같은 활성 DWORD를 `RawCheckingFlag`로 읽도록 연결했다. 새 인터넷/검증 서버 경로는 추가하지 않았다.

최초 Release 제품/집중 대상 빌드는 성공했다. 정적 원문 주소 대조에서 새 참조의00594fb8/00594fa0 구분을 교정했고, 실제 현재 미션 월드인 `core.bridgeState.mission`을 사용하도록 실행 검사 전에 맞췄다. 같은 독립 일반 Fort 입력도 현재 전역/상위를 명시하도록 바꿨고 기존 기대값은 유지했다. 이후 선택되지 않은 배치 장치를 요구하지 않도록 관문을 해당 반복 안으로 옮겼다. 세 제품/집중 빌드 모두 종료0·경고/오류0이며 최종 집중 **16사례/1명령/16실행·첫 검사 실패0**이다. 공유 부품/일반 지형/실제 일반 월드 바인딩과 군도 후보0인 기존 Fort 부모만 실행했다. 새 검사/Native/fixture는0이며 군도 MP root·새 패킷/통신 검사는0이다.

기존 보호2774개/fixture954개는 모두 변경0이다. 제품은 **894 / 파일·제품 버전11.01 / 10690560BYTE**, SHA-256은 `4b78a8fdeb57c1aa7d3787347c157ace787ae7b1866972c3a6be93897aa9e4a7`다. 초기/교정/선택장치 빌드·최초 검사·소스/원문 범위/보호 해시는 Git 제외 `extracted/cpp-resume-894/`에 보존했다. 커밋되는 증거는 [JSON](../../cpppj/terrain-generation-root-20261105.json)이다.

checking10=부모4/부분3/MP분류3과 완료부모38/완료부분3을 유지한다. 원문 할당NULL·전체SEH/진단UI·범위밖 메모리는 기존 호스트 경계다. 일반전투 State7 `004b6dd0`의 최소/최대 밀도 옵션 getter·게임 난수/강제 밀도 writer, 중앙섬 metadata와 전체 MP 부모는3절8번에 남겼다. 이 단위를 전체 MP/Pop 완료로 확대하지 않는다. 다음895는 공통 `0048fb30`의 현재 CoreData/캐시/Player·진단 정상 복귀 부모다. 이후 Factory/State/파일·객체UI/Edit·Windows·무검사MP/4~8차를 계속한다.

게임/GUI·포획/희생/특정 적 공격/승리·사용자 직접 확인·공중3종 전용·물리 입력/PCM/CD·전체71/CTest/34미션/ZIP 반복은 실행하지 않았다. push는 하지 않는다.
