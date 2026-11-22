namespace Netstorm.Core.Original; // 원문 DWORD 산술과 실제 FILE/키 장치를 분리한다.

/// <summary>Source RawBaseFileState의 실제 여섯 필드이며24BYTE ABI로 직렬화하지 않는다.</summary>
public sealed class RawBaseFileState
{
    /// <summary>현재 호스트 FILE 토큰이다.</summary>
    public nuint File;
    /// <summary>bit0만 부분 파일이며 나머지 비트는 정리 때 보존한다.</summary>
    public uint Flags;
    /// <summary>부분 파일 시작의 signed32 비트다.</summary>
    public int Offset;
    /// <summary>부분 파일 길이이며 EOF는 같은 끝 위치에서 아직false다.</summary>
    public int Length;
    /// <summary>현재 키 할당 토큰이며 Close만으로 반납하지 않는다.</summary>
    public nuint Key;
    /// <summary>키 길이이며 Destruct 뒤에도 보존한다.</summary>
    public uint KeyLength;
}

/// <summary>현재 FILE/키/진단 장치다. 본체는 선택한 장치만 요구한다.</summary>
public sealed class RawBaseFileIOHooks
{
    /// <summary>원문 fseek이며 반환 실패는 별도 재시도하지 않는다.</summary>
    public Action<nuint, int, int>? Seek { get; set; }
    /// <summary>원문 ftell의 signed32 결과다.</summary>
    public Func<nuint, int>? Tell { get; set; }
    /// <summary>원문 FILE._flag이며 일반 EOF는bit0x10을 그대로 반환한다.</summary>
    public Func<nuint, uint>? EofFlags { get; set; }
    /// <summary>현재 FILE을 닫는 실제 장치다.</summary>
    public Action<nuint>? Close { get; set; }
    /// <summary>현재 키 할당 반납이다.</summary>
    public Action<nuint>? FreeKey { get; set; }
    /// <summary>현재 토큰/길이의 읽기 전용 키 물리다.</summary>
    public Func<nuint, uint, ReadOnlyMemory<byte>>? Key { get; set; }
    /// <summary>fwrite(data,size,count,file)의 부분 반환은 본체가 사용하지 않는다.</summary>
    public Action<nuint, ReadOnlyMemory<byte>, uint, uint>? Write { get; set; }
    /// <summary>원문 진단은 복귀하면 현재 상태로 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>현재 원문 BYTE 이름과 rb/r+b/w+b 모드의 fopen이다.</summary>
    public Func<ReadOnlyMemory<byte>, string, nuint>? Open { get; set; }
    /// <summary>fread가 실제 작성한 접두 이외의 BYTE를 보존한다.</summary>
    public Action<nuint, Memory<byte>, uint, uint>? Read { get; set; }
    /// <summary>NUL을 포함한 키 실제 할당이다.</summary>
    public Func<uint, nuint>? AllocateKey { get; set; }
    /// <summary>현재 키 할당의 쓰기 물리다.</summary>
    public Func<nuint, uint, Memory<byte>>? MutableKey { get; set; }
}

/// <summary>cpppj/src/o/RawBaseFileIO.h/.cpp의 전체 본체를 현재 상태에 실행한다.</summary>
public static class RawBaseFileIO
{
    /// <summary>원문0052f454의 세 모드 포인터다.</summary>
    private static readonly string[] DiskModes = ["rb", "r+b", "w+b"];
    /// <summary>선택 위치에서만 실제 장치를 요구한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("BaseFile의 선택된 실제 I/O 장치 미연결");
    /// <summary>첫 NUL 앞의 같은 원시 입력 뷰다.</summary>
    private static ReadOnlyMemory<byte> Text(ReadOnlyMemory<byte> name) { int end = name.Span.IndexOf((byte)0); return end < 0 ? name : name[..end]; }
    /// <summary>원문 생성자가 쓰는 FILE/키/키길이만0으로 만든다.</summary>
    public static void ConstructBaseFile(RawBaseFileState state) { state.File = 0; state.Key = 0; state.KeyLength = 0; }
    /// <summary>부분 플래그/위치 초기화 뒤 실제 열기와 mode1 생성 재시도만 실행한다.</summary>
    public static bool OpenBaseFileDisk(RawBaseFileState state, ReadOnlyMemory<byte> name, uint mode, uint create, RawBaseFileIOHooks hooks)
    {
        state.Flags &= ~1u; state.Offset = 0; state.Length = 0;
        if (mode >= DiskModes.Length) throw new ArgumentOutOfRangeException(nameof(mode), "BaseFile의 실제 세 fopen 모드 범위");
        name = Text(name); state.File = Need(hooks.Open)(name, DiskModes[mode]);
        if (state.File == 0 && mode == 1 && create != 0) state.File = Need(hooks.Open)(name, "w+b"); return state.File != 0;
    }
    /// <summary>부분 파일은 성공 뒤에만 base/length/bit0/seek를 설정한다.</summary>
    public static bool OpenBaseFileSubfile(RawBaseFileState state, ReadOnlyMemory<byte> name, int offset, int length, RawBaseFileIOHooks hooks)
    {
        state.Flags &= ~1u; state.File = Need(hooks.Open)(Text(name), "rb");
        if (state.File != 0) { state.Length = length; state.Flags |= 1u; state.Offset = offset; Need(hooks.Seek)(state.File, offset, 0); } return state.File != 0;
    }
    /// <summary>이전 키 반납→길이→할당→현재 입력 BYTE의 첫 NUL까지 복사한다.</summary>
    public static void SetBaseFileKey(RawBaseFileState state, ReadOnlyMemory<byte>? source, RawBaseFileIOHooks hooks)
    {
        if (state.Key != 0) { Need(hooks.FreeKey)(state.Key); state.Key = 0; } state.KeyLength = 0; if (source is null) return;
        int length = source.Value.Span.IndexOf((byte)0); if (length < 0) throw new ArgumentOutOfRangeException(nameof(source), "BaseFile의 키 NUL 물리 누락");
        state.KeyLength = (uint)length; if (length == 0) return;
        state.Key = Need(hooks.AllocateKey)((uint)length + 1); if (state.Key == 0) throw new OutOfMemoryException(); var destination = Need(hooks.MutableKey)(state.Key, (uint)length + 1);
        // 할당 장치가 바꾼 현재 BYTE와 NUL도 복사 직전에 다시 읽는다.
        for (int index = 0; ; index++) { if (index >= source.Value.Length || index >= destination.Length) throw new ArgumentOutOfRangeException(nameof(source), "BaseFile의 현재 키 복사 물리 범위"); byte value = source.Value.Span[index]; destination.Span[index] = value; if (value == 0) break; }
    }
    /// <summary>부분 파일 origin2는 length+base-offset이며 DWORD 감김을 유지한다.</summary>
    public static void SeekBaseFile(RawBaseFileState state, int offset, int origin, RawBaseFileIOHooks hooks)
    {
        if ((state.Flags & 1) == 0) { Need(hooks.Seek)(state.File, offset, origin == 0 ? 0 : origin == 1 ? 1 : 2); return; }
        if (origin == 0) Need(hooks.Seek)(state.File, unchecked(state.Offset + offset), 0);
        else if (origin == 1) Need(hooks.Seek)(state.File, offset, 1);
        else if (origin == 2) Need(hooks.Seek)(state.File, unchecked(state.Length + state.Offset - offset), 0);
    }
    /// <summary>부분 EOF는 tell 뒤 현재 base/length를 strict signed 비교한다.</summary>
    public static uint BaseFileEof(RawBaseFileState state, RawBaseFileIOHooks hooks)
    {
        if ((state.Flags & 1) == 0) return Need(hooks.EofFlags)(state.File) & 0x10u;
        int position = Need(hooks.Tell)(state.File); return state.Length < unchecked(position - state.Offset) ? 1u : 0u;
    }
    /// <summary>일반 FILE은 tell→끝 seek→tell→원래 위치 seek로 길이를 구한다.</summary>
    public static int BaseFileLength(RawBaseFileState state, RawBaseFileIOHooks hooks)
    {
        if ((state.Flags & 1) != 0) return state.Length;
        int position = Need(hooks.Tell)(state.File); Need(hooks.Seek)(state.File, 0, 2); int length = Need(hooks.Tell)(state.File); Need(hooks.Seek)(state.File, position, 0); return length;
    }
    /// <summary>fclose 정상 반환 뒤 현재 부분 플래그/위치/FILE만 지운다.</summary>
    public static void CloseBaseFile(RawBaseFileState state, RawBaseFileIOHooks hooks)
    {
        if (state.File == 0) return; Need(hooks.Close)(state.File); state.Flags &= ~1u; state.Offset = 0; state.Length = 0; state.File = 0;
    }
    /// <summary>Close 뒤 그 시점의 키만 반납하며 keyLength는 보존한다.</summary>
    public static void DestructBaseFile(RawBaseFileState state, RawBaseFileIOHooks hooks)
    {
        CloseBaseFile(state, hooks); if (state.Key != 0) { Need(hooks.FreeKey)(state.Key); state.Key = 0; }
    }
    /// <summary>직접 쓰기는 한 element, 암호 쓰기는 요청 길이의4096BYTE 묶음이며 입력을 바꾸지 않는다.</summary>
    public static void WriteBaseFile(RawBaseFileState state, ReadOnlyMemory<byte> bytes, int offset, RawBaseFileIOHooks hooks)
    {
        if ((state.Flags & 1) != 0) Need(hooks.Assertion)(0x3a4);
        if (offset != -1) Need(hooks.Seek)(state.File, offset, 0);
        if (state.Key == 0) { Need(hooks.Write)(state.File, bytes, (uint)bytes.Length, 1); return; }
        if (state.KeyLength == 0) Need(hooks.Assertion)(0x3aa);
        bool subfile = (state.Flags & 1) != 0; int position = Need(hooks.Tell)(state.File); uint keyIndex = unchecked((uint)(subfile ? position - state.Offset : position)); int used = 0; byte[] scratch = new byte[4096];
        // 각 사용 묶음은 미작성 BYTE를 읽기 전에 실제 입력 전체로 덮는다.
        while (used < bytes.Length)
        {
            int count = Math.Min(bytes.Length - used, 4096); bytes.Slice(used, count).CopyTo(scratch); uint length = state.KeyLength; var key = Need(hooks.Key)(state.Key, length);
            if (length == 0 || key.Length < length) throw new ArgumentOutOfRangeException(nameof(hooks), "BaseFile의 현재 키 물리/길이 범위");
            for (int index = 0; index < count; index++) { scratch[index] ^= key.Span[(int)(keyIndex % length)]; keyIndex = unchecked(keyIndex + 1); } // 위치 DWORD가 감겨도 같은 순환 키를 사용한다.
            Need(hooks.Write)(state.File, scratch.AsMemory(0, count), (uint)count, 1); used += count;
        }
    }
    /// <summary>직접 읽기의 접두와 암호 읽기의 이전 작업 잔여를 그대로 보존한다.</summary>
    public static void ReadBaseFile(RawBaseFileState state, Memory<byte> bytes, int offset, RawBaseFileIOHooks hooks, Memory<byte> scratch = default)
    {
        if (offset != -1) { int position = (state.Flags & 1) != 0 ? unchecked(state.Offset + offset) : offset; Need(hooks.Seek)(state.File, position, 0); }
        if (state.Key == 0) { Need(hooks.Read)(state.File, bytes, (uint)bytes.Length, 1); return; }
        if (state.KeyLength == 0) Need(hooks.Assertion)(0x38b);
        bool subfile = (state.Flags & 1) != 0; int current = Need(hooks.Tell)(state.File); uint keyIndex = unchecked((uint)(subfile ? current - state.Offset : current)); int used = 0;
        // 읽힌 길이가 아닌 요청 길이만큼 진행하며 EOF에서 조기 중단하지 않는다.
        while (used < bytes.Length)
        {
            int count = Math.Min(bytes.Length - used, 4096); if (scratch.Length < count) throw new ArgumentOutOfRangeException(nameof(scratch), "BaseFile의 암호 읽기 작업 물리 누락");
            var block = scratch[..count]; Need(hooks.Read)(state.File, block, (uint)count, 1); uint length = state.KeyLength; var key = Need(hooks.Key)(state.Key, length);
            if (length == 0 || key.Length < length) throw new ArgumentOutOfRangeException(nameof(hooks), "BaseFile의 현재 읽기 키 물리/길이 범위");
            for (int index = 0; index < count; index++) { block.Span[index] ^= key.Span[(int)(keyIndex % length)]; keyIndex = unchecked(keyIndex + 1); } // 짧은 공급에서도 이전 scratch BYTE 전체를 다시 XOR한다.
            block.CopyTo(bytes.Slice(used, count)); used += count;
        }
    }
}
