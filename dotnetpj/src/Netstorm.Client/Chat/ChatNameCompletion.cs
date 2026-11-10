// 장기 후보/접두/순환 상태를 실제 Chat 기록과 연결하는 자동완성 본체다.
// 근거: cpppj `src/client/ChatNameCompletion.h`·`ChatNameCompletion.cpp`(0042e540).
// 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
using System.Text;

namespace Netstorm.Client.Chat;

// 채팅 이름 자동완성 범주를 담는 네임스페이스다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.

// 자동완성의 실제 BSS 배치(00548810..0054c89f)를 보존하는 물리 상태다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
public sealed class ChatNameCompletionState
{
    /// <summary>원문 BSS 전체 크기 0x4090바이트다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    public const int StateBytes = 0x4090;

    /// <summary>원문 BSS 그대로의 바이트 물리다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    public byte[] Bytes { get; } = new byte[StateBytes];
}

// 새 단어일 때만 번역과 현재 100행을 요구하는 외부 효과 묶음이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
public sealed class ChatNameCompletionHooks
{
    /// <summary>0042e9a0의 현재 번역 바이트다. 후보 슬롯 폭으로 자르지 않는다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    public Func<string, string>? Translate { get; set; }

    /// <summary>00558ed0..0055a18f의 실제 현재 100×48바이트 기록이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    public Func<byte[]>? Records { get; set; }

    /// <summary>길이 127 이상인 원문의 354 진단이다. 부르고 나면 복사를 계속한다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    public Action<uint>? Assertion { get; set; }
}

// 채팅 이름 자동완성 수집·순환 본체다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
public static class ChatNameCompletionBody
{
    /// <summary>순환 인덱스의 원문 DWORD 오프셋이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int IndexOffset = 0;

    /// <summary>접두 길이의 원문 DWORD 오프셋이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int LengthOffset = 4;

    /// <summary>접두 버퍼의 원문 오프셋이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int PrefixOffset = 8;

    /// <summary>수집 개수의 원문 DWORD 오프셋이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int CountOffset = 0x88;

    /// <summary>후보 배열의 원문 오프셋이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int CandidatesOffset = 0x90;

    /// <summary>후보 한 칸의 바이트 폭 128이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int SlotBytes = 128;

    /// <summary>후보 최대 128개다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int MaxCandidates = 128;

    /// <summary>현재 Chat 기록 전체 4800바이트(100행×48)다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int RecordsBytes = 4800;

    /// <summary>현재 Chat 기록 행 수 100이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int RecordRows = 100;

    /// <summary>기록 한 행의 바이트 폭 48이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int RecordStride = 48;

    /// <summary>행 안에서 비어 있지 않은지를 보는 오프셋 44다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private const int RecordActiveOffset = 44;

    /// <summary>원문 00531768의 NULL 전 8개 원시 채널 문자열이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    private static readonly string[] ChannelKeys = ["", "", "", "BATTLE", "", "EVERYONE", "ALLIES", "OBSERVER"];

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("이름 자동완성의 선택된 실제 장치 미연결");
        }

        return callback;
    }

    /// <summary>비정렬 원문 DWORD를 그대로 낮은 바이트부터 읽는다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="offset">읽을 바이트 위치다.</param>
    private static uint ReadDword(byte[] bytes, int offset)
    {
        if (offset < 0 || offset + 4 > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "자동완성 DWORD의 실제 물리 범위 오류");
        }

        uint value = 0u;
        // 4바이트를 낮은 자리부터 모으는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원문 고정 DWORD 위치에만 쓴다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="state">쓸 물리 상태다.</param>
    /// <param name="offset">쓸 바이트 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void WriteDword(ChatNameCompletionState state, int offset, uint value)
    {
        if (offset < 0 || offset + 4 > state.Bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "자동완성 DWORD 쓰기의 실제 물리 범위 오류");
        }

        // 낮은 바이트부터 4바이트를 두는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            state.Bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>슬롯 128을 넘어 이어진 원문 C문자열도 같은 물리에서 읽는다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="offset">읽을 바이트 위치다.</param>
    private static string ReadCString(byte[] bytes, int offset)
    {
        if (offset < 0 || offset > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "자동완성 문자열 시작의 실제 물리 범위 오류");
        }

        int end = Array.IndexOf(bytes, (byte)0, offset);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "자동완성 문자열의 실제 NUL 없음");
        }

        return Encoding.Latin1.GetString(bytes, offset, end - offset);
    }

    /// <summary>strcpy처럼 종료 바이트까지 복사한다. 번역을 슬롯 폭으로 잘라내지 않는다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="state">쓸 물리 상태다.</param>
    /// <param name="offset">쓸 바이트 위치다.</param>
    /// <param name="text">쓸 문자열이다. 첫 NUL까지만 본다.</param>
    private static void CopyCString(ChatNameCompletionState state, int offset, string text)
    {
        int nul = text.IndexOf('\0');
        if (nul >= 0)
        {
            text = text.Substring(0, nul);
        }

        if (offset < 0 || offset >= state.Bytes.Length || text.Length >= state.Bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "자동완성 후보 복사의 실제 물리 범위 오류");
        }

        // 종료 바이트까지 Latin1 바이트로 옮기는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
        for (int index = 0; index < text.Length; ++index)
        {
            state.Bytes[offset + index] = (byte)text[index];
        }

        state.Bytes[offset + text.Length] = 0;
    }

    /// <summary>원문 C locale의 ASCII 대소문자 변환이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 바이트 값이다.</param>
    private static byte Lower(byte value) =>
        value >= (byte)'A' && value <= (byte)'Z' ? (byte)(value + (byte)'a' - (byte)'A') : value;

    /// <summary>strnicmp처럼 첫 NUL 또는 명시 길이에서 멈추고 같은지 본다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="state">읽을 물리 상태다.</param>
    /// <param name="candidate">볼 후보 바이트 위치다.</param>
    /// <param name="length">볼 접두 길이다.</param>
    private static bool MatchesPrefix(ChatNameCompletionState state, int candidate, uint length)
    {
        // 명시 길이까지 원문 인접 상태를 읽으며 NUL 뒤에는 손대지 않는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
        for (uint index = 0; index < length; ++index)
        {
            if (candidate + index >= (uint)state.Bytes.Length || PrefixOffset + index >= (uint)state.Bytes.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(candidate), "자동완성 비교의 실제 물리 범위 오류");
            }

            byte left = Lower(state.Bytes[candidate + index]);
            byte right = Lower(state.Bytes[PrefixOffset + index]);
            if (left != right)
            {
                return false;
            }

            if (left == 0)
            {
                return true;
            }
        }

        return true;
    }

    /// <summary>NULL 조회는 저장 후보의 다음/이전 순환이다. 새 조회는 길이 바이트만큼 strncpy하고 나머지 접두는 남긴다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.</summary>
    /// <param name="state">읽고 고칠 물리 상태다. 호출자가 소유한다.</param>
    /// <param name="query">새로 볼 단어다. null이면 저장 후보를 순환한다.</param>
    /// <param name="length">접두로 볼 길이(strncpy 폭)다.</param>
    /// <param name="backward">역방향 순환이면 참이다.</param>
    /// <param name="hooks">부를 외부 효과다. 새 단어일 때만 번역·기록을 요구한다.</param>
    public static string? CompleteChatName(ChatNameCompletionState state, string? query, int length, bool backward, ChatNameCompletionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (query is null)
        {
            WriteDword(state, IndexOffset, unchecked(ReadDword(state.Bytes, IndexOffset) + (backward ? 0xffffffffu : 1u)));
        }
        else
        {
            uint collected = 0u;
            // 번역 전의 빈값/길이를 검사하고 번역 뒤 문자열을 그대로 복사하는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
            foreach (string key in ChannelKeys)
            {
                if (collected >= (uint)MaxCandidates)
                {
                    break;
                }

                if (key.Length == 0 || key.Length >= SlotBytes)
                {
                    continue;
                }

                CopyCString(state, CandidatesOffset + (int)collected * SlotBytes, Need(hooks.Translate)(key));
                ++collected;
            }

            byte[] records = Need(hooks.Records)();
            if (records.Length != RecordsBytes)
            {
                throw new ArgumentOutOfRangeException(nameof(hooks), "자동완성 실제 100행 기록의 바이트 수 오류");
            }

            // 비어 있지 않은 Subscriber의 빈 이름도 원문처럼 포함하는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
            for (int row = 0; row < RecordRows; ++row)
            {
                if (collected >= (uint)MaxCandidates)
                {
                    break;
                }

                if (ReadDword(records, row * RecordStride + RecordActiveOffset) == 0u)
                {
                    continue;
                }

                string name = ReadCString(records, row * RecordStride);
                if (name.Length < SlotBytes)
                {
                    CopyCString(state, CandidatesOffset + (int)collected * SlotBytes, name);
                    ++collected;
                }
            }

            WriteDword(state, CountOffset, collected);
            if (length > 126)
            {
                Need(hooks.Assertion)(0x354u);
            }

            uint width = unchecked((uint)length);
            if (width > (uint)(state.Bytes.Length - PrefixOffset))
            {
                throw new ArgumentOutOfRangeException(nameof(length), "자동완성 strncpy의 실제 물리 범위 오류");
            }

            int nul = query.IndexOf('\0');
            string text = nul >= 0 ? query.Substring(0, nul) : query;
            // strncpy의 NUL 채움과 미작성 접두 꼬리를 구별하는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
            for (uint index = 0; index < width; ++index)
            {
                state.Bytes[PrefixOffset + index] = index < (uint)text.Length ? (byte)text[(int)index] : (byte)0;
            }

            WriteDword(state, LengthOffset, width);
            WriteDword(state, IndexOffset, 0u);
        }

        bool wrapped = false;
        // 정/역방향의 첫 검색과 NULL 후보 순환에 같은 부호 있는 인덱스를 쓰는 반복문이다. 2026-11-11 F3 채팅 이름 자동완성 본체 이식에서 추가됐다.
        for (; ; )
        {
            int index = unchecked((int)ReadDword(state.Bytes, IndexOffset));
            int count = unchecked((int)ReadDword(state.Bytes, CountOffset));
            if (index < 0 || index >= count)
            {
                if (wrapped)
                {
                    return null;
                }

                wrapped = true;
                WriteDword(state, IndexOffset, unchecked((uint)count));
                if (!backward)
                {
                    WriteDword(state, IndexOffset, 0u);
                    continue;
                }
            }
            else if (MatchesPrefix(state, CandidatesOffset + index * SlotBytes, ReadDword(state.Bytes, LengthOffset)))
            {
                return ReadCString(state.Bytes, CandidatesOffset + index * SlotBytes);
            }

            WriteDword(state, IndexOffset, unchecked(ReadDword(state.Bytes, IndexOffset) + (backward ? 0xffffffffu : 1u)));
        }
    }
}
