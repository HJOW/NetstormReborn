using System.Buffers;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

/// <summary>
/// 같은 Client의 실제 구름 전역·주소 공간과 팔레트 이름을 메뉴·화면·세션보다 오래 소유한다(cpppj <c>client/ClientKencloudGlobals</c>).
/// 구름 계산은 <see cref="RawKencloud"/>·<see cref="RawKencloudRender"/>의 원문 본체를 쓰고, 여기서는 메모리(가짜 주소 공간)·파일·설정·화면 장치만 연결한다.
/// 사용: 생성 → <see cref="ReadOptions"/>(설정 변경마다) → 팔레트 전환 <see cref="Transition"/> 또는 <see cref="Update"/> → <see cref="Draw"/>를 화면에 표시한다.
/// </summary>
/// <remarks>
/// 0.381에서 추가했다(구름 L4a 차이 해소). C#에서 달라진 모양: ① cpppj의 <c>CurrentAllocationBytes</c>(실제 힙의 미초기화 내용 복사)는 결정적이지 않아
/// 기본을 0으로 둔다(<paramref name="allocation"/>으로 채움을 바꿀 수 있다). ② 구름 파일 읽기는 cpppj의 <c>ReadBackgroundGifFile</c>(입력 길이 선계산·치수 선행 쓰기·Decode 중 파일 수명)
/// 대신 <c>GameFileSystem</c> 전체 읽기 → <see cref="GifImage.DecodeBackgroundGif"/>로 옮겼다. 정상 경로의 색 번호·512 판정·반납 순서는 같고 부분 실패 경로의 중간 효과만 다르다.
/// ③ 렌더러 존재 토큰(<see cref="RendererToken"/>)과 전체 다시 그리기 요청(<see cref="WorldRedraw"/>)은 값만 둔다(Renderer 이식은 R5, cpppj <c>BindRenderer</c>는 그때 옮긴다).
/// </remarks>
public sealed class ClientKencloudGlobals
{
    /// <summary>원문의 현재 여덟 옵션 DWORD를 순서대로 식별한다.</summary>
    public enum Option
    {
        /// <summary>00540a7c cloudiness다.</summary>
        Cloudiness,

        /// <summary>00540a80 cloudSpeed(갱신당 행 이동 횟수)다.</summary>
        Speed,

        /// <summary>00540a84 cloudFury(warm-up 횟수 나눗셈 인자)다.</summary>
        Fury,

        /// <summary>00540a88 cloudSpacer다.</summary>
        Spacer,

        /// <summary>00540a8c cloudRecenter다.</summary>
        Recenter,

        /// <summary>00540a90 seed 개수다.</summary>
        SeedCount,

        /// <summary>00540a34 low 색이다.</summary>
        LowColor,

        /// <summary>high 색이다.</summary>
        HighColor,
    }

    /// <summary>연속 전역에서 구름 이름 버퍼가 시작하는 바이트 위치(8112)다. 뒤의 flags·phase까지 같은 물리다.</summary>
    private const int CloudNameByteOffset = 8112;

    /// <summary>실제 8216바이트의 이전 dirty/주소 4/현재 dirty/seed 1000/이름/flags/phase다.</summary>
    private readonly uint[] _state = new uint[KencloudGlobals.StateLength];

    /// <summary>보호 PE 00540a7c..90과 색 두 DWORD의 최초값이다.</summary>
    private readonly uint[] _options = [72, 2, 10, 32, 99999, 400, 228, 245];

    /// <summary>보호 PE 00540a98의 고정 행 순서다.</summary>
    private readonly uint[] _rows = [0, 8, 4, 12, 1, 11, 5, 15, 3, 10, 2, 7, 14, 9, 6, 13];

    /// <summary>005c8910 팔레트 이름의 최초 NUL이다. 세션 교체로 지우지 않는다.</summary>
    private readonly byte[] _paletteName = new byte[128];

    /// <summary>생존 주소의 실제 BYTE다(주소 오름차순). 해제 뒤의 조회는 실패한다.</summary>
    private readonly List<(uint Base, byte[] Bytes)> _blocks = [];

    /// <summary>다음 할당이 받을 host 주소 토큰이다(원문 DWORD 산술에 충분한 간격).</summary>
    private uint _nextAddress = 0x36000000U;

    /// <summary>실제 미작성 할당/지역 입력을 공급하는 장치다. 게임 계산은 하지 않는다.</summary>
    private readonly Action<Span<byte>> _allocation;

    /// <summary>PathSpec 반환을 부모 동기 호출 동안 안정된 NUL 물리로 보관한다(구름 파일).</summary>
    private byte[] _filePath = [0];

    /// <summary>PathSpec 반환을 부모 동기 호출 동안 안정된 NUL 물리로 보관한다(팔레트 파일).</summary>
    private byte[] _palettePath = [0];

    /// <summary>같은 연속 전역 위의 구름 이름 버퍼를 <see cref="Memory{T}"/>로 빌려 주는 관리자다.</summary>
    private readonly WordBytesManager _cloudName;

    /// <summary>같은 연속 전역·옵션·행표를 빌린 원문 전역 묶음이다.</summary>
    private readonly KencloudGlobals _globals;

    /// <summary>현재 렌더러의 존재 토큰(0이면 없음)이다. Renderer 이식(R5) 전까지는 값만 둔다.</summary>
    public uint RendererToken { get; set; }

    /// <summary>0059a8b0 월드 전체 다시 그리기 요청이며 팔레트 전환 뒤 렌더러가 있으면 1이 된다.</summary>
    public uint WorldRedraw { get; set; }

    /// <summary>PE 최초값과 BSS 0만 구성하며 파일·난수·구름 생성을 실행하지 않는다.</summary>
    /// <param name="allocation">새 블록 내용을 채우는 장치다. null이면 0이다(원문은 미초기화 힙 내용을 복사하지만 결정적이지 않아 0으로 둔다).</param>
    public ClientKencloudGlobals(Action<Span<byte>>? allocation = null)
    {
        _allocation = allocation ?? (_ => { });
        _globals = new KencloudGlobals(_state, _options, _rows);
        _cloudName = new WordBytesManager(_state, CloudNameByteOffset);
    }

    /// <summary>005697f0..0056b807의 같은 실제 물리와 옵션을 빌려 준다(사본이 아니다).</summary>
    public KencloudGlobals Globals() => _globals;

    /// <summary>실제 연속 전역 2054 DWORD이며 인접 별칭을 보존한다.</summary>
    public uint[] State() => _state;

    /// <summary>같은 현재 옵션 DWORD를 참조로 읽고 쓴다.</summary>
    /// <param name="option">옵션 종류다.</param>
    public ref uint Value(Option option) => ref _options[(int)option];

    /// <summary>보호 PE의 실제 고정 16행 순서다.</summary>
    public IReadOnlyList<uint> PhaseRows() => _rows;

    /// <summary>원문 팔레트 이름의 같은 128바이트 물리 버퍼다.</summary>
    public Memory<byte> PaletteName() => _paletteName;

    /// <summary>이름 뒤 kenCloud/algorithm/dirty/phase까지 같은 물리를 보존하는 구름 이름 버퍼다.</summary>
    public Memory<byte> CloudName() => _cloudName.Memory;

    /// <summary>
    /// 현재 값을 기본 입력으로 설정을 읽고 실제 성공한 store만 적용한다. 파일 생성은 하지 않는다.
    /// 클라이언트 시작과 설정 변경 때마다 부른다(원문 435220 위치).
    /// </summary>
    /// <param name="config">현재 설정이다.</param>
    public void ReadOptions(ConfigInterface config)
    {
        ArgumentNullException.ThrowIfNull(config);
        // 누락은 현재 필드를 보존하며 실제 성공 store만 적용한다.
        static void Read(ConfigInterface config, string key, ref uint field)
        {
            int value = unchecked((int)field);
            if (config.ReadInt(key, ref value)) field = unchecked((uint)value);
        }
        Read(config, "cloudiness", ref _options[0]);
        Read(config, "cloudSpeed", ref _options[1]);
        Read(config, "cloudFury", ref _options[2]);
        Read(config, "cloudSpacer", ref _options[3]);
        Read(config, "cloudRecenter", ref _options[4]);
        Read(config, "cloudAlgo", ref _state[2050]);
        Read(config, "kenCloud", ref _state[2049]);
    }

    /// <summary>겹치지 않는 32비트 주소와 정확한 실제 블록을 연결한다.</summary>
    /// <param name="bytes">요청 크기다(0이면 거부).</param>
    /// <returns>새 블록의 주소 토큰이다.</returns>
    public uint Allocate(uint bytes)
    {
        if (bytes == 0) throw new ArgumentOutOfRangeException(nameof(bytes), "구름 할당의 실제 물리 크기0");
        ulong extent = ((ulong)bytes + 0xfffUL) & ~0xfffUL;
        if ((ulong)_nextAddress + extent + 0x1000UL > uint.MaxValue) throw new OverflowException("구름 DWORD 주소 공간 소진");
        byte[] block = new byte[bytes];
        _allocation(block);
        uint token = _nextAddress;
        _blocks.Add((token, block));
        _nextAddress = unchecked(_nextAddress + (uint)(extent + 0x1000UL));
        return token; // 반환한 같은 블록만 원문 cache store에 공개된다.
    }

    /// <summary>내부 주소부터 남은 물리를 빌려 준다. 길이 0 one-past를 허용하고 해제된 주소는 거부한다.</summary>
    /// <param name="address">주소 토큰이다.</param>
    public Span<byte> Memory(uint address)
    {
        // 주소 이하의 가장 큰 base를 가진 생존 블록을 뒤에서부터 찾는다.
        for (int index = _blocks.Count - 1; index >= 0; index--)
        {
            (uint blockBase, byte[] bytes) = _blocks[index];
            if (blockBase > address) continue;
            ulong offset = (ulong)address - blockBase;
            if (offset > (ulong)bytes.Length) throw new ArgumentOutOfRangeException(nameof(address), "구름 현재 주소 물리 범위");
            return bytes.AsSpan((int)offset); // 마지막 0바이트와 내부 행 주소를 모두 같은 생존 BYTE에서 읽는다.
        }
        throw new ArgumentOutOfRangeException(nameof(address), "구름 현재 주소 물리 누락");
    }

    /// <summary>정확한 생존 base만 반납한다. 현재 cache 대입은 원문 부모에 남긴다.</summary>
    /// <param name="token">반납할 블록의 base다(0이면 아무것도 하지 않는다).</param>
    public void Release(uint token)
    {
        if (token == 0) return;
        int index = _blocks.FindIndex(block => block.Base == token);
        if (index < 0) throw new ArgumentOutOfRangeException(nameof(token), "구름 실제 반납 base 누락");
        _blocks.RemoveAt(index);
    }

    /// <summary>현재 실제 생존 블록 수를 관측한다.</summary>
    public int Allocations() => _blocks.Count;

    /// <summary>NUL 종료 바이트 열을 Latin1 문자열로 읽는다. NUL이 없으면 거부한다.</summary>
    /// <param name="bytes">NUL 포함 바이트 열이다.</param>
    private static string Text(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0) throw new ArgumentOutOfRangeException(nameof(bytes), "구름 현재 C문자열 물리 범위");
        return Encoding.Latin1.GetString(bytes[..end]);
    }

    /// <summary>문자열을 PathSpec 반환용 NUL 물리로 바꾼다.</summary>
    /// <param name="value">경로 문자열이다.</param>
    private static byte[] Path(string value)
    {
        byte[] bytes = new byte[value.Length + 1];
        Encoding.Latin1.GetBytes(value, bytes);
        return bytes;
    }

    /// <summary>실제 FileSpec → exists → 치수/픽셀 → 512 판정/free 부모다.</summary>
    /// <param name="files">파일 시스템이다.</param>
    /// <param name="config">현재 설정이다.</param>
    private uint LoadFile(GameFileSystem files, ConfigInterface config)
    {
        // 원문 지역 입력을 임의 0으로 만들지 않고 현재 할당 입력 장치에서 받는다.
        byte[] raw = new byte[264];
        _allocation(raw);
        var scratch = new KencloudFileScratch { Height = BitConverter.ToInt32(raw, 0), Width = BitConverter.ToInt32(raw, 4) };
        Buffer.BlockCopy(raw, 8, scratch.Path, 0, 256);
        var hooks = new KencloudFileHooks
        {
            PathSpec = (key, first, second, third) =>
            {
                if (first != 0 || second != 0 || third != 0) throw new InvalidOperationException("구름 실제 FileSpec 인자 계약");
                _filePath = Path(config.PathSpec(key));
                return _filePath;
            },
            Exists = path => files.Exists(Text(path)) ? 1U : 0U, // 원문 41ab10은 출처별 비영 코드를 돌려주며 구름 부모는 0 여부만 본다.
            Decode = (byte[] path, ref int width, ref int height) =>
            {
                var image = GifImage.DecodeBackgroundGif(files.ReadAllBytes(Text(path)));
                width = image.Width;
                height = image.Height;
                long size = (long)image.Width * image.Height;
                if (size <= 0 || size > 16L * 1024 * 1024) throw new ArgumentOutOfRangeException(nameof(path), "구름 GIF 실제 할당 폭");
                if (image.Indices.Length != size) throw new InvalidOperationException("구름 Native descriptor/decoder 치수 불일치");
                uint token = Allocate((uint)size); // 같은 실제 주소 할당은 두 치수 출력 뒤 Decode 앞이다(여기서는 해독 뒤 같은 순서 효과).
                image.Indices.CopyTo(Memory(token));
                return token;
            },
            Release = Release,
        };
        return RawKencloud.LoadKencloudFile(scratch, hooks); // 512 판정/잘못된 이미지 반납은 원문 부모가 수행한다.
    }

    /// <summary>실제 현재 메모리·할당·파일 장치만 만든다. 같은 동기 호출보다 오래 빌리지 않는다.</summary>
    /// <param name="files">파일 시스템이다.</param>
    /// <param name="config">현재 설정이다.</param>
    private KencloudHooks Devices(GameFileSystem files, ConfigInterface config) => new()
    {
        LoadFile = () => LoadFile(files, config),
        Allocate = Allocate,
        Memory = Memory,
    };

    /// <summary>같은 실제 난수와 파일 부모로 전체 원문 생성을 실행한다.</summary>
    /// <param name="files">파일 시스템이다.</param>
    /// <param name="config">현재 설정이다.</param>
    /// <param name="random">같은 실제 게임 난수다.</param>
    public void Build(GameFileSystem files, ConfigInterface config, GameRandom random) =>
        RawKencloud.BuildKencloud(_globals, random, Devices(files, config));

    /// <summary>같은 현재 캐시/행/dirty에 원문 행 이동을 실행한다.</summary>
    public void Step() => RawKencloud.StepKencloud(_globals, new KencloudHooks { Memory = Memory });

    /// <summary>같은 packed 2054 DWORD와 현재 speed를 표시/정리 부모에 빌려 준다.</summary>
    public KencloudRenderGlobals RenderGlobals() => new(_state, () => _options[(int)Option.Speed]);

    /// <summary>00470c50: 캐시가 없으면 같은 파일/난수로 생성하고 속도를 다시 읽어 캡처한 횟수만큼 행을 이동한다.</summary>
    /// <param name="files">파일 시스템이다.</param>
    /// <param name="config">현재 설정이다.</param>
    /// <param name="random">같은 실제 게임 난수다.</param>
    public void Update(GameFileSystem files, ConfigInterface config, GameRandom random) =>
        RawKencloudRender.UpdateKencloud(RenderGlobals(), new KencloudRenderHooks { Build = () => Build(files, config, random), Step = Step });

    /// <summary>같은 실제 이전/현재 행과 all/partial을 원문 순서로 전체 표시 요청으로 만든다.</summary>
    public void MarkDirty() => RawKencloudRender.MarkKencloudDirty(_state);

    /// <summary>실제 표시된 행만 지우며 다른 현재 dirty는 유지한다.</summary>
    public void CleanDirty() => RawKencloudRender.CleanKencloudDirty(_state);

    /// <summary>signed 나머지의 음수 인접 alias도 같은 물리에서 읽는다.</summary>
    /// <param name="first">행이다.</param>
    /// <param name="second">카메라 Y다.</param>
    public uint RowDirty(uint first, uint second) => RawKencloudRender.KencloudRowDirty(_state, first, second);

    /// <summary>
    /// 같은 실제 cache/clip/잠금/화면 픽셀에 두 표시 부모를 실행한다. clip·잠금·dirty 부분 효과와 정상 복구는 원문 부모가 소유한다.
    /// alternate(parallax DirectDraw 004a15d0/1650)는 미복원 장치라 명시 거부한다.
    /// </summary>
    /// <param name="screen">그릴 논리 화면이다.</param>
    /// <param name="area">left, top, right, bottom, x, y 여섯 DWORD다(x·y는 카메라 원점).</param>
    /// <param name="files">파일 시스템이다.</param>
    /// <param name="config">현재 설정이다.</param>
    /// <param name="random">같은 실제 게임 난수다.</param>
    /// <param name="alternate">두 번째 표시 부모 선택이다.</param>
    public void Draw(Screen screen, uint[] area, GameFileSystem files, ConfigInterface config, GameRandom random, bool alternate = false)
    {
        ArgumentNullException.ThrowIfNull(screen);
        static int Signed(uint value) => unchecked((int)value);
        var hooks = new KencloudRenderHooks
        {
            Build = () => Build(files, config, random),
            Clip = () =>
            {
                ScreenRect clip = screen.Clip; // 실제 현재 화면의 같은 네 값이다.
                return [unchecked((uint)clip.Left), unchecked((uint)clip.Top), unchecked((uint)clip.Right), unchecked((uint)clip.Bottom)];
            },
            SetClip = clip => screen.SetClip(Signed(clip[0]), Signed(clip[1]), Signed(clip[2]), Signed(clip[3])),
            Lock = special =>
            {
                if (special) throw new InvalidOperationException("구름 parallax DirectDraw 004a15d0 장치 미복원");
                screen.Lock();
            },
            Unlock = special =>
            {
                if (special) throw new InvalidOperationException("구름 parallax DirectDraw 004a1650 장치 미복원"); // 두 장치를 같은 GDI 잠금으로 속이지 않는다.
                screen.Unlock();
            },
            Blit = args => screen.BlitBuffer(Signed(args[0]), Signed(args[1]), Memory(args[2]), Signed(args[3]),
                new ScreenRect(Signed(args[4]), Signed(args[5]), Signed(args[6]), Signed(args[7]))), // 현재 cache의 같은 생존 BYTE를 복사본 없이 읽는다.
        };
        RawKencloudRender.DrawKencloud(RenderGlobals(), area, hooks, alternate);
    }

    /// <summary>
    /// 004b6100(팔레트 전환) 전체 부모를 같은 구름 전역·주소 공간에 실행한다. 이름 비교·로그·이름 복사·캐시 반납·생성·행 이동·경로·팔레트 읽기·색 표·dirty가
    /// 원문 순서로 일어난다. 호출자는 <paramref name="devices"/>에 <see cref="FortPaletteTransitionHooks.Log"/>·<see cref="FortPaletteTransitionHooks.LoadPalette"/>·
    /// <see cref="FortPaletteTransitionHooks.BuildMaps"/>를 채워 주고, 나머지(비교·반납·생성·이동·경로)는 여기서 채운다.
    /// </summary>
    /// <param name="name">새 팔레트 이름(NUL 포함)이다.</param>
    /// <param name="files">파일 시스템이다.</param>
    /// <param name="config">현재 설정이다.</param>
    /// <param name="random">같은 실제 게임 난수다.</param>
    /// <param name="devices">로그·팔레트 읽기·색 표 장치를 채운 묶음이다.</param>
    public void Transition(ReadOnlySpan<byte> name, GameFileSystem files, ConfigInterface config, GameRandom random, FortPaletteTransitionHooks devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        devices.Compare = (first, second) =>
        {
            // 두 이름 모두 NUL이 있는 현재 C문자열이어야 한다(없으면 거부). 비교는 ASCII 대소문자를 무시한다(CRT _stricmp).
            return string.Compare(Text(first), Text(second), StringComparison.OrdinalIgnoreCase);
        };
        devices.Release = Release;
        devices.BuildCloud = () => Build(files, config, random);
        devices.StepCloud = Step;
        devices.PathSpec = (key, input, second, third) =>
        {
            if (second != 0 || third != 0) throw new InvalidOperationException("팔레트 실제 FileSpec 인자 계약");
            _palettePath = Path(config.PathSpec(key, Text(input)));
            return _palettePath;
        };
        var globals = new FortPaletteTransitionGlobals
        {
            PaletteName = _paletteName,
            CloudName = _cloudName.Memory,
            GetCache = () => _state[KencloudGlobals.CacheIndex],
            SetCache = value => _state[KencloudGlobals.CacheIndex] = value,
            Count = () => _options[(int)Option.Speed],
            Renderer = () => RendererToken,
            GetDirty = () => WorldRedraw,
            SetDirty = value => WorldRedraw = value,
        };
        RawFortPaletteTransition.TransitionFortPalette(name, globals, devices); // 이름/반납/cache0/생성/이동/Screen/표/dirty의 원문 순서를 보존한다.
    }

    /// <summary>
    /// 연속 전역(<c>uint[]</c>) 위의 바이트 구간을 <see cref="Memory{T}"/>로 빌려 주는 관리자다.
    /// 원문의 구름 이름 버퍼는 뒤의 flags·phase와 같은 물리라 복사본을 만들면 별칭이 끊어지므로 같은 배열을 바이트로 다시 본다.
    /// </summary>
    private sealed class WordBytesManager : MemoryManager<byte>
    {
        /// <summary>바이트로 다시 볼 DWORD 배열(연속 전역)이다.</summary>
        private readonly uint[] _words;

        /// <summary>뷰가 시작하는 바이트 위치다.</summary>
        private readonly int _byteOffset;

        /// <summary>연속 전역과 시작 바이트 위치로 만든다.</summary>
        /// <param name="words">연속 전역이다.</param>
        /// <param name="byteOffset">뷰 시작 바이트 위치다.</param>
        public WordBytesManager(uint[] words, int byteOffset)
        {
            _words = words;
            _byteOffset = byteOffset;
        }

        /// <summary>같은 DWORD 배열의 바이트 뷰(시작 위치 이후)를 돌려준다.</summary>
        public override Span<byte> GetSpan() => MemoryMarshal.AsBytes(_words.AsSpan())[_byteOffset..];

        /// <summary>고정은 지원하지 않는다(호출처는 관리 메모리만 쓴다).</summary>
        /// <param name="elementIndex">요소 위치다.</param>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException();

        /// <summary>고정 해제는 지원하지 않는다.</summary>
        public override void Unpin() => throw new NotSupportedException();

        /// <summary>해제할 비관리 자원이 없다.</summary>
        /// <param name="disposing">관리 해제 여부다.</param>
        protected override void Dispose(bool disposing)
        {
        }
    }
}
