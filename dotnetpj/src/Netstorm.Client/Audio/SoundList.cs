using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

// 원본 이름 표의 가변 항목·현재 보고 복귀·명시 핸들을 같은 물리 배열에 보존한다.
namespace Netstorm.Client.Audio;

/// <summary>원본 bigSoundList의0x8000BYTE 이름/머리/복제 사슬 저장소다.</summary>
public sealed class SoundList
{
    /// <summary>보고 줄/기존 이름 견고성 검사가 선택하는 판본이다.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>선택적 정상 복귀 원문 보고다.</summary>
    private readonly Action<string, int>? _report;
    /// <summary>주소 고정 원본 전체 예약이며 항목마다 정렬 여백을 추가하지 않는다.</summary>
    private readonly byte[] _raw = new byte[SoundConstants.ListBytes];
    /// <summary>현재 첫 빈 바이트 오프셋이다.</summary>
    private uint _firstFree;
    /// <summary>Initialize가 등록한 대체 항목 핸들이다.</summary>
    private uint _fallback;
    /// <summary>이름 표만 한 번 초기화했는지다.</summary>
    private bool _initialized;
    /// <summary>빈 BSS형 표를 만들고 판본/현재 보고 효과를 빌린다.</summary>
    public SoundList(OriginalEdition edition, Action<string, int>? report = null) { _edition = edition; _report = report; }
    /// <summary>장치 초기화와 별개로 원문 대체 이름만 한 번 등록한다.</summary>
    public void Initialize()
    {
        if (_initialized) return;
        _initialized = true; _fallback = Lookup(SoundConstants.FallbackName);
    }
    /// <summary>현재 가변 항목 이름을 NUL/표 끝까지 같은 물리 보기로 빌린다.</summary>
    private ReadOnlySpan<byte> NameAt(uint offset)
    {
        if (offset >= SoundConstants.ListBytes) return [];
        ReadOnlySpan<byte> tail = _raw.AsSpan((int)offset); int end = tail.IndexOf((byte)0); return end >= 0 ? tail[..end] : tail;
    }
    /// <summary>정확히 같은DWORD 필드 폭만 쓴다.</summary>
    private void Put(uint offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(_raw.AsSpan(checked((int)offset)), value);
    /// <summary>선택적 원문 진단이 정상 복귀하면 호출 흐름을 계속한다.</summary>
    private void Report(string expression, int line) => _report?.Invoke(expression, line);
    /// <summary>C 로캘 _stricmp의 ASCII 대문자만 접고 나머지BYTE는 그대로 보존한다.</summary>
    private static byte Fold(byte value) => value >= 65 && value <= 90 ? (byte)(value + 32) : value;
    /// <summary>복사한 이름 대신 같은 현재 물리 보기의BYTE를 비교한다.</summary>
    internal static bool SameName(ReadOnlySpan<byte> left, ReadOnlySpan<byte> right)
    {
        if (left.Length != right.Length) return false;
        // 각 위치의 현재BYTE를 읽어 비교하며 문화권/유니코드 접기를 추가하지 않는다.
        for (int index = 0; index < left.Length; index++) if (Fold(left[index]) != Fold(right[index])) return false;
        return true;
    }
    /// <summary>원본 byte string 경계다. 문자열은1:1 Latin1로 보존하며 null은 표를 읽지 않고0이다.</summary>
    public uint Lookup(string? name) => name is null ? 0 : Lookup(Encoding.Latin1.GetBytes(name));
    /// <summary>순회→한도/보고→이름→머리→현재 빈 위치 순서로 실제 byte 이름을 등록한다.</summary>
    public uint Lookup(ReadOnlySpan<byte> name)
    {
        if (name.IsEmpty || name.IndexOf((byte)0) >= 0) throw new ArgumentException("소리 이름이 비었거나 NUL을 포함합니다", nameof(name));
        uint at = 0;
        // 기존 복제도 원문 물리 순서로 지나가며 같은 이름의 첫 항목을 선택한다.
        while (true)
        {
            ReadOnlySpan<byte> existing = NameAt(unchecked(at + SoundConstants.HeaderBytes));
            if (existing.IsEmpty) break;
            if (_edition == OriginalEdition.Cd1072 && existing[^1] != (byte)'v') Report("s->isRobust()", 0x1b6);
            if (SameName(existing, name)) return unchecked(SoundConstants.ListBase + at);
            at = unchecked(at + SoundConstants.HeaderBytes + (uint)existing.Length + 1);
        }
        if (at >= SoundConstants.ListLimit) return _fallback;
        if (at != _firstFree) Report("s == firstFreeSound", 0x1c7);
        ulong next = (ulong)at + SoundConstants.HeaderBytes + (uint)name.Length + 1;
        if (next > SoundConstants.ListBytes) throw new ArgumentOutOfRangeException(nameof(name), "소리 이름이 표의 끝을 넘습니다");
        name.CopyTo(_raw.AsSpan((int)(at + SoundConstants.HeaderBytes))); _raw[(int)(at + SoundConstants.HeaderBytes + (uint)name.Length)] = 0;
        Put(at, 0); Put(at + 4, SoundConstants.ListBase + at); Put(at + 8, 0); Put(at + 12, 0); Put(at + 20, 0); Put(at + 24, 0);
        _firstFree = (uint)next; return SoundConstants.ListBase + at;
    }
    /// <summary>표 내부 포인터 값에서0x1f를 빼며 결과를 새 항목으로 정규화하지 않는다.</summary>
    public uint LookupPointer(uint pointer)
    {
        if (pointer < SoundConstants.ListBase || pointer - SoundConstants.ListBase >= SoundConstants.ListBytes) throw new ArgumentOutOfRangeException(nameof(pointer), "표 안 소리 이름 포인터가 아닙니다");
        return unchecked(pointer - SoundConstants.PointerBias);
    }
    /// <summary>초기화가 만든 대체 핸들을 반환한다.</summary>
    public uint Fallback() => _fallback;
    /// <summary>첫 빈 원문 포인터 형식의 핸들이다.</summary>
    public uint FirstFree() => unchecked(SoundConstants.ListBase + _firstFree);
    /// <summary>현재 이름을 같은 byte 보기로 빌리며 빈 이름은 물리 끝 표식이다.</summary>
    public ReadOnlySpan<byte> NameBytes(uint entry)
    {
        if (entry < SoundConstants.ListBase || entry - SoundConstants.ListBase >= SoundConstants.ListBytes - SoundConstants.HeaderBytes) throw new ArgumentOutOfRangeException(nameof(entry), "소리 항목 값이 표 밖입니다");
        return NameAt(entry - SoundConstants.ListBase + SoundConstants.HeaderBytes);
    }
    /// <summary>장치의 byte 이름 경계로 전달할 현재 Latin1 보기를 만든다.</summary>
    public string Name(uint entry) => Encoding.Latin1.GetString(NameBytes(entry));
    /// <summary>현재 전체0x8000BYTE를 복사하지 않는 읽기 전용 보기다.</summary>
    public ReadOnlyMemory<byte> Raw() => _raw;
    /// <summary>머리 전체가 들어가는 항목 위치의 현재DWORD를 읽는다.</summary>
    public uint Field(uint entry, SoundField field)
    {
        if (entry < SoundConstants.ListBase || entry - SoundConstants.ListBase > SoundConstants.ListBytes - SoundConstants.HeaderBytes) throw new ArgumentOutOfRangeException(nameof(entry), "소리 항목 값이 표 밖입니다");
        return BinaryPrimitives.ReadUInt32LittleEndian(_raw.AsSpan(checked((int)(entry - SoundConstants.ListBase + (uint)field))));
    }
    /// <summary>같은 물리 범위의 현재DWORD만 쓰며 값의 뜻은 별도로 검사하지 않는다.</summary>
    public void SetField(uint entry, SoundField field, uint value)
    {
        if (entry < SoundConstants.ListBase || entry - SoundConstants.ListBase > SoundConstants.ListBytes - SoundConstants.HeaderBytes) throw new ArgumentOutOfRangeException(nameof(entry), "소리 항목 값이 표 밖입니다");
        Put(entry - SoundConstants.ListBase + (uint)field, value);
    }
    /// <summary>이름→Root/Next/Buffer→이전Next→현재 빈 위치 순서로 복제를 만들며 다른 머리 필드는 보존한다.</summary>
    public uint AppendDuplicate(uint named, uint previous, uint buffer)
    {
        byte[] name = NameBytes(named).ToArray(); uint root = Field(previous, SoundField.Root), at = _firstFree;
        ulong next = (ulong)at + SoundConstants.HeaderBytes + (uint)name.Length + 1;
        if (next > SoundConstants.ListBytes) throw new ArgumentOutOfRangeException(nameof(named), "복제 소리 항목이 표의 끝을 넘습니다");
        name.AsSpan().CopyTo(_raw.AsSpan((int)(at + SoundConstants.HeaderBytes))); _raw[(int)(at + SoundConstants.HeaderBytes + (uint)name.Length)] = 0;
        Put(at + 4, root); Put(at + 8, 0); Put(at, buffer); SetField(previous, SoundField.Next, SoundConstants.ListBase + at);
        _firstFree = (uint)next; return SoundConstants.ListBase + at;
    }
    /// <summary>현재 소리 표의 원문 판본이다.</summary>
    public OriginalEdition Edition => _edition;
}
