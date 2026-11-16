# DaisProcess 원시 필드·패킷·Kernel 수명 (WB04-E617)

기준선 `225553df`의 `o/RawDaisProcessData.cpp/.h` 107줄을
`www/core/raw-dais-process-data.js`로 옮겼다. 기준선 뒤 해당 cpppj 변경은0이다.
4-D D 생성자1815 `DaisTransmission`에 필요한 선행 모듈이며 기존
`RawDaisRegular`의 제단 정기 처리/희생 진행과 구별한다.

- `DaisProcessFields`는40BYTE를 보관하고 생성자의 값 인자는 padding까지 복사한다.
  명시 생성 필드와 base 소멸 표식만 쓰며 C++ vptr/PID를 원문 포인터로 해석하지 않는다.
- `readDaisProcessData`는 신규일 때만 parent DWORD를 읽어 반환한다.
  기존 객체의 parent 영역은 물리 읽기 없이 건너뛰며 bit2 기한 입력을 읽은 뒤에만
  현재 객체 저장을 요구한다. 다른 mask bit는 무시하고64비트 비트를 보존한다.
- writer는 force→접두→비영 parent/dirty1→dirty2 기한→길이→mask→dirty0 순서다.
  dirty1이 남아 있어도 실제 parentPayload가0이면 parent를 쓰지 않는 원문 동작과
  dirty 상위bit를 보존한다. 실패 때 앞서 쓴 접두/dirty는 롤백하지 않는다.
- 실제 `DaisProcess`는 `SquidProcess`를 상속한다. 원문4437f0의 RunFrame과
  480260의 Notify는 빈 몸체이며 기한/dirty로 별도 예약을 만들지 않는다.
  현재 Host Form/부모는 OnAttached와 WritePacket에서 같은 이미지에 반영한다.

## 검증과 한계

cpppj에는 이 파일을 직접 소비하는 전용 검사/원본 fixture가 없다.
새6검사는 **소스 계약 검사**이며 원본 기계어 관측 또는 실제 미션 비교로 세지 않는다.
최초 집중 실행은6/6 통과했다. Node v22.23.3·v26.13.0 전체는 각각5,912건 통과했고 실패·취소·건너뜀0·종료0이다. 두 전체 실행은 --test-concurrency=4를 사용했다. v24는 기존 지시로 생략·호환 가정이다. 대응표 오류0·UTF-8/구문·git diff --check를 확인했다.

1. 전체40BYTE의 생성/소멸·padding·입력 값 복사·빈 실행/통지.
2. mask256종의 신규/기존 parent와64bit 기한 비트·선택 밖 보존.
3. parent 생략·입력 부족·NULL 저장의 실패 순서와 실제 필드 범위.
4. dirty256종×parent2종×force2종=1,024조합의 헤더·payload·mask·길이·canary.
5. 짧은 버퍼의 부분 쓰기·force/parent dirty와 슬라이스 앞뒤 canary.
6. 실제 SID/공장/삭제/Kernel/Host에 일반 내용물 부모를 구성해 부착·writer·빈 실행·삭제.
   제단/사제 희생·포획·게임 플레이는 실행하지 않는다.

새 fixture는0이고 기존 fixture/manifest 내용은 유지했다. 이식 대응표는
port847·partial39·todo121·제외153, session0이다. 남은 이식량은
기준선 기준160단위/34,233줄이다. 검사 로그는 `extracted/webpj-e617-node22.log`,
`webpj-e617-node26.log`, 대응표는 `extracted/webpj-e617-port-map.log`다.

다음은 같은 현재 헤더/공통 버퍼의 Dais→Aviary→Construction→전투/자산 송신 합성과
walkerFall(1815), 다리 효과/로딩 깊이(1816..1824), Runtime 게시1825다.
Native6ah 수신기·전체 Session/Load·실제 미션 플레이는 잔여다.
공중3종 새 전용 생성/생산/행동·실제 적 공격·보호 원본/게임 실행은 하지 않았다.
