// PATCH Chat 기록 선택과 채널 플래그 변경을 같은 클라이언트 저장소에서 실행한다.
using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Client.Chat;

// Chat 채널 모듈을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// 원본 하위 몸체 진입을 검사에서 관찰한다. 2026-11-07 F2d Player 이식에서 추가됐다.
public enum ChatChannelCall
{
    /// <summary>이름·flags 쓰기 뒤 실제 이름 정제 본체에 진입하는 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    NameChanged,

    /// <summary>이름 정제 뒤 실제 빈 몸체에 진입하는 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    Refresh,
}

// 원본 기록 선택·복사·이름 정제를 복원한 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class RawChatChannels
{
    /// <summary>원본 기록 개수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const int Count = 100;

    /// <summary>한 기록의 바이트 간격이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const int Stride = 0x30;

    /// <summary>같은 실제 기록 저장소의 필수 물리 크기다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const int Bytes = Count * Stride;

    /// <summary>실제 이름 물리 폭이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int NameBytes = 40;

    /// <summary>바이트 플래그 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int FlagsOffset = 0x28;

    /// <summary>DWORD 식별자 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int SubscriberOffset = 0x2c;

    /// <summary>새 기록에서 NUL을 쓰는 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int FreshNulOffset = 0x27;

    /// <summary>이름 정제가 고치는 첫 두 바이트 뒤에 검사하는 금지 표다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private static readonly byte[] Forbidden =
    [
        0xa0, (byte)'%', (byte)'.', (byte)'?', (byte)'=', (byte)'(', (byte)')',
        (byte)'[', (byte)']', (byte)'~', (byte)'"', (byte)'`', (byte)';', (byte)':', (byte)' ',
    ];

    /// <summary>외부 실제 클라이언트가 소유하는 100×48바이트다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly byte[] _records;

    /// <summary>각 Add 첫 비교의 현재 값을 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly Func<uint> _mySubscriber;

    /// <summary>선택 검사 전용 진입 관찰이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly Action<ChatChannelCall, uint>? _observe;

    /// <summary>
    /// 저장소·getter는 처리기보다 오래 살아야 한다. 생성자는 기록을 초기화하지 않는다.
    /// </summary>
    /// <param name="edition">원본 판본이다. PATCH만 받는다.</param>
    /// <param name="records">4800바이트 저장소다.</param>
    /// <param name="mySubscriber">현재 값을 읽는다.</param>
    /// <param name="observe">진입 관찰이다. 비워도 된다.</param>
    /// <exception cref="ArgumentException">Chat 채널의 실제 PATCH 기록·현재 조회 누락이다.</exception>
    public RawChatChannels(OriginalEdition edition, byte[] records, Func<uint> mySubscriber, Action<ChatChannelCall, uint>? observe = null)
    {
        ArgumentNullException.ThrowIfNull(records);
        ArgumentNullException.ThrowIfNull(mySubscriber);
        if (edition != OriginalEdition.Patch1078 || records.Length != Bytes)
        {
            throw new ArgumentException("Chat 채널의 실제 PATCH 기록 크기 오류", nameof(records));
        }

        _records = records;
        _mySubscriber = mySubscriber;
        _observe = observe;
    }

    /// <summary>이 채널이 쓰는 저장소다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public byte[] Records => _records;

    /// <summary>
    /// 현재 자기 식별자·첫 빈칸·기존 식별자와 특수 -1 분기를 원본 검색 순서로 처리한다.
    /// </summary>
    /// <param name="subscriber">고를 식별자다.</param>
    /// <param name="name">쓸 이름이다. 비우면 자동 이름이다.</param>
    /// <param name="flags">쓸 플래그다. 하위 바이트만 쓴다.</param>
    /// <returns>고른 기록 번호다. 없으면 -1이다.</returns>
    public int Add(uint subscriber, string? name, uint flags)
    {
        if (subscriber == _mySubscriber())
        {
            return -1;
        }

        int empty = -1;
        int existing = -1;
        byte incoming = (byte)flags;
        if (subscriber == 0xffffffffu)
        {
            incoming &= 0xef;
            // 첫 빈 식별자만 고르는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Count; ++index)
            {
                if (Subscriber(index) == 0u)
                {
                    empty = index;
                    break;
                }
            }
        }
        else
        {
            // 빈칸 뒤에도 같은 식별자를 끝까지 찾는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Count; ++index)
            {
                uint current = Subscriber(index);
                if (current == subscriber)
                {
                    existing = index;
                    break;
                }

                if (current == 0u && empty == -1)
                {
                    empty = index;
                }
            }
        }

        int chosen = existing != -1 ? existing : empty;
        if (chosen == -1)
        {
            return -1;
        }

        Span<byte> record = _records.AsSpan(chosen * Stride, Stride);
        if (existing == -1)
        {
            // 새 기록에서만 실제 식별자 DWORD를 저장하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < 4; ++index)
            {
                record[SubscriberOffset + index] = (byte)(subscriber >> (index * 8));
            }

            if (name is not null)
            {
                CopyName(record, Encoding.Latin1.GetBytes(name));
            }
            else
            {
                string unknown = "Unknown" + subscriber.ToString("x8") + '\0';
                byte[] bytes = Encoding.Latin1.GetBytes(unknown);
                bytes.CopyTo(record);
            }
        }
        else
        {
            record[0] = 0;
            if (name is not null)
            {
                CopyName(record, Encoding.Latin1.GetBytes(name));
            }

            incoming |= record[FlagsOffset];
        }

        record[FreshNulOffset] = 0;
        record[FlagsOffset] = incoming;
        NameChanged(subscriber, record);
        Refresh();
        Refresh();
        return chosen;
    }

    /// <summary>
    /// 실제 8인자 호출에서 읽히는 1·2·6인자를 같은 바이트 필드에 적용한다.
    /// </summary>
    /// <param name="subscriber">고를 식별자다.</param>
    /// <param name="name">쓸 이름이다.</param>
    /// <param name="channel">쓸 채널 값이다.</param>
    /// <returns>고른 기록 번호다. 없으면 -1이다.</returns>
    public int AddToChannel(uint subscriber, string name, uint channel) => Add(subscriber, name, channel);

    /// <summary>
    /// 식별자가 비제로인 기록에만 flags에서 마스크를 지운다.
    /// </summary>
    /// <param name="mask">지울 마스크다. 하위 바이트만 쓴다.</param>
    public void Clear(uint mask)
    {
        byte keep = (byte)~(byte)mask;
        // 모든 100기록을 방문하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < Count; ++index)
        {
            if (Subscriber(index) != 0u)
            {
                _records[index * Stride + FlagsOffset] &= keep;
            }
        }
    }

    /// <summary>
    /// 현재 점유 행의 이름·flags·패딩·ID를 전부 반납한다.
    /// </summary>
    public void ResetOccupied()
    {
        // 원문처럼 현재 100행을 앞에서부터 순회하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < Count; ++index)
        {
            if (Subscriber(index) != 0u)
            {
                Array.Clear(_records, index * Stride, Stride);
            }
        }
    }

    /// <summary>높은 식별자 비트와 빈 기록 0을 구분하고 정렬되지 않은 기록을 직접 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="index">기록 번호다.</param>
    /// <returns>식별자 DWORD다.</returns>
    private uint Subscriber(int index)
    {
        uint result = 0u;
        // 원본 DWORD의 낮은 바이트부터 네 바이트를 조립하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index2 = 0; index2 < 4; ++index2)
        {
            result |= (uint)_records[index * Stride + SubscriberOffset + index2] << (index2 * 8);
        }

        return result;
    }

    /// <summary>물리 이름 40바이트를 모두 쓰되 입력 첫 NUL 뒤에는 더 읽지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="record">쓸 기록 뷰다.</param>
    /// <param name="name">읽을 이름 바이트다.</param>
    private static void CopyName(Span<byte> record, byte[] name)
    {
        bool ended = false;
        // strncpy의 0 보충과 정확히 40글자 입력을 같은 루프에서 보존하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < NameBytes; ++index)
        {
            if (index >= name.Length || (!ended && name[index] == 0))
            {
                ended = true;
            }

            record[index] = ended ? (byte)0 : name[index];
        }
    }

    /// <summary>금지 문자 삭제와 빈 이름의 대체 쓰기를 수행한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="subscriber">고를 식별자다.</param>
    /// <param name="record">고칠 기록 뷰다.</param>
    private void NameChanged(uint subscriber, Span<byte> record)
    {
        _observe?.Invoke(ChatChannelCall.NameChanged, subscriber);
        int length = 0;
        while (length < NameBytes && record[length] != 0)
        {
            ++length;
        }

        int index = 0;
        // 금지 문자를 지우면 같은 위치를 다시 검사하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        while (index < length)
        {
            if (Array.IndexOf(Forbidden, record[index]) < 0)
            {
                ++index;
                continue;
            }

            // 현재 NUL까지 왼쪽으로 한 칸 옮기는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int next = index; next < length; ++next)
            {
                record[next] = record[next + 1];
            }

            --length;
        }

        record[length] = 0;
        if (length == 0)
        {
            string replacement = "Cheat" + (subscriber & 0xffu).ToString(CultureInfo.InvariantCulture);
            byte[] bytes = Encoding.Latin1.GetBytes(replacement);
            bytes.CopyTo(record);
            record[bytes.Length] = 0;
        }
    }

    /// <summary>실제 빈 몸체를 호출하며 관찰만 남긴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private void Refresh() => _observe?.Invoke(ChatChannelCall.Refresh, 0u);
}
