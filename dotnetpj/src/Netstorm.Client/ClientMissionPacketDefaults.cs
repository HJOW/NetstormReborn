using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 장기 Client/연결/설정만 기본 패킷 표에 연결한다.
namespace Netstorm.Client;

/// <summary>cpppj/src/client/ClientCampaignTransport.cpp의 네 미션 observer·32h·0bh 기본 등록이다.</summary>
public static class ClientMissionPacketDefaults
{
    /// <summary>원문 네 가변 관찰기 종류다. receiver/version/길이 표는 이 목록의 설치에서 바꾸지 않는다.</summary>
    private static readonly uint[] MissionObserverTypes = [0x2a, 0x2b, 0x67, 0x81];
    /// <summary>현재 실제 진단으로 네 순수 송신 관찰기만 재사용하며 Session/World를 보관하지 않는다.</summary>
    public static void InstallMissionSendObservers(RawPacketDispatch dispatch, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(storage);
        // 각 종류의 observer만 교체하여 기존 고정 F/metadata를 유지한다.
        foreach (uint type in MissionObserverTypes) dispatch.ObservePacket(type, (packet, sending) => RawMissionPacketFields.ObserveClient(type, packet, sending, storage.AssertDisplay));
    }
    /// <summary>32h는 같은 장기7DWORD만 쓰며 입력 sender/connection/Session/phase를 조회하지 않는다.</summary>
    public static void InstallSettings(RawPacketDispatch dispatch, ClientGameplayGlobals game)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(game); var aux = game.GameAux;
        dispatch.Observe(0x32, static _ => { }); dispatch.Version(0x32, 1, 0); dispatch.RequireLength(0x32, 32);
        dispatch.RegisterCanonicalDirect(0x32, 0x0044dc20, (_, _, packet) => RawMissionSettingsPacket.ReceiveCurrent(aux.MissionSettings, packet));
    }
    /// <summary>현재0ch 생성 전역과 실제 Registry 송신을0bh 버전 조회 F에 연결한다. 설치에서 Player/연결을 조회하지 않는다.</summary>
    public static void InstallVersionReply(RawPacketDispatch dispatch, ClientGameplayGlobals game, ClientConnectionGlobals connections, ClientSurfaceGlobals surface)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(connections); ArgumentNullException.ThrowIfNull(surface);
        if (!ReferenceEquals(connections.LocalState.Gameplay, game) || !ReferenceEquals(surface.Gameplay, game)) throw new ArgumentException("버전 응답의 같은 실제 Client 소유자 필요");
        var storage = surface.Storage; var aux = storage.GameAux(); var registry = connections.Registry; var players = game.PlayerRecords;
        dispatch.ObservePacket(0x0b, (packet, sending) => ClientPacketDefaultObservers.Observe(0x0b, packet, sending, storage.AssertDisplay)); dispatch.Version(0x0b, 1, 0); dispatch.RequireLength(0x0b, 0xffff);
        dispatch.RegisterCanonicalDirect(0x0b, 0x004b3af0, (sender, _, _) =>
        {
            byte message = aux.Packet0cHeader[0], protocol = aux.Packet0cHeader[1]; ushort initial = aux.Packet0cInitialLength;
            byte[] packet = new byte[50]; packet.AsSpan().Fill(0xff); packet[1] = message; packet[0] = (byte)(protocol & 0x7f);
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2, 2), initial < 4 ? (ushort)4 : initial);
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), 10); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), 78);
            // 현재 slot/Player/NULL 관문 뒤 실제 Source의 가상 송신만 선택한다.0ch의 observer는 실제 송신 단계에서 실행한다.
            _ = registry.SendMutable(unchecked((int)sender), packet, players.Records);
        });
    }
}
