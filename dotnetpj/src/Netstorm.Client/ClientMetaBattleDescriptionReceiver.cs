// 0x71 전투 설명 수신의 원문 observer·버전·길이와 CS0 관문·고정 F를 같은 분배기에 설치한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaDescriptionReceiver(0x71 0x004b49c0)와 o/RawStateMetaBattleDescription.cpp 의 RegisterStateMetaBattleDescription(0040b0d0)을 이식한다.
// 0.943에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client;

// 0x71 전투 설명 수신의 같은 장기 분배 설치 부모다. 실제 세션·메뉴·전송 장치를 선조회하지 않는다.
public static class ClientMetaBattleDescriptionReceiver
{
    /// <summary>원문 0x71 전투 설명 종류다.</summary>
    private const uint Type = 0x71;

    /// <summary>원문 0x71 수신의 실제 고정 F 주소다.</summary>
    private const uint NativeEntry = 0x004b49c0;

    /// <summary>원문 0x71 송신 관찰기 진단 줄 번호다(0xae3).</summary>
    private const uint AssertLine = 0xae3;

    /// <summary>같은 장기 F가 있을 때만 현재 칸에 다시 등록하고 장기 observer·원문 metadata는 보존한다. 0.943에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    private static bool Borrow(RawPacketDispatch dispatch)
    {
        // 같은 고정 F의 실제 장기 수신기를 값으로 선택하며 현재 packet의 일반 수신 입구를 중계하지 않는다.
        CanonicalPacketReceiver? baseline = dispatch.Registration(Type).CanonicalReceive;
        if (baseline is null || baseline.NativeEntry != NativeEntry)
        {
            return false;
        }

        dispatch.RegisterCanonical(Type, baseline);
        return true;
    }

    /// <summary>송신 방향에서만 길이 BYTE를 다시 조회하고 선언 WORD를 쓴다. 0.943에서 추가됐다.</summary>
    /// <param name="packet">같은 현재 버퍼의 실제 물리다.</param>
    /// <param name="sending">원문 송신 방향 여부다.</param>
    /// <param name="storage">같은 Client의 실제 장기 진단 owner다.</param>
    private static void Observe(Memory<byte> packet, uint sending, RawSurfaceStorage storage)
    {
        // 수신 방향은 버퍼도 읽지 않는다.
        if (sending == 0)
        {
            return;
        }

        // 선택한 현재 BYTE를 읽는다. 범위 밖 접근은 선택 위치에서 거부한다.
        if (packet.Length < 10)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "전투 설명 관찰기의 실제 BYTE 물리 범위");
        }

        if (packet.Span[9] == 0xff)
        {
            storage.AssertDisplay("descLen != (unsigned char)-1", @"\Ns\O\_p_packets.cpp", AssertLine);
        }

        // 현재 packet+2의 WORD만 쓴다.
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Span.Slice(2, 2), unchecked((ushort)(packet.Span[9] + 10)));
    }

    /// <summary>0x71의 원문 observer·버전·길이와 CS0 관문·고정 F를 설치한다. 0.943에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="challenge">같은 Client의 실제 장기 CS owner다.</param>
    /// <param name="storage">같은 Client의 실제 장기 진단 owner다.</param>
    /// <param name="receive">CS가 비영일 때만 호출되는 실제 수신 입구다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientChallengeGlobals challenge, RawSurfaceStorage storage, Action<uint, ReadOnlyMemory<byte>> receive)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(storage);
        ArgumentNullException.ThrowIfNull(receive);

        // 동일 F가 이번 분배기에 있으면 장기 observer·원문 metadata를 그대로 유지한다.
        if (Borrow(dispatch))
        {
            return;
        }

        dispatch.ObservePacket(Type, (packet, sending) => Observe(packet, sending, storage));
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, 0xffff);

        // 원문은 mode·sender·known 관문 없이 현재 CS가 비영일 때만 자식을 호출한다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (sender, _, packet) =>
        {
            if (challenge.Current != 0)
            {
                receive(sender, packet);
            }
        });
    }
}
