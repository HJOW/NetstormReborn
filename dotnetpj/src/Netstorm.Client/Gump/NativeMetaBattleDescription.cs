// 전투 설명 수신의 원문 복사/NUL/제어 문자 정제와 256BYTE 확정을 실제 장치에 연결한다.
// cpppj/src/client/NativeMetaBattleDescription.cpp 의 ReceiveNativeMetaBattleDescription(0042cad0)/ConfirmNativeMetaBattleDescription 을 이식한다.
// 0.938에서 R4-6 전송·수신 묶음용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

// 전투 설명 수신이 선택한 효과의 실제 외부 장치다.
public sealed class NativeMetaBattleDescriptionHooks
{
    /// <summary>원문 진단 로그에 쓴다.</summary>
    public Action<string>? Log { get; set; }

    /// <summary>실제 가상 객체 표시를 새로고침한다.</summary>
    public Action<uint>? RefreshObject { get; set; }

    /// <summary>현재 전투 표시를 갱신한다.</summary>
    public Action<uint>? UpdateBattle { get; set; }
}

// 0042cad0 전투 설명 수신 본체다. 메뉴/세션을 요구하지 않는다.
public static class NativeMetaBattleDescription
{
    /// <summary>원문 전투 설명 길이 상한 29h이며 마지막 복사 BYTE를 NUL로 덮어쓴다. 0.938에서 추가됐다.</summary>
    public const uint DescriptionCapacity = 41;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.938에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("전투 설명 수신의 실제 장치 미연결");

    /// <summary>원문 비정렬 실제 DWORD를 낮은 BYTE부터 읽는다. 0.938에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "전투 설명 수신의 실제 읽기 범위");
        }

        uint value = 0;
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원문의 저장 폭만큼 낮은 BYTE부터 쓴다. 0.938에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "전투 설명 수신의 실제 쓰기 범위");
        }

        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>무효 전투는 로그만, 초과 길이는 로그 뒤 현재 번호 갱신만 수행한다. 0.938에서 추가됐다.</summary>
    /// <param name="challenge">같은 실제 CS 전투10 기록이다.</param>
    /// <param name="packet">원문 설명 패킷이다.</param>
    /// <param name="hooks">실제 진단/표시 장치다.</param>
    public static void Receive(ClientChallengeGlobals challenge, ReadOnlyMemory<byte> packet, NativeMetaBattleDescriptionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(hooks);
        Memory<byte> battle = challenge.Battle(Read(packet.Span, 4, 1));
        if ((Read(battle.Span, 0, 1) & 1u) == 0)
        {
            Need(hooks.Log)("ERROR in handleChatBattleUpdate: battle " + Read(packet.Span, 4, 1).ToString(System.Globalization.CultureInfo.InvariantCulture) + " is not valid!\n");
            return;
        }

        uint length = Read(packet.Span, 9, 1);
        if (length > DescriptionCapacity)
        {
            Need(hooks.Log)("ERROR in handleChatBattleUpdate: desc too long\n[bufferLen == 41, optionLen == " + length.ToString(System.Globalization.CultureInfo.InvariantCulture) + "]\n");
            Need(hooks.UpdateBattle)(Read(packet.Span, 4, 1));
            return;
        }

        int at = 0;
        for (uint words = length >> 2; words != 0; --words, at += 4)
        {
            Put(battle.Span, 0xf9 + at, Read(packet.Span, 10 + at));
        }

        for (uint bytes = length & 3u; bytes != 0; --bytes, ++at)
        {
            Put(battle.Span, 0xf9 + at, Read(packet.Span, 10 + at, 1), 1);
        }

        Put(battle.Span, 0xf8 + unchecked((int)Read(packet.Span, 9, 1)), 0, 1);
        at = 0xf9;
        uint value = Read(battle.Span, at, 1);
        while (value != 0)
        {
            switch (value)
            {
                case 7:
                case 8:
                case 12:
                case 10:
                case 13:
                case 9:
                case 11:
                    Put(battle.Span, at, 0x20, 1);
                    break;
                default:
                    break;
            }

            ++at;
            value = Read(battle.Span, at, 1);
        }

        Need(hooks.RefreshObject)(Read(battle.Span, 0x20));
        Need(hooks.UpdateBattle)(Read(packet.Span, 4, 1));
    }

    /// <summary>원문 확정은 첫 LF를 자르고 NUL부터 공백 검사를 수행하므로 후행 공백은 남긴다. 0.938에서 추가됐다.</summary>
    /// <param name="copied">256BYTE 실제 복사 버퍼다.</param>
    public static string Confirm(ReadOnlySpan<char> copied)
    {
        if (copied.Length < 256)
        {
            throw new ArgumentOutOfRangeException(nameof(copied), "설명 확정의 256BYTE 실제 복사 버퍼 부족");
        }

        Span<char> text = stackalloc char[256];
        copied.Slice(0, 256).CopyTo(text);
        text[255] = '\0';
        int end = text.IndexOf('\0');
        int line = text.Slice(0, end).IndexOf('\n');
        return new string(text.Slice(0, line < 0 ? end : line));
    }
}
