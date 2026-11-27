using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Client.Input;

// 같은 Client의 현재 Player·Registry·Source를 장기 Squid 송신 장치에 연결한다.
namespace Netstorm.Client;

/// <summary>cpppj/src/client/ClientMain.cpp의 최초 Native 방송·대상 송신 조립이다.</summary>
public sealed partial class ClientSurfaceGlobals
{
    /// <summary>종속 내용물의 UI 통지를 같은 실제 Client 입력 큐의 명령으로 예약한다.</summary>
    public void BindNativeContentCommand(InputQueue input)
    {
        ArgumentNullException.ThrowIfNull(input);
        Storage.BindContentCommand((code, x, y) => input.PushCommand(code, unchecked((int)x), unchecked((int)y)));
    }

    /// <summary>첫 Host 전에 실제 전송과 현재 선택 장치를 설치한다. 송신·선택·게임값 초기화는 실행하지 않는다.</summary>
    public void BindNativeTransmission(ClientConnectionGlobals connections, Action clearSelection)
    {
        ArgumentNullException.ThrowIfNull(connections); ArgumentNullException.ThrowIfNull(clearSelection);
        if (!ReferenceEquals(connections.LocalState, Clock.LocalState))
            throw new ArgumentException("Squid 전송의 같은 Client 연결/시계 owner 필요", nameof(connections));
        Storage.BindTransmissionDevices(CreateTransmissionDevices(connections));
        Storage.BindGumpType(Gameplay.GumpFormTypeBox);
        Storage.BindProcessDevices(new()
        {
            Selected = () => Gameplay.Selection.Selected.Value,
            ClearSelection = () =>
            {
                if (!Gameplay.Borrowed()) { clearSelection(); return; }
                var session = Gameplay.RequireSession();
                using var pin = Gameplay.PinCurrentRuntime(session);
                clearSelection();
            },
        });
        Storage.BindPlayerRegularState(Gameplay.Player);
    }

    /// <summary>같은 가변 버퍼를 재사용하고 각 효과 직전에 현재 연결과 Player를 다시 조회한다.</summary>
    private RawSurfaceTransmissionDevices CreateTransmissionDevices(ClientConnectionGlobals connections) => new()
    {
        Broadcast = packet =>
        {
            RawConnectionRegistry registry = connections.Registry;
            // Player flag→현재 NULL 연결→현재 로컬 제외 뒤 선택된 노드의 가상 송신만 실행한다.
            for (int owner = 1; owner < 9; owner++)
            {
                if (!CurrentTransmissionPlayer(owner)) continue;
                uint token = registry.Cleanup().Transceivers[owner];
                if (token == 0 || (uint)owner == Gameplay.Player.BattleOwner) continue;
                using NativeBattleSource source = connections.PacketSources.Resolve(registry.Node(token))
                    ?? throw new InvalidOperationException("Squid 방송의 선택된 실제 Source 수명 미공개");
                registry.SendDirect(token, packet);
            }
        },
        Connected = target => target > 0 && target < 9 && CurrentTransmissionPlayer(target),
        Send = (target, packet) =>
        {
            RawConnectionRegistry registry = connections.Registry;
            uint token = registry.Cleanup().Transceivers[target];
            using NativeBattleSource? source = token == 0 ? null : connections.PacketSources.Resolve(registry.Node(token))
                ?? throw new InvalidOperationException("Squid 대상 송신의 실제 Source 수명 미공개");
            registry.SendMutableNative(target, packet, Gameplay.PlayerRecords.Records, Storage.AssertDisplay);
        },
    };

    /// <summary>현재 Player의 활성 BYTE를 범위 검사 뒤 읽으며 이전 송신 전의 사본을 유지하지 않는다.</summary>
    private bool CurrentTransmissionPlayer(int owner)
    {
        ReadOnlySpan<byte> rows = Gameplay.PlayerRecords.Records.Span;
        int offset = checked(owner * 180 + 0x50);
        if ((uint)offset >= (uint)rows.Length) throw new ArgumentOutOfRangeException(nameof(owner), "Squid 송신의 현재 Player 물리 범위");
        return (rows[offset] & 1) != 0;
    }
}
