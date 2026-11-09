# 전체 MayPlace의 장기 소유 연결 — 1202

실제 Client의 전체 게임 배치 판정을 `ClientConstructionGlobals`로 옮겼다. `RawCanonPlacementPipeline`은 같은 장기 Pool·공간 hash·TYPE·Canon·도형·hotspot·장부와 현재 finder를 직접 읽는다. 기하·픽셀·충돌·지형·주변·관계·권한·기준점의 기존 본문을 사용하며, Session의 MayPlace 콜백을 빌리지 않는다. 이 연결은 다음 실제 너겟 배치의 World 없는 실행에 필요한 선행 작업이다.

게임 배치 상태·기준점·추가 SID 목록도 같은 Client 수명에 보유한다. Native Session의 커서·요청·기준점 조회는 이 객체들을 참조하고, 독립 typed Session은 기존 지역 본체와 상태를 유지한다. 지형 생성 전용의 별도 파이프라인은 기존 계약을 유지한다.

요청 직전에는 현재 Player 번호·방향 동맹표·editor·Graph 상태·서버 상태·밀어내기 genus·현재 패턴 TYPE와 같은 섬 기록의 제한/존재를 읽는다. Player owner만 보유하면 판정할 수 있으며, Player 9개 기록이나 건설 본체·World를 새로 만들지 않는다. 원본 PE의 `00540414`와 `00540cb0` 초기 DWORD가 각각1인 것도 확인했다. 이 초기값은 장기 결합 때 한 번만 설치하고 재결합에서 상태를 초기화하지 않는다.

Native Graph 기준점 진단은 Session을 캡처하던 서비스 대신 현재 Storage의 실제 Client 진단을 사용한다. 파일·행 정보와 진단 후 복귀를 보존하며, 중앙 assertion 통합은 계속 M7.5에 남는다. 장기 본체는 상태·backing보다 먼저 반납하고, 추가적인 Session 강한 역참조를 만들지 않는다.

| 항목 | 결과 |
| --- | --- |
| 장기 연결 | Native 요청과 Session의 게임 MayPlace가 같은 전체 본체·현재 상태·기준점·추가 목록을 사용한다. |
| 남은 대여 | 건설 큰 효과는17→16개, 별도 실제 너겟 배치 대여1개는 유지한다. 선택 시 weak 대여→실제 Runtime pin→함수 사본→호출이다. |
| 원문 근거 | Patch `0049b510/00462bc0/00462cb0/0048fdb0`, CD 대응4개 총8함수의 C와 전체 정밀 범위 PE, 초기 DWORD2개를 읽었다. |
| 집중 검사 | 비MP **117개 통과**. 새 owner 검사3개와 기존 MayPlace 양판본/보조판본 fixture3개를 포함한다. |
| 실제 실행 | Tutorial4 정상 생산·건설 요청→실제 TYPE75/owner1→600프레임 대기, 같은 Client800×600 Edit TEST01→메인 왕복2회, 모두 종료0이다. |
| 보호 | 원본/CD/fixture와 AGENTS.md **2800파일 불변**. 정밀 디컴파일 소스는 수정하지 않았다. |

새 검사3개는 World/Player 할당 전 실제 전체 판정과 Runtime 반납 뒤 동작, 현재 전역/동맹/TYPE 재조회, 재결합 시 객체·상태 정체성과 순환 없는 반납을 확인한다. 기존 Runtime pin 검사는 이미 장기로 옮긴 MayPlace 대신 남아 있는 실제 밀어내기 효과를 선택하도록 입력을 바꿨다. 원래 예외·호출 수·대여 수명·Player 미할당·패킷 미진입 조건을 유지했다. 검사 전용의 명시 강제 배치 입력이며 실제 게임 프로필/기본값은 바꾸지 않았다.

빌드는 Release/x64 제품2개와 집중5대상,4병렬이며 경고·오류 없이 종료했다. 빌드 중·이후 고정 소스7개는 불변이다. 제품 SHA·실행 결과·정적 대조와 원시 위치는 [검증 보고서](../../cpppj/placement-owner-20261109.json)에 있다. 전체34경로·전체 솔루션 실행·최종 ZIP 재생성은 이번 단위에서 반복하지 않았다. 신규 MP/패킷/통신 검사, 공격·승리·포획/희생·공중3종·Test Battle 검사는 실행하지 않았다.

**다음:** 실제 `PlaceBattleNuggets`와 난수 배치/완료·오벨리스크 생성/PostPop은 아직 Session에 있다. 실제 너겟 배치 전체를 장기로 연결하고, 전용26개 Pre/Post 부모·6개 통지·다른 Form/Regular·큰 AI·M6 전수 Caller/owner 감사를 계속한다. 기존57부모 연결 수는 바뀌지 않았고 전체 NoWorld 완료 수도 아니다. M6 및 후속 M7/M8/K/S/B는 미완료다.

마지막 정밀 디컴파일은 Client 양판본2026-11-03·R전용2026-11-07, 모두 **HJOW-Athlon**이며 현재 호스트와 같다. 현행 우선순위와 검사 제외는 [LEFT_JOBS.md](../../LEFT_JOBS.md)가 기준이다.
