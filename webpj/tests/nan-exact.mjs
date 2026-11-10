// 신호 NaN/NaN 페이로드를 비트 단위로 대조하는 시험 파일이 가장 먼저 가져오는 부작용 모듈이다: V8 의 최적화 컴파일러(TurboFan·Maglev)를 이 시험 프로세스에서만 끈다.
//  이유: Node v26(V8 최신판)의 최적화 JIT 는 JS double 필드/지역 값에 담긴 신호 NaN 의 quiet 비트를 켜고(예: 0x7ff00000_00000001 → 0x7ff80000_00000001),
//  두 NaN 의 덧셈에서 어느 피연산자의 페이로드를 남길지도 최적화 단계마다 달라진다. 함수가 데워진 뒤에만 일어나 같은 시험이 반복 횟수에 따라 실패하므로
//  원본 x86 관측의 NaN 비트 대조(carrier-fall-start·construction-data)는 인터프리터/기준 컴파일러(Ignition·Sparkplug)에서 돌려야 Node v22/v24/v26 모두에서 같다.
//  모르는 플래그는 V8 이 경고만 하고 무시한다(Node v22 에서도 안전). 이 파일을 가져온 시험 프로세스 밖에는 영향이 없다. (2026-11-10 WB04-E259: Node v26 업그레이드 뒤 시험 2건 실패 교정)
import v8 from 'node:v8';

// 최적화 컴파일러 두 종을 끈다(TurboFan = 최상위 단계, Maglev = 중간 단계).
v8.setFlagsFromString('--no-turbofan');
v8.setFlagsFromString('--no-maglev');
