using System.Buffers.Binary;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 창/하드웨어 없이 실제 파일IO와 장치 COM ABI·직접 포인터·개별 반납을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 DirectSound 하드웨어 성공과 구별되는 오프라인 장치·합성 COM ABI 검사다.</summary>
[SupportedOSPlatform("windows")]
public sealed class WindowsMusicBufferDeviceTests
{
    /// <summary>합성 COM이 명시 입력으로 반환하는 E_FAIL이다.</summary>
    private const int Failure = unchecked((int)0x80004005);

    /// <summary>실제 가상함수표·unmanaged 표본 메모리를 소유하고 COM 응답만 명시 공급한다. DLL/오디오 장치는 만들지 않는다.</summary>
    private sealed class ComBoundary : IDisposable
    {
        /// <summary>검사별 장치 가상함수표 메모리다.</summary>
        private readonly nint _deviceTable = Marshal.AllocHGlobal(7 * nint.Size);
        /// <summary>검사별 버퍼 가상함수표 메모리다.</summary>
        private readonly nint _bufferTable = Marshal.AllocHGlobal(21 * nint.Size);
        /// <summary>실제 제품 COM 호출이 읽을 장치 this 포인터다.</summary>
        private readonly nint _device = Marshal.AllocHGlobal(nint.Size);
        /// <summary>실제 제품 COM 호출이 읽을 버퍼 this 포인터다.</summary>
        private readonly nint _buffer = Marshal.AllocHGlobal(nint.Size);
        /// <summary>reverse PInvoke thunk가 수명 끝까지 살아 있도록 실제 delegate를 보관한다.</summary>
        private readonly List<Delegate> _roots = [];
        /// <summary>실제 Lock이 직접 반환하는176000BYTE 표본 메모리다.</summary>
        public nint Data { get; } = Marshal.AllocHGlobal((int)MusicChannel.BufferBytes);
        /// <summary>이번 Create HRESULT 입력이다.</summary>
        public int CreateResult;
        /// <summary>이번 GetStatus HRESULT 입력이다.</summary>
        public int StatusResult;
        /// <summary>이번 GetCurrentPosition HRESULT 입력이다.</summary>
        public int CursorResult;
        /// <summary>이번 Lock HRESULT 입력이다.</summary>
        public int LockResult;
        /// <summary>실제 장치 상태 출력 입력이다.</summary>
        public uint Status;
        /// <summary>실제 재생 커서 출력 입력이다.</summary>
        public uint PlayCursor = 17;
        /// <summary>실제 장치 쓰기 커서 출력 입력이다.</summary>
        public uint WriteCursor = 29;
        /// <summary>빈 첫 구간도 비영 포인터를 유지하는 입력이다.</summary>
        public bool EmptyFirst;
        /// <summary>정상 Create 호출 횟수다.</summary>
        public int Creates;
        /// <summary>개별 실제 Release 호출 횟수다.</summary>
        public int Releases;
        /// <summary>장치 Release 호출 횟수다.</summary>
        public int DeviceReleases;
        /// <summary>실제 Unlock 호출 횟수다.</summary>
        public int Unlocks;
        /// <summary>실제 Restore 호출 횟수다.</summary>
        public int Restores;
        /// <summary>실제 Stop 호출 횟수다.</summary>
        public int Stops;
        /// <summary>같은 실제 장치 버퍼의 음량 호출 입력이다.</summary>
        public int Volume;
        /// <summary>같은 실제 장치 버퍼의 재생 플래그 입력이다.</summary>
        public uint PlayFlags;
        /// <summary>실제 Lock이 반환한 첫 포인터다.</summary>
        private nint _first;
        /// <summary>실제 Lock이 반환한 첫 크기다.</summary>
        private uint _firstSize;
        /// <summary>실제 Lock이 반환한 두 번째 포인터다.</summary>
        private nint _second;
        /// <summary>실제 Lock이 반환한 두 번째 크기다.</summary>
        private uint _secondSize;
        /// <summary>현재 실제 Lock이 반납되지 않았는지 나타낸다.</summary>
        public bool Locked;
        /// <summary>실제 DSBUFFERDESC가 가리킨 지역 형식 사본이다.</summary>
        public byte[] CreatedFormat = [];

        /// <summary>제품 내부 실제 delegate 형식으로 thunk를 만들고 표본/함수표 메모리를 준비한다.</summary>
        public ComBoundary()
        {
            Marshal.Copy(new byte[7 * nint.Size], 0, _deviceTable, 7 * nint.Size);
            Marshal.Copy(new byte[21 * nint.Size], 0, _bufferTable, 21 * nint.Size);
            Marshal.WriteIntPtr(_device, _deviceTable);
            Marshal.WriteIntPtr(_buffer, _bufferTable);
            Marshal.Copy(Enumerable.Repeat((byte)0xa7, (int)MusicChannel.BufferBytes).ToArray(), 0, Data, (int)MusicChannel.BufferBytes);
            Bind(_deviceTable, 2, "Release", nameof(ReleaseDevice));
            BindCreate();
            Bind(_bufferTable, 2, "Release", nameof(ReleaseBuffer));
            Bind(_bufferTable, 4, "ReadCursors", nameof(Cursors));
            Bind(_bufferTable, 9, "ReadDword", nameof(GetStatus));
            Bind(_bufferTable, 11, "LockBuffer", nameof(Lock));
            Bind(_bufferTable, 12, "PlayBuffer", nameof(Play));
            Bind(_bufferTable, 15, "WriteLong", nameof(SetVolume));
            Bind(_bufferTable, 18, "Simple", nameof(Stop));
            Bind(_bufferTable, 19, "UnlockBuffer", nameof(Unlock));
            Bind(_bufferTable, 20, "Simple", nameof(Restore));
        }

        /// <summary>제품이 실제 사용하는 private ABI delegate와 정확히 같은 형식으로 검사 메서드를 연결한다.</summary>
        private void Bind(nint table, int slot, string typeName, string methodName)
        {
            Type type = typeof(WindowsSoundDevice).GetNestedType(typeName, BindingFlags.NonPublic)!;
            MethodInfo method = GetType().GetMethod(methodName, BindingFlags.NonPublic | BindingFlags.Instance)!;
            Delegate callback = Delegate.CreateDelegate(type, this, method);
            _roots.Add(callback);
            Marshal.WriteIntPtr(table, slot * nint.Size, Marshal.GetFunctionPointerForDelegate(callback));
        }

        /// <summary>private DSBUFFERDESC ref ABI를 유지한 IL 중계만 만들고 실제 제품이 작성한 구조체를 검사 메서드로 전달한다.</summary>
        private void BindCreate()
        {
            Type descriptor = typeof(WindowsSoundDevice).GetNestedType("BufferDescription", BindingFlags.NonPublic)!;
            Type signature = typeof(WindowsSoundDevice).GetNestedType("CreateBuffer", BindingFlags.NonPublic)!;
            var bridge = new DynamicMethod("검사COM생성", typeof(int), [typeof(ComBoundary), typeof(nint), descriptor.MakeByRefType(), typeof(nint).MakeByRefType(), typeof(nint)], typeof(ComBoundary), true);
            ILGenerator il = bridge.GetILGenerator();
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Ldarg_2);
            il.Emit(OpCodes.Ldobj, descriptor);
            il.Emit(OpCodes.Box, descriptor);
            il.Emit(OpCodes.Ldarg_3);
            il.Emit(OpCodes.Ldarg_S, (byte)4);
            il.Emit(OpCodes.Call, GetType().GetMethod(nameof(Create), BindingFlags.NonPublic | BindingFlags.Instance)!);
            il.Emit(OpCodes.Ret);
            Delegate callback = bridge.CreateDelegate(signature, this);
            _roots.Add(callback);
            Marshal.WriteIntPtr(_deviceTable, 3 * nint.Size, Marshal.GetFunctionPointerForDelegate(callback));
        }

        /// <summary>실제 장치를 초기화하지 않고 검사 가상함수표의 포인터만 제품 장치에 명시 주입한다.</summary>
        public void Attach(WindowsSoundDevice device) => typeof(WindowsSoundDevice).GetField("_device", BindingFlags.NonPublic | BindingFlags.Instance)!.SetValue(device, _device);

        /// <summary>제품이 작성한 호스트 폭 descriptor/PCM 사본을 검사하고 명시 Create 응답만 돌려준다.</summary>
        private int Create(nint self, object descriptor, out nint buffer, nint outer)
        {
            Assert.Equal(_device, self);
            Assert.Equal(nint.Zero, outer);
            Creates++;
            Type type = descriptor.GetType();
            Assert.Equal((uint)Marshal.SizeOf(type), (uint)type.GetField("Size")!.GetValue(descriptor)!);
            Assert.Equal(MusicChannel.BufferFlags, (uint)type.GetField("Flags")!.GetValue(descriptor)!);
            Assert.Equal(MusicChannel.BufferBytes, (uint)type.GetField("Bytes")!.GetValue(descriptor)!);
            Assert.Equal(0u, (uint)type.GetField("Reserved")!.GetValue(descriptor)!);
            Assert.Equal(Guid.Empty, (Guid)type.GetField("Algorithm")!.GetValue(descriptor)!);
            CreatedFormat = new byte[18];
            Marshal.Copy((nint)type.GetField("Format")!.GetValue(descriptor)!, CreatedFormat, 0, 18);
            buffer = CreateResult >= 0 ? _buffer : 0;
            return CreateResult;
        }
        /// <summary>합성 장치의 실제 IUnknown.Release 진입이다.</summary>
        private uint ReleaseDevice(nint self) { Assert.Equal(_device, self); DeviceReleases++; return 0; }
        /// <summary>합성 버퍼의 실제 IUnknown.Release 진입이다.</summary>
        private uint ReleaseBuffer(nint self) { Assert.Equal(_buffer, self); Assert.False(Locked); Releases++; return 0; }
        /// <summary>음악 GetStatus가 출력과 실패 HRESULT를 각각 보존하는지 확인한다.</summary>
        private int GetStatus(nint self, out uint status) { Assert.Equal(_buffer, self); status = Status; return StatusResult; }
        /// <summary>음악 GetCurrentPosition의 두DWORD 출력 ABI다.</summary>
        private int Cursors(nint self, out uint play, out uint write) { Assert.Equal(_buffer, self); play = PlayCursor; write = WriteCursor; return CursorResult; }
        /// <summary>합성 버퍼의 실제 Restore 진입이다.</summary>
        private int Restore(nint self) { Assert.Equal(_buffer, self); Restores++; return 0; }
        /// <summary>합성 버퍼의 실제 Stop 진입이다.</summary>
        private int Stop(nint self) { Assert.Equal(_buffer, self); Assert.False(Locked); Stops++; return 0; }
        /// <summary>음악 음량의 실제 LONG 입력을 보존한다.</summary>
        private int SetVolume(nint self, int value) { Assert.Equal(_buffer, self); Volume = value; return 0; }
        /// <summary>실제 Play의 예약/우선0과 파일 반복과 독립인 플래그를 확인한다.</summary>
        private int Play(nint self, uint reserved, uint priority, uint flags) { Assert.Equal(_buffer, self); Assert.Equal(0u, reserved); Assert.Equal(0u, priority); PlayFlags = flags; return 0; }

        /// <summary>실제 unmanaged 메모리의 링 끝/처음 주소를 반환한다. 관리 배열 사본을 반환하지 않는다.</summary>
        private int Lock(nint self, uint offset, uint count, out nint first, out uint firstSize, out nint second, out uint secondSize, uint flags)
        {
            Assert.Equal(_buffer, self); Assert.False(Locked); Assert.Equal(0u, flags);
            first = second = 0; firstSize = secondSize = 0;
            if (LockResult < 0) return LockResult;
            _first = first = Data + (int)offset;
            _firstSize = firstSize = EmptyFirst ? 0 : Math.Min(count, MusicChannel.BufferBytes - offset);
            _secondSize = secondSize = count - firstSize;
            _second = second = secondSize == 0 ? 0 : Data;
            Locked = true;
            return 0;
        }

        /// <summary>빈 첫 구간까지 실제 Lock이 돌려준 포인터·길이 네 값을 그대로 받았는지 확인한다.</summary>
        private int Unlock(nint self, nint first, uint firstSize, nint second, uint secondSize)
        {
            Assert.Equal(_buffer, self); Assert.True(Locked);
            Assert.Equal(_first, first); Assert.Equal(_firstSize, firstSize);
            Assert.Equal(_second, second); Assert.Equal(_secondSize, secondSize);
            Unlocks++; Locked = false;
            return 0;
        }

        /// <summary>제품 장치 종료 뒤 이 검사만 할당한 포인터·함수표를 반납한다.</summary>
        public void Dispose()
        {
            Assert.False(Locked);
            Marshal.FreeHGlobal(Data);
            Marshal.FreeHGlobal(_buffer); Marshal.FreeHGlobal(_device);
            Marshal.FreeHGlobal(_bufferTable); Marshal.FreeHGlobal(_deviceTable);
            GC.KeepAlive(_roots);
        }
    }

    /// <summary>원문과 같은 PCM18BYTE를 만들고 입력 cbSize의 별도 보존도 검사한다.</summary>
    private static byte[] Format()
    {
        byte[] bytes = new byte[18];
        BinaryPrimitives.WriteUInt16LittleEndian(bytes, 1);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(2), 2);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(4), 22050);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(8), 88200);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(12), 4);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(14), 16);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(16), 22);
        return bytes;
    }

    /// <summary>미초기화 실제 장치의 실패와 기존 실제 파일IO 보존을 확인하며 DLL·창을 열지 않는다.</summary>
    [Fact]
    public void UnopenedDeviceFailsCreationAndKeepsRealFileHooks()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        using var files = new WindowsMusicFileStore();
        using var device = new WindowsSoundDevice(new(OriginalEdition.Patch1078), new(), WindowsSoundFiles.MakeDiskResolver(data));
        MusicChannelHooks io = files.FileHooks();
        MusicChannelHooks bound = device.BindMusicBuffers(io);
        Assert.NotSame(io, bound);
        Assert.Same(io.Seek, bound.Seek); Assert.Same(io.Read, bound.Read); Assert.Same(io.Close, bound.Close);
        Assert.NotNull(bound.SetVolume); Assert.NotNull(bound.Stop); Assert.NotNull(bound.Release);
        uint token = 93;
        Assert.Equal(unchecked((int)0x88780078), device.MusicBuffers().Create!(Format(), MusicChannel.BufferFlags, MusicChannel.BufferBytes, ref token));
        Assert.Equal(93u, token);
        var header = new MusicFileHeader();
        Assert.True(files.Open(Path.Combine(data, "music", "anticipation.mus"), header));
        Assert.Equal((int)header.DataOffset, bound.Seek!(header.File, header.DataOffset));
        byte[] sample = new byte[4]; Assert.Equal(4, bound.Read!(header.File, sample));
        bound.Close!(header.File); Assert.Equal(-1, files.Position(header.File));
    }

    /// <summary>실제COM ABI가 작성한 직접Span을 Unlock 전부터 같은 native 메모리에서 관찰하고 개별Release의중복반납을막는다.</summary>
    [Fact]
    public void SyntheticComUsesDirectNativeRegionsAndReleasesEachTokenOnce()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var native = new ComBoundary();
        using var device = new WindowsSoundDevice(new(OriginalEdition.Patch1078), new(), (_, _) => null);
        native.Attach(device);
        MusicBufferHooks buffers = device.MusicBuffers();
        byte[] format = Format(); uint token = 0;
        Assert.Equal(0, buffers.Create!(format, MusicChannel.BufferFlags, MusicChannel.BufferBytes, ref token));
        Assert.NotEqual(0u, token);
        Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(native.CreatedFormat.AsSpan(16)));
        Assert.Equal((ushort)22, BinaryPrimitives.ReadUInt16LittleEndian(format.AsSpan(16)));
        Assert.Equal(0, buffers.Status!(token, out uint status)); Assert.Equal(0u, status);
        Assert.Equal(0, buffers.Restore!(token));
        Assert.Equal(1, native.Restores);
        Assert.Equal(0, buffers.Cursor!(token, out uint play, out uint write)); Assert.Equal(17u, play); Assert.Equal(29u, write);
        MusicBufferRegions regions = default;
        Assert.Equal(0, buffers.Lock!(token, MusicChannel.BufferBytes - 3, 8, ref regions));
        Assert.Equal(3, regions.First.Length); Assert.Equal(5, regions.Second.Length);
        new byte[] { 1, 2, 3 }.CopyTo(regions.First);
        new byte[] { 4, 5, 6, 7, 8 }.CopyTo(regions.Second);
        Assert.Equal((byte)1, Marshal.ReadByte(native.Data + (int)MusicChannel.BufferBytes - 3));
        Assert.Equal((byte)4, Marshal.ReadByte(native.Data));
        Assert.Equal(0, buffers.Unlock!(token, regions));
        MusicChannelHooks channel = device.BindMusicBuffers(new());
        Assert.Equal(0, channel.SetVolume!(token, -765)); Assert.Equal(-765, native.Volume);
        Assert.Equal(0, buffers.Play!(token, 1)); Assert.Equal(1u, native.PlayFlags);
        channel.Stop!(token);
        Assert.Equal(1, native.Stops);
        Assert.Equal(0, channel.Release!(token));
        Assert.Equal(1, native.Releases);
        Assert.Throws<ArgumentOutOfRangeException>(() => buffers.Status!(token, out _));
        device.Shutdown(); device.Shutdown();
        Assert.Equal(1, native.Releases); Assert.Equal(1, native.DeviceReleases);
    }

    /// <summary>실패HRESULT의별도출력·기존토큰/구간보존과0길이지만비영인Lock주소를확인한다.</summary>
    [Fact]
    public void SyntheticComPreservesFailureOutputsAndEmptyRegionAddress()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var native = new ComBoundary { CreateResult = Failure };
        using var device = new WindowsSoundDevice(new(OriginalEdition.Patch1078), new(), (_, _) => null);
        native.Attach(device);
        MusicBufferHooks hooks = device.MusicBuffers();
        uint token = 93;
        Assert.Equal(Failure, hooks.Create!(Format(), MusicChannel.BufferFlags, MusicChannel.BufferBytes, ref token));
        Assert.Equal(93u, token);
        native.CreateResult = 0;
        Assert.Equal(0, hooks.Create!(Format(), MusicChannel.BufferFlags, MusicChannel.BufferBytes, ref token));
        native.StatusResult = Failure; native.Status = 5;
        Assert.Equal(Failure, hooks.Status!(token, out uint status)); Assert.Equal(5u, status);
        native.CursorResult = Failure;
        Assert.Equal(Failure, hooks.Cursor!(token, out uint play, out uint write)); Assert.Equal(17u, play); Assert.Equal(29u, write);
        MusicBufferRegions regions = new(new byte[] { 0x71, 0x72 }, new byte[] { 0x73 });
        native.LockResult = Failure;
        Assert.Equal(Failure, hooks.Lock!(token, 7, 8, ref regions));
        Assert.Equal(new byte[] { 0x71, 0x72 }, regions.First.ToArray());
        Assert.Equal(new byte[] { 0x73 }, regions.Second.ToArray());
        native.LockResult = 0; native.EmptyFirst = true;
        Assert.Equal(0, hooks.Lock!(token, 7, 8, ref regions));
        Assert.Empty(regions.First.ToArray()); Assert.Equal(8, regions.Second.Length);
        regions.Second[0] = 0x42;
        Assert.Equal((byte)0x42, Marshal.ReadByte(native.Data));
        Assert.Equal(0, hooks.Unlock!(token, regions));
        byte[] codec = Format(); BinaryPrimitives.WriteUInt16LittleEndian(codec, 2);
        int created = native.Creates; uint unchanged = 97;
        Assert.Equal(unchecked((int)0x88780064), hooks.Create!(codec, MusicChannel.BufferFlags, MusicChannel.BufferBytes, ref unchanged));
        Assert.Equal(97u, unchanged); Assert.Equal(created, native.Creates);
        _ = device.BindMusicBuffers(new()).Release!(token);
    }

    /// <summary>실제보호사본음악→Winmm→실제Read/Fill→native두구간쓰기를잇고IO예외도직접쓴접두와Unlock을보존한다.</summary>
    [Fact]
    public void ActualMusicFileAndChannelFillReachDirectComMemory()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        using var native = new ComBoundary();
        using var files = new WindowsMusicFileStore();
        using var device = new WindowsSoundDevice(new(OriginalEdition.Patch1078), new(), (_, _) => null);
        native.Attach(device);
        MusicChannelHooks io = files.FileHooks(); MusicChannelRead read = io.Read!; bool throwRead = false;
        io.Read = (file, target) =>
        {
            if (throwRead) { target[0] = 0x5c; throw new IOException("직접구간부분쓰기뒤예외"); }
            return read(file, target);
        };
        var state = new MusicChannelState();
        var channel = new MusicChannel(OriginalEdition.Patch1078, state, device.BindMusicBuffers(io));
        MusicBufferHooks buffers = device.MusicBuffers();
        Assert.True(channel.Open("anticipation.mus", new("music"), files.OpenHooks(WindowsSoundFiles.MakeDiskResolver(data))));
        uint file = state.Field(MusicField.File);
        Assert.True(channel.EnsureBuffer(buffers)); Assert.True(channel.Start(1));
        Assert.True(channel.FillBuffer(buffers, new() { MusicVolume = -10000 }));
        Assert.Equal(-10000, native.Volume); Assert.Equal(1u, native.PlayFlags);
        Assert.Equal(MusicChannel.BufferBytes, state.Field(MusicField.ReadOffset));
        using var expected = System.IO.File.OpenRead(Path.Combine(data, "music", "anticipation.mus"));
        expected.Position = state.Field(MusicField.DataOffset) + state.Field(MusicField.ReadOffset);
        byte[] samples = new byte[8]; expected.ReadExactly(samples);
        state.SetField(MusicField.WriteOffset, MusicChannel.BufferBytes - 3);
        native.Status = 1; native.PlayCursor = 5;
        Assert.True(channel.FillBuffer(buffers, new() { MusicVolume = -10000 }));
        byte[] actual = new byte[8];
        Marshal.Copy(native.Data + (int)MusicChannel.BufferBytes - 3, actual, 0, 3);
        Marshal.Copy(native.Data, actual, 3, 5);
        Assert.Equal(samples, actual); Assert.Equal(5u, state.Field(MusicField.WriteOffset));
        int unlocks = native.Unlocks;
        throwRead = true; native.PlayCursor = 9;
        Assert.Throws<IOException>(() => channel.FillBuffer(buffers, new()));
        Assert.False(native.Locked); Assert.Equal(unlocks + 1, native.Unlocks);
        Assert.Equal((byte)0x5c, Marshal.ReadByte(native.Data + 5));
        Assert.True(channel.Active());
        channel.Stop();
        Assert.Equal(-1, files.Position(file)); Assert.Equal(1, native.Releases);
    }
}
