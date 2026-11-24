using Netstorm.Core.Original;
using Netstorm.Net;

// 0x25 메타 항해 오류의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaSailError.cpp 의 ReceiveNativeMetaSailError 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x25 등록을 이식한다.
// 0.956에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x25 메타 항해 오류가 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaSailErrorHooks
{
    /// <summary>같은 실제 CS 참가자40 기록 owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
    public UIntBox? Subscriber { get; set; }

    /// <summary>오류 1~4의 원문을 한 번 번역한 표시 장치다.</summary>
    public Func<string, string>? Translate { get; set; }

    /// <summary>현재 기록+b8의 같은 객체에 안내를 붙인다.</summary>
    public Action<uint, string>? Note { get; set; }

    /// <summary>오류 5~7의 원문 Tell 섹션을 실행한다.</summary>
    public Action<string>? Tell { get; set; }

    /// <summary>마지막 현재 기록+0 식별자를 ff로 표시한다.</summary>
    public Action<uint, uint>? Update { get; set; }
}

// 25h 항해 실패의 첫 자기 참가자 선택과 원문 안내/Tell/표시 순서를 복원한다.
public static class ClientMetaSailError
{
    /// <summary>원문 0x25 메타 항해 오류 종류다. 0.956에서 추가됐다.</summary>
    public const uint Type = 0x25;

    /// <summary>원문 0x25 수신의 실제 고정 F 주소다. 0.956에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b47e0;

    /// <summary>원문 0x25 수신의 실제 고정 길이다. 0.956에서 추가됐다.</summary>
    public const ushort FixedLength = 8;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.956에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 항해 오류의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.956에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 항해 오류의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.956에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 항해 오류의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>번역/Tell 뒤의 실제 기록 재조회 순서와 미일치의 부분 효과를 유지한다. 0.956에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·구독자·번역·안내·Tell·표시 장치다.</param>
    /// <param name="packet">원문 0x25 패킷이다.</param>
    public static void Receive(ClientMetaSailErrorHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        uint owner = Need(hooks.Subscriber).Value;
        ReadOnlySpan<byte> bytes = packet.Span;
        Memory<byte> record = default;
        // 원문 40기록의 첫 구독자만 검색하며 0/비활성도 제외하지 않는다.
        for (uint index = 0; index < RawChallengeState.ParticipantCount; index++)
        {
            Memory<byte> candidate = challenge.Participant(index);
            if (Read(candidate.Span, 4) == owner)
            {
                record = candidate;
                break;
            }
        }

        string message = string.Empty;
        string section = string.Empty;
        // 선택된 분기의 정적 원문 문자열이다.
        switch (Read(bytes, 6, 2))
        {
            case 1: message = "You have been kicked off!"; break;
            case 2: message = "ERROR: Invalid battle id"; break;
            case 3: message = "ERROR: Invalid battle slot id"; break;
            case 4: message = "That battle began before you got there"; break;
            case 5: section = "TooManyPirates"; break;
            case 6: section = "BattleVersionMismatch"; break;
            case 7: section = "BattleFailedDiags"; break;
            default: break;
        }

        if (message.Length != 0)
        {
            string text = NeedDelegate(hooks.Translate)(message);
            NeedDelegate(hooks.Note)(Read(record.Span, 0xb8), text);
        }
        else if (section.Length != 0)
        {
            NeedDelegate(hooks.Tell)(section);
        }

        NeedDelegate(hooks.Update)(Read(record.Span, 0), 0xffu);
    }

    /// <summary>0x25의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.956에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 CS·구독자·번역·안내·Tell·표시 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaSailErrorHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        // 40b380은 비NULL 빈 observer이며 원문 고정 길이는 8이다.
        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, FixedLength);
        // 모드/CS0/known/phase/송신자 선행 관문이 없는 원문 F다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
