using Netstorm.Core.Original;
using Netstorm.Net;

// 69h의 장기 목록과 항목마다 현재 Runtime을 선택하는 실제 표시 부모를 연결한다.
namespace Netstorm.Client;

/// <summary>ClientCampaignTransport.cpp의69h/version1/policy0/ffff/고정4b49e0 등록이다.</summary>
public static class ClientTechnologyPacketDefaults
{
    /// <summary>같은 GameAux 목록을 보관하되 현재 Session과 실제 전체 owner는 매 비영 항목에서만 빌린다.</summary>
    public static void Install(RawPacketDispatch dispatch, ClientGameplayGlobals game, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(storage);
        if (!ReferenceEquals(storage.GameAux(), game.GameAux)) throw new ArgumentException("기술 수신의 같은 실제 GameAux 필요", nameof(storage));
        byte[] typeUse = game.GameAux.TypeUse;
        dispatch.ObservePacket(0x69, (packet, sending) => RawTechnologyTypesPacket.Observe(packet, sending, storage.AssertDisplay));
        dispatch.Version(0x69, 1, 0); dispatch.RequireLength(0x69, 0xffff);
        dispatch.RegisterCanonicalDirect(0x69, 0x004b49e0, (_, _, packet) => RawTechnologyTypesPacket.Receive(packet, typeUse, type =>
        {
            var session = game.RequireSession();
            using var runtimePin = game.PinCurrentRuntime(session);
            session.IndicateTechnology(type);
        }));
    }
}
