using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client; // 같은 Client의 실제 임시 고리 소유자와 VFS 경로 고리 공유를 둔다.

// 같은 Client의 실제 임시 고리와 이전 할당 수명을 메뉴와 Editor에서 공유한다.
public sealed class ClientStaticStrings
{
    /// <summary>실제 할당과 요청 폭을 함께 소유하며 재초기화로 이전 할당을 임의 해제하지 않는다. 0.985에서 추가됐다.</summary>
    private sealed class Allocation
    {
        /// <summary>실제 바이트 소유자다.</summary>
        public byte[] Bytes = [];
        /// <summary>요청 폭이다.</summary>
        public uint Size;
    }

    /// <summary>고정 슬롯과 할당 주소와 콜백을 복제하지 않는다. 같은 네 DWORD와 실제 할당 수명을 소유한다.</summary>
    private readonly SortedDictionary<uint, Allocation> _allocations = new();
    /// <summary>원문 0055a760과 0055b160이며 RawStaticStrings의 가변 할당과 섞지 않는다. 실제 VFS와 소유권을 공유한다. 0.985에서 추가됐다.</summary>
    private readonly RawFileName _fileNames;
    /// <summary>원문 네 DWORD다. 실제 본체와 관측이 같은 객체를 공유한다.</summary>
    private readonly StaticStringGlobals _globals = new();
    /// <summary>호스트 포인터를 절단하지 않는 실제 할당 토큰 발급기다. 최초값은 원문과 같은 0x20000000이다.</summary>
    private uint _nextBase = 0x20000000u;

    /// <summary>최초 네 BSS0만 만들며 실제 Client는 VFS의 기존 경로 고리를 빌린다. 8192 할당은 첫 예약 위치다. 0.985에서 추가됐다.</summary>
    /// <param name="fileNames">VFS와 공유할 원문 10칸 경로 고리다. null이면 독립 BSS0을 만든다.</param>
    public ClientStaticStrings(RawFileName? fileNames = null)
    {
        _fileNames = fileNames ?? new RawFileName();
    }

    /// <summary>같은 네 DWORD를 실제 본체와 관측에 빌린다. 단일 실제 슬롯의 참조다. 0.985에서 추가됐다.</summary>
    public StaticStringGlobals Globals() => _globals;

    /// <summary>별도 고리의 현재 상태와 주소를 그대로 유지한다. HUD 교체와 미션 전환에서 초기화하지 않는 별도 10칸 경로 작성기를 빌린다. 0.985에서 추가됐다.</summary>
    public RawFileName FileNames() => _fileNames;

    /// <summary>같은 고정 owner에 실제 할당과 순수 물리와 진단 장치를 연결한다. 0.985에서 추가됐다.</summary>
    private StaticStringHooks Hooks()
    {
        return new StaticStringHooks
        {
            Allocate = Allocate,
            Memory = Memory,
            Assertion = line => Console.Error.WriteLine("Staticstring 원문 진단:" + line),
        };
    }

    /// <summary>원문 공통 시작은 10240이며 지연 예약의 8192와 구별한다. 재호출도 원문 진단과 재할당 순서다. 0.985에서 추가됐다.</summary>
    /// <param name="size">고정할 전체 크기다.</param>
    public void Initialize(uint size) => RawStaticStrings.InitializeStaticStrings(size, _globals, Hooks());

    /// <summary>호스트 포인터를 절단하지 않는 실제 할당과 비영 원문 주소 토큰이다. 0.985에서 추가됐다.</summary>
    /// <param name="size">요청 폭이다.</param>
    /// <returns>원문 주소 토큰이다.</returns>
    private uint Allocate(uint size)
    {
        ulong next = (ulong)_nextBase + size + 4096u;
        if (next > uint.MaxValue) throw new OverflowException("임시 고리 주소 토큰 소진");
        byte[] bytes = new byte[checked((int)size)];
        uint result = _nextBase;
        _allocations.Add(result, new Allocation { Bytes = bytes, Size = size });
        _nextBase = (uint)next;
        return result;
    }

    /// <summary>현재 할당 끝까지 같은 물리를 빌리며 끝 이후를 사본으로 늘리지 않는다. 0.985에서 추가됐다.</summary>
    /// <param name="address">원문 주소 토큰이다.</param>
    /// <returns>같은 현재 할당의 끝까지 빌린 물리다.</returns>
    private Memory<byte> Tail(uint address)
    {
        uint? found = null;
        // 주소 토큰 이하의 가장 큰 할당 base를 찾는다.
        foreach (uint key in _allocations.Keys)
        {
            if (key <= address) found = key;
            else break;
        }

        if (found is null) throw new ArgumentOutOfRangeException(nameof(address), "임시 고리 실제 할당 없음");
        Allocation allocation = _allocations[found.Value];
        uint offset = unchecked(address - found.Value);
        if (offset > allocation.Size) throw new ArgumentOutOfRangeException(nameof(address), "임시 고리 실제 주소 범위");
        return allocation.Bytes.AsMemory(checked((int)offset));
    }

    /// <summary>해당 원문 접근의 전체 물리 폭만 확인한다. 0.985에서 추가됐다.</summary>
    /// <param name="address">원문 주소 토큰이다.</param>
    /// <param name="size">요청 폭이다.</param>
    /// <returns>같은 물리의 앞 폭이다.</returns>
    private Memory<byte> Memory(uint address, uint size)
    {
        Memory<byte> tail = Tail(address);
        if ((uint)tail.Length < size) throw new ArgumentOutOfRangeException(nameof(size), "임시 고리 원문 접근 물리 폭");
        return tail[..checked((int)size)];
    }

    /// <summary>예약 폭 뒤도 같은 실제 할당 안에서는 쓸 수 있다. 임의 절단하지 않는다. 0.985에서 추가됐다.</summary>
    /// <param name="size">예약할 크기다.</param>
    /// <returns>같은 실제 할당 끝까지 빌린 물리다.</returns>
    public Memory<byte> Reserve(uint size)
    {
        uint address = RawStaticStrings.ReserveStaticString(size, _globals, Hooks());
        return Tail(address);
    }

    /// <summary>실제 CRT의 한 정수 서식 장치와 원문 예약과 별칭 생략을 사용한다. 0.985에서 추가됐다.</summary>
    /// <param name="size">예약할 크기다.</param>
    /// <param name="pattern">같은 고리 안 서식 바이트다. 첫 NUL 앞에서 자른다.</param>
    /// <param name="value">값인자 하나의 정수다.</param>
    /// <returns>예약 결과의 같은 물리 앞부분이다.</returns>
    public ReadOnlyMemory<byte> FormatInteger(uint size, ReadOnlyMemory<byte> pattern, int value)
    {
        uint source = 0x10000000u;
        if (MemoryMarshal.TryGetArray(pattern, out ArraySegment<byte> segment) && segment.Array is not null)
        {
            // 서식 바이트가 속한 같은 고리 할당을 찾아 원문 source 주소를 구한다.
            foreach (KeyValuePair<uint, Allocation> entry in _allocations)
            {
                if (ReferenceEquals(entry.Value.Bytes, segment.Array) && (uint)segment.Offset < entry.Value.Size)
                {
                    source = unchecked(entry.Key + (uint)segment.Offset);
                    break;
                }
            }
        }

        StaticStringHooks hooks = Hooks();
        byte[] patternCopy = pattern.ToArray();
        hooks.Format = (destination, _) =>
        {
            int nul = Array.IndexOf(patternCopy, (byte)0);
            string text = Encoding.Latin1.GetString(patternCopy, 0, nul < 0 ? patternCopy.Length : nul);
            string formatted = FormatSingleInteger(text, value);
            byte[] bytes = Encoding.Latin1.GetBytes(formatted + '\0');
            // 서식 결과와 끝 NUL까지 같은 할당 물리에 순서대로 쓴다.
            for (int index = 0; index < bytes.Length; index++) Memory(unchecked(destination + (uint)index), 1).Span[0] = bytes[index];
        };
        uint result = RawStaticStrings.FormatStaticString(size, source, _globals, hooks);
        return Tail(result);
    }

    /// <summary>원문 완료 안내의 80 예약과 고정 Wrote 서식과 두 파일명을 공유 고리에 쓴다. 80으로 절단하지 않는다. 0.985에서 추가됐다.</summary>
    /// <param name="first">첫 파일명이다. 첫 NUL 앞에서 자른다.</param>
    /// <param name="last">마지막 파일명이다. 첫 NUL 앞에서 자른다.</param>
    /// <returns>같은 고리 물리의 앞부분이다.</returns>
    public ReadOnlyMemory<byte> FormatCaptureRange(string first, string last)
    {
        Memory<byte> bytes = Reserve(80);
        int cutFirst = first.IndexOf('\0');
        int cutLast = last.IndexOf('\0');
        string begin = cutFirst < 0 ? first : first[..cutFirst];
        string end = cutLast < 0 ? last : last[..cutLast];
        string output = $"Wrote {begin} through {end}.\n";
        byte[] encoded = Encoding.Latin1.GetBytes(output + '\0');
        // 고정 Wrote 문장과 끝 NUL까지 같은 할당 물리에 순서대로 쓴다.
        for (int index = 0; index < encoded.Length; index++)
        {
            if ((uint)index >= (uint)bytes.Length) throw new ArgumentOutOfRangeException(nameof(first), "연속 저장 안내의 실제 공유 고리 물리");
            bytes.Span[index] = encoded[index];
        }

        return bytes;
    }

    /// <summary>한 정수 외 CRT 서식은 연결하지 않으며 값인자 하나 초과도 거부한다.</summary>
    /// <param name="pattern">첫 NUL 앞 서식 문자열이다.</param>
    /// <param name="value">값인자다.</param>
    /// <returns>서식 결과 문자열이다.</returns>
    private static string FormatSingleInteger(string pattern, int value)
    {
        MatchCollection matches = Regex.Matches(pattern, "%(?:%|[-+ #0]*[0-9]*(?:\\.[0-9]*)?[diuoxXc])");
        int position = 0;
        int arguments = 0;
        // % 변환 후보마다 원문 위치와 연속 일치를 검사한다.
        foreach (Match match in matches)
        {
            int found = pattern.IndexOf('%', position);
            if (found < 0 || found != match.Index) throw new InvalidOperationException("임시 고리 한 정수 외 CRT 서식 미연결");
            if (match.Value != "%%" && ++arguments > 1) throw new InvalidOperationException("임시 고리 실제 값인자 하나 초과");
            position = match.Index + match.Length;
        }

        int scan = pattern.IndexOf('%', position);
        if (scan >= 0) throw new InvalidOperationException("임시 고리 한 정수 외 CRT 서식 미연결");
        StringBuilder builder = new();
        int cursor = 0;
        bool consumed = false;
        // 서식 하나씩 실제 값으로 치환한다.
        // 서식 조각을 순서대로 실제 값과 리터럴 %로 치환한다.
        foreach (Match match in matches)
        {
            builder.Append(pattern[cursor..match.Index]);
            if (match.Value == "%%") builder.Append('%');
            else
            {
                if (consumed) throw new InvalidOperationException("임시 고리 실제 값인자 하나 초과");
                builder.Append(FormatConversion(match.Value, value));
                consumed = true;
            }

            cursor = match.Index + match.Length;
        }

        builder.Append(pattern[cursor..]);
        return builder.ToString();
    }

    /// <summary>C snprintf의 정수 변환 한 개를 같은 폭과 진법으로 만든다.</summary>
    /// <param name="spec">%로 시작하는 단일 변환이다.</param>
    /// <param name="value">값인자다.</param>
    /// <returns>변환 결과다.</returns>
    private static string FormatConversion(string spec, int value)
    {
        char conversion = spec[^1];
        string body = spec[1..^1];
        bool left = body.Contains('-');
        bool plus = body.Contains('+');
        bool space = body.Contains(' ');
        bool hash = body.Contains('#');
        bool zero = body.Contains('0') && !left;
        int width = 0;
        int precision = -1;
        int cursor = 0;
        // 부호와 플래그 뒤 너비를 읽는다.
        // 플래그 문자들을 건너뛰고 너비 시작을 찾는다.
        while (cursor < body.Length && body[cursor] is '+' or '-' or ' ' or '#' or '0') cursor++;
        int widthStart = cursor;
        // 너비 숫자를 읽는다.
        while (cursor < body.Length && char.IsDigit(body[cursor])) cursor++;
        if (cursor > widthStart) width = int.Parse(body[widthStart..cursor]);
        if (cursor < body.Length && body[cursor] == '.')
        {
            cursor++;
            int precisionStart = cursor;
            // 소수점 뒤 정밀도 숫자를 읽는다.
            while (cursor < body.Length && char.IsDigit(body[cursor])) cursor++;
            precision = cursor > precisionStart ? int.Parse(body[precisionStart..cursor]) : 0;
        }

        string digits;
        bool negative = false;
        switch (conversion)
        {
            case 'd':
            case 'i':
                digits = Math.Abs((long)value).ToString();
                negative = value < 0;
                break;
            case 'u':
                digits = unchecked((uint)value).ToString();
                break;
            case 'o':
                digits = Convert.ToString(unchecked((uint)value), 8);
                break;
            case 'x':
                digits = unchecked((uint)value).ToString("x");
                break;
            case 'X':
                digits = unchecked((uint)value).ToString("X");
                break;
            case 'c':
                return ((char)value).ToString();
            default: throw new InvalidOperationException("임시 고리 한 정수 외 CRT 서식 미연결");
        }

        if (conversion is 'o' or 'x' or 'X' && hash && unchecked((uint)value) != 0)
            digits = conversion == 'o' ? "0" + digits : (conversion == 'x' ? "0x" + digits : "0X" + digits);
        if (precision >= 0 && conversion != 'c')
        {
            // 정밀도 폭까지 앞에 0을 채운다.
            while (digits.Length < precision) digits = "0" + digits;
            if (precision == 0 && value == 0) digits = "";
        }

        string prefix = negative ? "-" : (plus && conversion is 'd' or 'i' ? "+" : (space && conversion is 'd' or 'i' && !negative ? " " : ""));
        string bodyText = prefix + digits;
        if (!left && zero && precision < 0)
        {
            // 0 채움은 부호 뒤 숫자 앞에만 넣는다.
            // 0 채움은 부호 뒤 숫자 앞에만 넣는다.
        while (bodyText.Length < width) bodyText = prefix.Length > 0 ? prefix + "0" + bodyText[prefix.Length..] : "0" + bodyText;
            return bodyText;
        }

        // 너비 폭까지 공백으로 채운다.
        while (bodyText.Length < width) bodyText = left ? bodyText + " " : " " + bodyText;
        return bodyText;
    }
}
