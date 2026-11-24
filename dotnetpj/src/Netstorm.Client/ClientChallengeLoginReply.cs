// 0x1d 챌린지 로그인 응답의 필드·버전 검사와 동기 Run 순서를 같은 분배기에 연결한다.
// cpppj/src/o/RawChallengeLoginReply.cpp 의 ReceiveChallengeLoginReply(004b8ac0)를 이식한다.
// 0.947에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client;

// 004b8ac0 로그인 응답이 선택한 효과의 실제 외부 장치다.
public sealed class ClientChallengeLoginReplyHooks
{
    /// <summary>원문 수준 1dh의 현재 응답코드 문구를 기록한다.</summary>
    public Action<string>? Log { get; set; }

    /// <summary>004b4160의 실제 메타 버전 검사다.</summary>
    public Func<uint, uint, bool>? Version { get; set; }

    /// <summary>현재 블록을 직접 28/12로 쓴 뒤 실제 동기 Run을 호출한다.</summary>
    public Action? Run { get; set; }

    /// <summary>같은 실제 로컬 상태 블록 owner다.</summary>
    public LocalMissionStateStorage? State { get; set; }

    /// <summary>같은 실제 구독자 번호 owner다.</summary>
    public UIntBox? Subscriber { get; set; }
}

// 004b8ac0 전체이며 거부한 버전도 같은 현재 블록으로 즉시 Run한다. 메뉴·세션을 요구하지 않는다.
public static class ClientChallengeLoginReply
{
    /// <summary>원문 0x1d 챌린지 로그인 응답 종류다. 0.947에서 추가됐다.</summary>
    private const uint Type = 0x1d;

    /// <summary>원문 0x1d 수신의 실제 고정 F 주소다. 0.947에서 추가됐다.</summary>
    private const uint NativeEntry = 0x004b8ac0;

    /// <summary>원문 0x1d 수신의 실제 고정 길이다. 0.947에서 추가됐다.</summary>
    private const ushort FixedLength = 13;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.947에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("챌린지 로그인 응답의 선택된 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.947에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "챌린지 로그인 응답의 실제 읽기 범위");
        }

        uint value = 0;
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>로그→구독자→현재 응답코드/28→버전 거부 시 12/0→동기 Run을 그대로 수행한다. 0.947에서 추가됐다.</summary>
    /// <param name="hooks">실제 진단·버전·Run·상태·구독자 장치다.</param>
    /// <param name="packet">원문 0x1d 패킷이다.</param>
    public static void Receive(ClientChallengeLoginReplyHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        LocalMissionStateStorage state = hooks.State ?? throw new InvalidOperationException("챌린지 로그인 응답의 실제 상태 미연결");
        UIntBox subscriber = hooks.Subscriber ?? throw new InvalidOperationException("챌린지 로그인 응답의 실제 구독자 미연결");
        Need(hooks.Log)("Received ZChalLoginReply, code=" + Read(packet.Span, 12, 1).ToString(System.Globalization.CultureInfo.InvariantCulture) + "\n");
        subscriber.Value = Read(packet.Span, 8);
        BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0x38, 4), Read(packet.Span, 12, 1));
        BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0, 4), 28);
        if (!Need(hooks.Version)(Read(packet.Span, 4, 2), Read(packet.Span, 6, 2)))
        {
            BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0, 4), 12);
            BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0x38, 4), 0);
        }

        // 원문은 거부한 버전도 같은 현재 블록으로 즉시 Run한다.
        Need(hooks.Run)();
    }

    /// <summary>0x1d의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.947에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 진단·버전·Run·상태·구독자 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientChallengeLoginReplyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        // 40b380은 비NULL 빈 observer이며 원문 고정 길이는 13이다.
        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, FixedLength);
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
