// 전체 객체 수신과 받침 진단이 공유하는 원문 현재 문맥 DWORD를 보관한다.
namespace Netstorm.Core.Original;

/// <summary>World/Session 교체로 초기화하지 않는 실제 표면 진단 전역이다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>0054d4e4의 최초 BSS0이며 실제 객체 수신이 쓰고 받침 오류 상세가 읽는 현재 TYPE 문맥이다.</summary>
    public uint SupportTypeWas { get; set; }
}
