# Session 경로 송신 합성 (WB04-E669)

기준선 `225553df`의 `RawCampaignSession.cpp` 3962..3967을 이식했다. 독립 typed Session의 `30h` 헤더와 16,384바이트 버퍼를 같은 Path writer에 제공하고 Dais→Aviary→Construction→전투→자산→Path 순서로 현재 Process Host에 합성한다. 같은 버퍼의 실제 작성 길이만 기존 `pathPacket`에 넘기고 길이 오류와 반납 뒤 호출을 거부한다. 해당 cpppj 파일은 기준선 뒤 변경이 없다.

신규 Session 검사에서 같은 Pool의 실제 Path를 Host에 부착해 송신된 패킷의 플래그·종류·Form·부모를 확인하고 설치 순서·중복·길이·반납 관문을 대조했다. 첫 검사에서는 헤더의 플래그와 종류 바이트 순서를 잘못 단언해 교정했다. 원본 PE `pathdata-x86.tsv` 중 10.78 384행(전체 파일 SHA-256 `dbd3e1413d6ae263da77fd1da763dc1e133939e34b88bdbd660d2da6c307b23f`)과 기존 송신 합성 시험을 포함해 집중 63건 통과했다. 실제 미션 Load는 미실행이며 `loaded=false`다. Native 장기 헤더/버퍼 분기는 4D-8에서 연결할 때까지 명시적으로 거부한다. 다음은 BridgeCarriers다.

Node v22.23.3·v26.13.0 전체 각각 6,090건 통과(실패·취소·건너뜀 0). 전체 로그는 Git 제외 `extracted/webpj-e669-node22.log`·`extracted/webpj-e669-node26.log`다. v24는 사용자 지시에 따라 생략한다.
