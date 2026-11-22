using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Client.Audio;

// 원문 Windows 소리 장치의 DLL/COM 수명과64비트 포인터·32비트 토큰 경계를 연결한다.
namespace Netstorm.Client.Host;

/// <summary>cpp SoundDevice의 실제 Windows 효과음 장치다. MonoGame 화면 HWND를 빌리고 DirectSound 자원은 직접 소유한다.</summary>
[SupportedOSPlatform("windows")]
public sealed partial class WindowsSoundDevice : IDisposable
{
    /// <summary>원문 DSERR_INVALIDPARAM HRESULT다.</summary>
    private const int InvalidParameter = unchecked((int)0x80070057);
    /// <summary>음악 worker와도 공유할 재귀 COM/토큰 잠금이다.</summary>
    private readonly object _gate = new();
    /// <summary>장치 종료 때 버퍼 필드만 비울 실제 이름표다.</summary>
    private readonly SoundList _list;
    /// <summary>재생기와 공유하는 원문 전역이다.</summary>
    private readonly SoundState _state;
    /// <summary>실제 주/보조 파일 탐색 효과다.</summary>
    private readonly Func<string, bool, string?> _resolver;
    /// <summary>언어 폴더 이름이다.</summary>
    private readonly string _language;
    /// <summary>효과음 폴더 이름이다.</summary>
    private readonly string _directory;
    /// <summary>선택적 짧은 비PCM 진단 효과다.</summary>
    private readonly Action<string, int>? _report;
    /// <summary>DLL의 동적 참조 소유권이다.</summary>
    private nint _module;
    /// <summary>IDirectSound COM 포인터다. 원문DWORD에 넣지 않는다.</summary>
    private nint _device;
    /// <summary>원문 음질 협상용 기본 버퍼다.</summary>
    private nint _primary;
    /// <summary>효과음/음악 버퍼 토큰마다 소유한 실제 COM 포인터다.</summary>
    private readonly Dictionary<uint, nint> _buffers = [];
    /// <summary>종료 후에도 되돌리지 않는 다음 원문 장치 토큰이다.</summary>
    private uint _next = 1;

    /// <summary>실행 오류 기록용 마지막 초기화 실패 위치/HRESULT다. 원문 상태/장치 호출 순서는 바꾸지 않는다.</summary>
    public string LastInitializationFailure { get; private set; } = "";

    /// <summary>호스트 포인터 폭에 맞춘 DSBUFFERDESC다.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct BufferDescription
    {
        /// <summary>이 호스트 구조체의 실제 바이트 크기다.</summary>
        public uint Size;
        /// <summary>원문 버퍼 기능 플래그다.</summary>
        public uint Flags;
        /// <summary>실제 표본 또는 고정 링 버퍼의 바이트 수다.</summary>
        public uint Bytes;
        /// <summary>0으로 전달하는 예약DWORD다.</summary>
        public uint Reserved;
        /// <summary>호출 중 유효한 WAVEFORMATEX18BYTE 주소다.</summary>
        public nint Format;
        /// <summary>원문 기본0 GUID다.</summary>
        public Guid Algorithm;
    }
    /// <summary>DirectSoundCreate의 실제 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int CreateDevice(nint guid, out nint device, nint outer);
    /// <summary>원문 버퍼 생성의 실제 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int CreateBuffer(nint self, ref BufferDescription description, out nint buffer, nint outer);
    /// <summary>원문 버퍼 복제의 실제 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int DuplicateBuffer(nint self, nint original, out nint copy);
    /// <summary>원문 창 협력 수준의 실제 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int CooperativeLevel(nint self, nint window, uint level);
    /// <summary>IUnknown.Release의 실제 ULONG 반환 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate uint Release(nint self);
    /// <summary>출력DWORD가 하나인 상태 조회 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int ReadDword(nint self, out uint value);
    /// <summary>입력DWORD가 하나인 위치 설정 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int WriteDword(nint self, uint value);
    /// <summary>입력LONG이 하나인 음량/좌우 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int WriteLong(nint self, int value);
    /// <summary>입력이 없는 정지/복구 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int Simple(nint self);
    /// <summary>재생 예약0·우선0·플래그의 실제 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int PlayBuffer(nint self, uint reserved, uint priority, uint flags);
    /// <summary>주 버퍼 형식 설정의 실제 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int SetFormat(nint self, nint format);
    /// <summary>원문 Lock의 두 포인터/크기 출력 ABI다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int LockBuffer(nint self, uint offset, uint count, out nint first, out uint firstSize, out nint second, out uint secondSize, uint flags);
    /// <summary>원문 Unlock은 받은 포인터/크기를 그대로 돌려준다.</summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] private delegate int UnlockBuffer(nint self, nint first, uint firstSize, nint second, uint secondSize);

    /// <summary>장치를 열지 않고 실제 이름표/전역/파일 효과를 보관한다.</summary>
    public WindowsSoundDevice(SoundList list, SoundState state, Func<string, bool, string?> resolver,
        string language = "english", string directory = "sound", Action<string, int>? report = null)
    {
        ArgumentNullException.ThrowIfNull(list); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(resolver);
        _list = list; _state = state; _resolver = resolver; _language = language; _directory = directory; _report = report;
    }
    /// <summary>Windows SDK의 해당 COM 가상함수를 현재 실제 포인터에서 읽는다.</summary>
    private static T Method<T>(nint instance, int slot) where T : Delegate => Marshal.GetDelegateForFunctionPointer<T>(Marshal.ReadIntPtr(Marshal.ReadIntPtr(instance), checked(slot * nint.Size)));
    /// <summary>소유한 버퍼를32비트 토큰에 넣고 실패 때 COM 참조를 반납한다.</summary>
    private uint Add(nint buffer)
    {
        if (_next == SoundConstants.SilentBuffer) { _ = Method<Release>(buffer, 2)(buffer); throw new InvalidOperationException("소리 버퍼 토큰이 소진됐습니다"); }
        uint token = _next; _next = unchecked(_next + 1);
        try { _buffers.Add(token, buffer); } catch { _ = Method<Release>(buffer, 2)(buffer); throw; }
        return token;
    }
    /// <summary>토큰이 소유한 현재 실제 버퍼만 허용한다.</summary>
    private nint Get(uint token) => _buffers.TryGetValue(token, out nint pointer) ? pointer : throw new ArgumentOutOfRangeException(nameof(token), "알 수 없는 소리 장치 버퍼입니다");
    /// <summary>원문 COM 생성 뒤 실패 정리를 수행하고 소리 옵션을 끈다.</summary>
    private bool Failed(string stage, int result) { LastInitializationFailure = $"{stage}: HRESULT 0x{unchecked((uint)result):X8}"; Shutdown(); _state.Enabled = false; return false; }
    /// <summary>원문 DLL→장치→창→주 버퍼→음질 하향 협상을 실제로 수행한다.</summary>
    public bool Initialize(nint window, int quality = 3)
    {
        lock (_gate)
        {
            _list.Initialize(); if (_state.Initialized) return false;
            if (window == 0) throw new ArgumentException("소리 장치 초기화에 창 핸들이 필요합니다", nameof(window));
            LastInitializationFailure = "";
            if (quality is < 0 or > 3) quality = 3;
            if (_module == 0 && !NativeLibrary.TryLoad("dsound.dll", out _module)) { LastInitializationFailure = "dsound.dll 적재 실패"; return false; }
            if (!NativeLibrary.TryGetExport(_module, "DirectSoundCreate", out nint create)) { LastInitializationFailure = "DirectSoundCreate 조회 실패"; return false; }
            int result = Marshal.GetDelegateForFunctionPointer<CreateDevice>(create)(0, out _device, 0);
            if (result < 0) return Failed("DirectSoundCreate", result);
            _state.Device = true;
            result = Method<CooperativeLevel>(_device, 6)(_device, window, 2);
            if (result < 0) return Failed("SetCooperativeLevel", result);
            var description = new BufferDescription { Size = (uint)Marshal.SizeOf<BufferDescription>(), Flags = 1 };
            result = Method<CreateBuffer>(_device, 3)(_device, ref description, out _primary, 0);
            if (result < 0) return Failed("CreateSoundBuffer(primary)", result);
            // 원문처럼22050Hz에서 스테레오16→스테레오8→모노8 순서로만 하향한다.
            while (quality > 0)
            {
                byte[] format = new byte[18]; ushort channels = quality == 1 ? (ushort)1 : (ushort)2, bits = quality == 3 ? (ushort)16 : (ushort)8;
                ushort align = (ushort)(channels * bits / 8); BinaryPrimitives.WriteUInt16LittleEndian(format, 1); BinaryPrimitives.WriteUInt16LittleEndian(format.AsSpan(2), channels);
                BinaryPrimitives.WriteUInt32LittleEndian(format.AsSpan(4), 22050); BinaryPrimitives.WriteUInt32LittleEndian(format.AsSpan(8), 22050u * align);
                BinaryPrimitives.WriteUInt16LittleEndian(format.AsSpan(12), align); BinaryPrimitives.WriteUInt16LittleEndian(format.AsSpan(14), bits);
                nint raw = Marshal.AllocHGlobal(18);
                try { Marshal.Copy(format, 0, raw, 18); result = Method<SetFormat>(_primary, 14)(_primary, raw); }
                finally { Marshal.FreeHGlobal(raw); }
                if (result >= 0) break; quality--;
            }
            _state.Initialized = true; return true;
        }
    }
    /// <summary>장치가 있으면 실제 파일·WAVE·버퍼 Lock을 거쳐 원문 표본 그대로 적재한다.</summary>
    private SoundLoad Load(string name)
    {
        if (_device == 0) return new SoundLoad(0, 0);
        string? path = WindowsSoundFiles.Find(name, _directory, _language, _resolver); if (path is null) return new SoundLoad(SoundConstants.SilentBuffer, 0);
        int attenuation = WindowsSoundFiles.Attenuation(path); SoundWave? wave = WindowsSoundFiles.ReadWave(path, _list.Edition, _report);
        if (wave is null) return new SoundLoad(SoundConstants.SilentBuffer, attenuation);
        byte[] format = (byte[])wave.Format.Clone(); if (BinaryPrimitives.ReadUInt16LittleEndian(format) == 1) BinaryPrimitives.WriteUInt16LittleEndian(format.AsSpan(16), 0);
        nint raw = Marshal.AllocHGlobal(18), buffer = 0;
        try
        {
            Marshal.Copy(format, 0, raw, 18); var description = new BufferDescription { Size = (uint)Marshal.SizeOf<BufferDescription>(), Flags = 0xe2, Bytes = (uint)wave.Samples.Length, Format = raw };
            if (Method<CreateBuffer>(_device, 3)(_device, ref description, out buffer, 0) < 0) return new SoundLoad(SoundConstants.SilentBuffer, attenuation);
            if (Method<LockBuffer>(buffer, 11)(buffer, 0, description.Bytes, out nint first, out uint firstSize, out nint second, out uint secondSize, 0) < 0) return new SoundLoad(SoundConstants.SilentBuffer, attenuation);
            try
            {
                if (firstSize != description.Bytes) return new SoundLoad(SoundConstants.SilentBuffer, attenuation);
                Marshal.Copy(wave.Samples, 0, first, wave.Samples.Length);
            }
            finally { _ = Method<UnlockBuffer>(buffer, 19)(buffer, first, firstSize, second, secondSize); }
            nint transferred = buffer; buffer = 0; return new SoundLoad(Add(transferred), attenuation);
        }
        finally { if (buffer != 0) _ = Method<Release>(buffer, 2)(buffer); Marshal.FreeHGlobal(raw); }
    }
    /// <summary>실제 같은 버퍼에 대한 여덟 원문 효과를 잠금으로 감싸 반환한다.</summary>
    public SoundDeviceHooks Hooks() => new()
    {
        Status = token => { lock (_gate) { nint buffer = Get(token); _ = Method<ReadDword>(buffer, 9)(buffer, out uint value); return value; } },
        Play = (token, flags) => { lock (_gate) { nint buffer = Get(token); _ = Method<PlayBuffer>(buffer, 12)(buffer, 0, 0, flags); } },
        SetPosition = (token, position) => { lock (_gate) { nint buffer = Get(token); _ = Method<WriteDword>(buffer, 13)(buffer, position); } },
        SetVolume = (token, volume) => { lock (_gate) { nint buffer = Get(token); return Method<WriteLong>(buffer, 15)(buffer, volume); } },
        SetPan = (token, pan) => { lock (_gate) { nint buffer = Get(token); return Method<WriteLong>(buffer, 16)(buffer, pan); } },
        Stop = token => { lock (_gate) { nint buffer = Get(token); _ = Method<Simple>(buffer, 18)(buffer); } },
        Duplicate = Duplicate,
        Load = name => { lock (_gate) return Load(name); },
    };
    /// <summary>원문 실패 때 출력을 건드리지 않는 같은 장치 버퍼 복제다.</summary>
    private int Duplicate(uint token, ref uint copy)
    {
        lock (_gate)
        {
            if (_device == 0 || token == 0 || token == SoundConstants.SilentBuffer) return InvalidParameter;
            int result = Method<DuplicateBuffer>(_device, 5)(_device, Get(token), out nint buffer);
            if (result >= 0) copy = Add(buffer); return result;
        }
    }
    /// <summary>버퍼→주 버퍼→장치→DLL을 반납하고 표의 Buffer만 비운다. 반복/재초기화가 가능하다.</summary>
    public void Shutdown()
    {
        lock (_gate)
        {
            // 실제 각 COM 소유권을 정지 후 정확히 한 번 반납한다.
            foreach (nint buffer in _buffers.Values) { _ = Method<Simple>(buffer, 18)(buffer); _ = Method<Release>(buffer, 2)(buffer); }
            _buffers.Clear();
            if (_primary != 0) { _ = Method<Release>(_primary, 2)(_primary); _primary = 0; }
            if (_device != 0) { _ = Method<Release>(_device, 2)(_device); _device = 0; }
            if (_module != 0) { NativeLibrary.Free(_module); _module = 0; }
            // 물리 이름 길이/사슬/음량/감쇠를 유지하고 버퍼DWORD만 초기화한다.
            for (uint offset = 0; offset + SoundConstants.HeaderBytes < SoundConstants.ListBytes;)
            {
                uint handle = SoundConstants.ListBase + offset; int length = _list.NameBytes(handle).Length; if (length == 0) break;
                _list.SetField(handle, SoundField.Buffer, 0); offset += SoundConstants.HeaderBytes + (uint)length + 1;
            }
            _state.Initialized = _state.Device = false; _state.Playing = 0;
        }
    }
    /// <summary>이 객체가 소유한 실제 소리 자원을 종료한다.</summary>
    public void Dispose() => Shutdown();
}
