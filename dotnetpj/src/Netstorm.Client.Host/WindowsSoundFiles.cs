using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;

// Windows 원문 파일 선택·감쇠·Winmm WAVE 읽기 경계를 보존한다.
namespace Netstorm.Client.Host;

/// <summary>cpp SoundDevice의 실제 디스크/Winmm 함수다. 읽기만 하며 오디오 장치를 열지 않는다.</summary>
[SupportedOSPlatform("windows")]
public static class WindowsSoundFiles
{
    /// <summary>MMCKINFO의 원문 DWORD 다섯 개다.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct Chunk
    {
        /// <summary>현재 청크의 FourCC다.</summary>
        public uint Id;
        /// <summary>Winmm이 파일/RIFF 끝으로 제한한 청크 크기다.</summary>
        public uint Size;
        /// <summary>RIFF의 WAVE 형식 식별자다.</summary>
        public uint Type;
        /// <summary>파일에서 청크 데이터 시작 위치다.</summary>
        public uint Offset;
        /// <summary>Winmm이 보유하는 청크 상태 플래그다.</summary>
        public uint Flags;
    }
    /// <summary>실제 FindFirstFileW 결과의 고정 Windows 배치다.</summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 4)]
    private struct FindData
    {
        /// <summary>디렉터리 여부를 판단하는 파일 속성이다.</summary>
        public uint Attributes;
        /// <summary>파일 생성 시각이다.</summary>
        public long Creation;
        /// <summary>최근 접근 시각이다.</summary>
        public long Access;
        /// <summary>최근 기록 시각이다.</summary>
        public long Write;
        /// <summary>파일 크기의 상위DWORD다.</summary>
        public uint SizeHigh;
        /// <summary>파일 크기의 하위DWORD다.</summary>
        public uint SizeLow;
        /// <summary>Win32 예약DWORD다.</summary>
        public uint Reserved0;
        /// <summary>Win32 예약DWORD다.</summary>
        public uint Reserved1;
        /// <summary>첫 검색 결과의 실제 대소문자 파일 이름이다.</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] public string Name;
        /// <summary>8.3 별칭이며 이 계층은 사용하지 않는다.</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 14)] public string Alternate;
    }
    /// <summary>변경하지 않는 실제 파일을 버퍼 있는 Winmm 읽기 모드로 연다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioOpenW", CharSet = CharSet.Unicode)]
    private static extern nint Open(string path, nint info, uint flags);
    /// <summary>현재 Winmm 파일을 닫는다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioClose")]
    private static extern uint Close(nint file, uint flags);
    /// <summary>부모 없이 WAVE RIFF를 찾는다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioDescend")]
    private static extern uint Descend(nint file, ref Chunk chunk, nint parent, uint flags);
    /// <summary>같은 RIFF 안의 다음 fmt/data를 현재 파일 위치부터 찾는다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioDescend")]
    private static extern uint Descend(nint file, ref Chunk chunk, ref Chunk parent, uint flags);
    /// <summary>현재 청크 끝으로 이동한다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioAscend")]
    private static extern uint Ascend(nint file, ref Chunk chunk, uint flags);
    /// <summary>최대 countBYTE를 읽고 실제 읽은 signed 길이를 돌려준다.</summary>
    [DllImport("winmm.dll", EntryPoint = "mmioRead")]
    private static extern int Read(nint file, [Out] byte[] target, int count);
    /// <summary>원문과 같은 첫 와일드카드 결과만 얻는다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "FindFirstFileW", CharSet = CharSet.Unicode)]
    private static extern nint FindFirst(string pattern, out FindData data);
    /// <summary>디스크 검색 핸들을 닫는다.</summary>
    [DllImport("kernel32.dll", EntryPoint = "FindClose")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool FindClose(nint handle);
    /// <summary>Windows SDK의 inline sscanf가 부르는 실제 UCRT 공통 함수다. args는 long32 출력 주소 한 개의 va_list다.</summary>
    [DllImport("ucrtbase.dll", EntryPoint = "__stdio_common_vsscanf", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private static extern int Scan(ulong options, string path, nuint length, string format, nint locale, nint args);
    /// <summary>디렉터리 끝에 구분자가 없을 때만 원문 역슬래시를 붙인다.</summary>
    private static string Join(string directory, string name) => directory.Length == 0 ? name : directory + (directory[^1] is '/' or '\\' ? "" : "\\") + name;

    /// <summary>실제 주/보조 디스크에서 첫 Win32 파일만 선택하는 장기 resolver를 만든다.</summary>
    public static Func<string, bool, string?> MakeDiskResolver(string primary, string secondary = "") => (pattern, alternate) =>
    {
        string root = alternate ? secondary : primary;
        if (root.Length == 0) return null;
        string query = Path.Combine(root, pattern); nint find = FindFirst(query, out FindData found);
        if (find == -1) return null;
        FindClose(find); if ((found.Attributes & 0x10) != 0) return null;
        return Path.Combine(Path.GetDirectoryName(query) ?? "", found.Name);
    };
    /// <summary>여덟 원문 후보를 주/보조→언어/기본→감쇠/일반 순서로 조회한다.</summary>
    public static string? Find(string name, string directory, string language, Func<string, bool, string?> resolver)
    {
        ArgumentNullException.ThrowIfNull(resolver);
        if (string.IsNullOrEmpty(name) || name.Contains('\0')) throw new ArgumentException("소리 파일 조회 인자가 올바르지 않습니다", nameof(name));
        int dot = name.LastIndexOf('.'); string stem = dot < 0 ? name : name[..dot], extension = dot < 0 ? "" : name[dot..];
        string[] patterns = [stem + "-*" + extension, stem + "*" + extension];
        // 주 경로 전체가 끝난 뒤에만 보조/CD 경로를 선택한다.
        foreach (bool alternate in new[] { false, true })
            // 언어 폴더의 두 패턴이 기본 폴더보다 우선한다.
            foreach (string current in new[] { Join(directory, language), directory })
                // 원문처럼 감쇠가 있는 파일 패턴부터 시도한다.
                foreach (string pattern in patterns)
                    if (resolver(Join(current, pattern), alternate) is { } file) return file;
        return null;
    }
    /// <summary>전체 경로 첫 '-' 이후의 MS CRT signed long32 감쇠를 원문 형식으로 읽는다.</summary>
    public static int Attenuation(string path)
    {
        nint value = Marshal.AllocHGlobal(4), args = Marshal.AllocHGlobal(nint.Size);
        try
        {
            Marshal.WriteInt32(value, 0); Marshal.WriteIntPtr(args, value);
            return Scan(0, path, nuint.MaxValue, "%*[^-]-%ld.%*[^-]", 0, args) == 1 ? Marshal.ReadInt32(value) : 0;
        }
        finally { Marshal.FreeHGlobal(args); Marshal.FreeHGlobal(value); }
    }

    /// <summary>실제 Winmm의 WAVE→fmt→data 읽기와 진단/크기 보정을 수행한다. 세 판본의 계약은 같다.</summary>
    public static SoundWave? ReadWave(string path, OriginalEdition edition, Action<string, int>? report = null)
    {
        _ = edition; nint file = Open(path, 0, 0x10000); if (file == 0) return null;
        try
        {
            var riff = new Chunk { Type = 0x45564157 }; if (Descend(file, ref riff, 0, 0x20) != 0) return null;
            var chunk = new Chunk { Id = 0x20746d66 }; if (Descend(file, ref chunk, ref riff, 0x10) != 0 || chunk.Size < 16) return null;
            byte[] format = new byte[18]; int size = (int)Math.Min(chunk.Size, 18);
            if (Read(file, format, size) != size) return null;
            ushort tag = BinaryPrimitives.ReadUInt16LittleEndian(format);
            if (size < 18)
            {
                if (tag != 1) report?.Invoke("wfFormat.wFormatTag == WAVE_FORMAT_PCM", 0xeb);
                uint denominator = unchecked((uint)BinaryPrimitives.ReadUInt16LittleEndian(format.AsSpan(2)) * BinaryPrimitives.ReadUInt32LittleEndian(format.AsSpan(4)));
                if (denominator == 0) return null;
                BinaryPrimitives.WriteUInt16LittleEndian(format.AsSpan(16), 0);
                uint bits = unchecked(BinaryPrimitives.ReadUInt32LittleEndian(format.AsSpan(8)) << 3) / denominator;
                BinaryPrimitives.WriteUInt16LittleEndian(format.AsSpan(14), unchecked((ushort)bits));
            }
            if (tag != 1 && BinaryPrimitives.ReadUInt16LittleEndian(format.AsSpan(16)) != 0) return null;
            if (Ascend(file, ref chunk, 0) != 0) return null;
            chunk = new Chunk { Id = 0x61746164 }; if (Descend(file, ref chunk, ref riff, 0x10) != 0 || chunk.Size == 0 || chunk.Size > int.MaxValue) return null;
            long diskSize;
            try { diskSize = new FileInfo(path).Length; }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException) { return null; }
            if (chunk.Offset > diskSize || chunk.Size > diskSize - chunk.Offset) return null;
            byte[] samples = new byte[(int)chunk.Size]; if (Read(file, samples, samples.Length) != samples.Length) return null;
            return new SoundWave { Format = format, DataOffset = chunk.Offset, Samples = samples };
        }
        finally { Close(file, 0); }
    }
}
