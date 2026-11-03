# CanonDecoder의 나머지 건설·섬/지형 소비자 연결

단위 811은 810의 현재 진단 본체를 실제 건설·설치·섬/지형 소비자에 연결했다. 건설 Confirm/Place/Clear/Request와 커서의 ConstructionLocal 상태가 같은 `metadata.geometry.decoderDiagnostics` 주소를 빌린다. 진단 객체를 복사해 보관하지 않으므로 checking과 callback 교체를 모두 현재 owner에서 읽는다.

Fort/캠페인 로더는 같은 포인터를 지역 Builder 상태까지 전달한다. RawIslandBuilder/RawIslandPlacement, PlayerGeyserBounds의 패턴 폭/높이, 일반 지형 PatternCount도 원문 위치의 Canon 진단을 실행한다. 명시 GameWorld 분석 조회는 Session의 읽기 전용 DecoderDiagnostics accessor로 같은 owner를 빌리며 이번에 실행하지 않았다. 새 포인터는 호스트 상태/콜백에만 추가했고 원본 SID 슬롯/패킷/저장 바이트 구조는 바꾸지 않았다.

현재 소스의 호출 18곳을 괄호 범위와 파일 SHA로 고정했다. 진단 인자를 명시한 호출 14곳, 공통 팩터리 정의 1곳, Client owner가 없는 분석 CLI 전용 3곳이다. 분석 경로는 app/main의 Canon 패턴/정적 GameWorld 보고, FrameBindingInspect, 옛 Islandbuilder다. 옛 Islandbuilder의 호출자는 InspectView/main과 정적 GameWorld 생성자이며 기본 GUI는 UberGump의 RawCampaignSession 생성자를 쓴다. 분류되지 않은 호출은 0이다. 이 분류는 현재 소스 범위이며 향후 새 호출자도 장치를 전달해야 한다.

추가 소비자 검사 8개와 기존 검사 30개, 합계 **38개가 실패 0**이다. 새 검사는 확정의 SID 확보 전, 통지 배치의 비용/Pop 전, 예측 정리의 finder/삭제 전, 로컬 배치의 미리보기 반납 뒤·예측 SID 확보 전, 요청의 MayPlace 뒤·abstract 탐색 전을 확인한다. 섬 Builder는 원문의 Add를 유지하고 청크 쓰기 전에 진단하며 Fort 부모는 Generate/Objects보다 먼저 진단한다. Player 범위는 실제 패턴 계산에서 진단한다. 단순 필드 대입만 검사한 것이 아니라 각 실제 소비자 본문을 호출해 진단 전후 효과 경계를 확인했다.

최종 로그는 CanonConsumers 8·CanonChecking 2·canon_type 4·건설 5종의 기존 세 판본 재생 15·IslandBuilder 3·FortWorldLoad 2·PlayerGeyserBounds 2·CampaignWorldLoad 2다. 기존 810의 원문 진단 156입력과 세 판본 정상 Canon 3,927행을 재사용했으며 새 Native 관측은 0이다. 새 검사의 최초/최종 실행은 모두 통과했다. Release 게임/집중 대상 빌드 2회 모두 경고·오류 0이며 두 번째 빌드는 Player 범위 검사에 남은 사용하지 않는 임시 span을 제거한 뒤 수행했다.

`00425860`은 810의 본체·커서/배치 연결에 이번 소비자 연결을 합쳐 현재 제품의 checking 완료 항목으로 바꾼다. 잔여는 **30함수(부모 21·부분 6·MP 3)**다. RawIslandAdd의 실제 편집 호출자 복원, 객체 메뉴/상태별 효과, 모든 시나리오 동일성을 이 완료 항목에 합산하지 않는다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이고 현재 호스트가 같다. 디컴파일 산출물 변경은 없다. 보호 2,739개/fixture 919개가 모두 불변이며 새 fixture는 없다. 다음 기준은 `extracted/cpp-resume-811/protected-after.json`이다. EXE SHA-256은 `0994cfcda1a7565784be7d399bbe9e2febec157cdb9678831f06ff400149ffad`다. 원시 로그/호출 감사는 `extracted/cpp-resume-811/`, 커밋되는 근거는 [JSON 보고서](../../cpppj/canon-consumers-checking-20261103.json)다.

실제 게임/GUI·공격/발사·포획/희생·승리·공중 3종 전용·MP 기능/통신·물리/PCM·전체 CTest/71대상·34경로·ZIP 실행은 없다. 실제 Client 조립은 소스/빌드 근거이며 810의 물리 범위·32비트 ABI/SEH·진단 UI 한계도 유지한다. 다음은 우선순위1의 나머지 객체 메뉴/커서 상태 효과와 수명이다. 전체 목표는 진행 중이다.
