// 원문26F 중39h/86h의 불변 수신 코드와 현재 실제 Session 배송을 연결한다.
namespace Netstorm.Client;

/// <summary>NativeGameplayPacketReceivers.cpp의 두 비행 행이며 전체26행 설치는 후속이다.</summary>
public static partial class NativeGameplayPacketReceivers
{
    /// <summary>한 행의 실제 메시지/불변 코드/길이/빈 observer 구별이다.</summary>
    /// <param name="Type">원문 메시지 번호다.</param><param name="Entry">원문 불변 수신 코드다.</param><param name="Length">원문 고정 길이 또는ffff다.</param><param name="Empty">비NULL ret4이면true, NULL이면false다.</param>
    private readonly record struct FlyerReceiverSpec(uint Type, uint Entry, ushort Length, bool Empty);
    /// <summary>39h 상태와86h 기한 명령의 검토한 두 원문 행이다.</summary>
    private static readonly FlyerReceiverSpec[] FlyerReceivers = [new(0x39, 0x0045e2b0, 0xffff, false), new(0x86, 0x00426490, 20, true)];
    /// <summary>두F만 설치하며 다른 표/현재 Session/Player 준비를 건드리지 않는다.</summary>
    public static void InstallFlyer(ClientGameplayGlobals game, Net.RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(dispatch);
        // 원문 NULL/비NULL 빈 observer를 구분하고 같은 현재 Runtime을 매 배송에서 선택한다.
        foreach (FlyerReceiverSpec spec in FlyerReceivers)
        {
            if (spec.Empty) dispatch.Observe(spec.Type, static _ => { }); else dispatch.Observe(spec.Type, null);
            dispatch.Version(spec.Type, 1, 0); dispatch.RequireLength(spec.Type, spec.Length);
            dispatch.RegisterCanonicalDirect(spec.Type, spec.Entry, (_, _, packet) =>
            {
                var session = game.RequireSession(); using var runtime = game.PinCurrentRuntime(session); session.ReceiveFixedGameplayPacket(spec.Entry, packet);
            });
        }
    }
    /// <summary>원래 caller 위치에서 같은F/metadata가 일치할 때만 현재 등록 대여에 붙인다.</summary>
    public static void BindFlyer(Net.RawPacketDispatch dispatch, uint type)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        // 검토한 두 행만 선택하며 모르는 번호를 다른F로 넘기지 않는다.
        foreach (FlyerReceiverSpec spec in FlyerReceivers)
        {
            if (type != spec.Type) continue; Net.PacketRegistration row = dispatch.Registration(type); Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(type);
            bool observer = spec.Empty ? row.Observer is not null && row.PacketObserver is null : row.Observer is null && row.PacketObserver is null;
            if (receiver is null || receiver.NativeEntry != spec.Entry || row.Version != 1 || row.Mismatch != 0 || row.Length != spec.Length || !observer) throw new InvalidOperationException("Native 비행의 실제 고정F/metadata 불일치");
            dispatch.RegisterCanonical(type, receiver); return;
        }
        throw new ArgumentException("미등록 Native 비행 메시지", nameof(type));
    }
}
