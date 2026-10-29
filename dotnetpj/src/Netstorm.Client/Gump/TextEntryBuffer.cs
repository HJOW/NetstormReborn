using System.Buffers.Binary;

namespace Netstorm.Client.Gump; // 표시 조각 버퍼와 구별한 원본 문자 편집/선택/LF 줄 표 영역이다.

/// <summary>원본0x28바이트의 실제10DWORD.포인터는 같은 가상 주소 공간의 주소+offset을 허용한다.</summary>
public sealed class TextEntryBufferState
{
    /// <summary>원본 할당 가상 표.</summary>
    public uint Vtable { get; set; }
    /// <summary>현재 줄offset 배열 주소.</summary>
    public uint Offsets { get; set; }
    /// <summary>줄 표의 현재 DWORD 칸 수.</summary>
    public int OffsetCapacity { get; set; }
    /// <summary>현재 표시 줄 수.</summary>
    public int LineCount { get; set; }
    /// <summary>비0이면 LF 줄 표를 다시 만든다.</summary>
    public uint Dirty { get; set; }
    /// <summary>정렬된 선택 시작.</summary>
    public int SelectionBegin { get; set; }
    /// <summary>정렬된 선택 끝.</summary>
    public int SelectionEnd { get; set; }
    /// <summary>원본 본문 길이.물리 할당폭과 구별한다.</summary>
    public int Length { get; set; }
    /// <summary>원본 본문 용량 필드.</summary>
    public int Capacity { get; set; }
    /// <summary>현재 NUL 본문 주소.</summary>
    public uint Data { get; set; }
}
/// <summary>최대256바이트 원본 NUL 클립보드 입력.</summary>
public delegate void TextEntryClipboardRead(Span<byte> target);
/// <summary>선택 또는 전체 본문의 명시 길이 클립보드 출력.</summary>
public delegate void TextEntryClipboardWrite(ReadOnlySpan<byte> source);
/// <summary>실제 소유 메모리와 순수 주소 조회/현재 클립보드 효과.</summary>
public sealed class TextEntryBufferHooks
{
    /// <summary>가상+0 할당.0은 실패다.</summary>
    public Func<uint, uint>? Allocate { get; set; }
    /// <summary>가상+4 반납.0도 원본처럼 전달한다.</summary>
    public Action<uint>? Release { get; set; }
    /// <summary>가상+8 재할당.실패는 이전 메모리를 보존한다.</summary>
    public Func<uint, uint, uint>? Resize { get; set; }
    /// <summary>base 또는 내부 주소 뒤의 현재 남은 물리 범위.조회 부작용은 없다.</summary>
    public Func<uint, Memory<byte>>? Bytes { get; set; }
    /// <summary>현재 클립보드 읽기.도달한 분기에서만 필수다.</summary>
    public TextEntryClipboardRead? ClipboardRead { get; set; }
    /// <summary>현재 클립보드 쓰기.도달한 분기에서만 필수다.</summary>
    public TextEntryClipboardWrite? ClipboardWrite { get; set; }
}
/// <summary>cpppj/src/client/TextEntryBuffer.cpp의 원시 주소·현재 재조회·부분 실패·선택/LF·복사 순서.</summary>
public sealed class TextEntryBuffer
{
    /// <summary>원본 편집 버퍼 가상 표.</summary>
    private const uint BufferVtable = 0x508da8;
    /// <summary>원본 기본 본문 용량 필드/요청 크기.</summary>
    private const uint InitialData = 0x800;
    /// <summary>원본 줄 표 최초/증가 할당 크기.</summary>
    private const uint OffsetBytes = 0x200;
    /// <summary>같은 실제0x28 필드.</summary>
    public TextEntryBufferState State { get; }
    /// <summary>현재 실제 소유 메모리/클립보드.</summary>
    private readonly TextEntryBufferHooks _hooks;
    /// <summary>실제 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("편집 버퍼 장치 경계 필요");
    /// <summary>같은 상태와 필수 메모리 장치를 빌린다.클립보드는 선택 전 요구하지 않는다.</summary>
    public TextEntryBuffer(TextEntryBufferState state, TextEntryBufferHooks hooks)
    { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Allocate); Require(hooks.Release); Require(hooks.Resize); Require(hooks.Bytes); State = state; _hooks = hooks; }
    /// <summary>원본 signed 복사 길이만 사용한다.</summary>
    private static int Count(int value) => value >= 0 ? value : throw new ArgumentOutOfRangeException(nameof(value), "편집 버퍼 음수 메모리 길이");
    /// <summary>원시 주소 합은 부호 확장 없이32비트에서 감긴다.</summary>
    private static uint Address(uint address, int offset) => unchecked(address + (uint)offset);
    /// <summary>실제 현재 주소의 필요한 물리 폭만 빌린다.0길이는 조회하지 않는다.</summary>
    private Span<byte> Memory(uint address, int count)
    { if (count == 0) return []; var bytes = address != 0 ? _hooks.Bytes!(address).Span : []; if (count < 0 || count > bytes.Length) throw new ArgumentOutOfRangeException(nameof(count), "편집 버퍼 현재 메모리 범위"); return bytes[..count]; }
    /// <summary>정렬되지 않은 실제 little endian DWORD.</summary>
    private uint Read(uint address) => BinaryPrimitives.ReadUInt32LittleEndian(Memory(address, 4));
    /// <summary>같은 현재 주소에 little endian DWORD를 쓴다.</summary>
    private void Write(uint address, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Memory(address, 4), value);
    /// <summary>현재 주소의 첫NUL 전 LF를 찾는다.원문 strchr처럼 정규화하지 않는다.</summary>
    private uint FindNewline(uint address)
    {
        var bytes = address != 0 ? _hooks.Bytes!(address).Span : [];
        // 현재 포인터의 남은 물리 범위만 검사한다.
        for (int i = 0; i < bytes.Length; i++) { if (bytes[i] == 0) return 0; if (bytes[i] == 10) return unchecked(address + (uint)i); } throw new ArgumentOutOfRangeException(nameof(address), "편집 버퍼 줄 검색 NUL 범위");
    }
    /// <summary>빈 입력은 원본null이다.나머지는 NUL이 포함된 범위를 빌린다.</summary>
    private static int CStringLength(ReadOnlySpan<byte> text) { if (text.IsEmpty) return 0; int end = text.IndexOf((byte)0); return end >= 0 ? end : throw new ArgumentException("편집 버퍼 NUL 입력 범위 필요"); }
    /// <summary>REP MOVSD→MOVSB처럼4바이트를 먼저 읽고 쓰며 겹침도 해당 순서를 유지한다.</summary>
    private static void ForwardCopy(Span<byte> target, ReadOnlySpan<byte> source, int count)
    {
        if (count < 0 || count > target.Length || count > source.Length) throw new ArgumentOutOfRangeException(nameof(count)); int offset = 0;
        // 한 DWORD의 읽기를 완료한 뒤 현재 목적지에 쓴다.
        for (; count - offset >= 4; offset += 4) { uint word = BinaryPrimitives.ReadUInt32LittleEndian(source[offset..]); BinaryPrimitives.WriteUInt32LittleEndian(target[offset..], word); }
        // 마지막0~3바이트도 앞에서부터 읽고 쓴다.
        for (; offset < count; offset++) target[offset] = source[offset];
    }
    /// <summary>요청0은2048바이트이며 성공한 capacity는 요청과 무관하게2048이다.첫 본문 바이트는 쓰지 않는다.</summary>
    public void Construct(uint requested = 0)
    { State.Vtable = BufferVtable; State.SelectionBegin = State.SelectionEnd = 0; State.Capacity = 0; uint data = _hooks.Allocate!(requested != 0 ? requested : InitialData); State.Data = data; if (data != 0) State.Capacity = (int)InitialData; State.Length = 0; State.SelectionBegin = State.SelectionEnd = 0; uint offsets = _hooks.Allocate!(OffsetBytes); State.LineCount = 0; State.Offsets = offsets; State.OffsetCapacity = 128; State.Dirty = 1; }
    /// <summary>data0이면 줄 표도 반납하지 않는다.본문 free→현재 data/capacity0→현재offset free→0 순서다.</summary>
    public void Destroy()
    { State.Vtable = BufferVtable; if (State.Data == 0) return; _hooks.Release!(State.Data); State.Data = 0; State.Capacity = 0; _hooks.Release!(State.Offsets); State.Offsets = 0; }
    /// <summary>양끝을 정렬하고 현재length로 제한한다.</summary>
    public void Select(int begin, int end)
    { if (end < begin) (begin, end) = (end, begin); if (begin < 0) begin = 0; else if (State.Length < begin) begin = State.Length; if (end < begin) end = begin; else if (State.Length < end) end = State.Length; State.SelectionBegin = begin; State.SelectionEnd = end; }
    /// <summary>입력 길이/선택 폭은 고정하고 재할당 뒤 현재 본문/선택 끝/길이를 다시 읽어 선택을 교체한다.</summary>
    public void Insert(ReadOnlySpan<byte> text = default)
    {
        int size = CStringLength(text), added = size, removed = unchecked(State.SelectionEnd - State.SelectionBegin);
        if (removed < added) { int required = unchecked(State.Length - removed + added); if (State.Capacity < required) { uint capacity = required > 0 ? unchecked(((((uint)State.Length - (uint)removed + (uint)added - 1U) >> 10) + 1U) << 10) : 0; uint data = _hooks.Resize!(State.Data, capacity); if (data == 0) return; State.Data = data; State.Capacity = unchecked((int)capacity); } }
        if (removed != added) { uint source = Address(State.Data, State.SelectionEnd), destination = Address(source, unchecked(added - removed)); int count = Count(unchecked(State.Length - State.SelectionEnd)); if (count != 0) Memory(source, count).CopyTo(Memory(destination, count)); State.Length = unchecked(State.Length + added - removed); }
        ForwardCopy(Memory(Address(State.Data, State.SelectionBegin), size), text, size); int selection = unchecked(added + State.SelectionBegin); uint end = Address(State.Data, State.Length); State.SelectionEnd = selection; State.SelectionBegin = selection; Memory(end, 1)[0] = 0; State.Dirty = 1;
    }
    /// <summary>현재 본문 첫NUL/길이/선택/dirty를 쓴 뒤 같은 입력 참조로 Insert한다.자기 본문 alias도 유지한다.</summary>
    public void Reset(ReadOnlySpan<byte> text = default) { Memory(State.Data, 1)[0] = 0; State.Length = 0; State.SelectionBegin = State.SelectionEnd = 0; State.Dirty = 1; Insert(text); }
    /// <summary>LF 뒤의 원시 줄offset/마지막length를 쓴다.확장 실패는 현재 줄 수/dirty/앞선 쓰기를 보존한다.</summary>
    public bool EnsureLines()
    {
        if (State.Dirty == 0) return true; State.LineCount = 1; Write(State.Offsets, 0); uint newline = FindNewline(State.Data);
        // 검색 포인터는 할당 콜백 중에도 고정하며 다음 검색에 같은 주소를 재사용한다.
        while (newline != 0) { newline = unchecked(newline + 1); Write(unchecked(State.Offsets + (uint)State.LineCount * 4), unchecked(newline - State.Data)); State.LineCount = unchecked(State.LineCount + 1); if (State.LineCount >= State.OffsetCapacity) { uint offsets = _hooks.Allocate!(unchecked((uint)State.OffsetCapacity * 4 + OffsetBytes)); if (offsets == 0) return false; int count = checked((int)unchecked((uint)State.OffsetCapacity * 4)); ForwardCopy(Memory(offsets, count), Memory(State.Offsets, count), count); _hooks.Release!(State.Offsets); int capacity = State.OffsetCapacity; State.Offsets = offsets; State.OffsetCapacity = unchecked(capacity + 128); } newline = FindNewline(newline); }
        Write(unchecked(State.Offsets + (uint)State.LineCount * 4), unchecked((uint)State.Length)); State.Dirty = 0; return true;
    }
    /// <summary>EnsureLines 실패와 무관하게 현재 줄 수를 반환한다.</summary>
    public int LineCount() { EnsureLines(); return State.LineCount; }
    /// <summary>LF가 있는 행 길이에는LF도 포함한다.마지막offset은 현재length다.</summary>
    public int LineLength(int line) { EnsureLines(); if (line < 0 || line >= State.LineCount) return -1; uint address = unchecked(State.Offsets + (uint)line * 4); return unchecked((int)(Read(address + 4) - Read(address))); }
    /// <summary>줄과NUL을 복사한 뒤에만 선택쌍을 쓴다.행/용량 거부는ffffffff이며 출력을 보존한다.</summary>
    public uint CopyLine(int line, Span<byte> output, Span<int> selection)
    {
        if (selection.Length != 2) throw new ArgumentException("편집 버퍼 선택쌍2개 필요"); EnsureLines(); if (line < 0 || line >= State.LineCount) return uint.MaxValue; uint address = unchecked(State.Offsets + (uint)line * 4); uint count = unchecked(Read(address + 4) - Read(address)); if (output.Length < unchecked((int)count + 1)) return uint.MaxValue; int size = Count(unchecked((int)count)); if (size >= output.Length) throw new ArgumentOutOfRangeException(nameof(output)); ForwardCopy(output, Memory(unchecked(State.Data + Read(address)), size), size); output[size] = 0; int begin = State.SelectionBegin;
        if (begin < State.Length && (unchecked((int)Read(address + 4)) <= begin || State.SelectionEnd < unchecked((int)Read(address)))) { selection[0] = selection[1] = -1; return count; } selection[0] = unchecked((int)Read(address)) < begin ? unchecked(begin - (int)Read(address)) : 0; selection[1] = unchecked((int)Read(address + 4)) <= State.SelectionEnd ? unchecked((int)count) : unchecked(State.SelectionEnd - (int)Read(address)); return count;
    }
    /// <summary>용량 거부는 출력에 손대지 않는다.성공은 실제NUL까지 복사하고 현재length를 반환한다.</summary>
    public uint CopyAll(Span<byte> output)
    { if (output.Length < unchecked(State.Length + 1)) return uint.MaxValue; var data = Data().Span; int end = data.IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(output), "편집 버퍼 본문 NUL 범위"); int size = checked(end + 1); if (size > output.Length) throw new ArgumentOutOfRangeException(nameof(output)); data[..size].CopyTo(output); return unchecked((uint)State.Length); }
    /// <summary>빈 선택은 복사에서만 전체 본문으로 간주하고 저장 선택은 바꾸지 않는다.</summary>
    public void Copy()
    { int begin = State.SelectionBegin, end = State.SelectionEnd; if (begin == end) { begin = 0; end = State.Length; } int size = unchecked(end - begin); if (size <= 0) return; Require(_hooks.ClipboardWrite)(Memory(Address(State.Data, begin), Count(size))); }
    /// <summary>빈 선택은 실제 전체 선택으로 바꾼 뒤 복사→null 삽입→dirty1을 수행한다.</summary>
    public void Cut() { if (State.SelectionBegin == State.SelectionEnd) { State.SelectionBegin = 0; State.SelectionEnd = State.Length; } Copy(); Insert(); State.Dirty = 1; }
    /// <summary>원본256바이트 클립보드에서 제외 문자를 지운 뒤 비어 있지 않으면 삽입한다.빈 입력도dirty1이다.</summary>
    public void Paste(ReadOnlySpan<byte> excluded = default)
    {
        byte[] text = new byte[256]; Require(_hooks.ClipboardRead)(text); int length = CStringLength(text);
        if (!excluded.IsEmpty) { int excludedLength = CStringLength(excluded), index = 0;
            // 문자를 지운 경우 같은 현재 칸을 다시 검사한다.원본 고정 length의 memmove 폭도 유지한다.
            while (index < text.Length && text[index] != 0) { if (!excluded[..excludedLength].Contains(text[index])) index++; else text.AsSpan(index + 1, length - index).CopyTo(text.AsSpan(index, length - index)); } text[^1] = 0;
        }
        if (text[0] != 0) Insert(text); State.Dirty = 1;
    }
    /// <summary>현재 본문의 남은 물리 범위.재할당/해제 뒤에는 이전 뷰를 재사용하지 않는다.</summary>
    public Memory<byte> Data() => State.Data != 0 ? _hooks.Bytes!(State.Data) : Memory<byte>.Empty;
    /// <summary>현재length 앞의 읽기 전용 BYTE 뷰.클립보드/표시는 같은 실제 본문을 읽는다.</summary>
    public ReadOnlySpan<byte> Text() => Memory(State.Data, Count(State.Length));
}
