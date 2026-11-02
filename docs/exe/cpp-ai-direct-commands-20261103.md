# 772 — 일반 간헐천과 AI 직접 명령

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위772 완료·전체 목표 미완료다.

원문 CreateInitialGeysers/BrainCrankMoney/AiParamSet의 이름/정수 명령을 같은 ScriptStringRing/FIFO/일반 입력에 연결했다. 기본 메뉴/별칭을 추가하지 않는다. 같은 실제 Session의 간헐천 전체부모, 현재 Player와 생존 AI 메모리를 사용하며 복제 지갑/임시World/가짜 성공을 만들지 않는다.

CrankMoney는 방문할 때마다 Player1..8의 현재b0를 읽고 생존brain+2d8에30000을 직접 쓴다. 기존 wallet Credit/일반SP는 바꾸지 않는다. AiParamSet은 low3 owner0이면 저장소를 읽기 전에 반환한다. bit4..11 selector와 signed high16 값을 쓰며 selector0은+220직전/+600직전 현재b0를 각각 재조회한다. selector1..4는 첫brain, 나머지는 현재Player/brain 확인 뒤 무동작이다. NULL재조회/없는본체/잘못된0xb08물리는 명시 실패하며 앞선 쓰기를 rollback하지 않는다.

동결 준비본의 AI 명령 press 직후 return은 원문의4d8bda 공통 꼬리를 잘랐다. 독립 통합 검토에서 이를 보완해 release만 즉시 반환하고 press는 현재Help/Console/정지/세계 입력을 이어간다. 기존770 createAtHandled와 새aiUtilityHandled를 늦은메뉴 dispatcher/미연결분류 두 곳에서 함께 검사한다. 새 dirty/정지/플레이어 관문은 추가하지 않는다. 기존CreateAt·DrawWater·현재 reader/Config/CMake 누적을 보존했다.

원문writer1940입력/3880관측, 일반입력72/144, 이름/실제고리/FIFO30과 root 독립 반복4054관측/3fixture가 바이트 단위로 일치했다. writer는 현재Player9·명시brain 물리의 일곱DWORD×9와 실제store순서를 실행했다. 일반입력은 Guide/Gump뒤 선택call/release와4d8bda 진입라벨에서 멈췄으므로 전체꼬리 상태효과의 실행 증거가 아니다. CreateInitialGeysers 하위전체는 기존 복원부모를 호출하는 경계다. 이 한계를 Native 전체UI/전략/가상객체/SEH 동등성으로 확대하지 않는다.

최종 Release 경고/오류0, 집중 **10개(새6개)/실패0**. 실제Runtime1은 기존CreateAi/현재메모리/AiMoney/RemoveAi/재생성을 사용해 SP·시간·할당수·패킷 불변과 같은 본체쓰기를 확인했다. 기존WorldGeyser/Bounds4개도 회귀했다. Kernel/AI전략프레임/실제생산·이동·공중3종·특정적공격·포획/희생/승패/MP는 새검사하지 않았다. 새실행대상0/기존66개·전체71대상 유지다.

기존 보호2698개 모두 불변·새fixture3 포함2701개/fixture881. 다음 기준은 extracted/cpp-resume-772/protected-after.json이며 제품 SHA-256은 `f2f42d346a4a5d983a1f941df8c6710a8bba75735f66cf94787ae355e0467f59`다. [보고서](../../cpppj/ai-direct-commands-20261103.json)와 extracted의 준비/반복Native·독립통합검토·build/tests를 따른다. 병합 전에 mixed CRLF/LF를 보존했다.

검토의 tail-scope-audit는 최근764/766/768의 다른호출꼬리 차이를 별도784로 남기며, 이를 AI지갑 writer 오류나 실제플레이오류로 과장하지 않는다. previous-audit-correction은4b54e0을SP Login Update가아닌 missionPost0/PlayerKnown의일반전투재접속save로 정정한다. 기존661분류대로후순위MP/공통전투저장 부모가선행하며 단순SP진단작업으로선택하지않는다. 783의41db70코드에는영향없다.

MakeAi/조건 AI메뉴·진단·개별reader·checking·초기화면과 남은 비멀티/호환성은 별도다. 새게임/GUI/CLI/MP·물리/PCM·금지플레이·자연완주·전체C++/34/ZIP 검사는0이다.
