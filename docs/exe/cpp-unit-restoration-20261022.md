# 워크샾25종 복원·실행 연결 점검

사용자 최신 지시대로 **썬6/레인7/윈드6/썬더6종의 구현·복원을 실제 플레이 테스트보다 먼저 진행한다.** 수정 TEST02에는 데빌메이커와 공중 운반체도 배치되어 있다. 처리기 파일이나 생성자 레시피가 존재하는 것과 실제 캠페인의 생성·이동·공격·삭제까지 연결된 것은 별개다. 현재25종 전체 완료로 판정하지 않는다.

보호PE·원본 `.type`·실제 생성자 레시피를 읽는 `tools/audit_unit_restoration.py`가 아래25종의 타입/생성자/가상 진입을 [JSON](../../cpppj/unit-restoration-inventory-20261022.json)에 고정한다. 썬 캐논128은 캠페인·Early Missions의 별도 자산이며 이25종 외에도 계속 지원한다. 표의 '기존 연결'은 해당 처리기를 호출하는 코드가 있다는 뜻이며 모든 동작의 동일성이나 실제 플레이 검증 완료를 뜻하지 않는다.

|원소|TYPE|원본 이름|현재 연결/남은 구현|
|---|---:|---|---|
|썬|71|Sun Disc Thrower|기존 Archer/발사기 처리기 연결,전체 수명 점검 대상|
|썬|72|Whirlibase|기존 SunAviary/훨리기그 생성·프레임·폭탄 연결|
|썬|78|Stone Tower|기존 Blocker 후처리와 일반 피해 연결|
|썬|111|Balloon|기존42 이동/그림자/내용물 연결|
|썬|133|Golem|기존40 이동/내용물 연결|
|썬|136|Sun Barricade|기존 Fence 연결·표식·보호막·Regular 연결|
|레인|76|Rain Generator|기존 Generator 후처리/공급 연결|
|레인|129|Ice Cannon|기존 Cannon/투사체/피해 연결|
|레인|143|Man o'War Pool|기존 RainAviary/맨오워 생성·프레임·빔 연결|
|레인|145|Acid Barricade|**전용48c7a0 후처리/48ea30 주기와 벽 상호작용 구현·연결 필요**|
|레인|146|Cloud Floater|단위332에서42aed0/42af60과42 이동/송수신/그림자/삭제 연결,실제 Kernel242프레임 대조 통과|
|레인|147|Ice Tower|기존 Blocker 후처리 연결,성장형150과의 연결 추가 점검|
|레인|156|Crystal Crab|기존 Carrier/Steal 처리기 있음,**Walker38 프레임과 실제 이동 시작 연결 필요**|
|윈드|75|Wind Generator|기존 Generator 후처리/공급 연결|
|윈드|137|Sail Skater|기존 Carrier 처리기 있음,**Walker38 프레임과 실제 이동 시작 연결 필요**|
|윈드|139|Devil Maker|상태/생산 Frame/Kernel 어댑터 복원,**세션 등록·더스트 데빌 생성/애니메이션/범위 피해 연결 필요**|
|윈드|140|Crossbow|기존 Archer/발사 연결|
|윈드|141|Air Ship|기존 벌룬과 같은42 이동/그림자/내용물 연결|
|윈드|142|Wind Tower|기존 Blocker 후처리와 일반 피해 연결|
|썬더|77|Thunder Generator|기존 Generator 후처리/공급 연결|
|썬더|79|Bulwark|기존 Blocker 후처리와 일반 피해 연결|
|썬더|131|Thunder Cannon|기존 Cannon/투사체/피해 연결|
|썬더|135|Bulf|기존41 회전/보행/내용물 연결|
|썬더|148|Vander Tower|기존 Archer/즉시 빔 연결|
|썬더|149|Arc Spire|**전용48c080 후처리/48c6e0 주기와 이웃 효과 구현·연결 필요**|

우선 클라우드 플로터의 시작 실패를 고치고,기존 TEST02 시작 검사가 드러내는 데빌메이커 및 Walker38 연결을 이어 복원한다. 애시드 바리케이드와 아크 스파이어도 별도 원문 몸체로 진행하며 썬 바리케이드로 대체하지 않는다. 소리·그림·내용물·공급·피해·삭제의 추가 파생이 드러나면 같은 목록에 기록한다. 사용자 플레이/원본 GUI 검증과 창을 띄우지 않는 빌드·원문 대조·실제 Kernel 검사는 구분한다.

현재·마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**다. 원본/CD/AGENTS는 수정하지 않으며 새 게임 실행은 아직 없다.
