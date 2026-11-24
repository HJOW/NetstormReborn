using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x88 메타 참가자 이름의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaParticipantName.cpp 의 WriteNativeMetaParticipantLabel/RebuildNativeMetaParticipantLabel/RefreshNativeMetaPlayers/ReceiveNativeMetaParticipantName 과
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x88 등록을 이식한다.
// 0.955에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x88 메타 참가자 이름이 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaParticipantNameHooks
{
    /// <summary>같은 실제 CS 참가자40 기록 owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>원문 표시 부호화의 실제 번역 장치다.</summary>
    public Func<string, string>? Translate { get; set; }

    /// <summary>같은 Core 10BYTE의 실제 등급 합계 장치다.</summary>
    public Func<ReadOnlyMemory<byte>, byte>? Weight { get; set; }

    /// <summary>같은 실제 참가자 표시 갱신 장치다.</summary>
    public Action<uint, uint>? Update { get; set; }

    /// <summary>같은 실제 NSInfo 갱신 장치다.</summary>
    public Action? RefreshInfo { get; set; }
}

// 004b4680 전체이며 메타 모드/송신자/known/현재 phase 관문을 추가하지 않는다.
public static class ClientMetaParticipantName
{
    /// <summary>원문 0x88 메타 참가자 이름 종류다. 0.955에서 추가됐다.</summary>
    public const uint Type = 0x88;

    /// <summary>원문 0x88 수신의 실제 고정 F 주소다. 0.955에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b4680;

    /// <summary>원문 0x88 수신의 실제 고정 길이다. 0.955에서 추가됐다.</summary>
    public const ushort FixedLength = 98;

    /// <summary>참가자 기록의 실제 이름표 시작 위치다. 0.955에서 추가됐다.</summary>
    private const int LabelOffset = 0xd8;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.955에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 참가자 이름의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.955에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 참가자 이름의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.955에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 이름의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>고정 필드명과 무관하게 원문처럼 첫 NUL까지만 읽는다. 0.955에서 추가됐다.</summary>
    private static string Text(ReadOnlySpan<byte> bytes, int offset)
    {
        // 시작 범위 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 문자열 시작 범위");
        }

        // 첫 NUL 뒤 stale BYTE는 읽거나 정리하지 않는다.
        int end = bytes.Slice(offset).IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 문자열 NUL 물리 부족");
        }

        return Encoding.Latin1.GetString(bytes.Slice(offset, end));
    }

    /// <summary>rank 인자는 weight 호출 전 push되어 남는 원문 스택 인자다. 0.955에서 추가됐다.</summary>
    /// <param name="record">같은 실제 참가자 기록이다.</param>
    /// <param name="prefix">길드 접두사다.</param>
    /// <param name="hooks">실제 번역·등급 장치다.</param>
    public static void Write(Memory<byte> record, string prefix, ClientMetaParticipantNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Span<byte> bytes = record.Span;
        string warning = (Read(bytes, 0x64) & 0x800u) != 0 ? "~[IfortGump.L1]" : "";
        string pattern = NeedDelegate(hooks.Translate)("~j%s%s~w%s\n~yLevel ~w%d ~yRank ~w%d\n");
        uint rank = Read(bytes, 0x58, 1) + 1u;
        byte level = NeedDelegate(hooks.Weight)(record.Slice(0x54, 10));
        string name = Text(bytes, 0x26);
        string guild = prefix ?? string.Empty;
        Memory<byte> destination = record.Slice(LabelOffset);
        for (;;)
        {
            // 알려진 버그 NB09: 최대 길이를 넘는 길드 태그는 원문에서 이름표 버퍼를 넘어 종료를 일으켰다. 태그 끝 글자를 한 글자씩 줄여 버퍼에 맞춘다.
            // 서식은 %s 세 개·%d 두 개 순서이며 다른 변환 지정은 원문에 없다.
            string formatted = ReplaceFirst(ReplaceFirst(ReplaceFirst(pattern, "%s", guild), "%s", warning), "%s", name);
            formatted = ReplaceFirst(ReplaceFirst(formatted, "%d", "{0}"), "%d", "{1}");
            formatted = string.Format(System.Globalization.CultureInfo.InvariantCulture, formatted, level, rank);
            byte[] encoded = Encoding.Latin1.GetBytes(formatted);
            if (encoded.Length < destination.Length)
            {
                encoded.CopyTo(destination);
                destination.Span[encoded.Length] = 0;
                return;
            }

            if (guild.Length > 3 && guild.EndsWith(':'))
            {
                guild = guild.Remove(guild.Length - 2, 1);
            }
            else if (guild.Length != 0)
            {
                guild = string.Empty;
            }
            else
            {
                throw new ArgumentOutOfRangeException(nameof(record), "메타 참가자 이름표의 실제 CS 문자열 물리 초과");
            }
        }
    }

    /// <summary>자리 표시자 치환을 순서대로 한 번씩만 수행한다. 0.955에서 추가됐다.</summary>
    private static string ReplaceFirst(string pattern, string placeholder, string value)
    {
        int at = pattern.IndexOf(placeholder, StringComparison.Ordinal);
        return at < 0 ? pattern : pattern[..at] + value + pattern[(at + placeholder.Length)..];
    }

    /// <summary>길드 접두사는 기록+8을 사용하며 참가자의 정제된 이름+26과 구별한다. 0.955에서 추가됐다.</summary>
    /// <param name="record">같은 실제 참가자 기록이다.</param>
    /// <param name="hooks">실제 번역·등급 장치다.</param>
    public static void Rebuild(Memory<byte> record, ClientMetaParticipantNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        string prefix = "~a" + Text(record.Span, 8) + ':';
        if (record.Span[8] == 0)
        {
            prefix = string.Empty;
        }

        Write(record, prefix, hooks);
    }

    /// <summary>표시/NSInfo 장치가 CS를 교체해도 다음 반복은 같은 owner의 현재 할당을 다시 읽는다. 0.955에서 추가됐다.</summary>
    /// <param name="hooks">실제 참가자·번역·등급·표시·NSInfo 장치다.</param>
    public static void Refresh(ClientMetaParticipantNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        // 원문 0..39의 현재 active BYTE만 검사하며 이름표/표시 호출 뒤 그 기록을 다시 읽지 않는다.
        for (uint index = 0; index < RawChallengeState.ParticipantCount; index++)
        {
            Memory<byte> record = challenge.Participant(index);
            if ((record.Span[0x64] & 1u) == 0)
            {
                continue;
            }

            Rebuild(record, hooks);
            NeedDelegate(hooks.Update)(index, 0xffu);
            NeedDelegate(hooks.RefreshInfo)();
        }
    }

    /// <summary>미발견 subscriber는 호스트 오류로 표시하고 active가 0이면 입력 이름/장치를 읽지 않는다. 0.955에서 추가됐다.</summary>
    /// <param name="hooks">실제 참가자·번역·등급·표시·NSInfo 장치다.</param>
    /// <param name="packet">원문 0x88 패킷이다.</param>
    public static void Receive(ClientMetaParticipantNameHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        ReadOnlySpan<byte> bytes = packet.Span;
        uint? found = challenge.FindParticipantBySubscriber(Read(bytes, 4));
        if (found is null)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "88h 구독자의 실제 CS 참가자 없음");
        }

        Memory<byte> record = challenge.Participant(found.Value);
        if ((record.Span[0x64] & 1u) == 0)
        {
            return;
        }

        // strcpy처럼 같은 BYTE를 읽고 바로 쓴 뒤 NUL을 검사하며 길이/송신자 선행 필터를 추가하지 않는다.
        int offset = 0;
        for (;;)
        {
            uint value = Read(bytes, 8 + offset, 1);
            if (8 + offset >= record.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "88h 이름 복사의 실제 CS 물리 초과");
            }

            record.Span[8 + offset] = (byte)value;
            if (value == 0)
            {
                break;
            }

            offset++;
        }

        Rebuild(record, hooks);
        NeedDelegate(hooks.Update)(Read(record.Span, 0), 0xffu);
        NeedDelegate(hooks.RefreshInfo)();
    }

    /// <summary>0x88의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.955에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 참가자·번역·등급·표시·NSInfo 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaParticipantNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        // 40b380은 비NULL 빈 observer이며 원문 고정 길이는 98이다.
        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, FixedLength);
        // 88h에는 메타 모드/송신자/known/현재 phase 관문을 넣지 않는다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
