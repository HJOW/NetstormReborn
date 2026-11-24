using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x24 메타 전투 삭제의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaBattleRemove.cpp 의 ReceiveNativeMetaBattleRemove 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x24 등록을 이식한다.
// 0.958에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x24 메타 전투 삭제가 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaBattleRemoveHooks
{
    /// <summary>같은 실제 CS 전투10 기록 owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>수준9의 없는 전투 진단 장치다.</summary>
    public Action<string>? Log { get; set; }

    /// <summary>같은 실제 섬 장비 하차 장치다.</summary>
    public Action<uint>? Leave { get; set; }

    /// <summary>Ch.cpp 696의 참가자 잔존 진단 장치다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

// Ch.cpp의 24h 전투 삭제의 실제 부모다.
public static class ClientMetaBattleRemove
{
    /// <summary>원문 0x24 메타 전투 삭제 종류다. 0.958에서 추가됐다.</summary>
    public const uint Type = 0x24;

    /// <summary>원문 0x24 수신의 실제 고정 F 주소다. 0.958에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b4600;

    /// <summary>원문 0x24 수신의 실제 고정 길이다. 0.958에서 추가됐다.</summary>
    public const ushort FixedLength = 5;

    /// <summary>초기화에 쓰는 DWORD 개수다(0x49×4=292BYTE 기록 전체). 0.958에서 추가됐다.</summary>
    private const int ClearWords = 0x49;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.958에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 전투 삭제의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.958에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 전투 삭제의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.958에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 전투 삭제의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원문 삭제의 부분 효과와 진입 CS/현재 패킷 식별자 재조회를 보존한다. 0.958에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·로그·하차·진단 장치다.</param>
    /// <param name="packet">원문 0x24 패킷이다.</param>
    public static void Receive(ClientMetaBattleRemoveHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        ReadOnlySpan<byte> bytes = packet.Span;
        uint id = Read(bytes, 4, 1);
        Memory<byte> record = challenge.Battle(id);
        if ((Read(record.Span, 0, 1) & 1u) == 0)
        {
            NeedDelegate(hooks.Log)("ERROR in handleChalBattleDel: battle " + id.ToString(System.Globalization.CultureInfo.InvariantCulture) + " doesn't exist!\n");
            return;
        }

        NeedDelegate(hooks.Leave)(Read(record.Span, 0x20));
        challenge.ClearArray(record.Slice(0x24, 16));
        // 원문 73DWORD가 292BYTE 기록 전체를 같은 순서로 지운다.
        for (int word = 0; word < ClearWords; word++)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(word * 4, 4), 0);
        }

        BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x18, 4), 0xffffffffu);
        BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x1c, 4), 0x100u);
        Memory<byte> cs = challenge.Bytes;
        // 원문 40기록은 active 필터 없이 현재 패킷 식별자를 매번 재조회한다.
        for (uint peer = 0; peer < RawChallengeState.ParticipantCount; peer++)
        {
            if (Read(cs.Span, RawChallengeState.ParticipantOffset + unchecked((int)peer) * RawChallengeState.ParticipantBytes + 0xa4) != id)
            {
                continue;
            }

            NeedDelegate(hooks.Assertion)("chalPlayer[i].battle != z->battle", ".\\Ns\\O\\Ch.cpp", 0x696);
        }
    }

    /// <summary>0x24의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.958에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 CS·로그·하차·진단 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaBattleRemoveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        // 원문 40b380은 비NULL RET이며 성공 ACK를 만들지 않는다.
        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, FixedLength);
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
