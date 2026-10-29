namespace Netstorm.Client.Gump; // 편집 버퍼 객체와 별도 본문/줄 표의 실제 소유 메모리 영역이다.

/// <summary>cpppj/src/client/TextEntryStorage.cpp의 안정된 객체·32비트 내부 주소·실제 재할당/반납.</summary>
public sealed class TextEntryStorage
{
    /// <summary>원본 요청 폭과 실제 CLR 배열.요청0도 물리1바이트를 소유하되 뷰는0바이트다.</summary>
    /// <param name="Bytes">실제 소유 배열.</param><param name="Size">원본 요청 폭.</param>
    private sealed record Block(byte[] Bytes, uint Size);
    /// <summary>같은 실제 편집 버퍼 객체.내부 상태의 주소는 새 삽입으로 바뀌지 않는다.</summary>
    private readonly Dictionary<uint, TextEntryBuffer> _buffers = [];
    /// <summary>같은 물리 본문/줄 표와 주소 순서.</summary>
    private readonly SortedList<uint, Block> _blocks = [];
    /// <summary>원본 포인터를 절단하지 않는 단조 증가 가상 주소.</summary>
    private uint _nextAddress = 0x20000000;
    /// <summary>도달할 때만 요구하는 실제 클립보드 읽기.</summary>
    private readonly TextEntryClipboardRead? _read;
    /// <summary>도달할 때만 요구하는 실제 클립보드 쓰기.</summary>
    private readonly TextEntryClipboardWrite? _write;
    /// <summary>장치 수명을 빌린다.선택 전에는 클립보드 연결을 요구하지 않는다.</summary>
    public TextEntryStorage(TextEntryClipboardRead? read = null, TextEntryClipboardWrite? write = null) { _read = read; _write = write; }
    /// <summary>새0x28 객체 토큰에 고정 상태/실제 장치를 연결한다.Construct는 호출자가 실행한다.</summary>
    public TextEntryBuffer Create(uint token)
    { if (token == 0 || _buffers.ContainsKey(token)) throw new InvalidOperationException("편집 버퍼 객체 토큰 중복"); var buffer = new TextEntryBuffer(new(), new() { Allocate = Allocate, Release = Release, Resize = Resize, Bytes = Bytes, ClipboardRead = target => (_read ?? throw new InvalidOperationException("실제 클립보드 읽기 미연결"))(target), ClipboardWrite = source => (_write ?? throw new InvalidOperationException("실제 클립보드 쓰기 미연결"))(source) }); _buffers.Add(token, buffer); return buffer; }
    /// <summary>살아 있는 실제 편집 버퍼만 반환한다.</summary>
    public TextEntryBuffer? Find(uint token) => _buffers.GetValueOrDefault(token);
    /// <summary>본문/줄 표 반납을 마친 뒤에만 객체 기록을 반납한다.</summary>
    public void ReleaseBuffer(uint token)
    { if (!_buffers.TryGetValue(token, out var buffer) || buffer.State.Data != 0 || buffer.State.Offsets != 0) throw new InvalidOperationException("편집 버퍼 본문 반납 전 객체 해제"); _buffers.Remove(token); }
    /// <summary>임의0/sentinel로 채우지 않고 CLR의 미초기화 배열 API로 확보한다.CLR/OS가 돌려주는0도 그대로 보존한다.</summary>
    public uint Allocate(uint size)
    { ulong step = ((ulong)size + 31) & ~15UL; if ((ulong)_nextAddress + step >= 0x60000000UL) throw new OverflowException("편집 메모리 주소 범위 소진"); byte[] bytes; try { bytes = GC.AllocateUninitializedArray<byte>((int)Math.Max(size, 1)); } catch (OutOfMemoryException) { return 0; } uint address = _nextAddress; _nextAddress = unchecked(_nextAddress + (uint)step); _blocks.Add(address, new(bytes, size)); return address; }
    /// <summary>할당 시작만 반납하고0은 허용한다.내부 포인터를 잘못 free하지 않는다.</summary>
    public void Release(uint address) { if (address != 0 && !_blocks.Remove(address)) throw new InvalidOperationException("편집 메모리 해제 주소 불일치"); }
    /// <summary>새 실제 저장소가 성공한 뒤 내용 보존→이전 반납이다.0크기는 실제 반납 후0을 반환한다.</summary>
    public uint Resize(uint address, uint size)
    { if (address == 0) return Allocate(size); if (!_blocks.TryGetValue(address, out var previous)) throw new InvalidOperationException("편집 메모리 재할당 주소 불일치"); if (size == 0) { Release(address); return 0; } uint next = Allocate(size); if (next == 0) return 0; int count = (int)Math.Min(size, previous.Size); previous.Bytes.AsSpan(0, count).CopyTo(_blocks[next].Bytes); Release(address); return next; }
    /// <summary>같은 실제 할당의 내부 주소 뒤 범위를 반환한다.정확한 끝 주소는 빈 범위다.</summary>
    public Memory<byte> Bytes(uint address)
    {
        int left = 0, right = _blocks.Count;
        // 주소보다 큰 첫 시작 위치를 이진 탐색해 바로 앞 소유 블록을 찾는다.
        while (left < right) { int middle = left + (right - left) / 2; if (_blocks.Keys[middle] <= address) left = middle + 1; else right = middle; } int index = left - 1; if (index < 0) throw new ArgumentOutOfRangeException(nameof(address)); uint offset = unchecked(address - _blocks.Keys[index]); var block = _blocks.Values[index]; if (offset > block.Size) throw new ArgumentOutOfRangeException(nameof(address)); return block.Bytes.AsMemory((int)offset, (int)(block.Size - offset));
    }
    /// <summary>게임 값을 바꾸지 않는 현재 편집 객체 수.</summary>
    public int BufferCount => _buffers.Count;
    /// <summary>본문/줄 표/제외 문자의 현재 실제 할당 수.</summary>
    public int AllocationCount => _blocks.Count;
}
