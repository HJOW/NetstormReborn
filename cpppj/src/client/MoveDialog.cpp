// 원문은565dd4를먼저캡처하고가상Push이후에도그포인터의현재필드를쓴다.
#include "client/MoveDialog.h"
#include <stdexcept>

namespace netstorm::client { // 원문 좌표 산술과 실제 Gump 필드를 연결한다.
// Push 뒤의 현재 크기/화면을 사용하고 같은 노드에 원문 순서대로 부분 좌표를 쓴다.
void MoveDialog(GumpCreateNode* current,std::string_view location,const int& screenWidth,const int& screenHeight,const MoveDialogHooks& hooks) {
    if (!current) return;
    if (!hooks.push || !hooks.attach) throw std::invalid_argument("MoveDialog의 실제 Push/Attach 미연결");
    hooks.push();location=location.substr(0,location.find('\0'));
    if (location.find('b')!=location.npos) current->y=static_cast<float>(static_cast<double>(screenHeight)-current->height);
    if (location.find('t')!=location.npos) current->y=0;
    if (location.find('r')!=location.npos) current->x=static_cast<float>(static_cast<double>(screenWidth)-current->width);
    if (location.find('l')!=location.npos) current->x=0;
    hooks.attach(current->x,current->y,0,0);
} // b보다t, r보다l이우선하며원문fild/fsub/fstp의저장지점을보존한다.
}
