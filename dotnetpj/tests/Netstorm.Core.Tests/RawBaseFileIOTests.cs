using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 같은 원문 FILE/키 상태와 독립 전체 관측을 대조한다.

/// <summary>Source BaseFileIOTests/BaseFileReadKeyTests의 명시 장치만 공급한다.</summary>
public sealed class RawBaseFileIOTests
{
    /// <summary>격리 원문 FILE/기존키/새키의 서로 다른 토큰이다.</summary>
    private const uint FileToken = 0x12200000, KeyToken = 0x12300000, NewKeyToken = KeyToken + 0x4000;
    /// <summary>원문 DWORD의 양의 십진수다.</summary>
    private static uint U(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>동일 DWORD 비트의 signed32 해석이다.</summary>
    private static int S(string value) => unchecked((int)U(value));
    /// <summary>실제 전체 BYTE를 Native의 빈 값 표기와 함께 직렬화한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => bytes.IsEmpty ? "-" : Convert.ToHexStringLower(bytes);
    /// <summary>같은 실제 여섯 필드의 DWORD 비트다.</summary>
    private static string Fields(RawBaseFileState state) => $"{state.File},{state.Flags},{unchecked((uint)state.Offset)},{unchecked((uint)state.Length)},{state.Key},{state.KeyLength}";
    /// <summary>제품 동작 전 독립 원시 입력의 여섯 필드만 만든다.</summary>
    private static RawBaseFileState State(string[] row) => new() { File = U(row[1]), Flags = U(row[2]), Offset = S(row[3]), Length = S(row[4]), Key = U(row[5]), KeyLength = U(row[6]) };
    /// <summary>같은 실제 할당을 유지한 구조체 대입 장치 효과다.</summary>
    private static void Assign(RawBaseFileState state, nuint file, uint flags, int offset, int length, nuint key, uint keyLength)
    {
        state.File = file; state.Flags = flags; state.Offset = offset; state.Length = length; state.Key = key; state.KeyLength = keyLength;
    }
    /// <summary>출력 디렉터리의 단일 원문 자료를 누락 시 실패하도록 읽는다.</summary>
    private static string[][] Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && line[0] != '#').Select(line => line.Split('\t')).ToArray();
    /// <summary>각 토큰의 명시 고정 입력이며 현재 길이가 실제 물리 이내인지 확인한다.</summary>
    private static ReadOnlyMemory<byte> Key(nuint token, uint length)
    {
        if (token == KeyToken && length <= 3) return "abc"u8.ToArray(); if (token == KeyToken + 128 && length <= 5) return "vwxyz"u8.ToArray(); throw new InvalidOperationException("명시 키 토큰/물리 범위");
    }
    /// <summary>Seek/EOF/Length/Close/Destruct/Write의 현재 원시 입력 장면이다.</summary>
    private sealed class IOScene
    {
        /// <summary>장치와 제품이 함께 쓰는 실제 상태다.</summary>
        public readonly RawBaseFileState Current;
        /// <summary>선택된 실제 하위 장치다.</summary>
        public readonly RawBaseFileIOHooks Hooks;
        /// <summary>현재 FILE 위치/끝·장치 flags와 명시 변이다.</summary>
        public uint Position, End, FileFlags, Scenario, Failed;
        /// <summary>제품이 변경하지 않는 쓰기 원시 입력이다.</summary>
        public readonly byte[] Input;
        /// <summary>장치가 실제 기록한 전체 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>출력 기대가 아닌 행의 입력 열만 설치한다.</summary>
        public IOScene(string[] row)
        {
            Current = State(row); Position = U(row[7]); End = U(row[8]); FileFlags = U(row[9]); Scenario = U(row[13]); Failed = U(row[14]); Input = new byte[checked((int)U(row[12]))];
            for (int index = 0; index < Input.Length; index++) Input[index] = unchecked((byte)(index * 29 + 7)); // 원문 장치 입력이다.
            Hooks = new RawBaseFileIOHooks
            {
                Seek = (file, offset, origin) => { Events.Add($"S:{file}:{unchecked((uint)offset)}:{origin}"); if (Failed == 0) Position = unchecked((uint)offset + (origin == 1 ? Position : origin == 2 ? End : 0)); },
                Tell = file => { Events.Add($"T:{file}"); if (Scenario == 2) { Current.Flags ^= 1; Current.Offset = 9; Current.Length = 11; } return unchecked((int)Position); },
                EofFlags = _ => FileFlags,
                Write = (file, bytes, size, count) => { Assert.Equal((long)size * count, bytes.Length); Events.Add($"W:{file}:{size}:{count}:{Hex(bytes.Span)}"); Position = unchecked(Position + (Failed != 0 ? size * count / 2 : size * count)); },
                Close = file => { Events.Add($"C:{file}"); if (Scenario == 1) Assign(Current, FileToken + 64, 255, 99, 100, KeyToken + 128, 5); },
                FreeKey = key => Events.Add($"F:{key}"), Key = RawBaseFileIOTests.Key,
                Assertion = line => { Events.Add($"A:{line}"); if (Scenario == 4 && line == 0x3aa) Current.KeyLength = 3; if (Scenario == 5 && line == 0x3a4) Current.Flags &= ~1u; }
            };
        }
        /// <summary>현재 행이 선택한 실제 본체 한 개만 호출한다.</summary>
        public string Run(string[] row)
        {
            int offset = S(row[10]);
            switch (row[0])
            {
                case "seek": RawBaseFileIO.SeekBaseFile(Current, offset, S(row[11]), Hooks); break;
                case "eof": return RawBaseFileIO.BaseFileEof(Current, Hooks).ToString(CultureInfo.InvariantCulture);
                case "length": return unchecked((uint)RawBaseFileIO.BaseFileLength(Current, Hooks)).ToString(CultureInfo.InvariantCulture);
                case "close": RawBaseFileIO.CloseBaseFile(Current, Hooks); break;
                case "destruct": RawBaseFileIO.DestructBaseFile(Current, Hooks); break;
                case "write": RawBaseFileIO.WriteBaseFile(Current, Input, offset, Hooks); break;
                default: throw new InvalidOperationException(row[0]);
            }
            return "-";
        }
    }
    /// <summary>생성/열기/키/부분 읽기의 명시 원시 입력 장면이다.</summary>
    private sealed class ReadScene
    {
        /// <summary>같은 실제 여섯 필드다.</summary>
        public readonly RawBaseFileState Current;
        /// <summary>현재 행의 실제 선택 장치다.</summary>
        public readonly RawBaseFileIOHooks Hooks;
        /// <summary>현재 파일 위치/공급량과 열기 성공 mask·현재 횟수·변이다.</summary>
        public uint Position, Available, Mask, Scenario, Opens;
        /// <summary>실제 읽기 시작의 키 유무다.</summary>
        public readonly nuint InitialKey;
        /// <summary>목적 출력과 실제 새 할당 키다.</summary>
        public byte[] Destination, Allocation = [];
        /// <summary>호출 사이 재초기화하지 않는 같은 작업 버퍼와 입력 키다.</summary>
        public readonly byte[] Scratch = new byte[4096], Source = new byte[32];
        /// <summary>실제 하위가 기록한 모든 호출 인자다.</summary>
        public readonly List<string> Events = [];
        /// <summary>독립 기대값을 사용하지 않고 입력 열과 장치 효과만 준비한다.</summary>
        public ReadScene(string[] row)
        {
            Current = State(row); InitialKey = Current.Key; Position = U(row[7]); Available = U(row[8]); Mask = U(row[14]); Scenario = U(row[15]); Destination = new byte[checked((int)U(row[10]))];
            for (int index = 0; index < Destination.Length; index++) Destination[index] = unchecked((byte)(index * 7 + 53)); // 실제 미작성 목적 바이트다.
            for (int index = 0; index < Scratch.Length; index++) Scratch[index] = unchecked((byte)(index * 11 + 165)); // 짧은 공급 뒤 재사용할 작업 입력이다.
            if (row[11] != "null") Convert.FromHexString(row[11]).CopyTo(Source, 0);
            Hooks = new RawBaseFileIOHooks
            {
                Open = (name, mode) => { Events.Add($"O:{Hex(name.Span)}:{mode}"); if (Scenario == 1) { Current.Flags = 255; Current.Offset = 19; Current.Length = 23; } return (Mask & (1u << (int)Opens++)) != 0 ? FileToken : 0; },
                Seek = (file, offset, origin) => { Events.Add($"S:{file}:{unchecked((uint)offset)}:{origin}"); Position = unchecked((uint)offset); if (Scenario == 2) Current.File = 0; },
                Tell = file => { Events.Add($"T:{file}"); if (Scenario == 2) { Current.Flags ^= 1; Current.Offset = 9; } return unchecked((int)Position); },
                Read = (file, bytes, size, count) =>
                {
                    uint read = Math.Min(unchecked(size * count), Available); Assert.Equal((long)size * count, bytes.Length);
                    for (int index = 0; index < read; index++) bytes.Span[index] = unchecked((byte)((Position + (uint)index) * 29 + 7)); // 실제 CRT가 작성한 접두뿐이다.
                    Events.Add($"R:{file}:{size}:{count}:{read}"); Available -= read; Position = unchecked(Position + read); if (Scenario == 3) { Current.File = FileToken + 64; Current.Key = KeyToken + 128; Current.KeyLength = 5; }
                },
                FreeKey = key => { Events.Add($"F:{key}"); if (Scenario == 1) { Current.Key = KeyToken + 128; Current.KeyLength = 99; Source[0] = (byte)'z'; Source[1] = 0; } },
                AllocateKey = length => { Events.Add($"N:{length}"); Allocation = Enumerable.Repeat((byte)205, checked((int)length)).ToArray(); if (Scenario == 2) Source[1] = 0; return NewKeyToken; },
                MutableKey = (key, length) => { Assert.Equal((nuint)NewKeyToken, key); Assert.Equal((uint)Allocation.Length, length); return Allocation; },
                Key = RawBaseFileIOTests.Key,
                Assertion = line => { Events.Add($"A:{line}"); if (Scenario == 4 && line == 0x38b) Current.KeyLength = 3; }
            };
        }
        /// <summary>입력 행이 선택한 실제 부모 한 개만 실행한다.</summary>
        public string Run(string[] row)
        {
            int offset = S(row[9]);
            switch (row[0])
            {
                case "construct": RawBaseFileIO.ConstructBaseFile(Current); break;
                case "disk": return RawBaseFileIO.OpenBaseFileDisk(Current, "asset\0tail"u8.ToArray(), U(row[12]), U(row[13]), Hooks) ? "1" : "0";
                case "subfile": return RawBaseFileIO.OpenBaseFileSubfile(Current, "asset\0tail"u8.ToArray(), offset, 33, Hooks) ? "1" : "0";
                case "key":
                    // 원문 NULL은 NullableMemory의 값 없음으로 명시한다.
                    ReadOnlyMemory<byte>? source = row[11] == "null" ? (ReadOnlyMemory<byte>?)null : new ReadOnlyMemory<byte>(Source);
                    RawBaseFileIO.SetBaseFileKey(Current, source, Hooks); break;
                case "read": RawBaseFileIO.ReadBaseFile(Current, Destination, offset, Hooks, Scratch); break;
                default: throw new InvalidOperationException(row[0]);
            }
            return "-";
        }
    }
    /// <summary>부분 EOF/끝 기준 부호/현재 필드·암호 쓰기의 전체322관측이다.</summary>
    [Fact]
    public void WholeIndependentNative322CursorCloseAndWrite()
    {
        var rows = Rows("basefile-io-x86.tsv"); Assert.Equal(322, rows.Length);
        foreach (var row in rows) { var scene = new IOScene(row); byte[] original = scene.Input.ToArray(); Assert.Equal(row[15], scene.Run(row)); Assert.Equal(row[16], Fields(scene.Current)); Assert.Equal(row[17], scene.Position.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[18], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); Assert.Equal(original, scene.Input); } // 기대는 원문 전체 결과만 읽는다.
    }
    /// <summary>현재 필드/목적/작업/할당/키 입력 전체235관측이다.</summary>
    [Fact]
    public void WholeIndependentNative235ReadOpenAndKey()
    {
        var rows = Rows("basefile-read-key-x86.tsv"); Assert.Equal(235, rows.Length);
        // 콜백이 바꾼 현재 필드와 짧은 요청 잔여를 각 행에서 비교한다.
        foreach (var row in rows)
        {
            var scene = new ReadScene(row); Assert.Equal(row[16], scene.Run(row)); Assert.Equal(row[17], Fields(scene.Current)); Assert.Equal(row[18], scene.Position.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[19], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); Assert.Equal(row[20], Hex(scene.Destination)); Assert.Equal(row[21], row[0] == "read" && scene.InitialKey != 0 && scene.Destination.Length != 0 ? Hex(scene.Scratch) : "-"); Assert.Equal(row[22], Hex(scene.Allocation)); Assert.Equal(row[23], Hex(scene.Source));
        }
    }
    /// <summary>미선택 장치를 요구하지 않고 fclose/키 오류 앞의 실제 부분 효과를 유지한다.</summary>
    [Fact]
    public void SelectedCloseAndWriteFailuresKeepEarlierEffects()
    {
        var state = new RawBaseFileState { Flags = 0x81, Offset = 7, Length = 9, KeyLength = 3 }; RawBaseFileIO.CloseBaseFile(state, new()); Assert.Equal(0x81u, state.Flags); Assert.Equal(7, state.Offset); RawBaseFileIO.SeekBaseFile(state, 1, 3, new()); Assert.Equal(9, RawBaseFileIO.BaseFileLength(state, new()));
        state.File = FileToken; var hooks = new RawBaseFileIOHooks { Close = file => { Assert.Equal((nuint)FileToken, file); state.Flags = 0x82; throw new IOException("명시 fclose 중단"); } }; Assert.Throws<IOException>(() => RawBaseFileIO.DestructBaseFile(state, hooks)); Assert.Equal((nuint)FileToken, state.File); Assert.Equal(0x82u, state.Flags); Assert.Equal(7, state.Offset);
        Assign(state, FileToken, 0x80, 7, 9, KeyToken, 0); int calls = 0; hooks.Assertion = line => { Assert.Equal(0x3aau, line); calls++; }; hooks.Tell = _ => { calls++; return 15; }; hooks.Key = (_, _) => ReadOnlyMemory<byte>.Empty;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBaseFileIO.WriteBaseFile(state, new byte[] { 4 }, -1, hooks)); Assert.Equal(2, calls); Assert.Equal(0u, state.KeyLength);
    }
    /// <summary>키 할당 실패/암호 작업 물리 부족/잘못된 모드는 앞선 저장 뒤 거부한다.</summary>
    [Fact]
    public void AllocationScratchAndModeFailuresPreservePrefix()
    {
        var state = new RawBaseFileState { File = FileToken, Flags = 0x81, Offset = 7, Length = 9, Key = KeyToken, KeyLength = 3 }; int freed = 0; var hooks = new RawBaseFileIOHooks { FreeKey = key => { Assert.Equal((nuint)KeyToken, key); freed++; }, AllocateKey = length => { Assert.Equal(4u, length); return 0; } };
        Assert.Throws<OutOfMemoryException>(() => RawBaseFileIO.SetBaseFileKey(state, "abc\0"u8.ToArray(), hooks)); Assert.Equal(1, freed); Assert.Equal((nuint)0, state.Key); Assert.Equal(3u, state.KeyLength); Assert.Equal((nuint)FileToken, state.File);
        state.Key = KeyToken; hooks.Tell = _ => 15; byte[] destination = [88]; Assert.Throws<ArgumentOutOfRangeException>(() => RawBaseFileIO.ReadBaseFile(state, destination, -1, hooks)); Assert.Equal(88, destination[0]);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBaseFileIO.OpenBaseFileDisk(state, "file"u8.ToArray(), 3, 1, new())); Assert.Equal(0x80u, state.Flags); Assert.Equal(0, state.Offset); Assert.Equal(0, state.Length); Assert.Equal((nuint)FileToken, state.File);
    }
}
