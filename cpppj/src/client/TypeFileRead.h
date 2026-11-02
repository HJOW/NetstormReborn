// 원문49c160의TYPE 전용41baa0 호출을 기존 파서와 분리한 명시 실시간 경계다.
#pragma once
#include "o/BaseFile.h"
#include "o/RawBaseFileWholeRead.h"
#include "o/RawFileName.h"
#include "o/RiftType.h"

namespace netstorm::client { // TYPE만 현재 FILE로 읽고 다른 자산·snapshot 판독은 그대로 둔다.
std::string_view FormatTypeFileName(o::RawFileName& names,std::string_view typeName); //49c160의 \\,d,name,.type 네 부품을 같은 고리 한 칸에 작성한다.
std::uintptr_t ReadTypeFileBytes(o::RawBaseFileState& state,std::span<const std::uint8_t> name,std::int32_t& length,const o::RawBaseFileIOHooks& io,const o::BaseFileWholeReadHooks& hooks,std::span<std::uint8_t> scratch={}); //check0/extra0을 전달하고 실패 전 length를 초기화하지 않는다.
o::RiftTypeDefinition ReadNativeTypeDefinition(const o::BaseFileSystem& files,std::string_view typeName); //완료 FILE/키는 Parse 전에 반납하고 입력 할당은 기존 Parse의 정상/예외 이탈까지 보관한다.
}
