using System.Buffers.Binary;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Net;

// 전투 종료의 고정 통지/현재 Player 쓰기와 상태 예약/선택된 외부 삭제 순서를 공유한다.
namespace Netstorm.Client;

/// <summary>004b6350이 같은 실제 상태/전역에서 읽고 쓰는 값이다.</summary>
public sealed record BattleAbortStateGlobals
{
    /// <summary>다음 상태와 세+84 필드가 있는 같은 실제 블록이다.</summary>
    public LocalMissionStateStorage State { get; init; } = new();
    /// <summary>005c85e0의 별도 중단 예약이다.</summary>
    public UIntBox AbortPending { get; init; } = new();
    /// <summary>005c85ac의 현재 결과/요청 상태다.</summary>
    public UIntBox Outcome { get; init; } = new();
    /// <summary>00594fa4의 현재 미션 후처리 모드다.</summary>
    public Func<bool>? MissionPost { get; init; }
    /// <summary>005c85a4의 현재 전체 Editor DWORD다.</summary>
    public Func<uint>? Editor { get; init; }
    /// <summary>00594fc8의 현재 메타맵 모드다.</summary>
    public Func<bool>? Metamap { get; init; }
    /// <summary>00594fbc의 현재 전투 모드다.</summary>
    public Func<bool>? Battle { get; init; }
}

/// <summary>양수 전투 이탈 이유가 공통 중단에 전달하는 같은 현재 전역이다.</summary>
public sealed record BattleLeaveStateGlobals
{
    /// <summary>공통 부모의 같은 상태/모드/결과다.</summary>
    public BattleAbortStateGlobals Abort { get; init; } = new();
    /// <summary>00542544의 현재 스크립트 중단 표식이다.</summary>
    public UIntBox ScriptBreak { get; init; } = new();
    /// <summary>0055a420의 시작 출처이며 선행 삭제 복귀 뒤0을 쓴다.</summary>
    public UIntBox SourceType { get; init; } = new();
    /// <summary>00595344의 현재 Player 번호 확정 여부다.</summary>
    public Func<uint>? PlayerKnown { get; init; }
}

/// <summary>0040aa60/004a71a0/004b3540과 양수 이탈 부모의 원문 직접 쓰기다.</summary>
public static class RawBattleLeave
{
    /// <summary>type52/version1/length5의 실제 규격이다.</summary>
    public const int PacketBytes = 5;
    /// <summary>00542a08의 실제 전투 떠나기 명령이다.</summary>
    public const uint LeaveBattleCommand = 0x020000c5;
    /// <summary>선택된 실제 하위만 요구하고 앞선 상태 쓰기를 유지한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("전투 이탈의 실제 선택 하위 미연결");
    /// <summary>생성자처럼 앞4바이트만 쓰고 본문/인접 물리를 보존한다.</summary>
    public static void InitializePacket(Span<byte> packet)
    {
        if (packet.Length < 4) throw new ArgumentOutOfRangeException(nameof(packet), "전투 종료 헤더 물리 부족"); packet[0] = 1; packet[1] = 0x52; packet[2] = 5; packet[3] = 0;
    }
    /// <summary>실제 State3처럼 고정 헤더 뒤 다섯째BYTE를0으로 쓴다.</summary>
    public static byte[] MakePacket() { byte[] packet = new byte[PacketBytes]; InitializePacket(packet); packet[4] = 0; return packet; }
    /// <summary>같은 실제 로컬9에 통지를 예약한다. 강제flush나 수신은 하지 않는다.</summary>
    public static void SendForState(Action<int, ReadOnlyMemory<byte>> send) => Need(send)(9, MakePacket());
    /// <summary>현재 sender의 실제 Player+50 DWORD에4를 합치며 payload/연결은 읽지 않는다.</summary>
    public static void Receive(uint sender, Memory<byte> players)
    {
        long at = (long)sender * 180 + 80; if (sender >= 9 || at + 4 > players.Length) throw new ArgumentOutOfRangeException(nameof(sender), "전투 종료 Player 물리 범위"); players.Span[(int)at] |= 4;
    }
    /// <summary>원본52h 빈observer/버전과 실제 수신을 등록하거나 같은 Native F만 대여한다.</summary>
    public static void Register(RawPacketDispatch dispatch, Memory<byte> players)
    {
        ArgumentNullException.ThrowIfNull(dispatch); PacketRegistration row = dispatch.Registration(0x52); CanonicalPacketReceiver? receiver = row.CanonicalReceive;
        if (receiver?.NativeEntry == 0x004a71a0)
        {
            if (row.Version != 1 || row.Mismatch != 0 || row.Length != 5 || row.Observer is null || row.PacketObserver is not null) throw new InvalidOperationException("Native 연결 수신의 같은 metadata 불일치"); dispatch.RegisterCanonical(0x52, receiver); return;
        }
        dispatch.Observe(0x52, static _ => { }); dispatch.Version(0x52, 1, 0); dispatch.Register(0x52, (sender, _, _) => Receive(sender, players));
    }
    /// <summary>다른 네 블록과 부가인자를 바꾸지 않고 next 첫DWORD만3으로 예약한다.</summary>
    public static void RequestMainMenu(LocalMissionStateStorage state) => BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span, 3);
    /// <summary>중단 선택/직접 쓰기 뒤에만 실제 저장 경로/삭제 부모를 호출한다.</summary>
    public static void PrepareAbort(BattleAbortStateGlobals globals, Action? deleteSavedState)
    {
        globals.AbortPending.Value = 1;
        if (!Need(globals.MissionPost)() && Need(globals.Editor)() == 0 && !Need(globals.Metamap)())
        {
            if (Need(globals.Battle)()) { if (globals.Outcome.Value == 0) globals.Outcome.Value = 3; BinaryPrimitives.WriteUInt32LittleEndian(globals.State.Next.Span, 6); }
        }
        else { globals.Outcome.Value = 0; RequestMainMenu(globals.State); }
        BinaryPrimitives.WriteUInt32LittleEndian(globals.State.Next.Span[132..], 0); BinaryPrimitives.WriteUInt32LittleEndian(globals.State.Immediate.Span[132..], 0); BinaryPrimitives.WriteUInt32LittleEndian(globals.State.Current.Span[132..], 0); Need(deleteSavedState)();
    }
    /// <summary>양수 이유별 선행/공통 삭제의 현재 전역과 부분 효과를 원문 순서대로 보존한다.</summary>
    public static void PrepareLeave(int reason, BattleLeaveStateGlobals globals, Action? deleteSavedState, Action? remoteLeave = null)
    {
        if (reason < 1) throw new ArgumentOutOfRangeException(nameof(reason), "전투 이탈 양수 이유 계약 오류");
        if (Need(globals.PlayerKnown)() != 0 && Need(globals.Abort.Battle)() && !Need(globals.Abort.MissionPost)() && reason != 2 && reason != 3) Need(remoteLeave)();
        globals.ScriptBreak.Value = 1;
        if (reason == 2) { RequestMainMenu(globals.Abort.State); return; }
        if (reason is 3 or 9) { Need(deleteSavedState)(); globals.SourceType.Value = 0; }
        PrepareAbort(globals.Abort, deleteSavedState);
    }
    /// <summary>떠나기 뗌은 소비하고 signed 이유0이하는 실제 ConfirmRetreat 안내만 선택한다.</summary>
    public static bool DispatchRetreatConfirmation(InputEvent input, Action<string>? tell)
    {
        if ((input.Code & 0x03ffffff) != LeaveBattleCommand) return false; if ((input.Code & InputCode.Release) != 0) return true; if (input.Y >= 1) return false; Need(tell)("ConfirmRetreat"); return true;
    }
}
