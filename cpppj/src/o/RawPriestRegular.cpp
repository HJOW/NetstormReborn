// 원본494580의601~605 선택을 재사용하며 큰 고유 후단은 기존 몸체에 위임한다.
#include "o/RawPriestRegular.h"
#include <stdexcept>
namespace netstorm::o { // 원문 가상 진입의 분기와 반환 상수를 보관한다.
float RunPriestRegular(Sid sid,std::uint32_t event,std::uint32_t count,float payload,const PriestRegularHooks& hooks) { //601의 선택 후 삭제만 실행하고602~605는 각각 현재 후단에 전달한다.
    if (event==601) { if (!count) return 3.0F;if (!hooks.destroy) throw std::logic_error("사제601 실제 삭제 미연결");hooks.destroy(sid,0x100000U);return -1.0F; }
    if (event>=602 && event<=605) { if (!hooks.unique) throw std::logic_error("사제 고유 정기 실제 후단 미연결");return hooks.unique(sid,event,count,payload); }
    if (!hooks.carrier) throw std::logic_error("사제 정기 실제 Carrier 부모 미연결");return hooks.carrier(sid,event,count,payload);
}
}
