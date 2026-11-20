using Netstorm.Core.Original;

// Core의 연구 본체를 같은 실제 Net 공유 표에 등록한다.
namespace Netstorm.Net;

/// <summary>cpppj RawFactoryResearch.cpp의 RegisterFactoryResearchCommand 원문 등록이다.</summary>
public static class FactoryResearchRegistration
{
    /// <summary>58h의 실제 빈 관찰기·버전1/정책0·길이16과 같은 연구 수신을 순서대로 설치한다.</summary>
    public static void Register(RawPacketDispatch dispatch, RawFactoryResearch research, Action? prepare = null)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(research);
        dispatch.Observe(0x58, _ => { }); dispatch.Version(0x58, 1, 0); dispatch.RequireLength(0x58, 16);
        dispatch.Register(0x58, (sender, _, packet) => { prepare?.Invoke(); research.Receive(sender, packet); });
    }
}
