# 현재 지도 Territory snapshot (WB04-E632)

기준선 `225553df`의 RawFortTerritorySave.cpp/h와 Session5853..5866을
옮겼다. Session의 전체 Fort/Battle snapshot도 사용하는 공용 자식이라
편집기 전용 제외를 교정했다. 해당 소스의 기준선 뒤 변경은0이다.
편집기 UI/파일 저장은 추가하지 않았다.

기존120BYTE를 읽은 뒤 special을 한 번 보관하고 지역0..19를 처리한다.
일반 분기는 clamp한 owner의 현재 섬이 존재하며 기존 record byte1의
bit0가 켜진 경우에만 현재 지도 record로 교체한다. special 분기는
owner 조건 없는 첫 지역 검색 뒤 누락 순번/같은 난수 seed를 쓴다.
noRegion과 manager는 원문 소비 위치마다 읽고 Create 반환값 대신
생성 뒤 현재 manager의 current stream을 보관한다. 진단 뒤 현재
data/pos로120BYTE를 쓰고 출력 뒤 pos/end를 다시 갱신한다.

섬 검색은 진입 count와 현재32BYTE 기록의 존재·현재/원래 owner·signed
지역 BYTE를 사용하고 첫 일치에서 반환한다. 지도 record는 같은
ChunkMap의 y 바깥/x 안쪽 첫 일치의6BYTE 사본이다. seed는 난수 호출
전에 첫 두 BYTE를 쓰고 Next(254) 반환 뒤 byte3만 갱신한다. 선택된
장치 누락이나 중단 앞의 효과를 복구하거나 성공으로 바꾸지 않는다.

최초 실행에서 신규5건 모두 통과했다. 원본 부모638행·섬 검색20,162행·
seed188행의 전체 raw/현재 전역·두 stream/출력·trace·부분 중단(부모6·seed2)을
대조했다. 기존 관측을 수정하거나 재수집하지 않았다. 물리 count 경계·
signed 섬 BYTE·현재 record 사본·누락 난수 앞의 부분 쓰기도 검사했다.

실제 Session 검사는 같은 World Runtime의 Territory reader·현재 섬
목록/청크·Template 저장소를 연결한다. 기존 bit0가 켜진 첫 영역만
현재 지도 record로 교체하고 다른 기록은 보존하며 새 노드/current·
Pool/난수 불변·공개 설치 전/반납 관문을 확인했다. 외곽 loaded를
미리 켜거나 미션 Load를 실행한 검사가 아니다. 공용 신전 상승의
120BYTE 일반 writer(E588)와 전체 snapshot 부모를 구별한다.

원시 C++ 단위는 헤더 포함105줄이다. 대응표 대상1,014단위/port855·
제외146, manifest816항목(original-x86 777)을 갱신했다.
관련 저장 부모/공용 writer·대응표/manifest를 합친 집중39건이 통과했고
대응표 오류0·git diff --check를 확인했다. 전체 Node v22.23.3·v26.13.0 각각5,996건 통과, 실패·취소·건너뜀0이다.
v24는 사용자 지시대로 생략하고 호환을 가정했다.
다음은 Money/Mission·Badges/CoreData와 전체 snapshot/외곽 공개·Load다.
앱 session0·실제 미션 플레이 불가, 자연 완주/포획·희생 및 공중3종
새 전용 생성/생산/행동 검사 제외를 유지했다. 게임 창은 실행하지 않았다.
