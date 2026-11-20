using Netstorm.Core.Original;

// 짧은 Login 수신이 기존 장기32h canonical F를 덮지 않게 한다.
namespace Netstorm.Net;

/// <summary>RawMissionSettingsPacket.cpp의 두 실제 등록 부모다.</summary>
public static class MissionSettingsRegistration
{
    /// <summary>보호10.78의 실제32h 불변 코드 주소다.</summary>
    private const uint NativeEntry = 0x0044dc20;
    /// <summary>같은 장기F가 있을 때만 현재 lease에 다시 등록하고 메타데이터/observer는 보존한다.</summary>
    private static bool Borrow(RawPacketDispatch dispatch)
    {
        CanonicalPacketReceiver? receiver = dispatch.Registration(0x32).CanonicalReceive; if (receiver is null || receiver.NativeEntry != NativeEntry) return false; dispatch.RegisterCanonical(0x32, receiver); return true;
    }
    /// <summary>장기F를 우선 빌리고 독립 typed는 원문 빈 observer/version1/정책0·같은 수신을 설치한다.</summary>
    public static void Register(RawPacketDispatch dispatch, RawMissionSettingsPacket packet)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(packet); if (Borrow(dispatch)) return;
        dispatch.Observe(0x32, static _ => { }); dispatch.Version(0x32, 1, 0); dispatch.Register(0x32, (_, _, bytes) => packet.Receive(bytes));
    }
    /// <summary>비Login의 실제7DWORD owner에 직접 쓰되 기존 장기F가 있으면 같은 F를 대여한다.</summary>
    public static void Register(RawPacketDispatch dispatch, Memory<uint> current)
    {
        ArgumentNullException.ThrowIfNull(dispatch); if (current.Length != 7) throw new ArgumentException("미션 설정의 실제7DWORD 누락", nameof(current)); if (Borrow(dispatch)) return;
        dispatch.Observe(0x32, null); dispatch.Version(0x32, 1, 0); dispatch.RequireLength(0x32, 32); dispatch.Register(0x32, (_, _, packet) => RawMissionSettingsPacket.ReceiveCurrent(current, packet));
    }
}
